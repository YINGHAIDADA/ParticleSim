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