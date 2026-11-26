/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/vulkan/vk_driver.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <string_view>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>

namespace vglx {

namespace {

struct VulkanSurfaceProperties {
    VkSurfaceCapabilitiesKHR capabilities {};
    std::vector<VkSurfaceFormatKHR> formats {};
    std::vector<VkPresentModeKHR> present_modes {};
};

struct VulkanPhysicalDevice {
    VkPhysicalDevice handle {VK_NULL_HANDLE};
    std::string name {};
    VulkanSurfaceProperties surface_info {};
    std::optional<uint32_t> graphics_queue_family {std::nullopt};
    std::optional<uint32_t> present_queue_family {std::nullopt};
    std::optional<uint32_t> compute_queue_family {std::nullopt};
    uint32_t vulkan_version {0};
    bool is_swapchain_extension {false};
    bool is_dedicated_gpu {false};
    bool is_shared_graphics_present {false};
};

auto get_device_extensions(VkPhysicalDevice device) -> std::expected<std::vector<VkExtensionProperties>, std::string> {
    auto count = uint32_t {0};
    auto result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to count device extensions: {}", string_VkResult(result)));
    }

    auto extensions = std::vector<VkExtensionProperties>(count);
    result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to enumerate device extensions: {}", string_VkResult(result)));
    }

    extensions.resize(count);
    return extensions;
}

auto has_device_extension(VkPhysicalDevice device, std::string_view name) -> std::expected<bool, std::string> {
    auto extensions = get_device_extensions(device);
    if (!extensions.has_value()) {
        return std::unexpected(extensions.error());
    }
    return std::ranges::contains(extensions.value(), name, &VkExtensionProperties::extensionName);
}

auto get_surface_properties(VkPhysicalDevice device, VkSurfaceKHR surface) -> std::expected<VulkanSurfaceProperties, std::string> {
    auto surface_properties = VulkanSurfaceProperties {};

    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_properties.capabilities);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to get device surface properties: {}", string_VkResult(result)));
    }

    uint32_t count = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to count surface formats: {}", string_VkResult(result)));
    }

    surface_properties.formats.resize(count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, surface_properties.formats.data());
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to get surface formats: {}", string_VkResult(result)));
    }

    surface_properties.formats.resize(count);

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to count surface present modes: {}", string_VkResult(result)));
    }

    surface_properties.present_modes.resize(count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, surface_properties.present_modes.data());
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to get surface present modes: {}", string_VkResult(result)));
    }

    surface_properties.present_modes.resize(count);

    return surface_properties;
}

auto get_physical_devices(VkInstance instance, VkSurfaceKHR surface) -> std::expected<std::vector<VulkanPhysicalDevice>, std::string> {
    uint32_t count = 0;
    auto result = vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to count Vulkan devices: {}", string_VkResult(result)));
    }

    auto devices = std::vector<VkPhysicalDevice>(count);
    result = vkEnumeratePhysicalDevices(instance, &count, devices.data());
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to enumerate Vulkan devices: {}", string_VkResult(result)));
    }

    devices.resize(count);

    if (devices.size() == 0) return {};

    auto physical_devices = std::vector<VulkanPhysicalDevice> {};
    physical_devices.reserve(devices.size());

    for (auto d : devices) {
        auto properties = VkPhysicalDeviceProperties {};
        vkGetPhysicalDeviceProperties(d, &properties);

        auto ext_swapchain = has_device_extension(d, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        if (!ext_swapchain.has_value()) {
            return std::unexpected(ext_swapchain.error());
        }

        auto device = VulkanPhysicalDevice {
            .handle = d,
            .name = properties.deviceName,
            .vulkan_version = properties.apiVersion,
            .is_swapchain_extension = ext_swapchain.value(),
            .is_dedicated_gpu = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        };

        vkGetPhysicalDeviceQueueFamilyProperties(d, &count, nullptr);
        auto queue_families = std::vector<VkQueueFamilyProperties>(count);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &count, queue_families.data());

        auto queue_idx = uint32_t {0};
        for (const auto& q : queue_families) {
            if (q.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                device.graphics_queue_family = queue_idx;
            }

            if (q.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                device.compute_queue_family = queue_idx;
            }

            auto surface_support = VkBool32 {false};
            result = vkGetPhysicalDeviceSurfaceSupportKHR(d, queue_idx, surface, &surface_support);
            if (result != VK_SUCCESS) {
                return std::unexpected(std::format("Failed to query surface support: {}", string_VkResult(result)));
            }

            if (surface_support) {
                device.present_queue_family = queue_idx;
            }

            queue_idx++;
        }

        device.is_shared_graphics_present = device.graphics_queue_family.has_value() &&
                                            device.graphics_queue_family == device.present_queue_family;

        if (device.present_queue_family.has_value()) {
            auto surface_properties = get_surface_properties(d, surface);
            if (!surface_properties.has_value()) {
                return std::unexpected(surface_properties.error());
            }
            device.surface_info = std::move(surface_properties.value());
        }

        physical_devices.emplace_back(device);
    }

    return physical_devices;
}

}

auto vk_create_device(VkInstance instance, VkSurfaceKHR surface) -> std::expected<VulkanDevice, std::string> {
    auto devices = get_physical_devices(instance, surface);
    if (!devices.has_value()) {
        return std::unexpected(devices.error());
    }

    return std::unexpected("Vulkan device creation is not implemented");
}

auto vk_destroy_device(VulkanDevice& device) -> void {

}

}
