#include "render.h"

void Render::createSyncObjects() {
    SPDLOG_INFO("Creating Synchronization objects");
    if(m_SwapchainImageCount == 0) {
        SPDLOG_ERROR("Swapchain image count is zero, cannot create synchronization objects");
        return;
    }

    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    vk::SemaphoreCreateInfo semaphoreInfo;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_ImageAvailableSemaphores[i] = VK_ERROR_CHECK(
            m_LogicalDevice.createSemaphore(semaphoreInfo),
            "Image Available Semaphore creating caused an error");
        m_InFlightFences[i] = VK_ERROR_CHECK(
            m_LogicalDevice.createFence(fenceInfo),
            "In Flight Fence creating caused an error");
    }
    createPresentSemaphores();
    m_ImageInFlight.resize(m_SwapchainImages.size(), vk::Fence());

    SPDLOG_INFO("Synchronization objects were created successfully");
}

void Render::createPresentSemaphores() {
    for (vk::Semaphore semaphore : m_RenderFinishedSemaphores) {
        if (semaphore) m_LogicalDevice.destroySemaphore(semaphore);
    }
    m_RenderFinishedSemaphores.assign(m_SwapchainImages.size(), vk::Semaphore{});

    vk::SemaphoreCreateInfo semaphoreInfo;
    for (vk::Semaphore& semaphore : m_RenderFinishedSemaphores) {
        semaphore = VK_ERROR_CHECK(
            m_LogicalDevice.createSemaphore(semaphoreInfo),
            "Render Finished Semaphore creating caused an error");
    }
}