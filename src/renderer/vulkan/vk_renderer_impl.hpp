/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include "vglx/core/renderer.hpp"
#include "vglx/core/window.hpp"

#include <expected>
#include <string>

namespace vglx {

class Renderer::Impl {
public:
    explicit Impl(const Renderer::Parameters& params);

    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;

    auto operator=(const Impl&) -> Impl& = delete;
    auto operator=(Impl&&) -> Impl& = delete;

    [[nodiscard]] auto Initialize(Window::Impl& window) -> std::expected<void, std::string>;

    auto Render(Scene* scene, Camera* camera, RenderTarget* target = nullptr) -> void;

    auto Clear(RenderTarget* target = nullptr) -> void;

    auto SetViewport(const Viewport& viewport) -> void;

    auto SetClearColor(const Color& color) -> void;

    auto SetAutoClear(bool auto_clear) -> void;

    auto SetToneMapping(ToneMapping tone_mapping) -> void;

    auto SetExposure(float exposure) -> void;

    auto SetShadowMap(ShadowMap shadow_map) -> void;

    [[nodiscard]] auto RenderedObjectsPerFrame() const {
        return std::size_t {0};
    }

    [[nodiscard]] auto GetLimits() const -> const Renderer::Limits& {
        return limits_;
    }

    [[nodiscard]] auto GetDriverInfo() const -> const Renderer::DriverInfo& {
        return info_;
    }

    ~Impl();

private:
    Renderer::Limits limits_;
    Renderer::DriverInfo info_;
};

}
