/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#pragma once

#include <volk.h>

#include <cstdint>
#include <expected>
#include <span>
#include <string>

namespace vglx {

struct VulkanInstance {
    VkInstance handle {VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT debug_messenger {VK_NULL_HANDLE};
};

struct VulkanQueue {
    VkQueue handle {VK_NULL_HANDLE};
    std::uint32_t family_index {0};
};

struct VulkanDevice {
    VkPhysicalDevice adapter {VK_NULL_HANDLE};
    VkDevice handle {VK_NULL_HANDLE};

    VulkanQueue graphics;
    VulkanQueue compute;
    VulkanQueue present;

    VkPhysicalDeviceProperties properties {};
    VkPhysicalDeviceMemoryProperties memory_properties {};
};

[[nodiscard]] auto vk_create_instance(std::span<const char* const> required_extensions) -> std::expected<VulkanInstance, std::string>;

[[nodiscard]] auto vk_create_device(VkInstance instance, VkSurfaceKHR surface) -> std::expected<VulkanDevice, std::string>;

auto vk_destroy_device(VulkanDevice& device) -> void;

auto vk_destroy_surface(VkInstance instance, VkSurfaceKHR surface) -> void;

auto vk_destroy_instance(VulkanInstance& instance) -> void;

}
