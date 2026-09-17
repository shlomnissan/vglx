/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/canvas/canvas.hpp"

#include "vglx/events/gamepad_event.hpp"
#include "vglx/events/keyboard_event.hpp"
#include "vglx/events/mouse_event.hpp"

namespace vglx {

namespace {

auto handle_node_updates(Node2D* node, float delta) -> void {
    node->OnUpdate(delta);
    for (const auto& child : node->GetChildren()) {
        handle_node_updates(child.get(), delta);
    }
}

auto handle_input_event(Node2D* node, Event* event) -> void {
    using enum Event::Type;

    for (const auto& child : node->GetChildren()) {
        handle_input_event(child.get(), event);
        if (event->handled) return;
    }

    const auto type = event->GetType();
    if (type == Keyboard)
        node->OnKeyboardEvent(static_cast<KeyboardEvent*>(event));
    if (type == Mouse)
        node->OnMouseEvent(static_cast<MouseEvent*>(event));
    if (type == Gamepad)
        node->OnGamepadEvent(static_cast<GamepadEvent*>(event));
}

}

Canvas::Canvas() {
    AttachSubtree(this);
}

auto Canvas::Resize(int width, int height) -> void {
    const auto size = Vector2 {
        static_cast<float>(width),
        static_cast<float>(height)
    };

    if (size_ == size) return;

    size_ = size;

    if (width <= 0 || height <= 0) return;

    projection_matrix = {
        2.0f / size.x, 0.0f, -1.0f,
        0.0f, -2.0f / size.y, 1.0f,
        0.0f, 0.0f, 1.0f
    };
}

auto Canvas::GetSize() const -> Vector2 {
    return size_;
}

auto Canvas::Advance(float delta) -> void {
    handle_node_updates(this, delta);
}

auto Canvas::HandleEvent(Event* event) -> void {
    handle_input_event(this, event);
}

}
