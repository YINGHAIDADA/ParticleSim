#include "render.h"

void Render::createSyncObjects() {
    spdlog::info("Creating Synchronization objects");

    m_ImageAvailableSemaphore = VK_ERROR_CHECK(
        m_LogicalDevice.createSemaphore(vk::SemaphoreCreateInfo {}),
        "Image Available Semaphore creating caused an error"
    );

    m_SubmitSemaphore = VK_ERROR_CHECK(
        m_LogicalDevice.createSemaphore(vk::SemaphoreCreateInfo {}),
        "Submit Semaphore creating caused an error"
    );

    vk::FenceCreateInfo fenceCreateInfo {};
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    m_RenderFinishedFence = VK_ERROR_CHECK(
        m_LogicalDevice.createFence(fenceCreateInfo),
        "Render Finished Fence creating caused an error"
    );

    spdlog::info("Synchronization objects were created successfully");
}