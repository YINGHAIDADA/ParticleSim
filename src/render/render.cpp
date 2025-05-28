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
	clearValue.color = { 0.1f, 0.1f, 0.1f, 1.0f };

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

void Render::draw_custom(vk::Extent2D& windowSize) {
	// Placeholder for custom rendering logic
	uint32_t imageIndex;
    vk::Result result = m_LogicalDevice.acquireNextImageKHR(
		m_Swapchain,
		UINT64_MAX,
		m_ImageAvailableSemaphore,
		nullptr,
		&imageIndex
	);
	
	vk::CommandBuffer cmd = m_CommandBuffers[imageIndex];
	vk::Image image = m_SwapchainImages[imageIndex];

    // 开始命令缓冲区
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    cmd.begin(beginInfo);

	// 1. 纹理数据更新
    if (g_texture_buffer && g_texture_width > 0 && g_texture_height > 0)
    {
        // 创建临时缓冲区和内存
        vk::DeviceSize bufferSize = g_texture_width * g_texture_height * 4;
        
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingMemory;
        createBuffer(bufferSize, 
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | 
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer, stagingMemory);
        
        // 映射内存并复制数据
        void* data;
        vkMapMemory(m_LogicalDevice, stagingMemory, 0, bufferSize, 0, &data);
        memcpy(data, g_texture_buffer, static_cast<size_t>(bufferSize));
        vkUnmapMemory(m_LogicalDevice, stagingMemory);
        
        // 复制到纹理
        transitionImageLayout(cmd, g_tex.image, 
                             vk::ImageLayout::eUndefined, 
                             vk::ImageLayout::eTransferDstOptimal);
        
        vk::BufferImageCopy region{};
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = g_texture_width;
        region.imageExtent.height = g_texture_height;
        region.imageExtent.depth = 1;
        
        cmd.copyBufferToImage(stagingBuffer, g_tex.image, 
                             vk::ImageLayout::eTransferDstOptimal, 
                             1, &region);
        
        transitionImageLayout(cmd, g_tex.image, 
                             vk::ImageLayout::eTransferDstOptimal, 
                             vk::ImageLayout::eShaderReadOnlyOptimal);
        
        // 清理临时资源
        m_LogicalDevice.destroyBuffer(stagingBuffer);
        m_LogicalDevice.freeMemory(stagingMemory);
    }

    // 2. 主场景渲染
    {
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = m_offscreen_render_pass;
        renderPassInfo.framebuffer = g_rt.framebuffer;
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = windowSize;
        
        vk::ClearValue clearColor;
        clearColor.color = vk::ClearColorValue(0.1f, 0.1f, 0.1f, 1.0f);
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        
        cmd.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
        
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
        
        // 设置视口
        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = windowSize.width;
        viewport.height = windowSize.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cmd.setViewport(0, 1, &viewport);
        
        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = windowSize;
        cmd.setScissor(0, 1, &scissor);
        
        // 绑定顶点和索引缓冲区
        vk::Buffer vertexBuffers[] = {m_VertexBuffer};
        vk::DeviceSize offsets[] = {0};
        cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
        cmd.bindIndexBuffer(m_IndexBuffer, 0, vk::IndexType::eUint16);
        
        // 绑定描述符集
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, 
                             m_pipeline_layout, 
                             0, 1, &m_main_descriptor_set, 0, nullptr);
        
        // 绘制命令
        cmd.drawIndexed(6, 1, 0, 0, 0);
        
        cmd.endRenderPass();
    }

    // 3. 后处理部分
    {
        // 亮度过滤通道
        //applyPostEffect(cmd, g_rt, g_bright_pass.rt, m_bright_pass_pipeline);
        
        // 模糊通道 (水平)
        //applyBlurPass(cmd, g_bright_pass.rt, g_blur_pass.rt, m_blur_pass_pipeline);
        
        // 模糊通道 (垂直)
        //applyBlurPass(cmd, g_blur_pass.rt, g_blur_pass.blur_render_target_b, m_blur_pass_pipeline);
        
        // 合成通道
        //applyPostEffect(cmd, g_rt, g_composite_pass.rt, m_composite_pipeline);
    }

    // 4. 获取交换链图像

    // 5. 最终呈现
    {
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_FrameBuffers[imageIndex];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = windowSize;
        
        vk::ClearValue clearColor;
        clearColor.color = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f);
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        
        cmd.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
        
        // 绘制后处理结果
        
        // 绘制UI
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_ui_pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, 
                             m_pipeline_layout, 
                             0, 1, &m_ui_descriptor_set, 0, nullptr);
        cmd.drawIndexed(6, 1, 0, 0, 0);
        
        cmd.endRenderPass();
    }
    
    // 结束命令缓冲区
    cmd.end();

    // 提交命令缓冲区
    m_LogicalDevice.resetFences(m_RenderFinishedFence);

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_SubmitSemaphore;
    
    m_GraphicQueue.submit(1, &submitInfo, m_RenderFinishedFence);

    // 呈现
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_SubmitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.pImageIndices = &imageIndex;
    m_GraphicQueue.presentKHR(presentInfo);

	m_LogicalDevice.waitForFences(
		1,
		&m_RenderFinishedFence,
		vk::True,
		UINT32_MAX
	);
}

// 重新创建交换链
void Render::recreateSwapchain(vk::Extent2D& newSize) {
    // 清理旧资源
    for (auto& framebuffer : m_FrameBuffers) {
        m_LogicalDevice.destroyFramebuffer(framebuffer);
    }
    m_FrameBuffers.clear();
    
    // 创建新交换链
    createSwapchain(newSize);
    createFrameBuffers(newSize);
}

// 辅助函数实现
void Render::applyPostEffect(vk::CommandBuffer cmd, vk::Pipeline pipeline, vk::DescriptorSet descriptorSet) {
    // 简化实现 - 实际应用中应设置特定帧缓冲和渲染通道
    // vk::RenderPassBeginInfo renderPassInfo;
    // renderPassInfo.renderPass = m_PostProcessRenderPass;
    // renderPassInfo.framebuffer = m_PostProcessFramebuffer;
    // renderPassInfo.renderArea = vk::Rect2D({0, 0}, {g_texture_width, g_texture_height});
    
    // cmd.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
    
    // cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    
    // vk::Viewport viewport;
    // viewport.x = 0.0f;
    // viewport.y = 0.0f;
    // viewport.width = static_cast<float>(g_texture_width);
    // viewport.height = static_cast<float>(g_texture_height);
    // viewport.minDepth = 0.0f;
    // viewport.maxDepth = 1.0f;
    // cmd.setViewport(0, 1, &viewport);
    
    // vk::Buffer vertexBuffers[] = {m_VertexBuffer};
    // vk::DeviceSize offsets[] = {0};
    // cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    
    // cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, 
    //                       m_PostProcessPipelineLayout, 
    //                       0, 1, &descriptorSet, 0, nullptr);
    
    // cmd.draw(6, 1, 0, 0);
    
    // cmd.endRenderPass();
}

void Render::applyBlurPass(vk::CommandBuffer cmd, vk::Pipeline pipeline, vk::DescriptorSet descriptorSet) {
    // 与applyPostEffect类似，但使用特定的模糊管线
    applyPostEffect(cmd, pipeline, descriptorSet);
}
void Render::transitionImageLayout(vk::CommandBuffer cmd, vk::Image image, 
                                  vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;
    
    if (oldLayout == vk::ImageLayout::eUndefined && 
        newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlags();
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && 
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && 
               newLayout == vk::ImageLayout::eTransferSrcOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        sourceStage = vk::PipelineStageFlagBits::eFragmentShader;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else {
        throw std::invalid_argument("Unsupported layout transition!");
    }
    
    cmd.pipelineBarrier(
        sourceStage, destinationStage,
        vk::DependencyFlags(),
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}
uint32_t Render::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = m_PhysicalDevice.getMemoryProperties();
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}