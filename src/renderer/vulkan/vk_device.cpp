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
    bool is_dedicated_compute_queue {false};
    bool is_shared_graphics_present_queue {false};
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

        for (auto i = uint32_t {0}; i < queue_families.size(); ++i) {
            const auto flags = queue_families[i].queueFlags;
            const bool supports_graphics = (flags & VK_QUEUE_GRAPHICS_BIT) != 0;
            const bool supports_compute = (flags & VK_QUEUE_COMPUTE_BIT) != 0;

            auto supports_present = VkBool32 {false};
            result = vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface, &supports_present);
            if (result != VK_SUCCESS) {
                return std::unexpected(std::format("Failed to query surface support: {}", string_VkResult(result)));
            }

            if (supports_graphics && supports_present) {
                device.graphics_queue_family = i;
                device.present_queue_family = i;
                device.is_shared_graphics_present_queue = true;
            } else {
                if (supports_graphics && !device.graphics_queue_family.has_value()) {
                    device.graphics_queue_family = i;
                }
                if (supports_present && !device.present_queue_family.has_value()) {
                    device.present_queue_family = i;
                }
            }

            if (supports_compute && !supports_graphics) {
                device.compute_queue_family = i;
                device.is_dedicated_compute_queue = true;
            } else if (supports_compute && !device.compute_queue_family.has_value()) {
                device.compute_queue_family = i;
            }
        }

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

auto select_physical_device(const std::vector<VulkanPhysicalDevice>& physical_devices) -> std::optional<VulkanPhysicalDevice> {
    auto curr_rating = 0;
    auto curr_match = std::optional<VulkanPhysicalDevice> {std::nullopt};

    for (const auto& device : physical_devices) {
        if (device.vulkan_version < VK_API_VERSION_1_3) continue;
        if (!device.is_swapchain_extension) continue;
        if (device.surface_info.formats.empty()) continue;
        if (device.surface_info.present_modes.empty()) continue;
        if (!device.graphics_queue_family.has_value()) continue;
        if (!device.present_queue_family.has_value()) continue;
        if (!device.compute_queue_family.has_value()) continue;

        auto features_13 = VkPhysicalDeviceVulkan13Features {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        auto features = VkPhysicalDeviceFeatures2 {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &features_13
        };

        vkGetPhysicalDeviceFeatures2(device.handle, &features);

        if (!features_13.synchronization2) continue;
        if (!features_13.dynamicRendering) continue;

        auto rating = 1;
        if (device.is_dedicated_gpu) rating += 100;
        if (features.features.samplerAnisotropy) rating += 10;
        if (device.is_shared_graphics_present_queue) rating += 5;
        if (device.is_dedicated_compute_queue) rating += 1;

        if (rating >= curr_rating) {
            curr_rating = rating;
            curr_match = device;
        }
    }

    return curr_match;
}

}

auto vk_create_device(VkInstance instance, VkSurfaceKHR surface) -> std::expected<VulkanDevice, std::string> {
    auto physical_devices = get_physical_devices(instance, surface);
    if (!physical_devices.has_value()) {
        return std::unexpected(physical_devices.error());
    }

    auto physical_device = select_physical_device(physical_devices.value());
    if (!physical_device) {
        return std::unexpected("Failed to find a suitable Vulkan-compatible device");
    }

    return std::unexpected("Vulkan device creation is not implemented");
}

auto vk_destroy_device(VulkanDevice& device) -> void {

}

}
