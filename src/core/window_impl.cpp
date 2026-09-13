/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "core/window_impl.hpp"

#include "vglx/events/keyboard_event.hpp"
#include "vglx/events/mouse_event.hpp"

#include "events/event_dispatcher.hpp"
#include "utilities/logger.hpp"

#include <memory>
#include <string>
#include <utility>

#ifdef VGLX_USE_IMGUI
#include "core/imgui_integration.hpp"
#endif

namespace vglx {

namespace {

auto glfw_get_error() -> std::string;
auto glfw_key_callback(GLFWwindow*, int key, int scancode, int action, int mods) -> void;
auto glfw_cursor_pos_callback(GLFWwindow*, double x, double y) -> void;
auto glfw_mouse_button_callback(GLFWwindow*, int button, int action, int mods) -> void;
auto glfw_scroll_callback(GLFWwindow*, double x, double y) -> void;
auto glfw_mouse_button_map(int button) -> MouseButton;
auto glfw_mouse_mod_map(int mods) -> int;
auto glfw_mouse_mods = 0;
auto glfw_keyboard_map(int key) -> Key;
auto glfw_framebuffer_size_callback(GLFWwindow*, int w, int h) -> void;
auto glfw_window_size_callback(GLFWwindow*, int w, int h) -> void;
auto glfw_window_content_scale_callback(GLFWwindow*, float x, float y) -> void;

}

Window::Impl::Impl(const Window::Parameters& params) : params_(params) {}

auto Window::Impl::Initialize() -> std::expected<void, std::string> {
    if (!glfwInit()) {
        return std::unexpected("Failed to initialize GLFW " + glfw_get_error());
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);

    #ifdef __APPLE__
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    #endif

    window_ = glfwCreateWindow(
        params_.width,
        params_.height,
        params_.title.c_str(),
        nullptr,
        nullptr
    );

    if (window_ == nullptr) {
        return std::unexpected("Failed to create a GLFW window " + glfw_get_error());
    }

    glfwMakeContextCurrent(window_);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return std::unexpected("Failed to initialize GLAD OpenGL loader");
    }

    glfwSwapInterval(params_.vsync ? 1 : 0);
    glfwSetWindowUserPointer(window_, this);
    glfwGetFramebufferSize(window_, &framebuffer_width, &framebuffer_height);
    glfwGetWindowSize(window_, &window_width, &window_height);
    glfwGetWindowContentScale(window_, &content_scale.x, &content_scale.y);

    // Report the initial size through the resize callback.
    did_resize = true;

    glfwSetKeyCallback(window_, glfw_key_callback);
    glfwSetCursorPosCallback(window_, glfw_cursor_pos_callback);
    glfwSetMouseButtonCallback(window_, glfw_mouse_button_callback);
    glfwSetScrollCallback(window_, glfw_scroll_callback);
    glfwSetFramebufferSizeCallback(window_, glfw_framebuffer_size_callback);
    glfwSetWindowSizeCallback(window_, glfw_window_size_callback);
    glfwSetWindowContentScaleCallback(window_, glfw_window_content_scale_callback);

#ifdef VGLX_USE_IMGUI
    imgui_initialize(window_);
#endif

    return {};
}

auto Window::Impl::PollEvents() -> void {
    glfwPollEvents();
    if (did_resize) {
        if (resize_callback_) {
            resize_callback_({
                framebuffer_width,
                framebuffer_height,
                window_width,
                window_height,
                content_scale
            });
            did_resize = false;
        }
    }
}

auto Window::Impl::BeginUIFrame() -> void {
#ifdef VGLX_USE_IMGUI
    imgui_begin_frame();
#endif
}

auto Window::Impl::EndUIFrame() -> void {
#ifdef VGLX_USE_IMGUI
    imgui_end_frame();
#endif
}

auto Window::Impl::SwapBuffers() -> void {
    glfwSwapBuffers(window_);
}

auto Window::Impl::RequestClose() -> void {
    should_close_ = true;
}

auto Window::Impl::ShouldClose() -> bool {
    if (!should_close_) {
        should_close_ = glfwWindowShouldClose(window_);
    }
    return should_close_;
}

auto Window::Impl::SetTitle(std::string_view title) -> void {
    glfwSetWindowTitle(window_, title.data());
}

auto Window::Impl::SetResizeCallback(ResizeCallback callback) -> void {
    resize_callback_ = std::move(callback);
}

Window::Impl::~Impl() {
#ifdef VGLX_USE_IMGUI
    imgui_shutdown();
#endif

    if (window_) {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
}

namespace {

auto glfw_get_error() -> std::string {
    auto error = static_cast<const char*>(nullptr);
    glfwGetError(&error);
    return error != nullptr ? error : "unknown error";
}

auto glfw_key_callback(GLFWwindow*, int key, int scancode, int action, int mods) -> void {
#ifdef VGLX_USE_IMGUI
    if (imgui_wants_input()) return;
#endif

    auto event = std::make_unique<KeyboardEvent>();
    event->type = KeyboardEvent::Type::Pressed;
    event->key = glfw_keyboard_map(key);

    if (action == GLFW_PRESS) {
        EventDispatcher::Get().Dispatch("keyboard_event", std::move(event));
    }

    if (action == GLFW_RELEASE) {
        event->type = KeyboardEvent::Type::Released;
        EventDispatcher::Get().Dispatch("keyboard_event", std::move(event));
    }
}

auto glfw_cursor_pos_callback(GLFWwindow* window, double x, double y) -> void {
    auto event = std::make_unique<MouseEvent>();
    auto instance = static_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
    instance->mouse_pos_x = x;
    instance->mouse_pos_y = y;

    event->type = MouseEvent::Type::Moved;
    event->button = MouseButton::None;
    event->mods = glfw_mouse_mod_map(glfw_mouse_mods);
    event->position = {static_cast<float>(x), static_cast<float>(y)};
    event->scroll = {0.0f, 0.0f};
    event->window_size = {
        static_cast<float>(instance->window_width),
        static_cast<float>(instance->window_height)
    };

    EventDispatcher::Get().Dispatch("mouse_event", std::move(event));
}

auto glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) -> void {
#ifdef VGLX_USE_IMGUI
    if (imgui_wants_input()) return;
#endif
    auto event = std::make_unique<MouseEvent>();
    auto instance = static_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
    glfw_mouse_mods = mods;

    event->type = MouseEvent::Type::ButtonPressed;
    event->button = glfw_mouse_button_map(button);
    event->mods = glfw_mouse_mod_map(glfw_mouse_mods);
    event->position = {
        static_cast<float>(instance->mouse_pos_x),
        static_cast<float>(instance->mouse_pos_y)
    };
    event->scroll = {0.0f, 0.0f};
    event->window_size = {
        static_cast<float>(instance->window_width),
        static_cast<float>(instance->window_height)
    };

    if (action == GLFW_PRESS) {
        EventDispatcher::Get().Dispatch("mouse_event", std::move(event));
    }

    if (action == GLFW_RELEASE) {
        event->type = MouseEvent::Type::ButtonReleased;
        EventDispatcher::Get().Dispatch("mouse_event", std::move(event));
    }
}

auto glfw_scroll_callback(GLFWwindow* window, double x, double y) -> void {
#ifdef VGLX_USE_IMGUI
    if (imgui_wants_input()) return;
#endif

    auto instance = static_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
    auto event = std::make_unique<MouseEvent>();

    event->type = MouseEvent::Type::Scrolled;
    event->button = MouseButton::None;
    event->mods = glfw_mouse_mod_map(glfw_mouse_mods);
    event->position = {
        static_cast<float>(instance->mouse_pos_x),
        static_cast<float>(instance->mouse_pos_y)
    };
    event->scroll = {static_cast<float>(x), static_cast<float>(y)};
    event->window_size = {
        static_cast<float>(instance->window_width),
        static_cast<float>(instance->window_height)
    };

    EventDispatcher::Get().Dispatch("mouse_event", std::move(event));
}

auto glfw_mouse_button_map(int button) -> MouseButton {
    switch(button) {
        case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
        default: Logger::Log(LogLevel::Error, "Unrecognized GLFW mouse button key {}", button);
    }
    return MouseButton::None;
}

auto glfw_mouse_mod_map(int mods) -> int {
    int engine_mods = 0;
    if (mods & GLFW_MOD_SHIFT) engine_mods |= std::to_underlying(MouseMod::Shift);
    if (mods & GLFW_MOD_CONTROL) engine_mods |= std::to_underlying(MouseMod::Control);
    if (mods & GLFW_MOD_ALT) engine_mods |= std::to_underlying(MouseMod::Alt);
    if (mods & GLFW_MOD_SUPER) engine_mods |= std::to_underlying(MouseMod::Super);
    if (mods & GLFW_MOD_CAPS_LOCK) engine_mods |= std::to_underlying(MouseMod::CapsLock);
    if (mods & GLFW_MOD_NUM_LOCK) engine_mods |= std::to_underlying(MouseMod::NumLock);
    return engine_mods;
}

auto glfw_framebuffer_size_callback(GLFWwindow* window, int w, int h) -> void {
    auto in = static_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
    in->framebuffer_width = w;
    in->framebuffer_height = h;
    in->did_resize = true;
}

auto glfw_window_size_callback(GLFWwindow* window, int w, int h) -> void {
    auto in = static_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
    in->window_width = w;
    in->window_height = h;
    in->did_resize = true;
}

auto glfw_window_content_scale_callback(GLFWwindow* window, float x, float y) -> void {
    auto in = static_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
    in->content_scale = {x, y};
    in->did_resize = true;
}

auto glfw_keyboard_map(int key) -> vglx::Key {
    switch(key) {
        case GLFW_KEY_SPACE: return vglx::Key::Space;
        case GLFW_KEY_APOSTROPHE: return vglx::Key::Apostrophe;
        case GLFW_KEY_COMMA: return vglx::Key::Comma;
        case GLFW_KEY_MINUS: return vglx::Key::Minus;
        case GLFW_KEY_PERIOD: return vglx::Key::Period;
        case GLFW_KEY_SLASH: return vglx::Key::Slash;
        case GLFW_KEY_0: return vglx::Key::Key0;
        case GLFW_KEY_1: return vglx::Key::Key1;
        case GLFW_KEY_2: return vglx::Key::Key2;
        case GLFW_KEY_3: return vglx::Key::Key3;
        case GLFW_KEY_4: return vglx::Key::Key4;
        case GLFW_KEY_5: return vglx::Key::Key5;
        case GLFW_KEY_6: return vglx::Key::Key6;
        case GLFW_KEY_7: return vglx::Key::Key7;
        case GLFW_KEY_8: return vglx::Key::Key8;
        case GLFW_KEY_9: return vglx::Key::Key9;
        case GLFW_KEY_SEMICOLON: return vglx::Key::Semicolon;
        case GLFW_KEY_EQUAL: return vglx::Key::Equal;
        case GLFW_KEY_A: return vglx::Key::A;
        case GLFW_KEY_B: return vglx::Key::B;
        case GLFW_KEY_C: return vglx::Key::C;
        case GLFW_KEY_D: return vglx::Key::D;
        case GLFW_KEY_E: return vglx::Key::E;
        case GLFW_KEY_F: return vglx::Key::F;
        case GLFW_KEY_G: return vglx::Key::G;
        case GLFW_KEY_H: return vglx::Key::H;
        case GLFW_KEY_I: return vglx::Key::I;
        case GLFW_KEY_J: return vglx::Key::J;
        case GLFW_KEY_K: return vglx::Key::K;
        case GLFW_KEY_L: return vglx::Key::L;
        case GLFW_KEY_M: return vglx::Key::M;
        case GLFW_KEY_N: return vglx::Key::N;
        case GLFW_KEY_O: return vglx::Key::O;
        case GLFW_KEY_P: return vglx::Key::P;
        case GLFW_KEY_Q: return vglx::Key::Q;
        case GLFW_KEY_R: return vglx::Key::R;
        case GLFW_KEY_S: return vglx::Key::S;
        case GLFW_KEY_T: return vglx::Key::T;
        case GLFW_KEY_U: return vglx::Key::U;
        case GLFW_KEY_V: return vglx::Key::V;
        case GLFW_KEY_W: return vglx::Key::W;
        case GLFW_KEY_X: return vglx::Key::X;
        case GLFW_KEY_Y: return vglx::Key::Y;
        case GLFW_KEY_Z: return vglx::Key::Z;
        case GLFW_KEY_LEFT_BRACKET: return vglx::Key::LeftBracket;
        case GLFW_KEY_BACKSLASH: return vglx::Key::Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return vglx::Key::RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return vglx::Key::GraveAccent;
        case GLFW_KEY_WORLD_1: return vglx::Key::World1;
        case GLFW_KEY_WORLD_2: return vglx::Key::World2;
        case GLFW_KEY_ESCAPE: return vglx::Key::Escape;
        case GLFW_KEY_ENTER: return vglx::Key::Enter;
        case GLFW_KEY_TAB: return vglx::Key::Tab;
        case GLFW_KEY_BACKSPACE: return vglx::Key::Backspace;
        case GLFW_KEY_INSERT: return vglx::Key::Insert;
        case GLFW_KEY_DELETE: return vglx::Key::Delete;
        case GLFW_KEY_RIGHT: return vglx::Key::Right;
        case GLFW_KEY_LEFT: return vglx::Key::Left;
        case GLFW_KEY_DOWN: return vglx::Key::Down;
        case GLFW_KEY_UP: return vglx::Key::Up;
        case GLFW_KEY_PAGE_UP: return vglx::Key::PageUp;
        case GLFW_KEY_PAGE_DOWN: return vglx::Key::PageDown;
        case GLFW_KEY_HOME: return vglx::Key::Home;
        case GLFW_KEY_END: return vglx::Key::End;
        case GLFW_KEY_CAPS_LOCK: return vglx::Key::CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return vglx::Key::ScrollLock;
        case GLFW_KEY_NUM_LOCK: return vglx::Key::NumLock;
        case GLFW_KEY_PRINT_SCREEN: return vglx::Key::PrintScreen;
        case GLFW_KEY_PAUSE: return vglx::Key::Pause;
        case GLFW_KEY_F1: return vglx::Key::F1;
        case GLFW_KEY_F2: return vglx::Key::F2;
        case GLFW_KEY_F3: return vglx::Key::F3;
        case GLFW_KEY_F4: return vglx::Key::F4;
        case GLFW_KEY_F5: return vglx::Key::F5;
        case GLFW_KEY_F6: return vglx::Key::F6;
        case GLFW_KEY_F7: return vglx::Key::F7;
        case GLFW_KEY_F8: return vglx::Key::F8;
        case GLFW_KEY_F9: return vglx::Key::F9;
        case GLFW_KEY_F10: return vglx::Key::F10;
        case GLFW_KEY_F11: return vglx::Key::F11;
        case GLFW_KEY_F12: return vglx::Key::F12;
        case GLFW_KEY_F13: return vglx::Key::F13;
        case GLFW_KEY_F14: return vglx::Key::F14;
        case GLFW_KEY_F15: return vglx::Key::F15;
        case GLFW_KEY_F16: return vglx::Key::F16;
        case GLFW_KEY_F17: return vglx::Key::F17;
        case GLFW_KEY_F18: return vglx::Key::F18;
        case GLFW_KEY_F19: return vglx::Key::F19;
        case GLFW_KEY_F20: return vglx::Key::F20;
        case GLFW_KEY_F21: return vglx::Key::F21;
        case GLFW_KEY_F22: return vglx::Key::F22;
        case GLFW_KEY_F23: return vglx::Key::F23;
        case GLFW_KEY_F24: return vglx::Key::F24;
        case GLFW_KEY_F25: return vglx::Key::F25;
        case GLFW_KEY_KP_0: return vglx::Key::Keypad0;
        case GLFW_KEY_KP_1: return vglx::Key::Keypad1;
        case GLFW_KEY_KP_2: return vglx::Key::Keypad2;
        case GLFW_KEY_KP_3: return vglx::Key::Keypad3;
        case GLFW_KEY_KP_4: return vglx::Key::Keypad4;
        case GLFW_KEY_KP_5: return vglx::Key::Keypad5;
        case GLFW_KEY_KP_6: return vglx::Key::Keypad6;
        case GLFW_KEY_KP_7: return vglx::Key::Keypad7;
        case GLFW_KEY_KP_8: return vglx::Key::Keypad8;
        case GLFW_KEY_KP_9: return vglx::Key::Keypad9;
        case GLFW_KEY_KP_DECIMAL: return vglx::Key::KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return vglx::Key::KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return vglx::Key::KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return vglx::Key::KeypadSubtract;
        case GLFW_KEY_KP_ADD: return vglx::Key::KeypadAdd;
        case GLFW_KEY_KP_ENTER: return vglx::Key::KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return vglx::Key::KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return vglx::Key::LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return vglx::Key::LeftControl;
        case GLFW_KEY_LEFT_ALT: return vglx::Key::LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return vglx::Key::LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return vglx::Key::RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return vglx::Key::RightControl;
        case GLFW_KEY_RIGHT_ALT: return vglx::Key::RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return vglx::Key::RightSuper;
        case GLFW_KEY_MENU: return vglx::Key::Menu;
        default: Logger::Log(LogLevel::Error, "Unrecognized GLFW key {}", key);
    }
    return vglx::Key::None;
}

}

}
