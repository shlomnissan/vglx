/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx/core/renderer.hpp"
#include "vglx/core/window.hpp"
#include "vglx/materials/depth_material.hpp"
#include "vglx/math/matrix4.hpp"
#include "vglx/math/vector2.hpp"
#include "vglx/scene/renderable.hpp"
#include "vglx/utilities/timer.hpp"

#include "renderer/gl/gl_background_pass.hpp"
#include "renderer/gl/gl_binding_state.hpp"
#include "renderer/gl/gl_buffers.hpp"
#include "renderer/gl/gl_device.hpp"
#include "renderer/gl/gl_environment.hpp"
#include "renderer/gl/gl_framebuffers.hpp"
#include "renderer/gl/gl_lights.hpp"
#include "renderer/gl/gl_present_pass.hpp"
#include "renderer/gl/gl_programs.hpp"
#include "renderer/gl/gl_scene_buffer.hpp"
#include "renderer/gl/gl_shadow_maps.hpp"
#include "renderer/gl/gl_state.hpp"
#include "renderer/gl/gl_textures.hpp"
#include "renderer/gl/gl_uniform_buffer.hpp"

#include <expected>
#include <memory>
#include <string>

namespace vglx {

class Canvas;
class CanvasRenderList;
class Renderable2D;
class RenderLists;
class RenderTarget;

struct alignas(16) FrameUniforms {
    Vector2 resolution {0.0f};
    Vector2 content_scale {1.0f};
    float time {0.0f};
};

struct alignas(16) CameraUniforms {
    Matrix4 projection {1.0f};
    Matrix4 view {1.0f};
};

class Renderer::Impl {
public:
    explicit Impl(const Renderer::Parameters& params);

    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;

    auto operator=(const Impl&) -> Impl& = delete;
    auto operator=(Impl&&) -> Impl& = delete;

    [[nodiscard]] auto Initialize(Window::Impl& window) -> std::expected<void, std::string>;

    auto Render(Scene* scene, Camera* camera, RenderTarget* target = nullptr) -> void;

    auto Clear(RenderTarget* target = nullptr) -> void;

    auto SetViewport(const Viewport& viewport) -> void;

    auto SetClearColor(const Color& color) -> void;

    auto SetAutoClear(bool auto_clear) -> void;

    auto SetToneMapping(ToneMapping tone_mapping) -> void;

    auto SetExposure(float exposure) -> void;

    auto SetShadowMap(ShadowMap shadow_map) -> void;

    [[nodiscard]] auto RenderedObjectsPerFrame() const {
        return rendered_objects_per_frame_;
    }

    [[nodiscard]] auto GetLimits() const -> const Renderer::Limits& {
        return gl::limits();
    }

    [[nodiscard]] auto GetDriverInfo() const -> const Renderer::DriverInfo& {
        return gl::driver_info();
    }

    ~Impl();

private:
    GLBackgroundPass background_pass_;
    GLEnvironment environment_;
    GLLights lights_;
    GLPresentPass present_pass_;
    GLPrograms programs_;
    GLSceneBuffer scene_buffer_;
    GLShadowMaps shadow_maps_;
    GLState state_;
    GLTextures textures_;
    GLFramebuffers framebuffers_ {textures_};
    GLBuffers buffers_;
    GLBindingState binding_state_ {buffers_};

    FrameUniforms frame_ {};
    GLUniformBuffer frame_uniforms_ {"ub_Frame", sizeof(FrameUniforms)};
    Timer timer_ {true};

    CameraUniforms camera_ {};
    GLUniformBuffer camera_uniforms_ {"ub_Camera", sizeof(CameraUniforms)};

    Window::Impl* window_ {nullptr};

    Viewport viewport_ {};
    bool viewport_pinned_ {false};

    std::unique_ptr<RenderLists> render_lists_;
    std::unique_ptr<RenderLists> shadow_render_lists_;
    std::unique_ptr<CanvasRenderList> canvas_render_list_;

    std::shared_ptr<DepthMaterial> depth_material_;

    size_t rendered_objects_counter_ {0};
    size_t rendered_objects_per_frame_ {0};

    Renderer::ShadowMap shadow_map_ {Renderer::ShadowMap::None};

    bool auto_clear_ {true};

    Renderer::ToneMapping tone_mapping_ {Renderer::ToneMapping::None};

    GLEnvironmentMaps env_maps_ {};

    float exposure_ {1.0f};

    auto ProcessLights(Camera* camera) -> void;

    auto RenderObjects(Scene* scene, Camera* camera) -> void;

    auto RenderObject(Renderable* renderable, Scene* scene, Camera* camera) -> void;

    auto RenderShadowMaps(Scene* scene, Camera* camera) -> void;

    auto RenderCanvas(Canvas* canvas) -> void;

    auto RenderObject2D(Renderable2D* renderable, const Matrix3& projection) -> void;

    auto SetUniforms(
        GLProgram* program,
        ProgramAttributes* attrs,
        Renderable* renderable,
        Camera* camera,
        Scene* scene
    ) -> void;

    auto UpdateFrameUniforms() -> void;

    auto UpdateCameraUniforms(Camera* camera) -> void;

    auto SyncWithWindow() -> void;
};

}
