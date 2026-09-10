/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/geometries/geometry.hpp"

#include "vglx/math/matrix3.hpp"
#include "vglx/math/utilities.hpp"
#include "vglx/math/vector3.hpp"

#include "geometries/generate_attributes.hpp"
#include "utilities/logger.hpp"

#include <algorithm>
#include <cstddef>
#include <format>

namespace vglx {

namespace {

auto create_bounding_box(const std::vector<float>& positions) {
    auto box = Box3 {};
    for (auto i = std::size_t {0}; i + 2 < positions.size(); i += 3) {
        box.ExpandWithPoint({
            positions[i],
            positions[i + 1],
            positions[i + 2]
        });
    }
    return box;
}

auto create_bounding_sphere(const std::vector<float>& positions, const Vector3& center) {
    auto max_distance_squared = 0.0f;
    for (auto i = std::size_t {0}; i + 2 < positions.size(); i += 3) {
        auto point = Vector3 {
            positions[i],
            positions[i + 1],
            positions[i + 2]
        };
        max_distance_squared = std::max(max_distance_squared, (center - point).LengthSquared());
    }
    return Sphere {center, math::Sqrt(max_distance_squared)};
}

}

auto Geometry::AddAttribute(std::shared_ptr<BufferAttribute> attribute) -> void {
    if (attribute == nullptr) return;

    auto error = [name = attribute->name](std::string_view message) {
        Logger::Log(LogLevel::Error, "Failed to add attribute {}. {}", name, message);
    };

    if (attribute->Disposed()) {
        error("Attribute is marked as disposed");
        return;
    }

    if (attribute->rate == BufferAttribute::Rate::Instance) {
        error("Instanced attributes should be added to InstancedMesh objects");
        return;
    }

    if (!attribute->IsValid()) {
        error("Invalid attribute, missing name or data");
        return;
    }

    if (GetAttribute(attribute->name) != nullptr) {
        error("An attribute with this name already exists");
        return;
    }

    auto element_count = VertexCount();
    if (element_count > 0 && element_count != attribute->ElementCount()) {
        error(std::format("Element count mismatch. Expecting {} elements", element_count));
        return;
    }

    attributes_.emplace_back(std::move(attribute));
}

auto Geometry::SetIndices(std::vector<uint32_t> index_data) -> void {
    index_data_ = std::move(index_data);

    auto it = std::ranges::max_element(index_data_);
    max_index_ = it != index_data_.end() ? *it : 0;

    index_version_++;
}

auto Geometry::GetAttribute(std::string_view name) const -> std::shared_ptr<BufferAttribute> {
    auto it = std::ranges::find(attributes_, name, &BufferAttribute::name);
    return it != attributes_.end() ? *it : nullptr;
}

auto Geometry::VertexCount() const -> uint32_t {
    return attributes_.empty() ? 0 : attributes_.front()->ElementCount();
}

auto Geometry::HasPositions() const -> bool {
    return GetAttribute(BufferAttribute::kPosition) != nullptr;
}

auto Geometry::GenerateNormals() -> void {
    auto position_attribute = GetAttribute(BufferAttribute::kPosition);
    if (position_attribute == nullptr || position_attribute->GetData().empty()) {
        Logger::Log(LogLevel::Error, "Failed to generate normals. Missing vertex position buffer");
        return;
    }

    auto normals = generate_normals(position_attribute->GetData(), index_data_);

    if (auto normal_attribute = GetAttribute(BufferAttribute::kNormal)) {
        normal_attribute->SetData(std::move(normals));
    } else {
        AddAttribute(BufferAttribute::Create({
            .name = BufferAttribute::kNormal,
            .format = BufferAttribute::Format::Float32x3,
            .rate = BufferAttribute::Rate::Vertex
        }, std::move(normals)));
    }
}

auto Geometry::GenerateTangents() -> void {
    auto position_attribute = GetAttribute(BufferAttribute::kPosition);
    if (position_attribute == nullptr || position_attribute->GetData().empty()) {
        Logger::Log(LogLevel::Error, "Failed to generate tangents. Missing vertex position buffer");
        return;
    }

    auto normal_attribute = GetAttribute(BufferAttribute::kNormal);
    if (normal_attribute == nullptr || normal_attribute->GetData().empty()) {
        Logger::Log(LogLevel::Error, "Failed to generate tangents. Missing vertex normal buffer");
        return;
    }

    auto uv_attribute = GetAttribute(BufferAttribute::kTexCoord);
    if (uv_attribute == nullptr || uv_attribute->GetData().empty()) {
        Logger::Log(LogLevel::Error, "Failed to generate tangents. Missing texture coordinate buffer");
        return;
    }

    auto tangents = generate_tangents(
        position_attribute->GetData(),
        normal_attribute->GetData(),
        uv_attribute->GetData(),
        index_data_
    );

    if (auto tangent_attribute = GetAttribute(BufferAttribute::kTangent)) {
        tangent_attribute->SetData(std::move(tangents));
    } else {
        AddAttribute(BufferAttribute::Create({
            .name = BufferAttribute::kTangent,
            .format = BufferAttribute::Format::Float32x4,
            .rate = BufferAttribute::Rate::Vertex
        }, std::move(tangents)));
    }
}

auto Geometry::ApplyTransform(const Matrix4& transform) -> void {
    using enum BufferAttribute::Format;

    auto position_attr = GetAttribute(BufferAttribute::kPosition);
    if (!position_attr || position_attr->format != Float32x3 || position_attr->GetData().empty()) {
        Logger::Log(LogLevel::Error, "Failed to apply transform. Missing or invalid vertex position buffer");
        return;
    }

    auto linear_transform = Matrix3 {transform};
    if (Determinant(linear_transform) == 0.0f) {
        Logger::Log(LogLevel::Error, "Failed to apply transform. Singular transformation cannot be applied to normals");
        return;
    }

    auto positions = position_attr->GetData();
    for (auto i = std::size_t {0}; i < positions.size(); i += position_attr->Components()) {
        auto p = transform * Vector3 {positions[i + 0], positions[i + 1], positions[i + 2]};
        positions[i + 0] = p.x; positions[i + 1] = p.y; positions[i + 2] = p.z;
    }

    position_attr->SetData(std::move(positions));

    auto normal_attr = GetAttribute(BufferAttribute::kNormal);
    auto has_normals = normal_attr && normal_attr->format == Float32x3 && !normal_attr->GetData().empty();
    if (has_normals) {
        auto normal_matrix = Transpose(Inverse(linear_transform));
        auto normals = normal_attr->GetData();
        for (auto i = std::size_t {0}; i < normals.size(); i += normal_attr->Components()) {
            auto n = Normalize(normal_matrix * Vector3 {normals[i + 0], normals[i + 1], normals[i + 2]});
            normals[i + 0] = n.x; normals[i + 1] = n.y; normals[i + 2] = n.z;
        }
        normal_attr->SetData(std::move(normals));
    }

    auto tangent_attr = GetAttribute(BufferAttribute::kTangent);
    auto has_tangents = tangent_attr && tangent_attr->format == Float32x4 && !tangent_attr->GetData().empty();
    if (has_tangents) {
        auto handedness_sign = Determinant(linear_transform) < 0.0f ? -1.0f : 1.0f;
        auto tangents = tangent_attr->GetData();
        for (auto i = std::size_t {0}; i < tangents.size(); i += tangent_attr->Components()) {
            auto t = Normalize(linear_transform * Vector3 {tangents[i + 0], tangents[i + 1], tangents[i + 2]});
            tangents[i + 0] = t.x; tangents[i + 1] = t.y; tangents[i + 2] = t.z;
            tangents[i + 3] *= handedness_sign;
        }
        tangent_attr->SetData(std::move(tangents));
    }
}

auto Geometry::BoundingBox() -> Box3 {
    auto position_attribute = GetAttribute(BufferAttribute::kPosition);
    if (position_attribute == nullptr || position_attribute->GetData().empty()) {
        Logger::Log(LogLevel::Error, "Failed to generate bounding box. Missing vertex position buffer");
        return {};
    }

    if (bounding_box_ && bounding_box_->second == position_attribute->GetVersion()) {
        return bounding_box_->first;
    }

    bounding_box_ = {
        create_bounding_box(position_attribute->GetData()),
        position_attribute->GetVersion()
    };

    return bounding_box_->first;
}

auto Geometry::BoundingSphere() -> Sphere {
    auto position_attribute = GetAttribute(BufferAttribute::kPosition);
    if (position_attribute == nullptr || position_attribute->GetData().empty()) {
        Logger::Log(LogLevel::Error, "Failed to generate bounding sphere. Missing vertex position buffer");
        return {};
    }

    if (bounding_sphere_ && bounding_sphere_->second == position_attribute->GetVersion()) {
        return bounding_sphere_->first;
    }

    bounding_sphere_ = {
        create_bounding_sphere(position_attribute->GetData(), BoundingBox().Center()),
        position_attribute->GetVersion()
    };

    return bounding_sphere_->first;
}

}
