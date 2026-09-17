/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/math/frustum.hpp"
#include "vglx/math/matrix4.hpp"
#include "vglx/math/vector3.hpp"
#include "vglx/scene/node.hpp"

namespace vglx {

/**
 * @brief Abstract base class for camera types.
 *
 * This class is not intended to be used directly. Use @ref PerspectiveCamera
 * or @ref OrthographicCamera unless you are defining your own camera,
 * in which case it should inherit from this class.
 *
 * @ingroup CamerasGroup
 */
class VGLX_EXPORT Camera : public Node {
public:
    /// @brief Projection matrix that maps camera space coordinates to clip space.
    Matrix4 projection_matrix {Matrix4::Identity()};

    /// @brief View matrix that maps world space coordinates to camera space.
    Matrix4 view_matrix {Matrix4::Identity()};

    /**
     * @brief Sets @ref view_matrix to the inverse of the camera's world transform.
     *
     * Called internally by the renderer before rendering a frame;
     * manual calls are rarely necessary.
     */
    auto UpdateViewMatrix() -> void;

    /**
     * @brief Updates the camera to reflect the current viewport size.
     *
     * Calls @ref OnResize so the derived camera can rebuild its
     * @ref projection_matrix.
     *
     * @param width  Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    auto Resize(int width, int height) -> void {
        OnResize(width, height);
    }

    /**
     * @brief Identifies this node as @ref Node::Type "Node::Type::Camera".
     */
    [[nodiscard]] auto GetNodeType() const -> Node::Type override {
        return Node::Type::Camera;
    }

    /**
     * @brief Computes a @ref Frustum from the combined projection and view matrices.
     *
     * Used for applying frustum culling to renderable nodes.
     */
    [[nodiscard]] auto GetFrustum() -> Frustum;

    /**
     * @brief Overrides @ref Node::LookAt to orient the camera toward a world space target.
     *
     * Accounts for the camera’s -Z viewing direction so the camera faces the given point.
     *
     * @param target World space position for the camera to look at.
     */
    auto LookAt(const Vector3& target) -> void override;

    /**
     * @brief Camera right axis in world space.
     */
    [[nodiscard]] auto Right() const -> Vector3 {
        const auto& mat = GetWorldTransform();
        return Vector3 { mat[0][0], mat[0][1], mat[0][2] };
    }

    /**
     * @brief Camera up axis in world space.
     */
    [[nodiscard]] auto Up() const -> Vector3 {
        const auto& mat = GetWorldTransform();
        return Vector3 { mat[1][0], mat[1][1], mat[1][2] };
    }

    /**
     * @brief Camera forward axis in world space.
     */
    [[nodiscard]] auto Forward() const -> Vector3 {
        const auto& mat = GetWorldTransform();
        return Vector3 { -mat[2][0], -mat[2][1], -mat[2][2] };
    }

    virtual ~Camera() = default;

protected:
    /**
     * @brief Updates @ref projection_matrix to reflect the new viewport size.
     *
     * @param width  Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    virtual auto OnResize(int width, int height) -> void = 0;
};

}