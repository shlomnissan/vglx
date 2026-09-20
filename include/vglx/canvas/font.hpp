/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/core/identity.hpp"
#include "vglx/math/rect.hpp"
#include "vglx/math/vector2.hpp"
#include "vglx/textures/texture_2d.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace vglx {

/**
 * @brief Bitmap font used to draw text in canvas space.
 *
 * A font pairs a texture atlas with the metrics needed to lay out text from
 * it. Each @ref Glyph names the region of the atlas holding one character
 * and how far to move the pen after drawing it, and kerning pairs adjust
 * the spacing between specific pairs of characters.
 *
 * Fonts are typically loaded from a font description rather than populated
 * directly. All metrics are in pixels at the size the atlas was authored
 * for, given by @ref size. To draw text at a different size, scale them by
 * the target size divided by @ref size.
 *
 * @ingroup CanvasGroup
 */
class VGLX_EXPORT Font : public Identity {
public:
    /**
     * @brief Metrics describing a single character of the font.
     */
    struct Glyph {
        /// @brief Sub-region of @ref page holding the character in pixels.
        Rect region;

        /// @brief Offset in pixels from the pen position to the region's top-left corner.
        Vector2 offset;

        /// @brief Distance in pixels to move the pen after drawing the character.
        float advance {0.0f};
    };

    /**
     * @brief Distance in pixels between the baselines of consecutive lines.
     *
     * Move the pen down by this amount when starting a new line.
     */
    float line_height {0.0f};

    /**
     * @brief Distance in pixels from the top of a line to the baseline.
     *
     * Glyph offsets are measured from the top of the line, so subtracting
     * this value gives a glyph's position relative to the baseline.
     */
    float base {0.0f};

    /// @brief Size in pixels the atlas was authored at.
    float size {0.0f};

    /**
     * @brief Texture atlas holding the rendered characters.
     *
     * Glyph regions select rectangles of this texture with a top-left
     * origin. Only a single atlas page is supported. Because canvas content
     * is composited after tone mapping, load the atlas with
     * @ref Texture::ColorSpace "ColorSpace::Linear" so the pixel values are
     * used as authored.
     */
    std::shared_ptr<Texture2D> page {nullptr};

    /**
     * @brief Creates a shared instance of @ref Font.
     */
    [[nodiscard]] static auto Create() -> std::shared_ptr<Font> {
        return std::make_shared<Font>();
    }

    /**
     * @brief Adds a character to the font.
     *
     * Replaces the existing entry if a character with the same code point
     * was already added.
     *
     * @param code Unicode code point of the character.
     * @param glyph Metrics describing the character.
     */
    auto AddGlyph(char32_t code, const Glyph& glyph) -> void;

    /**
     * @brief Adds a kerning adjustment between two characters.
     *
     * Replaces the existing entry if the same pair was already added.
     *
     * @param first Code point of the character on the left.
     * @param second Code point of the character on the right.
     * @param amount Adjustment in pixels added to the advance of the first character.
     */
    auto AddKerning(char32_t first, char32_t second, float amount) -> void;

    /**
     * @brief Returns the glyph for the given character.
     *
     * @param code Unicode code point to look up.
     * @return The matching glyph, or `std::nullopt` if none exists.
     */
    [[nodiscard]] auto GetGlyph(char32_t code) const -> std::optional<Glyph>;

    /**
     * @brief Returns the kerning adjustment between two characters.
     *
     * @param first Code point of the character on the left.
     * @param second Code point of the character on the right.
     * @return The adjustment in pixels, or `0.0` if the pair has no kerning.
     */
    [[nodiscard]] auto GetKerning(char32_t first, char32_t second) const -> float;

private:
    /// @cond INTERNAL
    std::vector<std::pair<char32_t, Glyph>> glyphs_;
    std::vector<std::pair<uint64_t, float>> kerning_;
    /// @endcond
};

}
