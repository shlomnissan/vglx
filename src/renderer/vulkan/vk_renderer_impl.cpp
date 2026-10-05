/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/vulkan/vk_renderer_impl.hpp"

#include "core/window_impl.hpp"

namespace vglx {

Renderer::Impl::Impl(const Renderer::Parameters& params) : params_(params) {}

auto Renderer::Impl::Initialize(Window::Impl& window) -> std::expected<void, std::string> {
    if (window_ != nullptr) {
        return std::unexpected("Vulkan renderer is already initialized");
    }

    const auto extensions = window.GetRequiredVulkanExtensions();
    if (!extensions) {
        return std::unexpected(extensions.error());
    }

    auto instance = vk_create_instance(*extensions);
    if (!instance) {
        return std::unexpected(instance.error());
    }

    auto surface = window.CreateVulkanSurface(instance->handle);
    if (!surface) {
        vk_destroy_instance(*instance);
        return std::unexpected(surface.error());
    }

    auto device = vk_create_device(instance->handle, *surface);
    if (!device) {
        vk_destroy_surface(instance->handle, *surface);
        vk_destroy_instance(*instance);
        return std::unexpected(device.error());
    }

    instance_ = *instance;
    surface_ = *surface;
    device_ = *device;
    window_ = &window;

    return {};
}

auto Renderer::Impl::Clear(RenderTarget* target) -> void {}

auto Renderer::Impl::Render(Scene* scene, Camera* camera, RenderTarget* target) -> void {}

auto Renderer::Impl::SetViewport(const Viewport& viewport) -> void {}

auto Renderer::Impl::SetClearColor(const Color& color) -> void {}

auto Renderer::Impl::SetAutoClear(bool auto_clear) -> void {}

auto Renderer::Impl::SetToneMapping(ToneMapping tone_mapping) -> void {}

auto Renderer::Impl::SetExposure(float exposure) -> void {}

auto Renderer::Impl::SetShadowMap(ShadowMap shadow_map) -> void {}

Renderer::Impl::~Impl() {
    if (device_.handle != VK_NULL_HANDLE) vk_destroy_device(device_);
    if (surface_ != VK_NULL_HANDLE) vk_destroy_surface(instance_.handle, surface_);
    if (instance_.handle != VK_NULL_HANDLE) vk_destroy_instance(instance_);
}

}
