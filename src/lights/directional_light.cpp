/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/lights/directional_light.hpp"

#include "vglx/geometries/buffer_attribute.hpp"
#include "vglx/geometries/geometry.hpp"
#include "vglx/materials/unlit_material.hpp"
#include "vglx/math/vector3.hpp"
#include "vglx/scene/mesh.hpp"

#include "utilities/assert.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace {

auto create_positions(std::vector<float> data) {
    return vglx::BufferAttribute::Create({
        .name = vglx::BufferAttribute::kPosition,
        .format = vglx::BufferAttribute::Format::Float32x3,
        .rate = vglx::BufferAttribute::Rate::Vertex
    }, std::move(data));
}

auto line_geometry() {
    auto geometry = vglx::Geometry::Create();
    geometry->AddAttribute(create_positions({0, 0, 0, 0, 0, 1}));
    geometry->primitive = vglx::Geometry::PrimitiveType::Lines;
    return geometry;
}

auto plane_geometry() {
    auto geometry = vglx::Geometry::Create();
    geometry->AddAttribute(create_positions({-1,  1, 0, 1,  1, 0, 1, -1, 0, -1, -1, 0}));
    geometry->primitive = vglx::Geometry::PrimitiveType::LineLoop;
    return geometry;
}

}

namespace vglx {

static constexpr auto debug_mesh_size = 0.5f;

struct DirectionalLight::Impl {
    Mesh* line {nullptr};
    Mesh* plane {nullptr};

    std::shared_ptr<UnlitMaterial> material;

    auto CreateDebugMesh(DirectionalLight* self) -> void {
        material = UnlitMaterial::Create();
        material->side = Material::Side::TwoSided;
        material->color = self->color;
        material->fog = false;

        line = self->Add(Mesh::Create(line_geometry(), material));
        plane = self->Add(Mesh::Create(plane_geometry(), material));
        plane->transform_auto_update = false;

        UpdateDebugMesh(self);
    }

    auto UpdateDebugMesh(DirectionalLight* self) -> void {
        const auto target_world_pos = self->target != nullptr
            ? self->target->GetWorldPosition()
            : Vector3::Zero();

        plane->LookAt(target_world_pos);
        line->LookAt(target_world_pos);
        material->color = self->color;

        const auto length = (target_world_pos - self->GetWorldPosition()).Length();
        plane->transform.SetScale(debug_mesh_size);
        line->transform.SetScale({1.0f, 1.0f, length});
    }

    auto RemoveDebugMesh(DirectionalLight* self) -> void {
        const auto is_child = [self](const Node* node) {
            return std::ranges::any_of(self->GetChildren(), [node](const auto& child) {
                return child.get() == node;
            });
        };

        if (is_child(line)) {
            self->Remove(line);
        }
        line = nullptr;

        if (is_child(plane)) {
            self->Remove(plane);
        }
        plane = nullptr;

        material.reset();
    }
};

DirectionalLight::DirectionalLight(const Parameters& params) :
    Light(params.color, params.intensity),
    target(params.target),
    cast_shadow(params.cast_shadow),
    impl_(std::make_unique<Impl>())
{
    SetName("directional light");
}

auto DirectionalLight::Direction() -> Vector3 {
    if (target == nullptr) {
        return Normalize(GetWorldPosition());
    }

    VGLX_ASSERT(
        target->GetScene() == GetScene(),
        "DirectionalLight target must belong to the same scene"
    );

    return Normalize(GetWorldPosition() - target->GetWorldPosition());
}

auto DirectionalLight::SetDebugMode(bool is_debug_mode) -> void {
    if (debug_mode_enabled_ != is_debug_mode) {
        is_debug_mode
        ? impl_->CreateDebugMesh(this)
        : impl_->RemoveDebugMesh(this);
        debug_mode_enabled_ = is_debug_mode;
    }
}

auto DirectionalLight::OnUpdate(float delta) -> void {
    if (debug_mode_enabled_) {
        impl_->UpdateDebugMesh(this);
    }
}

auto DirectionalLight::GetShadow() -> Shadow* {
    return cast_shadow ? &shadow : nullptr;
}

DirectionalLight::~DirectionalLight() = default;

}
