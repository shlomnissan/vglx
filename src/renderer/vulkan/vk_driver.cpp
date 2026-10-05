/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/vulkan/vk_driver.hpp"

namespace vglx {

auto vk_create_instance(std::span<const char* const> required_extensions) -> std::expected<VulkanInstance, std::string> {
    return std::unexpected("Vulkan instance creation is not implemented");
}

auto vk_create_device(VkInstance instance, VkSurfaceKHR surface) -> std::expected<VulkanDevice, std::string> {
    return std::unexpected("Vulkan device creation is not implemented");
}

auto vk_destroy_device(VulkanDevice& device) -> void {}

auto vk_destroy_surface(VkInstance instance, VkSurfaceKHR surface) -> void {}

auto vk_destroy_instance(VulkanInstance& instance) -> void {}

}
