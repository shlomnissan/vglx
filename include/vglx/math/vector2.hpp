/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/math/utilities.hpp"

#include <algorithm>
#include <cassert>

namespace vglx {

struct Vector2;
auto constexpr Dot(const Vector2& a, const Vector2& b) -> float;

/**
 * @brief Represents a 2D vector with floating-point components.
 *
 * Vector2 stores an `(x, y)` pair and is used for positions, directions,
 * UV coordinates, and general 2D math. It provides basic arithmetic,
 * normalization, and utility helpers.
 *
 * @ingroup MathGroup
 */
struct VGLX_EXPORT Vector2 {
    /// @brief X component.
    float x;

    /// @brief Y component.
    float y;

    /**
     * @brief Constructs an uninitialized 2D vector.
     */
    constexpr Vector2() = default;

    /**
     * @brief Constructs a vector with both components set to the same value.
     *
     * @param value Value to assign to both components.
     */
    constexpr Vector2(float value) : Vector2(value, value) {}

    /**
     * @brief Constructs a vector from individual components.
     *
     * @param x X component.
     * @param y Y component.
     */
    constexpr Vector2(float x, float y) : x(x), y(y) {}

    /**
     * @brief Returns a unit vector pointing to the right.
     */
    [[nodiscard]] static constexpr auto Right() -> Vector2 { return {1.0f, 0.0f}; }

    /**
     * @brief Returns a unit vector pointing up.
     */
    [[nodiscard]] static constexpr auto Up() -> Vector2 { return {0.0f, 1.0f}; }

    /**
     * @brief Returns the zero vector.
     */
    [[nodiscard]] static constexpr auto Zero() -> Vector2 { return {0.0f}; }

    /**
     * @brief Returns the vector length.
     */
    [[nodiscard]] constexpr auto Length() const -> float {
        return math::Sqrt(Dot(*this, *this));
    }

    /**
     * @brief Returns the squared vector length.
     */
    [[nodiscard]] constexpr auto LengthSquared() const -> float {
        return Dot(*this, *this);
    }

    /**
     * @brief Accesses a component by index.
     *
     * @param i Index: `0 → x`, `1 → y`.
     */
    [[nodiscard]] constexpr auto operator[](int i) -> float& {
        assert(i >= 0 && i < 2);
        switch (i) {
            case 0: return x;
            case 1: return y;
            default: return x; // unreachable
        }
    }

    /**
     * @brief Accesses a component by index.
     *
     * @param i Index: `0 → x`, `1 → y`.
     */
    [[nodiscard]] constexpr auto operator[](int i) const -> const float {
        assert(i >= 0 && i < 2);
        switch (i) {
            case 0: return x;
            case 1: return y;
            default: return x; // unreachable
        }
    }

    /**
     * @brief Adds another vector in-place.
     *
     * @param v Vector to add.
     */
    constexpr auto operator+=(const Vector2& v) -> Vector2& {
        x += v.x;
        y += v.y;
        return *this;
    }

    /**
     * @brief Subtracts another vector in-place.
     *
     * @param v Vector to subtract.
     */
    constexpr auto operator-=(const Vector2& v) -> Vector2& {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    /**
     * @brief Multiplies the vector by a scalar in-place.
     *
     * @param n Scalar value.
     */
    constexpr auto operator*=(float n) -> Vector2& {
        x *= n;
        y *= n;
        return *this;
    }

    /**
     * @brief Multiplies the vector component-wise by another vector in-place.
     *
     * @param v Vector to multiply.
     */
    constexpr auto operator*=(const Vector2& v) -> Vector2& {
        x *= v.x;
        y *= v.y;
        return *this;
    }

    /**
     * @brief Applies a component-wise minimum with another vector.
     *
     * @param v Vector to compare against.
     */
    constexpr auto Min(const Vector2& v) -> Vector2& {
        x = std::min(x, v.x);
        y = std::min(y, v.y);
        return *this;
    };

    /**
     * @brief Applies a component-wise maximum with another vector.
     *
     * @param v Vector to compare against.
     */
    constexpr auto Max(const Vector2& v) -> Vector2& {
        x = std::max(x, v.x);
        y = std::max(y, v.y);
        return *this;
    };

    /**
     * @brief Normalizes the vector in-place.
     *
     * If the length is zero, the vector is left unchanged.
     */
    constexpr auto Normalize() -> Vector2& {
        const auto len = Length();
        return len == 0.0f ? *this : (*this *= (1.0f / len));
    }

    /**
     * @brief Compares two vectors for equality.
     */
    constexpr auto operator==(const Vector2&) const -> bool = default;
};

/**
 * @brief Adds two 2D vectors.
 * @related Vector2
 *
 * @param a First vector.
 * @param b Second vector.
 */
[[nodiscard]] constexpr auto operator+(const Vector2& a, const Vector2& b) -> Vector2 {
    return {a.x + b.x, a.y + b.y};
}

/**
 * @brief Subtracts one 2D vector from another.
 * @related Vector2
 *
 * @param a First vector.
 * @param b Second vector.
 */
[[nodiscard]] constexpr auto operator-(const Vector2& a, const Vector2& b) -> Vector2 {
    return {a.x - b.x, a.y - b.y};
}

/**
 * @brief Multiplies a vector by a scalar.
 * @related Vector2
 *
 * @param v Input vector.
 * @param n Scalar value.
 */
[[nodiscard]] constexpr auto operator*(const Vector2& v, float n) -> Vector2 {
    return {v.x * n, v.y * n};
}

/**
 * @brief Multiplies a scalar by a vector.
 * @related Vector2
 *
 * @param n Scalar value.
 * @param v Input vector.
 */
[[nodiscard]] constexpr auto operator*(float n, const Vector2& v) -> Vector2 {
    return v * n;
}

/**
 * @brief Multiplies two vectors component-wise.
 * @related Vector2
 *
 * @param a First vector.
 * @param b Second vector.
 */
[[nodiscard]] constexpr auto operator*(const Vector2& a, const Vector2& b) -> Vector2 {
    return {a.x * b.x, a.y * b.y};
}

/**
 * @brief Divides a vector by a scalar.
 * @related Vector2
 *
 * @param v Input vector.
 * @param n Scalar value.
 */
[[nodiscard]] constexpr auto operator/(const Vector2& v, float n) -> Vector2 {
    n = 1.0f / n;
    return {v.x * n, v.y * n};
}

/**
 * @brief Computes the dot product of two 2D vectors.
 * @related Vector2
 *
 * Computes the scalar product ($a_x b_x + a_y b_y$), which measures how
 * aligned two vectors are.
 *
 * @param a First vector.
 * @param b Second vector.
 */
[[nodiscard]] constexpr auto Dot(const Vector2& a, const Vector2& b) -> float {
    return a.x * b.x + a.y * b.y;
}

/**
 * @brief Linearly interpolates between two vectors.
 * @related Vector2
 *
 * @param v1 Start vector.
 * @param v2 End vector.
 * @param f Interpolation factor in $[0, 1]$.
 */
[[nodiscard]] constexpr auto Lerp(const Vector2& v1, const Vector2& v2, float f) -> Vector2 {
    return v1 + (v2 - v1) * f;
}

/**
 * @brief Returns a normalized copy of a vector.
 * @related Vector2
 *
 * If the input has zero length, the zero vector is returned.
 *
 * @param v Input vector.
 */
[[nodiscard]] constexpr auto Normalize(const Vector2& v) -> Vector2 {
    const auto len = v.Length();
    return len == 0.0f ? Vector2::Zero() : v * (1.0f / len);
}

}