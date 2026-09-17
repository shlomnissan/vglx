/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/canvas/node2d.hpp"
#include "vglx/events/event.hpp"
#include "vglx/math/matrix3.hpp"
#include "vglx/math/vector2.hpp"

#include <memory>

namespace vglx {

/**
 * @brief Root node of a 2D canvas hierarchy.
 *
 * Canvas is the top-level container for all 2D nodes that participate in
 * rendering and updates. It owns the canvas hierarchy, defines the canvas
 * size in window coordinates, and maps that space to clip space through
 * @ref projection_matrix. Canvas coordinates use a top-left origin with Y
 * increasing downward, matching the coordinate space of mouse events.
 *
 * Call @ref Resize whenever the window size changes, typically alongside
 * @ref Camera::Resize, and call @ref Advance once per frame to drive updates.
 *
 * @code
 * auto canvas = vglx::Canvas::Create();
 *
 * window.OnResize([&](const vglx::ResizeParameters& p) {
 *   camera->Resize(p.window_width, p.window_height);
 *   canvas->Resize(p.window_width, p.window_height);
 * });
 *
 * // Inside the main loop:
 * canvas->Advance(delta);
 * @endcode
 *
 * @ingroup CanvasGroup
 */
class VGLX_EXPORT Canvas : public Node2D {
public:
    /**
     * @brief Projection matrix that maps canvas coordinates to clip space.
     *
     * Rebuilt by @ref Resize. Remains the identity until the first call
     * with a positive width and height.
     */
    Matrix3 projection_matrix {1.0f};

    /**
     * @brief Constructs a canvas.
     */
    Canvas();

    /**
     * @brief Creates an instance of @ref Canvas.
     */
    [[nodiscard]] static auto Create() -> std::unique_ptr<Canvas> {
        return std::make_unique<Canvas>();
    }

    /**
     * @brief Updates the canvas to reflect the current window size.
     *
     * Stores the size and rebuilds @ref projection_matrix so that canvas
     * coordinates from `(0, 0)` at the top-left to `(width, height)` at the
     * bottom-right fill the viewport. A non-positive dimension is stored but
     * leaves the projection unchanged.
     *
     * @param width  Canvas width in window coordinates.
     * @param height Canvas height in window coordinates.
     */
    auto Resize(int width, int height) -> void;

    /**
     * @brief Returns the canvas size in window coordinates.
     *
     * Returns zero until @ref Resize is called.
     */
    [[nodiscard]] auto GetSize() const -> Vector2;

    /**
     * @brief Advances the canvas by one frame.
     *
     * Propagates per-frame updates through the canvas hierarchy, calling
     * @ref Node2D::OnUpdate "OnUpdate" on the canvas and all attached nodes
     * in depth-first order.
     *
     * @param delta Elapsed time in seconds since the last frame.
     */
    auto Advance(float delta) -> void;

    /**
     * @brief Propagates an input event through the canvas hierarchy.
     *
     * Events travel bottom-up: children receive the event before their
     * parents, and the canvas itself receives it last. Propagation stops as
     * soon as a node marks the event as @ref Event::handled "handled".
     *
     * @param event Keyboard, mouse, or gamepad event to propagate.
     */
    auto HandleEvent(Event* event) -> void;

    /**
     * @brief Identifies this node as @ref Node2D::Type "Node2D::Type::Canvas".
     */
    [[nodiscard]] auto GetNodeType() const -> Node2D::Type override {
        return Node2D::Type::Canvas;
    }

private:
    /// @cond INTERNAL
    Vector2 size_ {0.0f, 0.0f};
    /// @endcond
};

}
