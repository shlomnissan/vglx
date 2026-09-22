/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/vulkan/vk_renderer_impl.hpp"


namespace vglx {

Renderer::Impl::Impl(const Renderer::Parameters& params) {}

auto Renderer::Impl::Initialize() -> std::expected<void, std::string> {
    return {};
}

auto Renderer::Impl::Clear(RenderTarget* target) -> void {}

auto Renderer::Impl::Render(Scene* scene, Camera* camera, RenderTarget* target) -> void {}

auto Renderer::Impl::SetViewport(int x, int y, int width, int height, Vector2 content_scale) -> void {}

auto Renderer::Impl::SetClearColor(const Color& color) -> void {}

auto Renderer::Impl::SetAutoClear(bool auto_clear) -> void {}

auto Renderer::Impl::SetToneMapping(ToneMapping tone_mapping) -> void {}

auto Renderer::Impl::SetExposure(float exposure) -> void {}

auto Renderer::Impl::SetShadowMap(ShadowMap shadow_map) -> void {}

Renderer::Impl::~Impl() = default;

}
