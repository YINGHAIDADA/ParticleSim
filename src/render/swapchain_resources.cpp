#include "render.h"
#include "render_utils.h"

void Render::selectSwapchainResources() {
    SPDLOG_INFO("Selecting Swapchain Resources");
    SPDLOG_INFO("  Selecting Swapchain Images");

    m_SwapchainImages = VK_ERROR_AND_EMPRY_CHECK(
        m_LogicalDevice.getSwapchainImagesKHR(m_Swapchain),
        "Swapchain Images getting caused an error",
        "Swapchain Images getting returned no results"
    );

    SPDLOG_INFO("  Creating Swapchain ImagesViews");
    std::vector<vk::ImageView> swapchainImagesViews {};
    swapchainImagesViews.reserve(m_SwapchainImages.size());

    for (vk::Image& image : m_SwapchainImages) {
        vk::ImageViewCreateInfo imageViewCreateInfo {};
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imageViewCreateInfo.format = vk::Format::eB8G8R8A8Unorm;
        imageViewCreateInfo.components = vk::ComponentSwizzle::eIdentity;
        imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        vk::ImageView imageView = VK_ERROR_CHECK(
            m_LogicalDevice.createImageView(imageViewCreateInfo),
            "Swapchain ImageView creating caused an error"
        );

        swapchainImagesViews.push_back(imageView);
    }

    m_SwapchainImagesViews = swapchainImagesViews;
    SPDLOG_INFO("Swapchain Resources selected successfully");
}