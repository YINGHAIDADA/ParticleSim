#include "render.h"

void Render::createSyncObjects() {
    SPDLOG_INFO("Creating Synchronization objects");
    if(m_SwapchainImageCount == 0) {
        SPDLOG_ERROR("Swapchain image count is zero, cannot create synchronization objects");
        return;
    }

    m_ImageAvailableSemaphores.resize(m_SwapchainImageCount);
    m_RenderFinishedSemaphores.resize(m_SwapchainImageCount);
    // 创建栅栏（初始化为已触发状态）
    m_InFlightFences.resize(m_SwapchainImageCount);
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    vk::SemaphoreCreateInfo semaphoreInfo;
    for (int i = 0; i < m_SwapchainImageCount; i++) {
        m_ImageAvailableSemaphores[i] = VK_ERROR_CHECK(
            m_LogicalDevice.createSemaphore(semaphoreInfo),
            "Image Available Semaphore creating caused an error");
        m_RenderFinishedSemaphores[i] = VK_ERROR_CHECK(
            m_LogicalDevice.createSemaphore(semaphoreInfo),
            "Render Finished Semaphore creating caused an error");
        m_InFlightFences[i] = VK_ERROR_CHECK(
            m_LogicalDevice.createFence(fenceInfo),
            "In Flight Fence creating caused an error");
    }

    vk::FenceCreateInfo fenceCreateInfo {};
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    // 初始化图像栅栏 (初始化为空) - 关键修改
    m_ImageInFlight.resize(m_SwapchainImages.size(), vk::Fence());

    SPDLOG_INFO("Synchronization objects were created successfully");
}