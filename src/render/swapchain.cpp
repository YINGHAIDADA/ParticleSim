#include "render.h"

void Render::createSwapchain(vk::Extent2D& windowSize) {
    SPDLOG_INFO("Creating Swapchain");

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = VK_ERROR_CHECK(
        m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface),
        "Physical Device capabilities getting caused an error"
    );

    uint32_t minImageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }

    m_SwapchainExtent = surfaceCapabilities.currentExtent;
    if (m_SwapchainExtent.height == UINT32_MAX || m_SwapchainExtent.width == UINT32_MAX) {
        m_SwapchainExtent.width = std::min(
            surfaceCapabilities.maxImageExtent.width,
            std::max(surfaceCapabilities.minImageExtent.width, windowSize.width)
        );

        m_SwapchainExtent.height = std::min(
            surfaceCapabilities.maxImageExtent.height,
            std::max(surfaceCapabilities.minImageExtent.height, windowSize.height)
        );
    }

    vk::SwapchainCreateInfoKHR swapchainCreateInfo {};
    swapchainCreateInfo.surface = m_Surface;
    swapchainCreateInfo.minImageCount = minImageCount;
    swapchainCreateInfo.imageFormat = vk::Format::eB8G8R8A8Unorm;
    swapchainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    swapchainCreateInfo.imageExtent = m_SwapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainCreateInfo.presentMode = vk::PresentModeKHR::eMailbox;
    swapchainCreateInfo.clipped = vk::True;
    swapchainCreateInfo.oldSwapchain = m_Swapchain;

    if (m_QueueGraphicFamilyIndex != m_QueuePresentFamilyIndex) {
        uint32_t queue[2] = {
            m_QueueGraphicFamilyIndex,
            m_QueuePresentFamilyIndex
        };
        
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queue;
    } else {
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapchainCreateInfo.queueFamilyIndexCount = 1;
        swapchainCreateInfo.pQueueFamilyIndices = &m_QueueGraphicFamilyIndex;
    }

    m_Swapchain = VK_ERROR_CHECK(
        m_LogicalDevice.createSwapchainKHR(swapchainCreateInfo),
        "Swapchain creating caused an error"
    );

    SPDLOG_INFO("Swapchain was created successfully");
}

// 重新创建交换链
void Render::recreateSwapchain(vk::Extent2D& newSize) {
    // 清理旧资源
    for (auto& framebuffer : m_FrameBuffers) {
        m_LogicalDevice.destroyFramebuffer(framebuffer);
    }
    m_FrameBuffers.clear();
    
    // 创建新交换链
    createSwapchain(newSize);
    selectSwapchainResources();
    createFrameBuffers(newSize);

    // 关键：重置图像栅栏数组
    m_ImageInFlight.clear();
    m_ImageInFlight.resize(m_SwapchainImages.size(), vk::Fence());
}