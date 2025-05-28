#include "render.h"
#include "render_utils.h"

void Render::createLogicalDevice() {
    spdlog::info("Creating Logical Device");

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
    spdlog::info("Logical Device was created successfully");
}