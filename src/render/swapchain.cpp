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
    swapchainCreateInfo.presentMode = vk::PresentModeKHR::eFifo;
    swapchainCreateInfo.clipped = vk::True;
    const vk::SwapchainKHR oldSwapchain = m_Swapchain;
    swapchainCreateInfo.oldSwapchain = oldSwapchain;

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
    if (oldSwapchain) {
        m_LogicalDevice.destroySwapchainKHR(oldSwapchain);
    }

    SPDLOG_INFO("Swapchain was created successfully");
}

// 重新创建交换链
void Render::destroyFramebuffersAndViews() {
    for (vk::Framebuffer framebuffer : m_FrameBuffers) {
        m_LogicalDevice.destroyFramebuffer(framebuffer);
    }
    m_FrameBuffers.clear();
    for (vk::ImageView view : m_SwapchainImagesViews) {
        m_LogicalDevice.destroyImageView(view);
    }
    m_SwapchainImagesViews.clear();
    m_SwapchainImages.clear();
}

void Render::recreateSwapchain(vk::Extent2D& newSize) {
    if (!m_LogicalDevice) return;
    if (newSize.width == 0 || newSize.height == 0) {
        m_SwapchainExtent = vk::Extent2D{0, 0};
        return;
    }

    m_LogicalDevice.waitIdle();
    destroyFramebuffersAndViews();
    createSwapchain(newSize);
    selectSwapchainResources();
    createPresentSemaphores();
    createFrameBuffers();
    m_ImageInFlight.assign(m_SwapchainImages.size(), vk::Fence{});
}