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
#include "vglx/math/vector2.hpp"
#include "vglx/scene/scene.hpp"

#include <cstddef>
#include <expected>
#include <memory>
#include <string>

namespace vglx {

class RenderTarget;

/**
 * @brief Renderer interface for drawing a scene with a given camera.
 *
 * The renderer owns GPU state and draw logic for rendering a @ref Scene with a
 * specified @ref Camera. Construct one alongside your @ref Window and call
 * @ref Render once per frame from your main loop.
 *
 * This class defines the rendering interface only. The actual rendering
 * implementation is provided by a backend, and multiple backends (for example,
 * OpenGL or Vulkan) may exist behind this interface.
 *
 * @code
 * vglx::Renderer renderer({
 *   .framebuffer_width = window.FramebufferWidth(),
 *   .framebuffer_height = window.FramebufferHeight(),
 *   .clear_color = 0x444444u
 * });
 *
 * auto ok = renderer.Initialize();
 * if (!ok) {
 *   HandleError(ok.error());
 * }
 * @endcode
 *
 * The renderer assumes a valid graphics context is current on the
 * calling thread. When the window is resized, call @ref SetViewport to adjust
 * the render area (or recreate with new parameters if you manage your own
 * framebuffers).
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
        int framebuffer_width {1280}; ///< Current framebuffer width in pixels.
        int framebuffer_height {720}; ///< Current framebuffer height in pixels.
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
     * @brief Initializes GPU state and allocates required resources.
     */
    [[nodiscard]] auto Initialize() -> std::expected<void, std::string>;

    /**
     * @brief Renders the given scene from the specified camera.
     *
     * The scene is expected to be in a consistent state for rendering.
     * If you are using the runtime path, this is handled automatically.
     * In direct initialization flows, call the per-frame update routine
     * @ref Scene::Advance prior to rendering.
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
     * @brief Sets the active viewport rectangle in pixels.
     *
     * Adjusts the area of the framebuffer that subsequent draw calls will target.
     * This should be called whenever the window or framebuffer size changes, or
     * when rendering to a specific sub-region of the target surface.
     *
     * When using the runtime-managed rendering path, the viewport is updated
     * automatically. In manual initialization flows, you are responsible for
     * calling this method whenever the framebuffer dimensions change.
     *
     * @param x Left pixel of the viewport.
     * @param y Bottom pixel of the viewport.
     * @param width Viewport width in pixels.
     * @param height Viewport height in pixels.
     * @param scale DPI scale factor per axis.
     */
    auto SetViewport(int x, int y, int width, int height, Vector2 scale) -> void;

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
