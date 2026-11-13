#include "render.h"
#include "render_utils.h"

#include <cstring>

void Render::createLogicalDevice() {
    SPDLOG_INFO("Creating Logical Device");

    const float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    vk::DeviceQueueCreateInfo graphicQueueCreateInfo {};
	graphicQueueCreateInfo.queueFamilyIndex = m_QueueGraphicFamilyIndex;
	graphicQueueCreateInfo.queueCount = 1;
	graphicQueueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(graphicQueueCreateInfo);

    if (m_QueuePresentFamilyIndex != m_QueueGraphicFamilyIndex) {
        vk::DeviceQueueCreateInfo presentQueueCreateInfo {};
        presentQueueCreateInfo.queueFamilyIndex = m_QueuePresentFamilyIndex;
        presentQueueCreateInfo.queueCount = 1;
        presentQueueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(presentQueueCreateInfo);
    }

    std::vector<const char*> requiredExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const auto deviceExtensions = m_PhysicalDevice.enumerateDeviceExtensionProperties();
    if (deviceExtensions.result == vk::Result::eSuccess) {
        for (const vk::ExtensionProperties& extension : deviceExtensions.value) {
            if (std::strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0) {
                requiredExtensions.push_back("VK_KHR_portability_subset");
                break;
            }
        }
    }

    vk::DeviceCreateInfo deviceCreateInfo {};
	deviceCreateInfo.queueCreateInfoCount = CONTAINER_COUNT(queueCreateInfos);
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = CONTAINER_COUNT(requiredExtensions);
	deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

    m_LogicalDevice = VK_ERROR_CHECK(
        m_PhysicalDevice.createDevice(deviceCreateInfo),
        "Logical Device creating caused an error"
    );

    m_GraphicQueue = m_LogicalDevice.getQueue(m_QueueGraphicFamilyIndex, 0);
    m_PresentQueue = m_LogicalDevice.getQueue(m_QueuePresentFamilyIndex, 0);
    SPDLOG_INFO("Logical Device was created successfully");
}