#include "render.h"

Render::Render(SDL_Window* window) { m_Window = window; }
Render::~Render() {}

void Render::init(vk::Extent2D& windowSize) {
    try {
        spdlog::info("Vulkan init");

        Render::createInstance();
        Render::createDebugMessenger();
        Render::createSurface();
        Render::selectPhysicalDevice();
        Render::selectQueueFamilyIndexes();
        Render::createLogicalDevice();
        Render::createShaderModules();
        Render::createSyncObjects();
        Render::createSwapchain(windowSize);
        Render::selectSwapchainResources();
        Render::createCommandPool();
        Render::createCommandBuffers();
        Render::createRenderPass();
        Render::createFrameBuffers(windowSize);
        Render::createPipeline();
    } catch(const std::runtime_error& error) {
        spdlog::error("Vulkan init error: {}", error.what());
    }
}

void Render::resize(vk::Extent2D& newWindowSize) {
    try {
        Render::createSwapchain(newWindowSize);
        Render::selectSwapchainResources();
        Render::createCommandBuffers();
        Render::createFrameBuffers(newWindowSize);
    } catch(const std::runtime_error& error) {
        spdlog::error("Vulkan resize error: {}", error.what());
    }
}

void Render::draw(vk::Extent2D& windowSize) {
	uint32_t imageIndex = m_LogicalDevice.acquireNextImageKHR(
		m_Swapchain,
		UINT64_MAX,
		m_ImageAvailableSemaphore,
		nullptr
	).value;
	
	vk::CommandBuffer commandBuffer = m_CommandBuffers[imageIndex];
	vk::Image image = m_SwapchainImages[imageIndex];

	vk::CommandBufferBeginInfo beginInfo {};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(beginInfo);

	vk::ClearValue clearValue {};
	clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	vk::RenderPassBeginInfo renderPassBeginInfo {};
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.renderArea.extent = windowSize;
	renderPassBeginInfo.framebuffer = m_FrameBuffers[imageIndex];
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearValue;

	commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

	vk::Rect2D scissor {};
	scissor.extent = windowSize;

	vk::Viewport viewport {};
	viewport.width = windowSize.width;
	viewport.height = windowSize.height;
	viewport.maxDepth = 1.0f;

	commandBuffer.setScissor(0, 1, &scissor);
	commandBuffer.setViewport(0, 1, &viewport);
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);

	commandBuffer.draw(3, 1, 0, 0);

	commandBuffer.endRenderPass();
	commandBuffer.end();

	m_LogicalDevice.resetFences(m_RenderFinishedFence);

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submitInfo {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_SubmitSemaphore;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = &waitStage;
	m_GraphicQueue.submit(submitInfo, m_RenderFinishedFence);

	vk::PresentInfoKHR presentInfo {};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_SubmitSemaphore;
	m_GraphicQueue.presentKHR(presentInfo);

	m_LogicalDevice.waitForFences(
		1,
		&m_RenderFinishedFence,
		vk::True,
		UINT32_MAX
	);
}