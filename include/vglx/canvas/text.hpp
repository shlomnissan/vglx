/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/canvas/font.hpp"
#include "vglx/canvas/renderable2d.hpp"
#include "vglx/geometries/geometry.hpp"
#include "vglx/math/vector2.hpp"
#include "vglx/textures/texture_2d.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace vglx {

/**
 * @brief Text drawn in canvas space from a bitmap font.
 *
 * Text lays out a UTF-8 string as one quad per glyph using its font's
 * metrics. The glyphs are rebuilt when the text, font, alignment, or line
 * spacing changes. The font is read only at that point, so it must be
 * complete before it is assigned. Later changes to it are not picked up.
 *
 * Metrics are in the font's atlas pixels so text draws at the size the
 * font was authored at until scaled through @ref transform. The node's
 * position is the top-left corner of the text's bounds unless the
 * @ref Renderable2D::anchor "anchor" moves it.
 *
 * @code
 * auto font = vglx::LoadFont("assets/fonts/roboto.fnt");
 * if (font.has_value()) {
 *   auto label = canvas.Add(vglx::Text::Create(font.value(), "Score 0"));
 *   label->anchor = {1.0f, 0.0f};
 *   label->transform.SetPosition({canvas.GetSize().x - 24.0f, 24.0f});
 * }
 * @endcode
 *
 * @ingroup CanvasGroup
 */
class VGLX_EXPORT Text : public Renderable2D {
public:
    /**
     * @brief Horizontal alignment of lines within the text's bounds.
     */
    enum class Alignment {
        Left, ///< Lines start at the left edge of the bounds.
        Center, ///< Lines are centered within the bounds.
        Right ///< Lines end at the right edge of the bounds.
    };

    /**
     * @brief Constructs a text node.
     *
     * @param font Font to lay out with or `nullptr` to assign one later.
     * @param text UTF-8 string to display.
     */
    Text(std::shared_ptr<Font> font, std::string_view text);

    /**
     * @brief Creates an instance of @ref Text.
     *
     * @param font Font to lay out with or `nullptr` to assign one later.
     * @param text UTF-8 string to display.
     */
    [[nodiscard]] static auto Create(std::shared_ptr<Font> font, std::string_view text) -> std::unique_ptr<Text> {
        return std::make_unique<Text>(std::move(font), text);
    }

    /**
     * @brief Replaces the displayed string.
     *
     * Characters the font has no glyph for are skipped, as are malformed UTF-8 sequences.
     *
     * @param text UTF-8 string to display.
     */
    auto SetText(std::string_view text) -> void;

    /**
     * @brief Returns the displayed string.
     */
    [[nodiscard]] auto GetText() const -> const std::string& {
        return text_;
    }

    /**
     * @brief Replaces the font.
     *
     * @param font Font to lay out with or `nullptr` to draw nothing.
     */
    auto SetFont(std::shared_ptr<Font> font) -> void;

    /**
     * @brief Returns the font.
     */
    [[nodiscard]] auto GetFont() const -> std::shared_ptr<Font> {
        return font_;
    }

    /**
     * @brief Sets the horizontal alignment of lines.
     *
     * @param alignment Alignment to apply.
     */
    auto SetAlignment(Alignment alignment) -> void;

    /**
     * @brief Returns the horizontal alignment of lines.
     */
    [[nodiscard]] auto GetAlignment() const -> Alignment {
        return alignment_;
    }

    /**
     * @brief Sets the spacing between lines as a multiple of the font's line height.
     *
     * @param spacing Multiplier applied to the font's line height. Defaults to `1.0`.
     */
    auto SetLineSpacing(float spacing) -> void;

    /**
     * @brief Returns the line spacing multiplier.
     */
    [[nodiscard]] auto GetLineSpacing() const -> float {
        return line_spacing_;
    }

    /**
     * @brief Returns the size of the text's bounds in canvas units before scaling.
     *
     * The width is the widest line measured by glyph advances and kerning,
     * and the height covers every line. Returns zero for an empty string or
     * a missing font.
     */
    [[nodiscard]] auto GetSize() const -> Vector2 override {
        return size_;
    }

    /**
     * @brief Identifies this node as @ref Node2D::Type "Node2D::Type::Text".
     */
    [[nodiscard]] auto GetNodeType() const -> Node2D::Type override {
        return Node2D::Type::Text;
    }

    /**
     * @brief Returns the glyph quads laid out in canvas units.
     *
     * The geometry may be replaced when the text or font changes so the
     * returned pointer should not be cached.
     */
    [[nodiscard]] auto GetGeometry() -> std::shared_ptr<Geometry> override {
        return geometry_;
    }

    /**
     * @brief Returns the font's atlas page or `nullptr` without a font.
     */
    [[nodiscard]] auto GetTexture() const -> std::shared_ptr<Texture2D> override {
        return font_ != nullptr ? font_->page : nullptr;
    }

private:
    /// @cond INTERNAL
    Vector2 size_ {0.0f, 0.0f};

    Alignment alignment_ {Alignment::Left};

    std::shared_ptr<Font> font_;

    std::shared_ptr<Geometry> geometry_;

    std::string text_;

    std::size_t capacity_ {0};

    float line_spacing_ {1.0f};

    auto Layout() -> void;

    auto EnsureCapacity(std::size_t glyphs) -> void;
    /// @endcond
};

}
