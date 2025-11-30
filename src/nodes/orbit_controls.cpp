/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/nodes/orbit_controls.hpp"

#include "vglx/math/spherical.hpp"
#include "vglx/math/vector2.hpp"
#include "vglx/math/vector3.hpp"

namespace vglx {

struct OrbitControls::Impl {
    Camera* camera;

    Spherical spherical {};

    Vector3 target = Vector3::Zero();
    Vector2 curr_pos {0.0f, 0.0f};
    Vector2 prev_pos {0.0f, 0.0f};

    MouseButton curr_button {MouseButton::None};

    float orbit_speed {0.0f};
    float pan_speed {0.0f};
    float zoom_speed {0.0f};
    float curr_scroll_offset {0.0f};

    auto OnUpdate([[maybe_unused]] float delta) {
        const auto offset = curr_pos - prev_pos;
        const auto do_orbit = curr_button == MouseButton::Left;
        const auto do_pan = curr_button == MouseButton::Right;
        const auto do_zoom = curr_scroll_offset != 0.0f;

        if (do_orbit) {
            spherical.phi -= offset.x * orbit_speed;
            spherical.theta += offset.y * orbit_speed;
        }

        if (do_zoom) {
            spherical.radius -= curr_scroll_offset * zoom_speed;
            spherical.radius = std::max(0.1f, spherical.radius);
            curr_scroll_offset = 0.0f;
        }

        if (do_pan) {
            const auto speed = pan_speed * spherical.radius;
            const auto right = camera->Right();
            const auto up = camera->Up();
            target -= (right * offset.x - up * offset.y) * speed;
        }

        prev_pos = curr_pos;

        spherical.MakeSafe();
        camera->transform.SetPosition(target + spherical.ToVector3());
        camera->LookAt(target);
    }
};

OrbitControls::OrbitControls(Camera* camera, const Parameters& params)
    : impl_(std::make_unique<Impl>())
{
    impl_->camera = camera;
    impl_->spherical.radius = params.radius;
    impl_->spherical.phi = params.yaw;
    impl_->spherical.theta = params.pitch;
    impl_->orbit_speed = params.orbit_speed;
    impl_->pan_speed = params.pan_speed;
    impl_->zoom_speed = params.zoom_speed;
};

auto OrbitControls::OnMouseEvent(MouseEvent* event) -> void {
    impl_->curr_pos = event->position;

    const auto is_pressed = event->type == MouseEvent::Type::ButtonPressed;
    if (is_pressed && impl_->curr_button == MouseButton::None) {
        impl_->curr_button = event->button;
    }

    const auto is_released = event->type == MouseEvent::Type::ButtonReleased;
    if (is_released && event->button == impl_->curr_button) {
        impl_->curr_button = MouseButton::None;
    }

    if (event->type == MouseEvent::Type::Scrolled) {
        impl_->curr_scroll_offset = event->scroll.y;
    }
}

auto OrbitControls::OnUpdate(float delta) -> void {
    impl_->OnUpdate(delta);
}

OrbitControls::~OrbitControls() = default;

}