/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/scene/scene.hpp"

#include "events/event_dispatcher.hpp"

namespace vglx {

namespace {

auto handle_node_updates(Node* node, float delta) -> void {
    node->OnUpdate(delta);
    for (const auto& child : node->GetChildren()) {
        handle_node_updates(child.get(), delta);
    }
}

auto handle_input_event(Node* node, Event* event) -> void {
    using enum Event::Type;

    for (const auto& child : node->GetChildren()) {
        if (event->handled) return;
        handle_input_event(child.get(), event);
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

struct Scene::Impl {
    std::shared_ptr<EventListener> event_listener;
};

Scene::Scene() : impl_(std::make_unique<Impl>()) {
    using enum Event::Type;

    impl_->event_listener = std::make_shared<EventListener>([&](Event* event) {
        auto type = event->GetType();

        if (type == Keyboard || type == Mouse || type == Gamepad) {
            canvas.HandleEvent(event);

            if (event->handled) {
                return;
            }

            if (type == Keyboard) OnKeyboardEvent(static_cast<KeyboardEvent*>(event));
            if (type == Mouse) OnMouseEvent(static_cast<MouseEvent*>(event));
            if (type == Gamepad) OnGamepadEvent(static_cast<GamepadEvent*>(event));

            for (const auto& child : GetChildren()) {
                handle_input_event(child.get(), event);
            }
        }
    });

    EventDispatcher::Get().AddEventListener("keyboard_event", impl_->event_listener);
    EventDispatcher::Get().AddEventListener("mouse_event", impl_->event_listener);
    EventDispatcher::Get().AddEventListener("gamepad_event", impl_->event_listener);

    AttachSubtree(this);
}

auto Scene::Advance(float delta) -> void {
    OnUpdate(delta);
    for (const auto& child : GetChildren()) {
        handle_node_updates(child.get(), delta);
    }
    canvas.Advance(delta);
}

Scene::~Scene() {
    EventDispatcher::Get().RemoveEventListener("keyboard_event", impl_->event_listener);
    EventDispatcher::Get().RemoveEventListener("mouse_event", impl_->event_listener);
    EventDispatcher::Get().RemoveEventListener("gamepad_event", impl_->event_listener);
}

}
