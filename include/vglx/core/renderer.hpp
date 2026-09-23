/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx_export.h"

#include "vglx/cameras/camera.hpp"
#include "vglx/math/color.hpp"
#include "vglx/scene/scene.hpp"

#include <cstddef>
#include <expected>
#include <memory>
#include <string>

namespace vglx {

class RenderTarget;
class Window;

/**
 * @brief Renderer interface for drawing a scene with a given camera.
 *
 * The renderer owns GPU state and draw logic for rendering a @ref Scene with a
 * specified @ref Camera. Construct one alongside your @ref Window, and call @ref Render once per frame.
 *
 * This class defines the rendering interface only. The actual rendering
 * implementation is provided by a backend, and multiple backends (for example,
 * OpenGL or Vulkan) may exist behind this interface.
 *
 * @code
 * vglx::Renderer renderer({
 *   .clear_color = 0x444444u
 * });
 *
 * auto ok = renderer.Initialize(window);
 * if (!ok) {
 *   HandleError(ok.error());
 * }
 * @endcode
 *
 * Once initialized, the renderer reads the framebuffer size and content scale
 * from the window at the start of every frame, so the viewport follows window
 * resizes without any user code. Call @ref SetViewport only to draw into a
 * sub-region of the framebuffer. The renderer must be destroyed before the
 * window it was initialized with.
 *
 * @ingroup CoreGroup
 */
class VGLX_EXPORT Renderer {
public:
    /**
     * @brief Tone mapping operator applied to the final HDR frame before display.
     */
    enum class ToneMapping {
        None, ///< No tone mapping. HDR values are clamped to [0, 1].
        ACESFilmic ///< ACES filmic curve. Compresses highlights and preserves shadows.
    };

    /**
     * @brief Shadow mapping method applied to shadow-casting lights.
     */
    enum class ShadowMap {
        None, ///< Shadows disabled.
        Basic, ///< Single-sample depth-mapped shadows with hard edges.
        PCF ///< Percentage-closer filtered (PCF) shadows with soft edges.
    };

    /// @brief Parameters for constructing a @ref Renderer object.
    struct Parameters {
        int sample_count {1}; ///< Antialiasing level (e.g., 4x MSAA).
        Color clear_color {0x000000u}; ///< Clear color used at the start of a frame.
        bool auto_clear {true}; ///< Automatic buffer clearing at the start of a frame.
        ToneMapping tone_mapping {ToneMapping::None}; ///< Tone mapping operator applied to the final frame.
        float exposure {1.0f}; ///< Exposure scale applied to HDR values before tone mapping.
        ShadowMap shadow_map {ShadowMap::None}; ///< Shadow mapping method applied to the scene.
    };

    /**
     * @brief Hardware limits reported by the rendering backend.
     */
    struct Limits {
        float max_anisotropy {1.0f}; ///< Max anisotropy for texture sampling.
        int max_samples {0}; ///< Max MSAA sample count.
        int max_texture_units {0}; ///< Max texture units per shader.
        int max_texture_size {0}; ///< Max 2D texture dimension.
        int max_cube_map_size {0}; ///< Max cube map face dimension.
        int max_renderbuffer_size {0}; ///< Max renderbuffer dimension.
    };

    /**
     * @brief Driver and hardware identity strings reported by the rendering backend.
     */
    struct DriverInfo {
        std::string vendor; ///< Driver vendor string.
        std::string renderer; ///< Hardware renderer string.
        std::string version; ///< Graphics API version string.
        std::string glsl_version; ///< Shading language version string.
    };

    /**
     * @brief Viewport rectangle in framebuffer pixels.
     *
     * The origin is the bottom-left corner of the framebuffer.
     */
    struct Viewport {
        int x {0}; ///< Left edge in pixels.
        int y {0}; ///< Bottom edge in pixels.
        int width {0}; ///< Width in pixels.
        int height {0}; ///< Height in pixels.

        auto operator==(const Viewport&) const -> bool = default;
    };

    /**
     * @brief Constructs a renderer.
     *
     * GPU resources are not created until @ref Initialize is called.
     *
     * @param params @ref Renderer::Parameters "Initialization parameters"
     * for constructing the renderer.
     */
    explicit Renderer(const Renderer::Parameters& params);

    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept = delete;

    auto operator=(const Renderer&) -> Renderer& = delete;
    auto operator=(Renderer&&) noexcept -> Renderer& = delete;

    /**
     * @brief Initializes GPU state and binds the renderer to a window.
     *
     * The window must be initialized first. From this point on the renderer
     * reads the framebuffer size and content scale from the window at the
     * start of every frame that targets the default framebuffer so resizes
     * are handled automatically. The renderer keeps a reference to the window
     * for its lifetime and must be destroyed before it.
     *
     * @param window The window whose framebuffer the renderer presents to.
     */
    [[nodiscard]] auto Initialize(Window& window) -> std::expected<void, std::string>;

    /**
     * @brief Renders the given scene from the specified camera.
     *
     * The scene is expected to be in a consistent state for rendering.
     * If you are using the runtime path, this is handled automatically.
     * In direct initialization flows, call the per-frame update routine
     * @ref Scene::Advance prior to rendering.
     *
     * When rendering to the default framebuffer, the scene's
     * @ref Scene::canvas "canvas" is drawn on top of the tone-mapped frame
     * in display space. Canvas is skipped when rendering to a render target.
     *
     * @param scene Pointer to the scene to render.
     * @param camera Pointer to the active camera.
     * @param target Pointer to the render target to clear or `nullptr`
     * for default framebuffer.
     */
    auto Render(Scene* scene, Camera* camera, RenderTarget* target = nullptr) -> void;

    /**
     * @brief Clears the color and depth buffers of the given target.
     *
     * Intended for compositing flows where automatic clearing is disabled
     * (see @ref SetAutoClear). Call it once at the start of a frame, then
     * render multiple scenes into the same target.
     *
     * @param target Pointer to the render target to clear or `nullptr`
     * for default framebuffer.
     */
    auto Clear(RenderTarget* target = nullptr) -> void;

    /**
     * @brief Pins the viewport to an explicit rectangle.
     *
     * By default the viewport covers the full framebuffer of the window passed
     * to @ref Initialize and follows it across resizes. Calling this method
     * replaces that behavior with a fixed rectangle.
     *
     * @param viewport @ref Renderer::Viewport "Viewport rectangle" to draw
     * into, in framebuffer pixels.
     */
    auto SetViewport(const Viewport& viewport) -> void;

    /**
     * @brief Sets the clear color for subsequent frames.
     *
     * The color is applied at the start of each frame when the framebuffer
     * is cleared. Typically used to define the background color of the
     * rendering surface.
     *
     * @param color Clear color in RGB format.
     */
    auto SetClearColor(const Color& color) -> void;

    /**
     * @brief Sets whether the color and depth buffers are cleared before drawing.
     *
     * When enabled (the default), each call to @ref Render clears the color
     * and depth buffers before drawing. Disable it to composite multiple
     * scenes into the same frame, such as rendering a screen-space overlay
     * on top of a previously rendered scene, using @ref Clear to clear
     * explicitly when needed.
     *
     * @param auto_clear Clear the depth and color buffers automatically at the start
     * of each frame.
     */
    auto SetAutoClear(bool auto_clear) -> void;

    /**
     * @brief Sets the tone mapping operator for subsequent frames.
     *
     * Changes how HDR color values are mapped to the display range. Takes
     * effect immediately on the next call to @ref Render.
     *
     * @param tone_mapping Tone mapping operator to apply.
     */
    auto SetToneMapping(ToneMapping tone_mapping) -> void;

    /**
     * @brief Sets the exposure scale for subsequent frames.
     *
     * Multiplies HDR color values before tone mapping is applied. Higher
     * values brighten the image; lower values darken it.
     *
     * @param exposure Linear exposure multiplier. Defaults to 1.0.
     */
    auto SetExposure(float exposure) -> void;

    /**
     * @brief Sets the shadow mapping method for subsequent frames.
     *
     * Switching methods may compile new shader variants on the next frame.
     * Selecting @ref ShadowMap::None "ShadowMap::None" releases all shadow
     * map resources. Switching back re-creates them on demand.
     *
     * @param shadow_map @ref ShadowMap "Shadow mapping method" to apply.
     */
    auto SetShadowMap(ShadowMap shadow_map) -> void;

    /**
     * @brief Returns the number of renderable objects drawn in the last frame.
     *
     * Intended for statistics overlays and debugging.
     */
    [[nodiscard]] auto RenderedObjectsPerFrame() const -> size_t;

    /**
     * @brief Returns hardware limits reported by the rendering backend.
     *
     * Must be called after @ref Initialize has succeeded.
     */
    [[nodiscard]] auto GetLimits() const -> const Limits&;

    /**
     * @brief Returns driver and hardware identity strings reported by the rendering backend.
     *
     * Must be called after @ref Initialize has succeeded.
     */
    [[nodiscard]] auto GetDriverInfo() const -> const DriverInfo&;

    virtual ~Renderer();

private:
    /// @cond INTERNAL
    class Impl;
    std::unique_ptr<Impl> impl_;
    /// @endcond
};

}
