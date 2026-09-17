/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx/math/euler.hpp"
#include "vglx/math/matrix4.hpp"
#include "vglx/math/quaternion.hpp"
#include "vglx/math/utilities.hpp"
#include "vglx/math/vector3.hpp"

namespace vglx {

/**
 * @brief 3D affine transform with position, rotation, and scale.
 *
 * Transform3 represents a 3D transform combining translation, non-uniform
 * scaling, and quaternion-based rotation. It lazily builds a @ref Matrix4
 * suitable for use as a world transform in scene graphs and rendering code.
 *
 * @ingroup MathGroup
 */
class Transform3 {
public:
    /// @brief Dirty flag indicating the cached matrix needs to be recomputed.
    bool touched {true};

    /// @brief Translation in 3D space.
    Vector3 position {0.0f};

    /// @brief Non-uniform scale in 3D.
    Vector3 scale {1.0f};

    /// @brief Rotation stored as a unit quaternion.
    Quaternion rotation {};

    /**
     * @brief Constructs an identity transform.
     */
    constexpr Transform3() = default;

    /**
     * @brief Constructs a transform from a 4×4 affine matrix.
     *
     * Decomposes the matrix into @ref position, @ref rotation, and @ref scale.
     * See @ref SetFromMatrix for the assumptions the decomposition makes about
     * the input.
     *
     * @param mat Affine transform matrix to decompose.
     */
    explicit constexpr Transform3(const Matrix4& mat) {
        Decompose(mat);
    }

    /**
     * @brief Translates the transform in local space.
     *
     * If the rotation is not empty, the input vector is rotated by the current
     * orientation before being added to @ref position.
     *
     * @param value Translation vector.
     */
    constexpr auto Translate(const Vector3& value) -> void {
        position += rotation * value;
        touched = true;
    }

    /**
     * @brief Translates the transform a distance along an axis in local space.
     *
     * Convenience overload mirroring @ref Rotate. The axis is normalized
     * internally, so `distance` is a true distance along it. For an arbitrary
     * offset, use the single-vector overload instead.
     *
     * @param axis Translation axis; normalized internally.
     * @param distance Distance to translate along the axis.
     */
    constexpr auto Translate(const Vector3& axis, float distance) -> void {
        Translate(Normalize(axis) * distance);
    }

    /**
     * @brief Scales the transform.
     *
     * @param value Scale factors to apply.
     */
    constexpr auto Scale(const Vector3& value) -> void {
        scale *= value;
        touched = true;
    }

    /**
     * @brief Applies an additional rotation around an axis in local space.
     *
     * The rotation is composed after the current orientation, so the axis is
     * interpreted in the transform's local frame.
     *
     * @param axis Rotation axis; normalized internally.
     * @param angle Rotation angle in radians.
     */
    constexpr auto Rotate(const Vector3& axis, float angle) -> void {
        rotation = rotation * Quaternion::FromAxisAngle(axis, angle);
        touched = true;
    }

    /**
     * @brief Sets the rotation such that the object looks at a target point.
     *
     * Computes an orientation that looks from `position` toward `target`,
     * using `world_up` to resolve roll and construct a stable basis.
     *
     * @param position Object position.
     * @param target Target point to look at.
     * @param world_up World up direction.
     */
    constexpr auto LookAt(const Vector3& position, const Vector3& target, const Vector3& world_up) -> void {
        auto forward = Normalize(target - position);
        if (forward == Vector3::Zero()) {
            // The position and target are the same,
            // so we can't determine a forward vector.
            forward = Vector3::UnitZ();
        }

        auto right = Cross(world_up, forward);
        if (right.Length() == 0.0f) {
            // If the right vector is zero, the forward vector
            // is parallel to the world up vector.
            if (std::abs(world_up.z) == 1.0f) {
                forward.x += 0.0001f;
            } else {
                forward.z += 0.0001f;
            }
            forward.Normalize();
            right = Cross(world_up, forward);
        }

        right.Normalize();
        auto up = Cross(forward, right);

        rotation = Quaternion {{
            right.x, up.x, forward.x, 0.0f,
            right.y, up.y, forward.y, 0.0f,
            right.z, up.z, forward.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        }};

        touched = true;
    }

    /**
     * @brief Sets the translation component.
     *
     * @param position New position.
     */
    constexpr auto SetPosition(const Vector3& position) -> void {
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
    constexpr auto SetScale(const Vector3& scale) -> void {
        if (this->scale != scale) {
            this->scale = scale;
            touched = true;
        }
    }

    /**
     * @brief Sets the rotation component.
     *
     * @param rotation New rotation quaternion.
     */
    constexpr auto SetRotation(const Quaternion& rotation) -> void {
        if (this->rotation != rotation) {
            this->rotation = rotation;
            touched = true;
        }
    }

    /**
     * @brief Sets the rotation component from Euler angles.
     *
     * Provided as a convenience; the angles are converted to and stored as a
     * quaternion.
     *
     * @param rotation New Euler rotation.
     */
    constexpr auto SetRotation(const Euler& rotation) -> void {
        SetRotation(Quaternion::FromEuler(rotation));
    }

    /**
     * @brief Replaces every component with those of a 4×4 affine matrix.
     *
     * Decomposes the matrix into @ref position, @ref rotation, and @ref scale,
     * inverting the composition performed by @ref Get.
     *
     * The matrix is assumed to be a translation-rotation-scale affine
     * transform. Shear is not representable and will not survive the round
     * trip. A mirrored basis (negative determinant) is resolved by negating the
     * x scale, and a degenerate basis with a zero-length axis carries no
     * recoverable orientation, so the rotation is left as identity.
     *
     * @param mat Affine transform matrix to decompose.
     */
    constexpr auto SetFromMatrix(const Matrix4& mat) -> void {
        Decompose(mat);
    }

    /**
     * @brief Returns the rotation as Euler angles.
     *
     * Provided as a convenience for inspection; the conversion is lossy near
     * gimbal lock and only resolves angles within the @ref Euler reconstruction
     * range. The quaternion remains the authoritative representation.
     */
    [[nodiscard]] constexpr auto GetEuler() const -> Euler {
        return Euler {rotation.GetMatrix()};
    }

    /**
     * @brief Returns the 4×4 transform matrix.
     *
     * Recomputes the underlying matrix if any component has changed since the
     * last call, then returns the cached @ref Matrix4.
     */
    [[nodiscard]] constexpr auto Get() -> Matrix4 {
        if (touched) {
            touched = false;
            transform_ = ComputeMatrix();
        }
        return transform_;
    }

    /**
     * @brief Returns the 4×4 transform matrix without updating the cache.
     *
     * Computes the matrix on the fly if any component has changed since the
     * last update, otherwise returns the cached @ref Matrix4. The @ref touched
     * flag is not cleared, so a later non-const call still refreshes the cache.
     */
    [[nodiscard]] constexpr auto Get() const -> Matrix4 {
        return touched ? ComputeMatrix() : transform_;
    }

private:
    /// @cond INTERNAL
    Matrix4 transform_ {1.0f};

    constexpr auto ComputeMatrix() const -> Matrix4 {
        const auto r = rotation.GetMatrix();
        return Matrix4 {
            scale.x * r(0, 0), scale.y * r(0, 1), scale.z * r(0, 2), position.x,
            scale.x * r(1, 0), scale.y * r(1, 1), scale.z * r(1, 2), position.y,
            scale.x * r(2, 0), scale.y * r(2, 1), scale.z * r(2, 2), position.z,
            0.0f, 0.0f, 0.0f, 1.0f
        };
    }

    constexpr auto Decompose(const Matrix4& mat) -> void {
        const auto axis_x = Vector3 {mat(0, 0), mat(1, 0), mat(2, 0)};
        const auto axis_y = Vector3 {mat(0, 1), mat(1, 1), mat(2, 1)};
        const auto axis_z = Vector3 {mat(0, 2), mat(1, 2), mat(2, 2)};

        auto scale_x = axis_x.Length();
        const auto scale_y = axis_y.Length();
        const auto scale_z = axis_z.Length();

        if (Dot(Cross(axis_x, axis_y), axis_z) < 0.0f) {
            scale_x = -scale_x;
        }

        position = {mat(0, 3), mat(1, 3), mat(2, 3)};
        scale = {scale_x, scale_y, scale_z};

        if (scale_x == 0.0f || scale_y == 0.0f || scale_z == 0.0f) {
            rotation = Quaternion {};
        } else {
            const auto inv_x = 1.0f / scale_x;
            const auto inv_y = 1.0f / scale_y;
            const auto inv_z = 1.0f / scale_z;

            rotation = Quaternion {{
                axis_x.x * inv_x, axis_y.x * inv_y, axis_z.x * inv_z, 0.0f,
                axis_x.y * inv_x, axis_y.y * inv_y, axis_z.y * inv_z, 0.0f,
                axis_x.z * inv_x, axis_y.z * inv_y, axis_z.z * inv_z, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            }};
        }

        touched = true;
    }
    /// @endcond
};

}
