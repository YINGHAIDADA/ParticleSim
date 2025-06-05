// #define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "render.h"
// VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

Render::Render(SDL_Window* window) { m_Window = window; }
Render::~Render() {
    // cleanResources();
}

void Render::init(vk::Extent2D& windowSize) {
    try {
        SPDLOG_INFO("Vulkan init");

        Render::createInstance();
        Render::createDebugMessenger();
        Render::createSurface();
        Render::selectPhysicalDevice();
        Render::selectQueueFamilyIndexes();
        Render::createLogicalDevice();
        Render::createShaderModules();
        Render::createSwapchain(windowSize);
        Render::selectSwapchainResources();
        Render::createSyncObjects();
        Render::createCommandPool();
        Render::createCommandBuffers();
        Render::createRenderPass();
        Render::createFrameBuffers(windowSize);
        Render::createPipeline();

        //Render::initResources();


    } catch(const std::runtime_error& error) {
        spdlog::error("Vulkan init error: {}", error.what());
    }
    
    // Utility function to begin a single-time command buffer
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

    m_LogicalDevice.waitForFences(
        1,
        &m_InFlightFences[m_CurrentFrame],
        vk::True,
        UINT64_MAX
    );
    m_LogicalDevice.resetFences(1,&m_InFlightFences[m_CurrentFrame]);
    
    vk::ResultValue<uint32_t> acquireResult = m_LogicalDevice.acquireNextImageKHR(
        m_Swapchain,
		UINT64_MAX,
		m_ImageAvailableSemaphores[m_CurrentFrame],
		nullptr
	);
    
    uint32_t imageIndex = acquireResult.value;
    

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


	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submitInfo {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];
	m_GraphicQueue.submit(submitInfo, m_InFlightFences[m_CurrentFrame]);

	vk::PresentInfoKHR presentInfo {};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];

	m_PresentQueue.presentKHR(presentInfo);


	// 更新帧索引
    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Render::draw_custom(vk::Extent2D& windowSize) {

    m_LogicalDevice.waitForFences(
		1,
		&m_InFlightFences[m_CurrentFrame],
		vk::True,
		UINT32_MAX
	);

    m_LogicalDevice.resetFences(1,&m_InFlightFences[m_CurrentFrame]);
	// Placeholder for custom rendering logic
	uint32_t imageIndex;
    vk::Result result = m_LogicalDevice.acquireNextImageKHR(
		m_Swapchain,
		UINT64_MAX,
		m_ImageAvailableSemaphores[m_CurrentFrame],
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

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];
    
    m_GraphicQueue.submit(1, &submitInfo, m_InFlightFences[m_CurrentFrame]);

    // 呈现
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.pImageIndices = &imageIndex;

    m_GraphicQueue.presentKHR(presentInfo);

    // 更新帧索引
    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

void Render::draw_ui(vk::Extent2D &windowSize)
{
    // 开始UI渲染通道
    vk::CommandBufferBeginInfo beginInfo{};
    m_CommandBuffers[0].begin(beginInfo);
    
    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.renderPass = m_RenderPass;
    renderPassInfo.framebuffer = m_FrameBuffers[0];
    renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
    renderPassInfo.renderArea.extent = windowSize;
    
    vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    m_CommandBuffers[0].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    
    // 设置视口和裁剪
    vk::Viewport viewport(0.0f, 0.0f, 
                         static_cast<float>(windowSize.width), 
                         static_cast<float>(windowSize.height), 
                         0.0f, 1.0f);
    m_CommandBuffers[0].setViewport(0, viewport);
    
    vk::Rect2D scissor({0, 0}, windowSize);
    m_CommandBuffers[0].setScissor(0, scissor);
    
    // 渲染文本
    float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    drawText(m_CommandBuffers[0], "Hello Vulkan!", 50.0f, 50.0f, 1.0f, white);
    
    float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    drawText(m_CommandBuffers[0], "SDL3 + Vulkan Text Rendering", 50.0f, 100.0f, 1.0f, red);
    
    // 结束渲染通道
    m_CommandBuffers[0].endRenderPass();
    m_CommandBuffers[0].end();
}

void Render::drawText(vk::CommandBuffer cmd, const std::string &text, float x, float y, float scale, const float color[4])
{
    // 确保有字符数据可用
    if (m_glyphs.empty() || text.empty()) {
        return;
    }
    // 更新描述符集以绑定字体纹理
    vk::DescriptorImageInfo imageInfo(
        g_tex_font.sampler,
        g_tex_font.view,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );
    
    vk::WriteDescriptorSet descriptorWrite{};
    descriptorWrite.dstSet = m_ui_descriptor_set;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
    
    m_LogicalDevice.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
    // 绑定UI管线
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_ui_pipeline);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, 
        m_pipeline_layout, 
        0, 
        1, 
        &m_ui_descriptor_set, 
        0, 
        nullptr
    );
    // 设置文本颜色 (通过push constant)
    cmd.pushConstants(
        m_pipeline_layout,
        vk::ShaderStageFlagBits::eFragment,
        0, 
        4 * sizeof(float),
        static_cast<const void*>(color)
    );
    // 计算屏幕到NDC的转换因子
    // Use the swapchain extent for width and height
    const float ndcScaleX = 2.0f / static_cast<float>(m_SwapchainExtent.width);
    const float ndcScaleY = 2.0f / static_cast<float>(m_SwapchainExtent.height);
    const float ndcOffsetX = -1.0f;
    const float ndcOffsetY = -1.0f;
    // 为每个字符准备顶点数据
    float startX = x;
    for (char c : text) {
        // 跳过未定义的字符
        if (m_glyphs.find(c) == m_glyphs.end()) continue;
        
        const GlyphInfo& g = m_glyphs[c];
        
        // 计算屏幕位置
        float x0 = startX + g.xoff * scale;
        float y0 = y + g.yoff * scale;
        float x1 = x0 + (g.x1 - g.x0) * scale;
        float y1 = y0 + (g.y1 - g.y0) * scale;
        
        // 转换为NDC坐标
        float ndcX0 = x0 * ndcScaleX + ndcOffsetX;
        float ndcY0 = y0 * ndcScaleY + ndcOffsetY;
        float ndcX1 = x1 * ndcScaleX + ndcOffsetX;
        float ndcY1 = y1 * ndcScaleY + ndcOffsetY;
        
        // 计算UV坐标
        float u0 = g.x0 / static_cast<float>(g_tex_font.width);
        float v0 = g.y0 / static_cast<float>(g_tex_font.height);
        float u1 = g.x1 / static_cast<float>(g_tex_font.width);
        float v1 = g.y1 / static_cast<float>(g_tex_font.height);
        
        // 为字符创建四边形 (两个三角形)
        TextVertex vertices[6] = {
            // 第一个三角形
            {{ndcX0, ndcY1}, {u0, v1}}, // 左下
            {{ndcX0, ndcY0}, {u0, v0}}, // 左上
            {{ndcX1, ndcY0}, {u1, v0}}, // 右上
            
            // 第二个三角形
            {{ndcX0, ndcY1}, {u0, v1}}, // 左下
            {{ndcX1, ndcY0}, {u1, v0}}, // 右上
            {{ndcX1, ndcY1}, {u1, v1}}  // 右下
        };
        
        // 上传顶点数据到动态缓冲区
        void* data;
        vkMapMemory(
            m_LogicalDevice, 
            m_DynamicVertexBufferMemory, 
            0, 
            sizeof(vertices), 
            0, 
            &data
        );
        memcpy(data, vertices, sizeof(vertices));
        vkUnmapMemory(m_LogicalDevice, m_DynamicVertexBufferMemory);
        
        // 绑定顶点缓冲区
        VkBuffer vertexBuffers[] = {m_DynamicVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        
        // 绘制字符
        vkCmdDraw(cmd, 6, 1, 0, 0);
        
        // 前进到下一个字符位置
        startX += g.xadvance * scale;
    }
}


void Render::createTexture(Texture& tex, uint32_t width, uint32_t height, 
                          vk::Format format, vk::ImageUsageFlags usage, 
                          void* initialData) {
    // 创建图像
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    
    tex.image = VK_ERROR_CHECK(m_LogicalDevice.createImage(imageInfo), "Failed to create texture image");
    
    // 分配内存
    vk::MemoryRequirements memRequirements = m_LogicalDevice.getImageMemoryRequirements(tex.image);
    
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, 
                                             vk::MemoryPropertyFlagBits::eDeviceLocal);
    
    tex.memory = VK_ERROR_CHECK(m_LogicalDevice.allocateMemory(allocInfo), "Failed to allocate texture memory");
    m_LogicalDevice.bindImageMemory(tex.image, tex.memory, 0);
    
    // 创建图像视图
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = tex.image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    tex.view = VK_ERROR_CHECK(m_LogicalDevice.createImageView(viewInfo), "Failed to create texture image view");
    
    // 创建采样器
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    tex.sampler = VK_ERROR_CHECK(m_LogicalDevice.createSampler(samplerInfo), "Failed to create texture sampler");
    
    // 如果有初始数据，上传到纹理
    if (initialData) {
        vk::CommandBuffer cmd = beginSingleTimeCommands();
        
        // 转换图像布局
        transitionImageLayout(cmd, tex.image, 
                             vk::ImageLayout::eUndefined, 
                             vk::ImageLayout::eTransferDstOptimal);
        
        // 创建暂存缓冲区
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingMemory;
        createBuffer(width * height * 4, 
                    vk::BufferUsageFlagBits::eTransferSrc,
                    vk::MemoryPropertyFlagBits::eHostVisible | 
                    vk::MemoryPropertyFlagBits::eHostCoherent,
                    stagingBuffer, stagingMemory);
        
        // 复制数据到暂存缓冲区
        void* data;
        vkMapMemory(m_LogicalDevice, stagingMemory, 0, width * height * 4, 0, &data);
        memcpy(data, initialData, static_cast<size_t>(width * height * 4));
        vkUnmapMemory(m_LogicalDevice, stagingMemory);
        
        // 从缓冲区复制到图像
        vk::BufferImageCopy region;
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = vk::Offset3D{0, 0, 0};
        region.imageExtent = vk::Extent3D{width, height, 1};
        
        cmd.copyBufferToImage(stagingBuffer, tex.image, 
                             vk::ImageLayout::eTransferDstOptimal, 
                             1, &region);
        
        // 转换到着色器只读布局
        transitionImageLayout(cmd, tex.image, 
                             vk::ImageLayout::eTransferDstOptimal, 
                             vk::ImageLayout::eShaderReadOnlyOptimal);
        
        endSingleTimeCommands(cmd);
        
        // 清理暂存资源
        m_LogicalDevice.destroyBuffer(stagingBuffer);
        m_LogicalDevice.freeMemory(stagingMemory);
    }
}

void Render::initResources() {
    
    // 创建描述符集布局
    vk::DescriptorSetLayoutBinding samplerBinding;
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;
    
    m_descriptor_set_layout = VK_ERROR_CHECK(m_LogicalDevice.createDescriptorSetLayout(layoutInfo), "Failed to create descriptor set layout");
    // 创建描述符池
    vk::DescriptorPoolSize poolSize;
    poolSize.type = vk::DescriptorType::eCombinedImageSampler;
    poolSize.descriptorCount = 3; // 主纹理、UI纹理和渲染目标
    
    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 3;
    
    m_descriptor_pool = VK_ERROR_CHECK(m_LogicalDevice.createDescriptorPool(poolInfo), "Failed to create descriptor pool!");
    // 分配描述符集
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = m_descriptor_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptor_set_layout;
    
    m_main_descriptor_set = VK_ERROR_AND_EMPRY_CHECK(m_LogicalDevice.allocateDescriptorSets(allocInfo),
                            "Failed to allocate descriptor sets",
                            "return EmptyDescriptorSet")[0];
    m_ui_descriptor_set = VK_ERROR_AND_EMPRY_CHECK(m_LogicalDevice.allocateDescriptorSets(allocInfo),
                            "Failed to allocate descriptor sets",
                            "return EmptyDescriptorSet")[0];
    // 顶点数据
    float v_data[] = {
        // Positions  UVs
        -1.0f, -1.0f,  0.0f, 0.0f, // Top Left
         1.0f, -1.0f,  1.0f, 0.0f, // Top Right 
        -1.0f,  1.0f,  0.0f, 1.0f, // Bottom Left
         1.0f,  1.0f,  1.0f, 1.0f   // Bottom Right
    };
    uint32_t i_data[] = {
        0, 2, 3,
        3, 1, 0
    };
    // 创建顶点缓冲区
    createBuffer(sizeof(v_data), 
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | 
                vk::MemoryPropertyFlagBits::eHostCoherent,
                m_VertexBuffer, m_VertexBufferMemory);
    
    // 映射内存并复制顶点数据
    void* data;
    vkMapMemory(m_LogicalDevice, m_VertexBufferMemory, 0, sizeof(v_data), 0, &data);
    memcpy(data, v_data, sizeof(v_data));
    vkUnmapMemory(m_LogicalDevice, m_VertexBufferMemory);
    // 创建索引缓冲区
    createBuffer(sizeof(i_data), 
                vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | 
                vk::MemoryPropertyFlagBits::eHostCoherent,
                m_IndexBuffer, m_IndexBufferMemory);
    
    // 映射内存并复制索引数据
    vkMapMemory(m_LogicalDevice, m_IndexBufferMemory, 0, sizeof(i_data), 0, &data);
    memcpy(data, i_data, sizeof(i_data));
    vkUnmapMemory(m_LogicalDevice, m_IndexBufferMemory);

    // 创建动态顶点缓冲区
    createBuffer(sizeof(TextVertex) * 6 * 128, 
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | 
                vk::MemoryPropertyFlagBits::eHostCoherent,
                m_DynamicVertexBuffer, m_DynamicVertexBufferMemory);

    // 初始化纹理缓冲区
    g_texture_buffer = new uint8_t[g_texture_width * g_texture_height * 4];
    memset(g_texture_buffer, 0, g_texture_width * g_texture_height * 4);

    g_ui_buffer = new uint8_t[g_texture_width * g_texture_height * 4];
    memset(g_ui_buffer, 0, g_texture_width * g_texture_height * 4);
    
    // 创建主纹理
    createTexture(g_tex, g_texture_width, g_texture_height, 
                 vk::Format::eR8G8B8A8Unorm, 
                 vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                 g_texture_buffer);
    // 创建UI纹理
    createTexture(g_tex_ui, g_texture_width, g_texture_height, 
                 vk::Format::eR8G8B8A8Unorm, 
                 vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                 g_ui_buffer);
    // 创建离屏渲染目标
    createRenderTarget(g_rt, g_texture_width, g_texture_height, 
                      vk::Format::eR8G8B8A8Unorm,
                      vk::ImageUsageFlagBits::eColorAttachment | 
                      vk::ImageUsageFlagBits::eSampled);
    // 创建离屏渲染通道
    createOffscreenRenderPass();
    // 创建主渲染管线
    createMainPipeline();
    createUIPipeline();
    // 创建后处理通道
    //createPostEffectPasses();
    // 加载UI字体
    construct_font_data();
}

void Render::cleanResources()
{
    // 销毁描述符集
    m_LogicalDevice.destroyDescriptorSetLayout(m_descriptor_set_layout);
    m_LogicalDevice.destroyDescriptorPool(m_descriptor_pool);

    // 销毁信号量和栅栏
    for (int i = 0; i < m_SwapchainImageCount; i++) {
        m_LogicalDevice.destroySemaphore(m_ImageAvailableSemaphores[i]);
        m_LogicalDevice.destroySemaphore(m_RenderFinishedSemaphores[i]);
        m_LogicalDevice.destroyFence(m_InFlightFences[i]);
    }
}

void Render::construct_font_data() {
    // 假设我们有一个字体文件路径
    const char* fontPath = "assets/fonts/default.ttf";
    
    // 字体参数
    const int fontSize = 24;
    const int atlasWidth = 512;
    const int atlasHeight = 512;
    
    // 使用stb_truetype加载字体
    FILE* fontFile = fopen(fontPath, "rb");
    if (!fontFile) {
        spdlog::error("Failed to open font file: {}", fontPath);
        return;
    }
    
    // 读取字体数据
    fseek(fontFile, 0, SEEK_END);
    size_t fontSizeBytes = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);
    
    unsigned char* fontBuffer = new unsigned char[fontSizeBytes];
    fread(fontBuffer, 1, fontSizeBytes, fontFile);
    fclose(fontFile);
    
    // 初始化stb_truetype
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, fontBuffer, 0)) {
        spdlog::error("Failed to initialize font");
        delete[] fontBuffer;
        return;
    }
    
    // 创建字体纹理图集
    unsigned char* bitmap = new unsigned char[atlasWidth * atlasHeight];
    memset(bitmap, 0, atlasWidth * atlasHeight);
    
    // 计算缩放因子
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
    
    // 存储字符信息
    std::unordered_map<char, stbtt_bakedchar> charData;
    stbtt_bakedchar* cdata = new stbtt_bakedchar[256];
    
    // 烘焙字体到纹理
    stbtt_BakeFontBitmap(fontBuffer, 0, fontSize, 
                         bitmap, atlasWidth, atlasHeight, 
                         32, 96, cdata);
    
    // 转换单通道位图为RGBA
    uint8_t* rgbaBitmap = new uint8_t[atlasWidth * atlasHeight * 4];
    for (int y = 0; y < atlasHeight; y++) {
        for (int x = 0; x < atlasWidth; x++) {
            uint8_t value = bitmap[y * atlasWidth + x];
            rgbaBitmap[(y * atlasWidth + x) * 4 + 0] = 255; // R
            rgbaBitmap[(y * atlasWidth + x) * 4 + 1] = 255; // G
            rgbaBitmap[(y * atlasWidth + x) * 4 + 2] = 255; // B
            rgbaBitmap[(y * atlasWidth + x) * 4 + 3] = value; // A
        }
    }
    
    // 创建字体纹理
    createTexture(g_tex_font, atlasWidth, atlasHeight, 
                 vk::Format::eR8G8B8A8Unorm,
                 vk::ImageUsageFlagBits::eSampled | 
                 vk::ImageUsageFlagBits::eTransferDst,
                 rgbaBitmap);
    
    // 存储字符信息
    for (int i = 32; i < 128; i++) {
        char c = static_cast<char>(i);
        stbtt_bakedchar* b = &cdata[i - 32];
        
        GlyphInfo glyph;
        glyph.x0 = b->x0;
        glyph.y0 = b->y0;
        glyph.x1 = b->x1;
        glyph.y1 = b->y1;
        glyph.xoff = b->xoff;
        glyph.yoff = b->yoff;
        glyph.xadvance = b->xadvance;
        
        m_glyphs[c] = glyph;
    }
    
    // 清理临时资源
    delete[] bitmap;
    delete[] rgbaBitmap;
    delete[] cdata;
    delete[] fontBuffer;
    
    SPDLOG_INFO("Font loaded successfully: {}", fontPath);
}

void Render::createRenderTarget(Texture& rt, uint32_t width, uint32_t height, 
                               vk::Format format, vk::ImageUsageFlags usage) {
    // 创建图像
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    
    rt.image = VK_ERROR_CHECK(m_LogicalDevice.createImage(imageInfo),"createRenderTarget: Failed to create render target image");
    
    // 分配内存
    vk::MemoryRequirements memRequirements = m_LogicalDevice.getImageMemoryRequirements(rt.image);
    
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, 
                                             vk::MemoryPropertyFlagBits::eDeviceLocal);
    
    rt.memory = VK_ERROR_CHECK(m_LogicalDevice.allocateMemory(allocInfo),"createRenderTarget: Failed to allocate image memory");
    m_LogicalDevice.bindImageMemory(rt.image, rt.memory, 0);
    
    // 创建图像视图
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = rt.image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    rt.view = VK_ERROR_CHECK(m_LogicalDevice.createImageView(viewInfo),"createRenderTarget: Failed to create image view");
    
    // 创建帧缓冲
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = m_offscreen_render_pass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &rt.view;
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;
    
    rt.framebuffer = VK_ERROR_CHECK(m_LogicalDevice.createFramebuffer(framebufferInfo),"createRenderTarget: Failed to create framebuffer");
}
void Render::createOffscreenRenderPass() {
    // 颜色附件
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = vk::Format::eR8G8B8A8Unorm;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    
    // 附件引用
    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
    
    // 子通道
    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    // 依赖关系
    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlags();
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    
    // 创建渲染通道
    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    m_offscreen_render_pass = VK_ERROR_CHECK(m_LogicalDevice.createRenderPass(renderPassInfo),"createOffscreenRenderPass: Failed to create render pass");
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