#include "render.h"
#include "render_utils.h"

void Render::createCommandPool() {
    SPDLOG_INFO("Creating Command Pool");

    vk::CommandPoolCreateInfo commandPoolCreateInfo {};
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCreateInfo.queueFamilyIndex = m_QueueGraphicFamilyIndex;

    m_CommandPool = VK_ERROR_CHECK(
        m_LogicalDevice.createCommandPool(commandPoolCreateInfo),
        "Command Pool creating caused an error"
    );

    SPDLOG_INFO("Command Pool was created successfully");
}

void Render::createCommandBuffers() {
    SPDLOG_INFO("Creating Command Buffers");

    vk::CommandBufferAllocateInfo commandBufferAllocInfo {};
    commandBufferAllocInfo.commandBufferCount = CONTAINER_COUNT(m_SwapchainImages);
    commandBufferAllocInfo.commandPool = m_CommandPool;
    commandBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;

    m_CommandBuffers = VK_ERROR_AND_EMPRY_CHECK(
        m_LogicalDevice.allocateCommandBuffers(commandBufferAllocInfo),
        "Command Buffer creating caused an error",
        "Command Buffer creating returned no results"

    );

    SPDLOG_INFO("Command Buffer was created successfully");
}

vk::CommandBuffer Render::beginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    commandBuffer = VK_ERROR_AND_EMPRY_CHECK(m_LogicalDevice.allocateCommandBuffers(allocInfo),
        "Command Buffer creating caused an error",
        "Command Buffer creating returned no results")[0];

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void Render::endSingleTimeCommands(vk::CommandBuffer &commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    m_GraphicQueue.submit(1, &submitInfo, nullptr);
    m_GraphicQueue.waitIdle();

    m_LogicalDevice.freeCommandBuffers(m_CommandPool, 1, &commandBuffer);
}