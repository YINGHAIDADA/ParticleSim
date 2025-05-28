#include "render.h"
#include "render_utils.h"

void Render::selectPhysicalDevice() {
    spdlog::info("Select Physical Device");

    std::vector<vk::PhysicalDevice> physicalDevices = VK_ERROR_AND_EMPRY_CHECK(
        m_Instance.enumeratePhysicalDevices(),
        "Physical Devices enumeration caused an error",
        "Physical Devices enumeration returned no results"
    );

    std::unordered_set<std::string_view> requiredExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    vk::PhysicalDevice localPhysicalDevice;

    for (vk::PhysicalDevice& physicalDevice : physicalDevices) {
        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

        std::vector<vk::ExtensionProperties> supportedExtensions = VK_ERROR_AND_EMPRY_CHECK(
            physicalDevice.enumerateDeviceExtensionProperties(),
            "Physical Device extensions enumeration caused an error",
            "Physical Device extensions enumeration returned no results"
        );

        std::unordered_set<std::string_view> availableExtensions;
        INSERT_ELEMENTS_M(availableExtensions, supportedExtensions, extensionName);

        spdlog::info(
            "Check required extensions for {} ({})",
            properties.deviceName.data(),
            properties.deviceID
        );

        bool requiredExtensionsSupported = true;
        for (std::string_view requiredExtension : requiredExtensions) {
            if (availableExtensions.contains(requiredExtension)) {
                spdlog::info("  + {}", requiredExtension);
            } else {
                spdlog::info("  - {}", requiredExtension);
                requiredExtensionsSupported = false;
            }
        }

        if (requiredExtensionsSupported && !localPhysicalDevice) {
            spdlog::info("  DEVICE SELECTED");
            localPhysicalDevice = physicalDevice;
        }
    }

    if (localPhysicalDevice) {
        m_PhysicalDevice = localPhysicalDevice;
        return;
    }

    throw std::runtime_error("Suitable physical devise was not found");
}