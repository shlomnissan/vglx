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
#include <unordered_set>
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
    VkPhysicalDeviceProperties properties {};
    VulkanSurfaceProperties surface_info {};
    VulkanDeviceFeatures features {};
    std::optional<uint32_t> graphics_queue_family {std::nullopt};
    std::optional<uint32_t> present_queue_family {std::nullopt};
    std::optional<uint32_t> compute_queue_family {std::nullopt};
    bool is_swapchain_extension {false};
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
            .properties = properties,
            .is_swapchain_extension = ext_swapchain.value(),
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

auto select_physical_device(std::vector<VulkanPhysicalDevice>& physical_devices) -> std::optional<VulkanPhysicalDevice> {
    auto curr_rating = 0;
    auto curr_match = std::optional<VulkanPhysicalDevice> {std::nullopt};

    for (auto& device : physical_devices) {
        if (device.properties.apiVersion < kVulkanTargetVersion) continue;
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

        device.features.synchronization2 = true;
        device.features.dynamic_rendering = true;
        device.features.sampler_anisotropy = features.features.samplerAnisotropy;

        auto rating = 1;
        if (device.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) rating += 100;
        if (device.features.sampler_anisotropy) rating += 10;
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

    auto selection = select_physical_device(physical_devices.value());
    if (!selection) {
        return std::unexpected("Failed to find a suitable Vulkan-compatible device");
    }

    auto& physical_device = selection.value();

    auto unique_queues = std::unordered_set<uint32_t> {{
        physical_device.graphics_queue_family.value(),
        physical_device.present_queue_family.value(),
        physical_device.compute_queue_family.value()
    }};

    auto queue_priority = 1.0f;
    auto queues = std::vector<VkDeviceQueueCreateInfo>();
    for (auto unique_queue : unique_queues) {
        queues.push_back({
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = unique_queue,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        });
    }

    auto features_13 = VkPhysicalDeviceVulkan13Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = physical_device.features.synchronization2,
        .dynamicRendering = physical_device.features.dynamic_rendering
    };

    auto features = VkPhysicalDeviceFeatures2 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features_13,
        .features = {
            .samplerAnisotropy = physical_device.features.sampler_anisotropy
        }
    };

    auto extensions = std::vector<const char*> {"VK_KHR_swapchain"};

    auto ext_portability = has_device_extension(physical_device.handle, "VK_KHR_portability_subset");
    if (!ext_portability.has_value()) {
        return std::unexpected(ext_portability.error());
    } else if (ext_portability.value()) {
        extensions.emplace_back("VK_KHR_portability_subset");
    }

    auto create_info = VkDeviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .queueCreateInfoCount = static_cast<uint32_t>(queues.size()),
        .pQueueCreateInfos = queues.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = nullptr
    };

    auto device = VulkanDevice {
        .adapter = physical_device.handle,
        .graphics = VulkanQueue {.family_index = physical_device.graphics_queue_family.value()},
        .compute = VulkanQueue {.family_index = physical_device.compute_queue_family.value()},
        .present = VulkanQueue {.family_index = physical_device.present_queue_family.value()},
        .properties = physical_device.properties,
        .features = physical_device.features
    };

    auto result = vkCreateDevice(physical_device.handle, &create_info, nullptr, &device.handle);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to create Vulkan device: {}", string_VkResult(result)));
    }

    volk::volkLoadDevice(device.handle);

    auto allocator = VmaAllocatorCreateInfo {
        .physicalDevice = device.adapter,
        .device = device.handle,
        .instance = instance,
        .vulkanApiVersion = kVulkanTargetVersion
    };

    auto functions = VmaVulkanFunctions {};
    result = vmaImportVulkanFunctionsFromVolk(&allocator, &functions);
    if (result != VK_SUCCESS) {
        vkDestroyDevice(device.handle, nullptr);
        return std::unexpected(std::format(
            "Failed to import Vulkan functions: {}", string_VkResult(result)
        ));
    }

    allocator.pVulkanFunctions = &functions;

    result = vmaCreateAllocator(&allocator, &device.allocator);
    if (result != VK_SUCCESS) {
        vkDestroyDevice(device.handle, nullptr);
        return std::unexpected(std::format("Failed to create Vulkan memory allocator: {}", string_VkResult(result)));
    }

    vkGetDeviceQueue(device.handle, device.graphics.family_index, 0, &device.graphics.handle);
    vkGetDeviceQueue(device.handle, device.compute.family_index, 0, &device.compute.handle);
    vkGetDeviceQueue(device.handle, device.present.family_index, 0, &device.present.handle);

    return device;
}

auto vk_destroy_device(VulkanDevice& device) -> void {
    if (device.allocator != nullptr) {
        vmaDestroyAllocator(device.allocator);
    }

    if (device.handle != VK_NULL_HANDLE) {
        vkDestroyDevice(device.handle, nullptr);
    }

    device = {};
}

}
