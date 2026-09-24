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
#include "vglx/textures/texture_2d.hpp"

#include <memory>

namespace vglx {

/**
 * @brief Base class for canvas nodes that can be drawn.
 *
 * Renderable2D extends @ref Node2D with the interface the renderer needs to
 * issue a draw call. It is the common base for all drawable 2D node types,
 * such as @ref Sprite.
 *
 * The transforms let a node draw shared geometry without rewriting vertices.
 * A sprite draws a unit quad and supplies a transform that scales it to its
 * size and shifts it by its anchor, plus a texture transform that selects
 * its region. A node that owns geometry laid out in node space, such as
 * text, leaves both at identity.
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
     */
    [[nodiscard]] virtual auto GetGeometryTransform() const -> Matrix3 {
        return Matrix3 {1.0f};
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
};

}
