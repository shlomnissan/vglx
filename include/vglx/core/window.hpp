/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "vglx/math/vector2.hpp"

namespace vglx {

class Renderer;

/**
 * @brief Parameters passed to resize callbacks.
 *
 * @related Window
 */
struct ResizeParameters {
    int framebuffer_width; ///< Framebuffer width in physical pixels.
    int framebuffer_height; ///< Framebuffer height in physical pixels.
    int window_width; ///< Logical window width in screen coordinates.
    int window_height; ///< Logical window height in screen coordinates.
    Vector2 content_scale; ///< Content scale per axis: the display's DPI relative to the platform default.
};

/**
 * @brief Function signature for window-resize notifications.
 *
 * The callback is invoked when the window’s client-area size changes.
 * Use the provided sizes to update camera projections and canvas layout.
 *
 * @related Window
 */
using ResizeCallback = std::function<void(const ResizeParameters& params)>;

/**
 * @brief Represents a cross-platform application window.
 *
 * This class creates and manages the operating system window, the
 * graphics context, and propagates input events from the OS. Construct
 * one in your @c main, call @ref Initialize, then drive it with
 * @ref PollEvents and @ref SwapBuffers each frame.
 *
 * @code
 * vglx::Window window({
 *   .title = "My App",
 *   .width = 1920,
 *   .height = 1080,
 *   .antialiasing = 4,
 *   .vsync = true
 * });
 *
 * auto ok = window.Initialize();
 * if (!ok) {
 *   HandleError(ok.error());
 * }
 * @endcode
 *
 * This class defines the windowing interface only. The actual
 * implementation may be platform-specific or provided by an external
 * dependency such as GLFW.
 *
 * @ingroup CoreGroup
 */
class VGLX_EXPORT Window {
public:
    /// @cond INTERNAL
    class Impl;
    /// @endcond

    /// @brief Parameters for constructing a Window object.
    struct Parameters {
        std::string title {""}; ///< Window title.
        int width {1280}; ///< Client-area width in pixels.
        int height {720}; ///< Client-area height in pixels.
        bool vsync {true}; ///< Enable or disable vertical sync.
    };

    /**
     * @brief Constructs a window.
     *
     * Resources are not created until @ref Initialize is called.
     *
     * @param params @ref Window::Parameters "Initialization parameters"
     * for constructing the window.
     */
    explicit Window(const Window::Parameters& params);

    Window(const Window&) = delete;
    Window(Window&&) noexcept;

    auto operator=(const Window&) -> Window& = delete;
    auto operator=(Window&&) noexcept -> Window&;

    /**
     * @brief Initializes the underlying OS window and graphics context.
     */
    [[nodiscard]] auto Initialize() -> std::expected<void, std::string>;

    /**
     * @brief Processes pending window and input events.
     *
     * Polls the operating system for events such as input, window resize,
     * or close requests. Must be called at the top of the main loop to keep
     * the application responsive.
     */
    auto PollEvents() -> void;

    /**
     * @brief Marks the beginning of a new UI frame.
     *
     * Must be called before issuing any UI commands. If no UI system is
     * active, this is a no-op.
     */
    auto BeginUIFrame() -> void;

    /**
     * @brief Marks the end of the current UI frame.
     *
     * Must be called after all UI commands have been issued. If no UI system
     * is active, this is a no-op.
     */
    auto EndUIFrame() -> void;

    /**
     * @brief Swaps the front and back buffers.
     *
     * Presents the rendered frame to the display.
     */
    auto SwapBuffers() -> void;

    /**
     * @brief Requests that the window be closed.
     *
     * Sets the internal close flag so that @ref ShouldClose returns true.
     * This allows the application to exit the main loop gracefully.
     */
    auto RequestClose() -> void;

    /**
     * @brief Returns whether the window has been flagged for closing.
     *
     * This flag is set when the user requests the window to close through
     * the operating system or when @ref RequestClose is called.
     */
    auto ShouldClose() -> bool;

    /**
     * @brief Returns the current framebuffer width in pixels.
     *
     * On high-DPI displays this may differ from @ref Width.
     */
    [[nodiscard]] auto FramebufferWidth() const -> int;

    /**
     * @brief Returns the current framebuffer height in pixels.
     *
     * On high-DPI displays this may differ from @ref Height.
     */
    [[nodiscard]] auto FramebufferHeight() const -> int;

    /**
     * @brief Returns the current logical window width in pixels.
     */
    [[nodiscard]] auto Width() const -> int;

    /**
     * @brief Returns the current logical window height in pixels.
     */
    [[nodiscard]] auto Height() const -> int;

    /**
     * @brief Returns the aspect ratio (width / height).
     */
    [[nodiscard]] auto AspectRatio() const -> float;

    /**
     * @brief Updates the window title.
     *
     * @param title UTF-8 string to display in the window title bar.
     */
    auto SetTitle(std::string_view title) -> void;

    /**
     * @brief Registers a callback to be invoked when the window is resized.
     *
     * The callback is executed whenever the client-area dimensions change,
     * including user drag-resizes and OS-driven scaling changes on high-DPI
     * displays. It is also invoked once with the initial size on the first
     * @ref PollEvents call, so size-dependent state can be fully initialized
     * through this callback. The callback receives both logical window sizes
     * and the framebuffer sizes in physical pixels.
     *
     * @param callback A function to invoke on resize with the new sizes.
     */
    auto OnResize(ResizeCallback callback) -> void;

    ~Window();

private:
    /// @cond INTERNAL
    friend class Renderer;

    std::unique_ptr<Impl> impl_;
    /// @endcond
};

}
