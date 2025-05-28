#include "render.h"
#include "render_utils.h"

void Render::createFrameBuffers(vk::Extent2D& windowSize) {
    spdlog::info("Creating Frame Buffers");

    vk::FramebufferCreateInfo frameBufferCreateInfo {};
    frameBufferCreateInfo.height = windowSize.height;
    frameBufferCreateInfo.width = windowSize.width;
    frameBufferCreateInfo.renderPass = m_RenderPass;
    frameBufferCreateInfo.layers = 1;
    frameBufferCreateInfo.attachmentCount = 1;

    std::vector<vk::Framebuffer> frameBuffers {};
    frameBuffers.reserve(m_SwapchainImagesViews.size());

    for (auto& imageView : m_SwapchainImagesViews) {
        frameBufferCreateInfo.pAttachments = &imageView;

        vk::Framebuffer frameBuffer = VK_ERROR_CHECK(
            m_LogicalDevice.createFramebuffer(frameBufferCreateInfo),
            "Frame Buffer creating caused an error"
        );

        frameBuffers.push_back(frameBuffer);
    }

    m_FrameBuffers = frameBuffers;
    spdlog::info("Frame Buffer created successfully");
}

void Render::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) {
    vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eExclusive);
    buffer = VK_ERROR_CHECK(m_LogicalDevice.createBuffer(bufferInfo), "Buffer creating caused an error");
    spdlog::info("Buffer created successfully");
    vk::MemoryRequirements memRequirements = m_LogicalDevice.getBufferMemoryRequirements(buffer);
    vk::MemoryAllocateInfo allocInfo(memRequirements.size, findMemoryType(memRequirements.memoryTypeBits, properties));
    bufferMemory = VK_ERROR_CHECK(m_LogicalDevice.allocateMemory(allocInfo), "Buffer memory allocation caused an error");
    spdlog::info("Buffer memory allocated successfully");
    m_LogicalDevice.bindBufferMemory(buffer, bufferMemory, 0);
}