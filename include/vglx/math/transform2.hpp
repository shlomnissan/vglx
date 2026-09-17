/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx/math/matrix3.hpp"
#include "vglx/math/vector2.hpp"
#include "vglx/math/utilities.hpp"

namespace vglx {

/**
 * @brief 2D affine transform with position, rotation, scale, and center.
 *
 * Transform2 represents a 2D transform that combines translation, rotation,
 * and non-uniform scaling around an arbitrary center point. It lazily builds
 * a @ref Matrix3 suitable for use in 2D rendering pipelines and UI
 * layouts.
 *
 * @ingroup MathGroup
 */
class Transform2 {
public:
    /// @brief Dirty flag indicating the cached matrix needs to be recomputed.
    bool touched {true};

    /// @brief Translation in 2D space.
    Vector2 position {0.0f, 0.0f};

    /// @brief Non-uniform scale in 2D.
    Vector2 scale {1.0f, 1.0f};

    /// @brief Pivot point for rotation and scaling.
    Vector2 center {0.0f, 0.0f};

    /// @brief Rotation angle in radians.
    float rotation {0.0f};

    /**
     * @brief Constructs an identity transform.
     */
    constexpr Transform2() = default;

    /**
     * @brief Translates the transform in local space.
     *
     * The input vector is rotated by the current rotation before being added
     * to @ref position.
     *
     * @param value Translation vector.
     */
    constexpr auto Translate(const Vector2& value) -> void {
        const float s = math::Sin(rotation);
        const float c = math::Cos(rotation);
        const Vector2 rotated_value = {
            value.x * c - value.y * s,
            value.x * s + value.y * c
        };
        position += rotated_value;
        touched = true;
    }

    /**
     * @brief Scales the transform.
     *
     * @param value Scale factors to apply.
     */
    constexpr auto Scale(const Vector2& value) -> void {
        scale *= value;
        touched = true;
    }

    /**
     * @brief Applies an additional rotation.
     *
     * @param angle Rotation angle in radians to add.
     */
    constexpr auto Rotate(float angle) -> void {
        rotation += angle;
        touched = true;
    }

    /**
     * @brief Sets the translation component.
     *
     * @param position New position.
     */
    constexpr auto SetPosition(const Vector2& position) -> void {
        if (this->position != position) {
            this->position = position;
            touched = true;
        }
    }

    /**
     * @brief Sets the scale component.
     *
     * @param scale New scale factors.
     */
    constexpr auto SetScale(const Vector2& scale) -> void {
        if (this->scale != scale) {
            this->scale = scale;
            touched = true;
        }
    }

    /**
     * @brief Sets the rotation component.
     *
     * @param rotation New rotation angle in radians.
     */
    constexpr auto SetRotation(float rotation) -> void {
        if (this->rotation != rotation) {
            this->rotation = rotation;
            touched = true;
        }
    }

    /**
     * @brief Sets the pivot point used for rotation and scaling.
     *
     * @param center New pivot point.
     */
    constexpr auto SetCenter(const Vector2& center) -> void {
        if (this->center !=center) {
            this->center = center;
            touched = true;
        }
    }

    /**
     * @brief Returns the 3×3 transform matrix.
     *
     * Recomputes the underlying matrix if any component has changed since the
     * last call, then returns the cached @ref Matrix3.
     */
    [[nodiscard]] constexpr auto Get() -> Matrix3 {
        if (touched) {
            touched = false;
            transform_ = ComputeMatrix();
        }
        return transform_;
    }

    /**
     * @brief Returns the 3×3 transform matrix without updating the cache.
     *
     * Computes the matrix on the fly if any component has changed since the
     * last update, otherwise returns the cached @ref Matrix3. The @ref touched
     * flag is not cleared, so a later non-const call still refreshes the cache.
     */
    [[nodiscard]] constexpr auto Get() const -> Matrix3 {
        return touched ? ComputeMatrix() : transform_;
    }

private:
    /// @cond INTERNAL
    Matrix3 transform_ {1.0f};

    constexpr auto ComputeMatrix() const -> Matrix3 {
        const float rc = math::Cos(rotation);
        const float rs = math::Sin(rotation);
        const float tx = -scale.x * (rc * center.x - rs * center.y) + center.x + position.x;
        const float ty = -scale.y * (rs * center.x + rc * center.y) + center.y + position.y;
        return Matrix3 {
            scale.x * rc, -scale.x * rs, tx,
            scale.y * rs,  scale.y * rc, ty,
            0.0f, 0.0f, 1.0f
        };
    }
    /// @endcond
};

}
