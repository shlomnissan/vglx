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

namespace vglx {

namespace {

constexpr auto kMinVulkanVersionRequired = VK_API_VERSION_1_3;

auto get_instance_extension_properties() -> std::expected<std::vector<VkExtensionProperties>, std::string> {
    auto count = uint32_t {0};
    auto result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to count Vulkan extensions: {}", string_VkResult(result)));
    }

    auto extensions = std::vector<VkExtensionProperties>(count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
    if (result != VK_SUCCESS) {
        return std::unexpected(std::format("Failed to enumerate Vulkan extensions: {}", string_VkResult(result)));
    }

    extensions.resize(count);
    return extensions;
}

auto has_instance_extension(std::string_view name) -> std::expected<bool, std::string> {
    auto extensions = get_instance_extension_properties();
    if (!extensions.has_value()) {
        return std::unexpected(extensions.error());
    }
    return std::ranges::contains(extensions.value(), name, &VkExtensionProperties::extensionName);
}

}

auto vk_create_instance(std::span<const char* const> required_extensions) -> std::expected<VulkanInstance, std::string> {
    auto version = volkGetInstanceVersion();
    if (version < kMinVulkanVersionRequired) {
        return std::unexpected(
            std::format(
                "Vulkan minimum required version {}.{}.{} not supported",
                VK_API_VERSION_MAJOR(kMinVulkanVersionRequired),
                VK_API_VERSION_MINOR(kMinVulkanVersionRequired),
                VK_API_VERSION_PATCH(kMinVulkanVersionRequired)
            )
        );
    }

    auto application_info = VkApplicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pEngineName = "VGLX",
        .apiVersion = kMinVulkanVersionRequired
    };

    return std::unexpected("Vulkan instance creation is not implemented");
}

auto vk_create_device(VkInstance instance, VkSurfaceKHR surface) -> std::expected<VulkanDevice, std::string> {
    return std::unexpected("Vulkan device creation is not implemented");
}

auto vk_destroy_device(VulkanDevice& device) -> void {}

auto vk_destroy_surface(VkInstance instance, VkSurfaceKHR surface) -> void {}

auto vk_destroy_instance(VulkanInstance& instance) -> void {}

}
