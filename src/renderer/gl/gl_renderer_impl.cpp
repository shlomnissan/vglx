/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/gl/gl_renderer_impl.hpp"

#include "vglx/cameras/camera.hpp"
#include "vglx/canvas/canvas.hpp"
#include "vglx/canvas/sprite.hpp"
#include "vglx/core/render_target.hpp"
#include "vglx/geometries/geometry.hpp"
#include "vglx/lights/light.hpp"
#include "vglx/materials/billboard_material.hpp"
#include "vglx/materials/material.hpp"
#include "vglx/materials/pbr_material.hpp"
#include "vglx/materials/phong_material.hpp"
#include "vglx/materials/shader_material.hpp"
#include "vglx/materials/unlit_material.hpp"
#include "vglx/math/matrix3.hpp"
#include "vglx/math/vector4.hpp"
#include "vglx/scene/billboard.hpp"
#include "vglx/scene/fog.hpp"
#include "vglx/scene/instanced_mesh.hpp"
#include "vglx/scene/mesh.hpp"
#include "vglx/scene/scene.hpp"
#include "vglx/textures/texture.hpp"
#include "vglx/textures/texture_2d.hpp"

#include "core/canvas_render_list.hpp"
#include "core/program_attributes.hpp"
#include "core/render_lists.hpp"
#include "utilities/logger.hpp"
#include "utilities/scoped_timer.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <glad/glad.h>

namespace vglx {

Renderer::Impl::Impl(const Renderer::Parameters& params)
  : scene_buffer_({
        params.framebuffer_width,
        params.framebuffer_height,
        params.sample_count,
    }),
    viewport_width_(params.framebuffer_width),
    viewport_height_(params.framebuffer_height),
    render_lists_(std::make_unique<RenderLists>()),
    shadow_render_lists_(std::make_unique<RenderLists>()),
    canvas_render_list_(std::make_unique<CanvasRenderList>()),
    depth_material_(DepthMaterial::Create()),
    shadow_map_(params.shadow_map),
    auto_clear_(params.auto_clear),
    tone_mapping_(params.tone_mapping),
    exposure_(params.exposure)
{
    state_.SetViewport(0, 0, params.framebuffer_width, params.framebuffer_height);
    state_.SetClearColor(params.clear_color);

    depth_material_->fog = false;
}

auto Renderer::Impl::Initialize() -> std::expected<void, std::string> {
#if !defined(NDEBUG)
    const auto timer = ScopedTimer(
        "Renderer initialization time",
        ScopedTimer::Unit::Milliseconds,
        LogLevel::Info
    );
#endif

    const auto& info = gl::driver_info();
    Logger::Log(LogLevel::Info, "Vendor: {}", info.vendor);
    Logger::Log(LogLevel::Info, "Renderer: {}", info.renderer);
    Logger::Log(LogLevel::Info, "Version: {}", info.version);
    Logger::Log(LogLevel::Info, "GLSL Version: {}", info.glsl_version);

    if (auto result = scene_buffer_.Initialize(); !result.has_value()) {
        return std::unexpected(result.error());
    }

    if (auto result = present_pass_.Initialize(); !result.has_value()) {
        return std::unexpected(result.error());
    }

    if (auto result = background_pass_.Initialize(); !result.has_value()) {
        return std::unexpected(result.error());
    }

    if (auto result = environment_.Initialize(); !result.has_value()) {
        return std::unexpected(result.error());
    }

    if (auto result = shadow_maps_.Initialize(); !result.has_value()) {
        return std::unexpected(result.error());
    }

    state_.SetDepthFunction(Material::Depth::LessEqual);
    state_.SetSeamlessCubemapFiltering();

    return {};
}

auto Renderer::Impl::RenderObjects(Scene* scene, Camera* camera) -> void {
    for (auto renderable : render_lists_->Opaque()) {
        RenderObject(renderable, scene, camera);
    }

    state_.SetDepthWrites(false);

    if (scene->background) {
        // Background draws at the far plane (clip-space z = 1.0), so it needs
        // LessEqual depth to pass against the cleared depth buffer. Cull-face
        // is off because a cube skybox is viewed from inside, where its
        // outward-wound faces would otherwise be culled.
        state_.SetDepthTest(true);
        state_.SetDepthFunction(Material::Depth::LessEqual);
        state_.SetSide(Material::Side::TwoSided);
        state_.SetBlending(Material::Blending::None);
        if (textures_.Bind(scene->background, 0) != 0u) {
            background_pass_.Render(scene->background);
        }

        // The background pass binds its own vertex array objects directly,
        // which invalidates the binding state's current VAO tracking.
        binding_state_.Reset();
    }

    for (auto renderable : render_lists_->Transparent()) {
        RenderObject(renderable, scene, camera);
    }

    state_.SetDepthWrites(true);
}

auto Renderer::Impl::RenderObject(Renderable* renderable, Scene* scene, Camera* camera) -> void {
    auto geometry = renderable->GetGeometry();
    auto material = renderable->GetMaterial().get();

    const auto is_mesh_type = renderable->GetNodeType() == Node::Type::Mesh ||
                              renderable->GetNodeType() == Node::Type::InstancedMesh;
    const auto is_instanced = renderable->GetNodeType() == Node::Type::InstancedMesh;
    if (!is_instanced && material->wireframe && is_mesh_type) {
        geometry = static_cast<Mesh*>(renderable)->GetWireframeGeometry();
    }

    auto attrs = ProgramAttributes {renderable, {
        .directional = lights_.directional,
        .point = lights_.point,
        .spot = lights_.spot,
        .enable_shadow_maps = shadow_map_ != ShadowMap::None && lights_.has_shadow_casters,
        .enable_pcf_shadows = shadow_map_ == ShadowMap::PCF && lights_.has_shadow_casters,
        .enable_point_shadow_maps = shadow_map_ != ShadowMap::None && lights_.has_point_shadow_casters
    }, scene, geometry.get(), material};

    auto program = programs_.GetProgram(attrs);
    if (!program->IsValid()) {
        return;
    }

    state_.ProcessMaterial(material);

    const auto vao = is_instanced
        ? binding_state_.Bind(*static_cast<InstancedMesh*>(renderable), *program)
        : binding_state_.Bind(*geometry, *program);
    if (vao == 0) {
        return;
    }

    SetUniforms(program, &attrs, renderable, camera, scene);

    state_.UseProgram(program->ProgramId());
    program->UpdateUniforms();

    auto primitive = GL_TRIANGLES;
    if (geometry->primitive == Geometry::PrimitiveType::Lines) {
        primitive = GL_LINES;
    }
    if (geometry->primitive == Geometry::PrimitiveType::LineLoop) {
        primitive = GL_LINE_LOOP;
    }

    const auto index_size = geometry->GetIndexData().size();
    const auto vertex_size = geometry->VertexCount();

    if (is_instanced) {
        const auto count = static_cast<InstancedMesh*>(renderable)->GetDrawCount();
        index_size
            ? glDrawElementsInstanced(primitive, index_size, GL_UNSIGNED_INT, nullptr, count)
            : glDrawArraysInstanced(primitive, 0, vertex_size, count);
    } else {
        index_size
            ? glDrawElements(primitive, index_size, GL_UNSIGNED_INT, nullptr)
            : glDrawArrays(primitive, 0, vertex_size);
    }

    rendered_objects_counter_++;
}

auto Renderer::Impl::SetUniforms(
    GLProgram* program,
    ProgramAttributes* attrs,
    Renderable* renderable,
    Camera* camera,
    Scene* scene
) -> void {
    auto material = renderable->GetMaterial().get();
    const auto& model = renderable->GetCachedWorldTransform();

    auto next_texture_unit = 0;

    program->SetUniform(Uniform::Model, &model);
    program->SetUniform(Uniform::Opacity, &material->opacity);

    if (attrs->alpha_test) {
        program->SetUniform(Uniform::AlphaTest, &material->alpha_test);
    }

    static const auto kIdentity = Matrix3::Identity();
    program->SetUniform(Uniform::TextureTransform, &kIdentity);

    const auto bind_texture = [&](Uniform uniform, const std::shared_ptr<Texture>& tex) {
        auto texture_unit = next_texture_unit++;
        if (textures_.Bind(tex, static_cast<uint8_t>(texture_unit)) == 0u) return;
        program->SetUniform(uniform, &texture_unit);

        if (tex->GetType() == Texture::Type::Texture2D) {
            const auto& transform = static_cast<Texture2D*>(tex.get())->transform.Get();
            program->SetUniform(Uniform::TextureTransform, &transform);
        }
    };

    if (scene->fog) {
        const auto& fog = scene->fog.value();
        program->SetUniform(Uniform::FogType, &fog.type);
        if (fog.type == Fog::Type::Linear) {
            program->SetUniform(Uniform::FogColor, &fog.color);
            program->SetUniform(Uniform::FogNear, &fog.near);
            program->SetUniform(Uniform::FogFar, &fog.far);
        }

        if (fog.type == Fog::Type::Exponential) {
            program->SetUniform(Uniform::FogColor, &fog.color);
            program->SetUniform(Uniform::FogDensity, &fog.density);
        }
    }

    if (lights_.HasLights()) {
        program->SetUniform(Uniform::AmbientLight, &lights_.ambient_light);
    }

    if (attrs->type == Material::Type::PBRMaterial) {
        auto m = static_cast<PBRMaterial*>(material);

        if (attrs->ibl) {
            const auto irradiance_unit = next_texture_unit++;
            const auto prefiltered_unit = next_texture_unit++;
            const auto brdf_lut_unit = next_texture_unit++;
            const auto prefiltered_max_lod = static_cast<float>(env_maps_.prefiltered_mips - 1);

            glActiveTexture(GL_TEXTURE0 + irradiance_unit);
            glBindTexture(GL_TEXTURE_CUBE_MAP, env_maps_.irradiance);

            glActiveTexture(GL_TEXTURE0 + prefiltered_unit);
            glBindTexture(GL_TEXTURE_CUBE_MAP, env_maps_.prefiltered);

            glActiveTexture(GL_TEXTURE0 + brdf_lut_unit);
            glBindTexture(GL_TEXTURE_2D, environment_.BrdfLut());

            program->SetUniform(Uniform::IrradianceMap, &irradiance_unit);
            program->SetUniform(Uniform::PrefilteredMap, &prefiltered_unit);
            program->SetUniform(Uniform::BrdfLut, &brdf_lut_unit);
            program->SetUniform(Uniform::PrefilteredMaxLod, &prefiltered_max_lod);

            const auto env_intensity = scene->environment_intensity * m->environment_intensity;
            program->SetUniform(Uniform::EnvironmentIntensity, &env_intensity);
        }

        if (lights_.HasLights()) {
            program->SetUniform(Uniform::MaterialColor, &m->color);
            program->SetUniform(Uniform::MaterialMetallic, &m->metallic);
            program->SetUniform(Uniform::MaterialRoughness, &m->roughness);

            if (attrs->shadow_maps) {
                auto texture_unit = next_texture_unit++;
                glActiveTexture(GL_TEXTURE0 + texture_unit);
                glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_maps_.GetTexture2D());
                program->SetUniform(Uniform::ShadowMaps2D, &texture_unit);
                program->SetUniform(Uniform::ReceiveShadow, &renderable->receive_shadow);

                if (attrs->point_shadow_maps) {
                    auto point_texture_unit = next_texture_unit++;
                    glActiveTexture(GL_TEXTURE0 + point_texture_unit);
                    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, shadow_maps_.GetPointTexture());
                    program->SetUniform(Uniform::PointShadowMaps, &point_texture_unit);
                }
            }
        }

        program->SetUniform(Uniform::EmissiveColor, &m->emissive_color);
        program->SetUniform(Uniform::EmissiveIntensity, &m->emissive_intensity);

        if (attrs->albedo_map) {
            bind_texture(Uniform::AlbedoMap, m->albedo_map);
        }
        if (attrs->alpha_map) {
            bind_texture(Uniform::AlphaMap, m->alpha_map);
        }
        if (attrs->ao_map) {
            bind_texture(Uniform::AOMap, m->ao_map);
            program->SetUniform(Uniform::AOIntensity, &m->ao_intensity);
        }
        if (attrs->emissive_map) {
            bind_texture(Uniform::EmissiveMap, m->emissive_map);
        }
        if (attrs->normal_map) {
            bind_texture(Uniform::NormalMap, m->normal_map);
            program->SetUniform(Uniform::NormalIntensity, &m->normal_intensity);
        }
        if (attrs->metallic_map) {
            bind_texture(Uniform::MetallicMap, m->metallic_map);
        }
        if (attrs->roughness_map) {
            bind_texture(Uniform::RoughnessMap, m->roughness_map);
        }
    }

    if (attrs->type == Material::Type::PhongMaterial) {
        auto m = static_cast<PhongMaterial*>(material);
        if (lights_.HasLights()) {
            program->SetUniform(Uniform::MaterialDiffuseColor, &m->color);
            program->SetUniform(Uniform::MaterialSpecularColor, &m->specular_color);
            program->SetUniform(Uniform::MaterialShininess, &m->shininess);

            if (attrs->shadow_maps) {
                auto texture_unit = next_texture_unit++;
                glActiveTexture(GL_TEXTURE0 + texture_unit);
                glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_maps_.GetTexture2D());
                program->SetUniform(Uniform::ShadowMaps2D, &texture_unit);
                program->SetUniform(Uniform::ReceiveShadow, &renderable->receive_shadow);

                if (attrs->point_shadow_maps) {
                    auto point_texture_unit = next_texture_unit++;
                    glActiveTexture(GL_TEXTURE0 + point_texture_unit);
                    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, shadow_maps_.GetPointTexture());
                    program->SetUniform(Uniform::PointShadowMaps, &point_texture_unit);
                }
            }
        }

        program->SetUniform(Uniform::EmissiveColor, &m->emissive_color);
        program->SetUniform(Uniform::EmissiveIntensity, &m->emissive_intensity);

        if (attrs->albedo_map) {
            bind_texture(Uniform::AlbedoMap, m->albedo_map);
        }
        if (attrs->alpha_map) {
            bind_texture(Uniform::AlphaMap, m->alpha_map);
        }
        if (attrs->ao_map) {
            bind_texture(Uniform::AOMap, m->ao_map);
            program->SetUniform(Uniform::AOIntensity, &m->ao_intensity);
        }
        if (attrs->emissive_map) {
            bind_texture(Uniform::EmissiveMap, m->emissive_map);
        }
        if (attrs->environment_map) {
            bind_texture(Uniform::EnvironmentMap, m->environment_map);
            program->SetUniform(Uniform::Reflectivity, &m->reflectivity);
        }
        if (attrs->normal_map) {
            bind_texture(Uniform::NormalMap, m->normal_map);
            program->SetUniform(Uniform::NormalIntensity, &m->normal_intensity);
        }
        if (attrs->specular_map) {
            bind_texture(Uniform::SpecularMap, m->specular_map);
        }
    }

    if (attrs->type == Material::Type::ShaderMaterial) {
        auto m = static_cast<ShaderMaterial*>(material);
        for (const auto& [name, value] : m->uniforms_) {
            program->SetUniform(name, &value);
        }
        for (const auto& [name, tex] : m->textures_) {
            const auto tex_unit = next_texture_unit++;
            if (textures_.Bind(tex, static_cast<uint8_t>(tex_unit)) == 0u) continue;
            program->SetUniform(name, &tex_unit);

            if (tex->GetType() == Texture::Type::Texture2D) {
                const auto& transform = static_cast<Texture2D*>(tex.get())->transform.Get();
                program->SetUniform(Uniform::TextureTransform, &transform);
            }
        }

        if (attrs->shadow_maps) {
            auto texture_unit = next_texture_unit++;
            glActiveTexture(GL_TEXTURE0 + texture_unit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_maps_.GetTexture2D());
            program->SetUniform(Uniform::ShadowMaps2D, &texture_unit);
            program->SetUniform(Uniform::ReceiveShadow, &renderable->receive_shadow);

            if (attrs->point_shadow_maps) {
                auto point_texture_unit = next_texture_unit++;
                glActiveTexture(GL_TEXTURE0 + point_texture_unit);
                glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, shadow_maps_.GetPointTexture());
                program->SetUniform(Uniform::PointShadowMaps, &point_texture_unit);
            }
        }
    }

    if (attrs->type == Material::Type::BillboardMaterial) {
        auto m = static_cast<BillboardMaterial*>(material);
        auto r = static_cast<Billboard*>(renderable);

        program->SetUniform(Uniform::Anchor, &r->anchor);
        program->SetUniform(Uniform::Color, &m->color);
        program->SetUniform(Uniform::Rotation, &r->rotation);

        if (attrs->texture_map) {
            bind_texture(Uniform::TextureMap, m->texture_map);
        }
    }

    if (attrs->type == Material::Type::UnlitMaterial) {
        auto m = static_cast<UnlitMaterial*>(material);
        program->SetUniform(Uniform::Color, &m->color);

        if (attrs->alpha_map) {
            bind_texture(Uniform::AlphaMap, m->alpha_map);
        }
        if (attrs->texture_map) {
            bind_texture(Uniform::TextureMap, m->texture_map);
        }
    }
}

auto Renderer::Impl::UpdateFrameUniforms() -> void {
    frame_uniforms_.UploadIfNeeded(&frame_, sizeof(frame_));
}

auto Renderer::Impl::UpdateCameraUniforms(Camera* camera) -> void {
    camera_.projection = camera->projection_matrix;
    camera_.view = camera->view_matrix;
    camera_uniforms_.UploadIfNeeded(&camera_, sizeof(camera_));
}

auto Renderer::Impl::ProcessLights(Camera* camera) -> void {
    lights_.Reset();

    for(auto light : render_lists_->Lights()) {
        GLShadowMap* shadow_map {shadow_maps_.GetShadowMap(light)};
        if (shadow_map != nullptr) {
            lights_.AddLight(light, camera, &shadow_map->transform, shadow_map->map_idx);
        } else {
            lights_.AddLight(light, camera);
        }
    }

    if (lights_.HasLights()) lights_.Update();
}

static auto configure_depth_material(Material* source, DepthMaterial* destination) -> void {
    destination->opacity = source->opacity;
    destination->alpha_test = source->alpha_test;
    destination->albedo_map = nullptr;
    destination->alpha_map = nullptr;

    if (source->alpha_test <= 0.0f) return;

    switch (source->GetType()) {
        case Material::Type::PBRMaterial: {
            auto m = static_cast<PBRMaterial*>(source);
            destination->albedo_map = m->albedo_map;
            destination->alpha_map = m->alpha_map;
        }
        break;
        case Material::Type::PhongMaterial: {
            auto m = static_cast<PhongMaterial*>(source);
            destination->albedo_map = m->albedo_map;
            destination->alpha_map = m->alpha_map;
        }
        break;
        case Material::Type::UnlitMaterial: {
            auto m = static_cast<UnlitMaterial*>(source);
            destination->albedo_map = m->texture_map;
            destination->alpha_map = m->alpha_map;
        }
        break;
        default: break;
    }
}

auto Renderer::Impl::RenderShadowMaps(Scene* scene, Camera* camera) -> void {
    auto lights = std::vector<Light*> {};
    auto count_2d = 0u;
    auto count_point = 0u;
    auto max_map_size_2d = 0u;
    auto max_map_size_point = 0u;

    auto slots = 0;
    for (auto light : render_lists_->Lights()) {
        if (light->GetType() == Light::Type::Ambient) {
            continue;
        }

        if (++slots > GLLights::kMaxLights) {
            break;
        }

        auto shadow = light->GetShadow();
        if (shadow == nullptr) {
            continue;
        }

        lights.emplace_back(light);
        if (light->GetType() == Light::Type::Point) {
            count_point++;
            max_map_size_point = std::max(max_map_size_point, shadow->map_size);
        } else {
            count_2d++;
            max_map_size_2d = std::max(max_map_size_2d, shadow->map_size);
        }
    }

    auto result = shadow_maps_.StartFrame(
        count_2d,
        max_map_size_2d,
        count_point,
        max_map_size_point
    );

    if (!result.has_value()) {
        Logger::Log(LogLevel::Error, "{}", result.error());
        shadow_maps_.EndFrame();
        return;
    }

    const auto render_depth_pass = [&](Camera* shadow_camera, unsigned int map_size) {
        state_.SetViewport(0, 0, map_size, map_size);
        state_.SetDepthTest(true);
        state_.SetDepthWrites(true);

        glClear(GL_DEPTH_BUFFER_BIT);

        shadow_render_lists_->ProcessScene(scene, shadow_camera, false);
        UpdateCameraUniforms(shadow_camera);

        for (auto renderable : shadow_render_lists_->Opaque()) {
            if (!renderable->cast_shadow) continue;

            auto geometry = renderable->GetGeometry();
            if (geometry->primitive != Geometry::PrimitiveType::Triangles) {
                continue;
            }

            auto material = renderable->GetMaterial().get();
            configure_depth_material(material, depth_material_.get());

            auto attrs = ProgramAttributes {
                renderable,
                {},
                scene,
                renderable->GetGeometry().get(),
                depth_material_.get()
            };

            auto program = programs_.GetProgram(attrs);
            if (!program->IsValid()) {
                continue;
            }

            auto is_instanced = renderable->GetNodeType() == Node::Type::InstancedMesh;
            const auto& model = renderable->GetCachedWorldTransform();

            using enum Material::Side;
            switch (material->side) {
                case Front: state_.SetSide(Back); break;
                case Back: state_.SetSide(Front); break;
                case TwoSided: state_.SetSide(TwoSided); break;
            }

            state_.UseProgram(program->ProgramId());
            auto next_texture_unit = 0;

            program->SetUniform(Uniform::Model, &model);

            if (attrs.alpha_test) {
                program->SetUniform(Uniform::Opacity, &depth_material_->opacity);
                program->SetUniform(Uniform::AlphaTest, &depth_material_->alpha_test);

                static const auto kIdentity = Matrix3::Identity();
                program->SetUniform(Uniform::TextureTransform, &kIdentity);

                const auto bind_alpha_texture = [&](
                    Uniform uniform,
                    const std::shared_ptr<Texture>& tex
                ) {
                    auto texture_unit = next_texture_unit++;
                    if (textures_.Bind(tex, static_cast<uint8_t>(texture_unit)) == 0u) return;
                    program->SetUniform(uniform, &texture_unit);
                    if (tex->GetType() == Texture::Type::Texture2D) {
                        const auto& transform = static_cast<Texture2D*>(tex.get())->transform.Get();
                        program->SetUniform(Uniform::TextureTransform, &transform);
                    }
                };

                if (attrs.albedo_map) {
                    bind_alpha_texture(
                        Uniform::AlbedoMap,
                        depth_material_->albedo_map
                    );
                }

                if (attrs.alpha_map) {
                    bind_alpha_texture(
                        Uniform::AlphaMap,
                        depth_material_->alpha_map
                    );
                }
            }

            program->UpdateUniforms();

            const auto index_size = geometry->GetIndexData().size();
            const auto vertex_size = geometry->VertexCount();

            const auto vao = is_instanced
                ? binding_state_.Bind(*static_cast<InstancedMesh*>(renderable), *program)
                : binding_state_.Bind(*geometry, *program);
            if (vao == 0) {
                continue;
            }

            if (is_instanced) {
                const auto count = static_cast<InstancedMesh*>(renderable)->GetDrawCount();
                index_size
                    ? glDrawElementsInstanced(GL_TRIANGLES, index_size, GL_UNSIGNED_INT, nullptr, count)
                    : glDrawArraysInstanced(GL_TRIANGLES, 0, vertex_size, count);
            } else {
                index_size
                    ? glDrawElements(GL_TRIANGLES, index_size, GL_UNSIGNED_INT, nullptr)
                    : glDrawArrays(GL_TRIANGLES, 0, vertex_size);
            }
        }
    };

    for (auto light : lights) {
        auto config = light->GetShadow();
        if (config == nullptr) {
            Logger::Log(
                LogLevel::Error,
                "Failed to read shadow config from light source {}",
                light->DisplayName()
            );
            continue;
        }

        if (light->GetType() == Light::Type::Point) {
            for (auto i = 0u; i < 6u; ++i) {
                auto result = shadow_maps_.BindShadowMap(light, camera, i);
                if (!result.has_value()) {
                    Logger::Log(LogLevel::Error, "{}", result.error());
                    break;
                }

                auto needs_update = config->auto_update || config->needs_update;
                if (!needs_update) {
                    break;
                }

                render_depth_pass(result.value(), max_map_size_point);
            }
        } else {
            auto result = shadow_maps_.BindShadowMap(light, camera);
            if (!result.has_value()) {
                Logger::Log(LogLevel::Error, "{}", result.error());
                continue;
            }

            auto needs_update = config->auto_update || config->needs_update;
            if (needs_update) {
                render_depth_pass(result.value(), config->map_size);
            }
        }

        config->needs_update = false;
    }

    shadow_maps_.EndFrame();
}

auto Renderer::Impl::Render(Scene* scene, Camera* camera, RenderTarget* target) -> void {
    frame_.resolution = target != nullptr
        ? Vector2 { static_cast<float>(target->width), static_cast<float>(target->height) }
        : Vector2 { static_cast<float>(viewport_width_), static_cast<float>(viewport_height_) };

    frame_.time = static_cast<float>(timer_.GetElapsedSeconds());

    if (scene->environment) {
        if (const auto texture_id = textures_.Bind(scene->environment, 0); texture_id != 0u) {
            auto env_maps = environment_.GetOrProcess(scene->environment, texture_id);
            if (env_maps.has_value()) {
                env_maps_ = env_maps.value();
            }
        }
    }

    scene->UpdateTransformHierarchy();
    camera->UpdateViewMatrix();

    render_lists_->ProcessScene(scene, camera);

    if (shadow_map_ != ShadowMap::None) {
        RenderShadowMaps(scene, camera);
    }

    const auto use_default_target = target == nullptr;
    use_default_target ? scene_buffer_.Begin() : framebuffers_.Begin(target);

    if (auto_clear_) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    UpdateFrameUniforms();

    UpdateCameraUniforms(camera);

    ProcessLights(camera);

    RenderObjects(scene, camera);

    use_default_target ? scene_buffer_.End() : framebuffers_.End(target);

    binding_state_.Reset();
    state_.Reset();
    textures_.Reset();

    if (use_default_target) {
        present_pass_.Present(scene_buffer_, tone_mapping_, exposure_);
        RenderCanvas(&scene->canvas);
    }

    rendered_objects_per_frame_ = rendered_objects_counter_;
    rendered_objects_counter_ = 0;
}

auto Renderer::Impl::RenderCanvas(Canvas* canvas) -> void {
    const auto size = canvas->GetSize();
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    auto program = programs_.GetCanvasProgram();
    if (!program->IsValid()) return;

    canvas->UpdateTransformHierarchy();
    canvas_render_list_->ProcessCanvas(canvas);
    if (canvas_render_list_->Renderables().empty()) return;

    state_.SetDepthTest(false);
    state_.SetBlending(Material::Blending::Normal);
    state_.SetSide(Material::Side::TwoSided);
    state_.UseProgram(program->ProgramId());

    for (auto node : canvas_render_list_->Renderables()) {
        if (node->GetNodeType() == Node2D::Type::Sprite) {
            RenderSprite(static_cast<Sprite*>(node), canvas->projection_matrix);
        }
    }

    binding_state_.Reset();
    state_.Reset();
    textures_.Reset();
}

auto Renderer::Impl::RenderSprite(Sprite* sprite, const Matrix3& projection) -> void {
    if (sprite->texture == nullptr || sprite->texture->image == nullptr) return;

    auto program = programs_.GetCanvasProgram();

    const auto texture_size = Vector2 {
        static_cast<float>(sprite->texture->image->width),
        static_cast<float>(sprite->texture->image->height)
    };

    if (texture_size.x <= 0.0f || texture_size.y <= 0.0f) return;

    const auto& geometry = sprite->GetGeometry();

    constexpr auto texture_unit = 0;
    if (textures_.Bind(sprite->texture, texture_unit) == 0u) return;
    if (binding_state_.Bind(*geometry, *program) == 0u) return;

    const auto region = sprite->region.value_or(
        Rect {0.0f, 0.0f, texture_size.x, texture_size.y}
    );

    // Map the pixel region to normalized texture space as (u, v, width, height)
    // with v flipped to match the quad's top-down texture coordinates since
    // images are flipped on load.
    const auto uv_rect = Vector4 {
        region.x / texture_size.x,
        1.0f - region.y / texture_size.y,
        region.width / texture_size.x,
        -region.height / texture_size.y
    };

    const auto size = sprite->GetSize();
    const auto opacity = sprite->GetWorldOpacity();
    const auto& model = sprite->GetCachedWorldTransform();

    program->SetUniform(Uniform::Model, &model);
    program->SetUniform(Uniform::Anchor, &sprite->anchor);
    program->SetUniform(Uniform::Color, &sprite->color);
    program->SetUniform(Uniform::Opacity, &opacity);
    program->SetUniform(Uniform::TextureMap, &texture_unit);
    program->SetUniform("u_Projection", &projection);
    program->SetUniform("u_Size", &size);
    program->SetUniform("u_UVRect", &uv_rect);
    program->UpdateUniforms();

    const auto index_count = static_cast<GLsizei>(geometry->GetIndexData().size());
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);

    rendered_objects_counter_++;
}

auto Renderer::Impl::Clear(RenderTarget* target) -> void {
    target == nullptr ? scene_buffer_.Begin() : framebuffers_.Begin(target);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    framebuffers_.Reset();
}

auto Renderer::Impl::SetViewport(int x, int y, int width, int height, Vector2 content_scale) -> void {
    viewport_width_ = width;
    viewport_height_ = height;
    state_.SetViewport(x, y, width, height);
    scene_buffer_.ResizeViewport(width, height);
    frame_.content_scale = content_scale;
}

auto Renderer::Impl::SetClearColor(const Color& color) -> void {
    state_.SetClearColor(color);
}

auto Renderer::Impl::SetAutoClear(bool auto_clear) -> void {
    auto_clear_ = auto_clear;
}

auto Renderer::Impl::SetToneMapping(ToneMapping tone_mapping) -> void {
    tone_mapping_ = tone_mapping;
}

auto Renderer::Impl::SetExposure(float exposure) -> void {
    exposure_ = exposure;
}

auto Renderer::Impl::SetShadowMap(ShadowMap shadow_map) -> void {
    if (shadow_map == shadow_map_) {
        return;
    }

    shadow_map_ = shadow_map;
    if (shadow_map_ == ShadowMap::None) {
        shadow_maps_.Clear();
    }
}

Renderer::Impl::~Impl() = default;

}
