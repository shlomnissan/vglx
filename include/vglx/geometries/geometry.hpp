/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/core/disposable.hpp"
#include "vglx/geometries/buffer_attribute.hpp"
#include "vglx/math/box3.hpp"
#include "vglx/math/matrix4.hpp"
#include "vglx/math/sphere.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace vglx {

/**
 * @brief Represents renderable mesh data as a set of named vertex attributes.
 *
 * Geometry is the core unit of renderable mesh data. It stores a
 * collection of named @ref BufferAttribute "buffer attributes" (positions,
 * normals, UVs, tangents and so on) alongside an optional index buffer. The
 * renderer matches attributes to shader inputs by name, so custom attributes
 * only need a matching declaration in shader code.
 *
 * All attributes must share the same element count, which defines the
 * geometry's vertex count. A geometry can be rendered using different
 * primitive types (triangles, lines, line loops) and provide access to
 * derived bounding volumes such as axis-aligned bounding boxes and bounding
 * spheres, useful for frustum culling or collision queries.
 *
 * Instances are usually created via the static @ref Geometry::Create factory
 * method and populated with attributes using @ref Geometry::AddAttribute.
 *
 * @code
 * auto geometry = vglx::Geometry::Create();
 *
 * geometry->AddAttribute(vglx::BufferAttribute::Create({
 *   .name = vglx::BufferAttribute::kPosition,
 *   .format = vglx::BufferAttribute::Format::Float32x3,
 *   .rate = vglx::BufferAttribute::Rate::Vertex
 * }, {
 *   0.5f, -0.5f, 0.0f,
 *   0.0f,  0.5f, 0.0f,
 *  -0.5f, -0.5f, 0.0f,
 * }));
 *
 * auto material = vglx::PhongMaterial::Create({.color = 0x049EF4u});
 *
 * my_scene->Add(vglx::Mesh::Create(geometry, material));
 * @endcode
 *
 * @ingroup GeometryGroup
 */
class VGLX_EXPORT Geometry : public Disposable {
public:
    /**
     * @brief Primitive topology used when rendering this geometry.
     *
     * Controls how the vertex and index data are interpreted by the renderer.
     */
    enum class PrimitiveType {
        Triangles, ///< Render as triangle list.
        Lines, ///< Render as line list.
        LineLoop ///< Render as a closed line loop.
    };

    /// @brief Primitive topology used by this geometry (triangles by default).
    PrimitiveType primitive { PrimitiveType::Triangles };

    /**
     * @brief Constructs an empty geometry.
     *
     * The geometry is created without any attributes or index data. Data can
     * be assigned later using @ref AddAttribute and @ref SetIndices.
     */
    Geometry() = default;

    /**
     * @brief Creates a shared instance of an empty @ref Geometry.
     *
     * The geometry has no attributes or index data and must be populated before rendering.
     */
    [[nodiscard]] static auto Create() -> std::shared_ptr<Geometry> {
        return std::make_shared<Geometry>();
    }

    /**
     * @brief Adds a vertex attribute to this geometry.
     *
     * The attribute must have a vertex rate, a unique name within the
     * geometry and an element count that matches any previously added
     * attributes. Attributes that violate these constraints are reported
     * and rejected.
     *
     * @param attribute Buffer attribute to add.
     */
    auto AddAttribute(std::shared_ptr<BufferAttribute> attribute) -> void;

    /**
     * @brief Replaces the index buffer.
     *
     * Indices reference elements in the geometry's vertex attributes. Passing
     * an empty list makes the geometry non-indexed.
     *
     * @param index_data Index buffer (empty for non-indexed geometry).
     */
    auto SetIndices(std::vector<uint32_t> index_data) -> void;

    /**
     * @brief Returns the list of vertex attributes added to this geometry.
     */
    [[nodiscard]] auto GetAttributes() const -> const std::vector<std::shared_ptr<BufferAttribute>>& { return attributes_; }

    /**
     * @brief Returns the vertex attribute with the given name.
     *
     * @param name Attribute name to look up.
     * @return The matching attribute, or `nullptr` if none exists.
     */
    [[nodiscard]] auto GetAttribute(std::string_view name) const -> std::shared_ptr<BufferAttribute>;

    /**
     * @brief Generates vertex normals from the position attribute.
     *
     * Normals are computed per face and averaged across shared vertices,
     * producing smooth normals for indexed geometry and flat normals for
     * non-indexed geometry.
     */
    auto GenerateNormals() -> void;

    /**
     * @brief Generates vertex tangents from the position, normal, and
     * texture coordinate attributes.
     *
     * Tangents are computed per face from the UV parameterization, averaged
     * across shared vertices, and stored with handedness in `w`.
     */
    auto GenerateTangents() -> void;

    /**
     * @brief Applies a transform to the geometry's vertex data.
     *
     * Positions are transformed by the matrix, normals by its inverse
     * transpose and tangents by its upper $3 \times 3$ preserving handedness.
     * Other attributes are left unchanged.
     *
     * @param transform Affine transform to apply.
     */
    auto ApplyTransform(const Matrix4& transform) -> void;

    /**
     * @brief Returns an axis-aligned bounding box that encloses the geometry.
     *
     * The box is computed from the position attribute and cached. It is
     * recomputed automatically when the position data changes.
     */
    [[nodiscard]] auto BoundingBox() -> Box3;

    /**
     * @brief Returns a bounding sphere that encloses the geometry.
     *
     * The sphere is computed from the position attribute and cached. It is
     * recomputed automatically when the position data changes.
     */
    [[nodiscard]] auto BoundingSphere() -> Sphere;

    /**
     * @brief Returns the number of vertices in the geometry.
     *
     * The vertex count is derived from the element count of the geometry's
     * attributes. For a geometry with no attributes this value is zero.
     */
    [[nodiscard]] auto VertexCount() const -> uint32_t;

    /**
     * @brief Checks whether the geometry has a position attribute.
     */
    [[nodiscard]] auto HasPositions() const -> bool;

    /**
     * @brief Returns a read-only reference to the index buffer.
     *
     * If the geometry is non-indexed, this buffer is empty.
     */
    [[nodiscard]] auto GetIndexData() const -> const std::vector<uint32_t>& { return index_data_; }

    /**
     * @brief Returns the index version incremented when indices are replaced.
     *
     * The renderer compares versions to decide when the index buffer needs to
     * be re-uploaded.
     */
    [[nodiscard]] auto GetIndexVersion() const -> uint32_t { return index_version_; }

    /**
     * @brief Returns the largest index in the index buffer.
     *
     * Used by the renderer to validate that indices reference valid vertices.
     * For non-indexed geometry this value is zero.
     */
    [[nodiscard]] auto GetMaxIndex() const -> uint32_t { return max_index_; }

private:
    /// @cond INTERNAL
    std::vector<std::shared_ptr<BufferAttribute>> attributes_ {};

    std::vector<uint32_t> index_data_ {};

    std::optional<std::pair<Box3, uint32_t>> bounding_box_ {};

    std::optional<std::pair<Sphere, uint32_t>> bounding_sphere_ {};

    uint32_t index_version_ {0};

    uint32_t max_index_ {0};
    /// @endcond
};

}
