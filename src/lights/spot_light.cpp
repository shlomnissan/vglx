/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/lights/spot_light.hpp"

#include "vglx/geometries/buffer_attribute.hpp"
#include "vglx/geometries/geometry.hpp"
#include "vglx/materials/unlit_material.hpp"
#include "vglx/math/utilities.hpp"
#include "vglx/math/vector3.hpp"
#include "vglx/scene/mesh.hpp"

#include "utilities/assert.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace {

auto lines_geometry() {
    // lines for the cone
    auto points = std::vector<float> {
        0, 0, 0,  0,  0, 1,
        0, 0, 0,  1,  0, 1,
        0, 0, 0, -1,  0, 1,
        0, 0, 0,  0,  1, 1,
        0, 0, 0,  0, -1, 1,
    };

    // circle for the cone base
    static constexpr auto circle_line_segments = 64;
    for (unsigned i = 0, j = 1, l = circle_line_segments; i < l; i++, j++) {
        const auto p1 = (static_cast<float>(i) / static_cast<float>(l)) * vglx::math::two_pi;
        const auto p2 = (static_cast<float>(j) / static_cast<float>(l)) * vglx::math::two_pi;
        points.insert(points.end(), {
            vglx::math::Cos(p1), vglx::math::Sin(p1), 1.0f,
            vglx::math::Cos(p2), vglx::math::Sin(p2), 1.0f
        });
    }

    auto geometry = vglx::Geometry::Create();
    geometry->AddAttribute(vglx::BufferAttribute::Create({
        .name = vglx::BufferAttribute::kPosition,
        .format = vglx::BufferAttribute::Format::Float32x3,
        .rate = vglx::BufferAttribute::Rate::Vertex
    }, std::move(points)));
    geometry->SetName("spot light cone");
    geometry->primitive = vglx::Geometry::PrimitiveType::Lines;
    return geometry;
}

}

namespace vglx {

struct SpotLight::Impl {
    Mesh* cone {nullptr};

    std::shared_ptr<UnlitMaterial> material;

    auto CreateDebugMesh(SpotLight* self) -> void {
        material = UnlitMaterial::Create();
        material->side = Material::Side::TwoSided;
        material->color = self->color;
        material->fog = false;

        cone = self->Add(Mesh::Create(lines_geometry(), material));

        UpdateDebugMesh(self);
    }

    auto UpdateDebugMesh(SpotLight* self) -> void {
        const auto target_world_pos = self->target != nullptr
            ? self->target->GetWorldPosition()
            : Vector3::Zero();

        const auto cone_length = (target_world_pos - self->GetWorldPosition()).Length() + 1.0f;
        const auto cone_width = std::tan(self->angle) * cone_length;

        cone->LookAt(target_world_pos);
        cone->transform.SetScale({cone_width, cone_width, cone_length});
        material->color = self->color;
    }

    auto RemoveDebugMesh(SpotLight* self) -> void {
        const auto is_child = std::ranges::any_of(self->GetChildren(), [this](const auto& child) {
            return child.get() == cone;
        });

        if (is_child) {
            self->Remove(cone);
        }
        cone = nullptr;

        material.reset();
    }
};

SpotLight::SpotLight(const Parameters& params) :
    Light(params.color, params.intensity),
    angle(params.angle),
    penumbra(params.penumbra),
    target(params.target),
    range(params.range),
    cast_shadow(params.cast_shadow),
    impl_(std::make_unique<Impl>())
{
    SetName("spot light");
}

auto SpotLight::Direction() -> Vector3 {
    if (target == nullptr) {
        return Normalize(GetWorldPosition());
    }

    VGLX_ASSERT(
        target->GetScene() == GetScene(),
        "SpotLight target must belong to the same scene"
    );

    return Normalize(GetWorldPosition() - target->GetWorldPosition());
}

auto SpotLight::SetDebugMode(bool is_debug_mode) -> void {
    if (debug_mode_enabled_ != is_debug_mode) {
        is_debug_mode
        ? impl_->CreateDebugMesh(this)
        : impl_->RemoveDebugMesh(this);
        debug_mode_enabled_ = is_debug_mode;
    }
}

auto SpotLight::OnUpdate(float delta) -> void {
    if (debug_mode_enabled_) {
        impl_->UpdateDebugMesh(this);
    }
}

auto SpotLight::GetShadow() -> Shadow* {
    return cast_shadow ? &shadow : nullptr;
}

SpotLight::~SpotLight() = default;

}
