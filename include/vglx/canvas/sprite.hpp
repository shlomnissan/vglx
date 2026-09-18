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
#include "vglx/math/rect.hpp"
#include "vglx/math/vector2.hpp"
#include "vglx/textures/texture_2d.hpp"

#include <memory>
#include <optional>

namespace vglx {

/**
 * @brief Textured quad drawn in canvas space.
 *
 * Sprite draws a @ref Texture2D or a rectangular @ref region of it at the
 * node's position. The sprite's size in canvas units matches the texture or
 * region size in pixels so a sprite draws at its natural size until scaled
 * through @ref transform. The @ref anchor selects which point of the quad
 * sits at the node's position. The default is the top-left corner.
 *
 * Sprites are composited after tone mapping in display space. Load their
 * textures with @ref Texture::ColorSpace "ColorSpace::Linear" so the pixel
 * values are used as authored.
 *
 * @code
 * using enum vglx::Texture::ColorSpace;
 *
 * auto texture = vglx::LoadTexture("assets/icon.png", Linear);
 * if (texture.has_value()) {
 *   auto sprite = canvas.Add(vglx::Sprite::Create(texture.value()));
 *   sprite->transform.SetPosition({20.0f, 20.0f});
 * }
 * @endcode
 *
 * @ingroup CanvasGroup
 */
class VGLX_EXPORT Sprite : public Node2D {
public:
    /// @brief Texture drawn by the sprite. A null texture draws nothing.
    std::shared_ptr<Texture2D> texture;

    /**
     * @brief Optional sub-region of the texture to draw in pixels.
     *
     * Selects a rectangle of the texture with a top-left origin which is
     * how a frame is picked out of a sprite sheet. When unset the whole
     * texture is drawn.
     */
    std::optional<Rect> region;

    /**
     * @brief Normalized anchor point inside the sprite.
     *
     * Defines which point of the quad is placed at the node's position.
     * Because that point sits at the origin it also acts as the pivot
     * for rotation and scale unless @ref Transform2::center is set.
     *
     * - `(0.0, 0.0)` top-left corner of the sprite (default).
     * - `(0.5, 0.5)` center of the sprite.
     * - `(1.0, 1.0)` bottom-right corner of the sprite.
     */
    Vector2 anchor {0.0f, 0.0f};

    /// @brief Color multiplied with the texture. Defaults to white.
    Color color {0xFFFFFFu};

    /**
     * @brief Constructs a sprite.
     *
     * @param texture Texture to draw or `nullptr` to assign one later.
     */
    explicit Sprite(std::shared_ptr<Texture2D> texture);

    /**
     * @brief Creates an instance of @ref Sprite.
     *
     * @param texture Texture to draw or `nullptr` to assign one later.
     */
    [[nodiscard]] static auto
    Create(std::shared_ptr<Texture2D> texture) -> std::unique_ptr<Sprite> {
        return std::make_unique<Sprite>(std::move(texture));
    }

    /**
     * @brief Returns the sprite's size in canvas units before scaling.
     *
     * Equals the region size when a region is set, otherwise the texture's
     * dimensions in pixels. Returns zero when there is no texture.
     */
    [[nodiscard]] auto GetSize() const -> Vector2;

    /**
     * @brief Identifies this node as @ref Node2D::Type "Node2D::Type::Sprite".
     */
    [[nodiscard]] auto GetNodeType() const -> Node2D::Type override {
        return Node2D::Type::Sprite;
    }

    /**
     * @brief Returns `true`, identifying this node as renderable.
     */
    [[nodiscard]] auto IsRenderable() const -> bool override {
        return true;
    }

    /// @cond INTERNAL
    [[nodiscard]] auto GetGeometry() const -> std::shared_ptr<Geometry> {
        return SharedGeometry();
    }
    /// @endcond

private:
    /// @cond INTERNAL
    static auto SharedGeometry() -> std::shared_ptr<Geometry>&;
    /// @endcond
};

}
