/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx/math/vector2.hpp"

namespace vglx {

/**
 * @brief Axis-aligned rectangle defined by an origin and a size.
 *
 * Rect represents a 2D region with a top-left origin and Y increasing
 * downward, matching canvas and image coordinates. It is used to select a
 * sub-region of a texture.
 *
 * @note This class is `constexpr` where possible allowing Rect instances to be
 * constructed and manipulated at compile time.
 *
 * @ingroup MathGroup
 */
struct Rect {
    /// @brief Left edge.
    float x {0.0f};

    /// @brief Top edge.
    float y {0.0f};

    /// @brief Width of the rectangle.
    float width {0.0f};

    /// @brief Height of the rectangle.
    float height {0.0f};

    /**
     * @brief Constructs an empty rectangle at the origin.
     */
    constexpr Rect() = default;

    /**
     * @brief Constructs a rectangle from its origin and size.
     *
     * @param x Left edge.
     * @param y Top edge.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     */
    constexpr Rect(float x, float y, float width, float height)
        : x(x), y(y), width(width), height(height) {}

    /**
     * @brief Returns the size of the rectangle.
     */
    [[nodiscard]] constexpr auto Size() const -> Vector2 {
        return {width, height};
    }

    constexpr auto operator==(const Rect&) const -> bool = default;
};

}
