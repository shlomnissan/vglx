/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/canvas/node2d.hpp"
#include "vglx/geometries/geometry.hpp"
#include "vglx/math/color.hpp"
#include "vglx/math/matrix3.hpp"
#include "vglx/math/vector2.hpp"
#include "vglx/textures/texture_2d.hpp"

#include <memory>

namespace vglx {

/**
 * @brief Base class for canvas nodes that can be drawn.
 *
 * Renderable2D extends @ref Node2D with the interface the renderer needs to
 * issue a draw call. It is the common base for all drawable 2D node types,
 * such as @ref Sprite and @ref Text.
 *
 * The transforms let a node draw shared geometry without rewriting vertices.
 * A sprite draws a unit quad and supplies a transform that scales it to its
 * size, plus a texture transform that selects its region. A node that owns
 * geometry laid out in node space, such as text, keeps the defaults, which
 * shift the geometry by the anchor and leave texture coordinates alone.
 *
 * This class is not instantiated directly. Use one of the concrete node
 * types, or derive from it to implement a custom drawable.
 *
 * @ingroup CanvasGroup
 */
class VGLX_EXPORT Renderable2D : public Node2D {
public:
    /// @brief Color multiplied with the texture. Defaults to white.
    Color color {0xFFFFFFu};

    /**
     * @brief Normalized anchor point inside the node's bounds.
     *
     * Defines which point of the bounds as reported by @ref GetSize is
     * placed at the node's position.
     *
     * - `(0.0, 0.0)` top-left corner of the bounds (default).
     * - `(0.5, 0.5)` center of the bounds.
     * - `(1.0, 0.0)` top-right corner, for right-aligned labels.
     */
    Vector2 anchor {0.0f, 0.0f};

    /**
     * @brief Returns the node's size in canvas units before scaling.
     *
     * The @ref anchor is measured against this size. Returns zero when
     * there is nothing to draw.
     */
    [[nodiscard]] virtual auto GetSize() const -> Vector2 = 0;

    /**
     * @brief Returns the geometry used to draw this node.
     */
    [[nodiscard]] virtual auto GetGeometry() -> std::shared_ptr<Geometry> = 0;

    /**
     * @brief Returns the texture sampled by the geometry or `nullptr` when there's
     * nothing to draw.
     */
    [[nodiscard]] virtual auto GetTexture() const -> std::shared_ptr<Texture2D> = 0;

    /**
     * @brief Returns the transform that maps geometry vertices into node space.
     *
     * The default shifts the geometry by the @ref anchor. Nodes that draw
     * shared geometry compose their own scaling.
     */
    [[nodiscard]] virtual auto GetGeometryTransform() const -> Matrix3 {
        return AnchorTranslation();
    }

    /**
     * @brief Returns the transform that maps geometry texture coordinates into the texture.
     */
    [[nodiscard]] virtual auto GetTextureTransform() const -> Matrix3 {
        return Matrix3 {1.0f};
    }

    /**
     * @brief Identifies this node as @ref Node2D::Type "Node2D::Type::Renderable".
     */
    [[nodiscard]] auto GetNodeType() const -> Node2D::Type override {
        return Node2D::Type::Renderable;
    }

    /**
     * @brief Returns `true`, identifying this node as renderable.
     */
    [[nodiscard]] auto IsRenderable() const -> bool override {
        return true;
    }

    ~Renderable2D() override = default;

protected:
    Renderable2D() = default;

    [[nodiscard]] auto AnchorTranslation() const -> Matrix3 {
        const auto size = GetSize();
        return Matrix3 {
            1.0f, 0.0f, -anchor.x * size.x,
            0.0f, 1.0f, -anchor.y * size.y,
            0.0f, 0.0f, 1.0f
        };
    }
};

}
