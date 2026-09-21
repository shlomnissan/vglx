/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "vglx/core/renderer.hpp"

#include "vglx/core/render_target.hpp"

#ifdef VGLX_RENDERER_OPENGL
#include "renderer/gl/gl_renderer_impl.hpp"
#endif

#ifdef VGLX_RENDERER_VULKAN
#include "renderer/vulkan/vk_renderer_impl.hpp"
#endif

namespace vglx {

Renderer::Renderer(const Renderer::Parameters& params)
    : impl_(std::make_unique<Impl>(params)) {}

auto Renderer::Initialize() -> std::expected<void, std::string> {
    return impl_->Initialize();
}

auto Renderer::Render(Scene* scene, Camera* camera, RenderTarget* target) -> void {
    impl_->Render(scene, camera, target);
}

auto Renderer::Clear(RenderTarget* target) -> void {
    impl_->Clear(target);
}

auto Renderer::SetViewport(int x, int y, int width, int height, Vector2 content_scale) -> void {
    impl_->SetViewport(x, y, width, height, content_scale);
}

auto Renderer::SetClearColor(const Color& color) -> void {
    impl_->SetClearColor(color);
}

auto Renderer::SetAutoClear(bool auto_clear) -> void {
    impl_->SetAutoClear(auto_clear);
}

auto Renderer::SetToneMapping(ToneMapping tone_mapping) -> void {
    impl_->SetToneMapping(tone_mapping);
}

auto Renderer::SetExposure(float exposure) -> void {
    impl_->SetExposure(exposure);
}

auto Renderer::SetShadowMap(ShadowMap shadow_map) -> void {
    impl_->SetShadowMap(shadow_map);
}

auto Renderer::RenderedObjectsPerFrame() const -> size_t {
    return impl_->RenderedObjectsPerFrame();
}

auto Renderer::GetLimits() const -> const Limits& {
    return impl_->GetLimits();
}

auto Renderer::GetDriverInfo() const -> const DriverInfo& {
    return impl_->GetDriverInfo();
}

Renderer::~Renderer() = default;

}
