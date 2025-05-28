#include "render.h"
#include "render_utils.h"

void Render::createCommandPool() {
    spdlog::info("Creating Command Pool");

    vk::CommandPoolCreateInfo commandPoolCreateInfo {};
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCreateInfo.queueFamilyIndex = m_QueueGraphicFamilyIndex;

    m_CommandPool = VK_ERROR_CHECK(
        m_LogicalDevice.createCommandPool(commandPoolCreateInfo),
        "Command Pool creating caused an error"
    );

    spdlog::info("Command Pool was created successfully");
}

void Render::createCommandBuffers() {
    spdlog::info("Creating Command Buffers");

    vk::CommandBufferAllocateInfo commandBufferAllocInfo {};
    commandBufferAllocInfo.commandBufferCount = CONTAINER_COUNT(m_SwapchainImages);
    commandBufferAllocInfo.commandPool = m_CommandPool;
    commandBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;

    m_CommandBuffers = VK_ERROR_AND_EMPRY_CHECK(
        m_LogicalDevice.allocateCommandBuffers(commandBufferAllocInfo),
        "Command Buffer creating caused an error",
        "Command Buffer creating returned no results"

    );

    spdlog::info("Command Buffer was created successfully");
}