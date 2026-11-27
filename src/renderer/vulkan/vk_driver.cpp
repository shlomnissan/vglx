/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

#include "renderer/vulkan/vk_driver.hpp"

#include <algorithm>
#include <format>
#include <string_view>
#include <vector>
#include <ranges>

#include <vulkan/vk_enum_string_helper.h>

#include "utilities/logger.hpp"

namespace vglx {

namespace {

#ifdef NDEBUG
constexpr auto kDebugEnabled = false;
#else
constexpr auto kDebugEnabled = true;
#endif

auto get_instance_extensions() -> std::expected<std::vector<VkExtensionProperties>, std::string> {
    auto count = uint32_t {0};
    auto result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to count extensions: {}", string_VkResult(result)));
    }

    auto extensions = std::vector<VkExtensionProperties>(count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to enumerate extensions: {}", string_VkResult(result)));
    }

    extensions.resize(count);
    return extensions;
}

auto has_instance_extension(std::string_view name) -> std::expected<bool, std::string> {
    auto extensions = get_instance_extensions();
    if (!extensions.has_value()) {
        return std::unexpected(extensions.error());
    }
    return std::ranges::contains(extensions.value(), name, &VkExtensionProperties::extensionName);
}

auto get_instance_layers() -> std::expected<std::vector<VkLayerProperties>, std::string> {
    auto count = uint32_t {0};
    auto result = vkEnumerateInstanceLayerProperties(&count, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to count layers: {}", string_VkResult(result)));
    }

    auto layers = std::vector<VkLayerProperties>(count);
    result = vkEnumerateInstanceLayerProperties(&count, layers.data());
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to enumerate layers: {}", string_VkResult(result)));
    }

    layers.resize(count);
    return layers;
}

auto has_instance_layer(std::string_view name) -> std::expected<bool, std::string> {
    auto layers = get_instance_layers();
    if (!layers.has_value()) {
        return std::unexpected(layers.error());
    }
    return std::ranges::contains(layers.value(), name, &VkLayerProperties::layerName);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*
) {
    auto level = severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
        ? LogLevel::Error
        : LogLevel::Warning;
    Logger::Log {level, "Vulkan: {}", data->pMessage};
    return VK_FALSE;
}

}

auto vk_create_instance(std::span<const char* const> extensions) -> std::expected<VulkanInstance, std::string> {
    auto version = volkGetInstanceVersion();
    if (version < kVulkanTargetVersion) {
        return std::unexpected(
            std::format(
                "Vulkan minimum required version {}.{}.{} not supported",
                VK_API_VERSION_MAJOR(kVulkanTargetVersion),
                VK_API_VERSION_MINOR(kVulkanTargetVersion),
                VK_API_VERSION_PATCH(kVulkanTargetVersion)
            )
        );
    }

    auto flags = VkInstanceCreateFlags {0};
    auto required_extensions = std::vector<const char*>(extensions.begin(), extensions.end());
    auto debug_utils_enabled = false;

    auto ext_portability = has_instance_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    if (!ext_portability.has_value()) {
        return std::unexpected(ext_portability.error());
    } else if (ext_portability.value()) {
        required_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    if (kDebugEnabled) {
        auto ext_debug_utils = has_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        if (!ext_debug_utils.has_value()) {
            return std::unexpected(ext_debug_utils.error());
        } else if (ext_debug_utils.value()) {
            required_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            debug_utils_enabled = true;
        }
    }

    auto application_info = VkApplicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pEngineName = "VGLX",
        .apiVersion = kVulkanTargetVersion
    };

    auto debug_info = VkDebugUtilsMessengerCreateInfoEXT {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback
    };

    auto layers = std::vector<const char*> {};
    if (debug_utils_enabled) {
        auto result = has_instance_layer("VK_LAYER_KHRONOS_validation");
        if (!result.has_value()) {
            return std::unexpected(result.error());
        } else if (result.value()) {
            layers.emplace_back("VK_LAYER_KHRONOS_validation");
        }
    }

    auto create_info = VkInstanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = debug_utils_enabled ? &debug_info : nullptr,
        .flags = flags,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data(),
    };

    auto instance = VulkanInstance {};
    auto result = vkCreateInstance(&create_info, nullptr, &instance.handle);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to create Vulkan instance: {}", string_VkResult(result)));
    }

    volk::volkLoadInstance(instance.handle);

    if (debug_utils_enabled) {
        result = volk::vkCreateDebugUtilsMessengerEXT(instance.handle, &debug_info, nullptr, &instance.debug_messenger);
        if (result != VK_SUCCESS) {
            volk::vkDestroyInstance(instance.handle, nullptr);
            return std::unexpected(std::format("Failed to create Vulkan debug utilities: {}", string_VkResult(result)));
        }
    }

    return instance;
}

auto vk_destroy_surface(VkInstance instance, VkSurfaceKHR surface) -> void {
    if (instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
}

auto vk_destroy_instance(VulkanInstance& instance) -> void {
    if (instance.debug_messenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(instance.handle, instance.debug_messenger, nullptr);
    }

    if (instance.handle != VK_NULL_HANDLE) {
        vkDestroyInstance(instance.handle, nullptr);
    }

    instance = {};
}

}
