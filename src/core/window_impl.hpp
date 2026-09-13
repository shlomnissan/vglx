/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx/core/window.hpp"

#include <expected>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "vglx/math/vector2.hpp"

namespace vglx {

class Window::Impl {
public:
    double mouse_pos_x {0.0};
    double mouse_pos_y {0.0};

    int framebuffer_width {0};
    int framebuffer_height {0};
    int window_width {0};
    int window_height {0};

    Vector2 content_scale {0.0f};

    bool did_resize {false};

    explicit Impl(const Window::Parameters& params);

    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;

    auto operator=(const Impl&) -> Impl& = delete;
    auto operator=(Impl&&) -> Impl& = delete;

    [[nodiscard]] auto Initialize() -> std::expected<void, std::string>;

    auto PollEvents() -> void;

    auto BeginUIFrame() -> void;

    auto EndUIFrame() -> void;

    auto SwapBuffers() -> void;

    auto RequestClose() -> void;

    auto ShouldClose() -> bool;

    auto SetTitle(std::string_view title) -> void;

    auto SetResizeCallback(ResizeCallback callback) -> void;

    ~Impl();

private:
    Window::Parameters params_;

    GLFWwindow* window_ {nullptr};

    ResizeCallback resize_callback_ {nullptr};

    bool should_close_ {false};
};

}
