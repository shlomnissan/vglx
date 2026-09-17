/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/scene/renderable.hpp"

#include "utilities/logger.hpp"

namespace vglx {

auto Renderable::BoundingBox() -> Box3 {
    return GetGeometry()->BoundingBox();
}

auto Renderable::BoundingSphere() -> Sphere {
    return GetGeometry()->BoundingSphere();
}

auto Renderable::CanRender() const -> bool {
    const auto geometry = GetGeometry();
    const auto material = GetMaterial();

    if (geometry == nullptr || material == nullptr) {
        return false;
    }

    if (geometry->Disposed()) {
        Logger::Log(LogLevel::Error, "Skipped rendering a node with disposed geometry {}", *this);
        return false;
    }

    if (geometry->VertexCount() == 0) {
        Logger::Log(LogLevel::Error, "Skipped node with no geometry data {}", *this);
        return false;
    }

    if (!geometry->HasPositions()) {
        Logger::Log(LogLevel::Error, "Skipped node with no vertex positions {}", *this);
        return false;
    }

    const auto node_type = GetNodeType();
    const auto material_type = material->GetType();

    if (node_type == Node::Type::Billboard && material_type != Material::Type::BillboardMaterial) {
        Logger::Log(LogLevel::Error, "Skipped billboard with non-billboard material {}", *this);
        return false;
    }

    if (material_type == Material::Type::BillboardMaterial && node_type != Node::Type::Billboard) {
        Logger::Log(LogLevel::Error, "Skipped non-billboard node with billboard material {}", *this);
        return false;
    }

    return true;
}

auto Renderable::InFrustum(const Frustum& frustum) -> bool {
    auto bounding_sphere = BoundingSphere();
    bounding_sphere.ApplyTransform(GetCachedWorldTransform());
    return frustum.IntersectsWithSphere(bounding_sphere);
}

}
