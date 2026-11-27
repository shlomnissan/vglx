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

#include "vk_mem_alloc.h"

namespace vglx {

struct VulkanInstance {
    VkInstance handle {VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT debug_messenger {VK_NULL_HANDLE};
};

struct VulkanQueue {
    VkQueue handle {VK_NULL_HANDLE};
    uint32_t family_index {0};
};

struct VulkanDeviceFeatures {
    bool synchronization2 {false};
    bool dynamic_rendering {false};
    bool sampler_anisotropy {false};
};

struct VulkanDevice {
    VkPhysicalDevice adapter {VK_NULL_HANDLE};
    VkDevice handle {VK_NULL_HANDLE};
    VmaAllocator allocator {nullptr};

    VulkanQueue graphics {};
    VulkanQueue compute {};
    VulkanQueue present {};

    VkPhysicalDeviceProperties properties {};
    VulkanDeviceFeatures features {};
};

[[nodiscard]] auto vk_create_instance(std::span<const char* const> extensions) -> std::expected<VulkanInstance, std::string>;

[[nodiscard]] auto vk_create_device(VkInstance instance, VkSurfaceKHR surface) -> std::expected<VulkanDevice, std::string>;

auto vk_destroy_device(VulkanDevice& device) -> void;

auto vk_destroy_surface(VkInstance instance, VkSurfaceKHR surface) -> void;

auto vk_destroy_instance(VulkanInstance& instance) -> void;

}
