/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/lights/point_light.hpp"

#include "vglx/materials/unlit_material.hpp"
#include "vglx/primitives/sphere_geometry.hpp"
#include "vglx/scene/mesh.hpp"

#include <algorithm>

namespace vglx {

static constexpr auto debug_mesh_size = 0.2f;

struct PointLight::Impl {
    Mesh* sphere {nullptr};

    std::shared_ptr<UnlitMaterial> material;

    auto CreateDebugMesh(PointLight* self) -> void {
        material = UnlitMaterial::Create();
        material->side = Material::Side::TwoSided;
        material->color = self->color;
        material->wireframe = true;
        material->fog = false;

        sphere = self->Add(
            Mesh::Create(SphereGeometry::Create({
                .radius = 1,
                .width_segments = 4,
                .height_segments = 2
            }), material)
        );

        UpdateDebugMesh(self);
    }

    auto UpdateDebugMesh(PointLight* self) -> void {
        sphere->transform.SetScale(debug_mesh_size);
        material->color = self->color;
    }

    auto RemoveDebugMesh(PointLight* self) -> void {
        const auto is_child = std::ranges::any_of(self->GetChildren(), [this](const auto& child) {
            return child.get() == sphere;
        });

        if (is_child) {
            self->Remove(sphere);
        }
        sphere = nullptr;

        material.reset();
    }
};

PointLight::PointLight(const Parameters& params)  :
    Light(params.color, params.intensity),
    range(params.range),
    cast_shadow(params.cast_shadow),
    impl_(std::make_unique<Impl>())
{
    SetName("point light");
}

auto PointLight::SetDebugMode(bool is_debug_mode) -> void {
    if (debug_mode_enabled_ != is_debug_mode) {
        is_debug_mode
        ? impl_->CreateDebugMesh(this)
        : impl_->RemoveDebugMesh(this);
        debug_mode_enabled_ = is_debug_mode;
    }
}

auto PointLight::OnUpdate(float delta) -> void {
    if (debug_mode_enabled_) {
        impl_->UpdateDebugMesh(this);
    }
}

auto PointLight::GetShadow() -> Shadow* {
    return cast_shadow ? &shadow : nullptr;
}

PointLight::~PointLight() = default;

}
