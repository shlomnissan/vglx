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

struct Vector3;
auto constexpr Dot(const Vector3& a, const Vector3& b) -> float;

/**
 * @brief Represents a 3D vector with floating-point components.
 *
 * Vector3 stores an `(x, y, z)` triple and is used for positions, directions,
 * normals, and general 3D math. It provides basic arithmetic, normalization,
 * and utility helpers.
 *
 * @ingroup MathGroup
 */
struct VGLX_EXPORT Vector3 {
    /// @brief X component.
    float x;
    /// @brief Y component.
    float y;
    /// @brief Z component.
    float z;

    /**
     * @brief Constructs an uninitialized 3D vector.
     */
    constexpr Vector3() = default;

    /**
     * @brief Constructs a vector with all components set to the same value.
     *
     * @param value Value to assign to all components.
     */
    constexpr Vector3(float value) : Vector3(value, value, value) {}

    /**
     * @brief Constructs a vector from individual components.
     *
     * @param x X component.
     * @param y Y component.
     * @param z Z component.
     */
    constexpr Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    /**
     * @brief Returns a unit vector pointing forward.
     */
    [[nodiscard]] static constexpr auto Forward() -> Vector3 { return {0.0f, 0.0f, 1.0f}; }

    /**
     * @brief Returns a unit vector pointing right.
     */
    [[nodiscard]] static constexpr auto Right() -> Vector3 { return {1.0f, 0.0f, 0.0f}; }

    /**
     * @brief Returns a unit vector pointing up.
     */
    [[nodiscard]] static constexpr auto Up() -> Vector3 { return {0.0f, 1.0f, 0.0f}; }

    /**
     * @brief Returns the zero vector.
     */
    [[nodiscard]] static constexpr auto Zero() -> Vector3 { return {0.0f}; }

    /**
     * @brief Returns the vector length.
     */
    [[nodiscard]] constexpr auto Length() const -> float { return math::Sqrt(Dot(*this, *this)); }

    /**
     * @brief Returns the squared vector length.
     *
     * Useful when comparing lengths without paying the cost of a square root.
     */
    [[nodiscard]] constexpr auto LengthSquared() const -> float { return Dot(*this, *this); }

    /**
     * @brief Accesses a component by index.
     *
     * @param i Index: `0 → x`, `1 → y`, `2 → z`.
     */
    [[nodiscard]] constexpr auto operator[](int i) -> float& {
        assert(i >= 0 && i < 3);
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: return x; // unreachable
        }
    }

    /**
     * @brief Accesses a component by index.
     *
     * @param i Index: `0 → x`, `1 → y`, `2 → z`.
     */
    [[nodiscard]] constexpr auto operator[](int i) const -> const float {
        assert(i >= 0 && i < 3);
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: return x; // unreachable
        }
    }

    /**
     * @brief Adds another vector in-place.
     *
     * @param v Vector to add.
     */
    constexpr auto operator+=(const Vector3& v) -> Vector3& {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    /**
     * @brief Subtracts another vector in-place.
     *
     * @param v Vector to subtract.
     */
    constexpr auto operator-=(const Vector3& v) -> Vector3& {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    /**
     * @brief Multiplies the vector by a scalar in-place.
     *
     * @param n Scalar value.
     */
    constexpr auto operator*=(float n) -> Vector3& {
        x *= n;
        y *= n;
        z *= n;
        return *this;
    }

    /**
     * @brief Multiplies the vector component-wise by another vector in-place.
     *
     * @param v Vector to multiply.
     */
    constexpr auto operator*=(const Vector3& v) -> Vector3& {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }

    /**
     * @brief Applies a component-wise minimum with another vector.
     *
     * @param v Vector to compare against.
     */
    constexpr auto Min(const Vector3& v) -> Vector3& {
        x = std::min(x, v.x);
        y = std::min(y, v.y);
        z = std::min(z, v.z);
        return *this;
    };

    /**
     * @brief Applies a component-wise maximum with another vector.
     *
     * @param v Vector to compare against.
     */
    constexpr auto Max(const Vector3& v) -> Vector3& {
        x = std::max(x, v.x);
        y = std::max(y, v.y);
        z = std::max(z, v.z);
        return *this;
    };

    /**
     * @brief Normalizes the vector in-place.
     *
     * If the length is zero, the vector is left unchanged.
     */
    constexpr auto Normalize() -> Vector3& {
        const auto len = Length();
        return len == 0.0f ? *this : (*this *= (1.0f / len));
    }

    /**
     * @brief Compares two vectors for equality.
     */
    constexpr auto operator==(const Vector3&) const -> bool = default;
};

/**
 * @brief Adds two 3D vectors.
 * @related Vector3
 *
 * @param a First vector.
 * @param b Second vector.
 */
[[nodiscard]] constexpr auto operator+(const Vector3& a, const Vector3& b) -> Vector3 {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

/**
 * @brief Subtracts one 3D vector from another.
 * @related Vector3
 *
 * @param a First vector.
 * @param b Second vector.
 */
[[nodiscard]] constexpr auto operator-(const Vector3& a, const Vector3& b) -> Vector3 {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

/**
 * @brief Multiplies a vector by a scalar.
 * @related Vector3
 *
 * @param v Input vector.
 * @param n Scalar value.
 */
[[nodiscard]] constexpr auto operator*(const Vector3& v, float n) -> Vector3 {
    return {v.x * n, v.y * n, v.z * n};
}

/**
 * @brief Multiplies a scalar by a vector.
 * @related Vector3
 *
 * @param n Scalar value.
 * @param v Input vector.
 */
[[nodiscard]] constexpr auto operator*(float n, const Vector3& v) -> Vector3 {
    return v * n;
}

/**
 * @brief Multiplies two vectors component-wise.
 * @related Vector3
 *
 * @param a First vector.
 * @param b Second vector.
 */
[[nodiscard]] constexpr auto operator*(const Vector3& a, const Vector3& b) -> Vector3 {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

/**
 * @brief Divides a vector by a scalar.
 * @related Vector3
 *
 * @param v Input vector.
 * @param n Scalar value.
 */
[[nodiscard]] constexpr auto operator/(const Vector3& v, float n) -> Vector3 {
    n = 1.0f / n;
    return {v.x * n, v.y * n, v.z * n};
}

/**
 * @brief Computes the cross product of two 3D vectors.
 * @related Vector3
 *
 * Returns a vector perpendicular to both inputs, following the right-hand rule.
 *
 * @param a First vector.
 * @param b Second vector.
 */
[[nodiscard]] constexpr auto Cross(const Vector3& a, const Vector3& b) -> Vector3 {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

/**
 * @brief Computes the dot product of two 3D vectors.
 * @related Vector3
 *
 * Computes the scalar product ($a_x b_x + a_y b_y + a_z b_z$), which measures
 * how aligned the two vectors are.
 *
 * @param a First vector.
 * @param b Second vector.
 */
[[nodiscard]] constexpr auto Dot(const Vector3& a, const Vector3& b) -> float {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
 * @brief Linearly interpolates between two 3D vectors.
 * @related Vector3
 *
 * @param v1 Start vector.
 * @param v2 End vector.
 * @param f Interpolation factor in $[0, 1]$.
 */
[[nodiscard]] constexpr auto Lerp(const Vector3& v1, const Vector3& v2, float f) -> Vector3 {
    return v1 + (v2 - v1) * f;
}

/**
 * @brief Returns a normalized copy of a vector.
 * @related Vector3
 *
 * If the input has zero length, the zero vector is returned.
 *
 * @param v Input vector.
 */
[[nodiscard]] constexpr auto Normalize(const Vector3& v) -> Vector3 {
    const auto len = v.Length();
    return len == 0.0f ? Vector3::Zero() : v * (1.0f / len);
}

}