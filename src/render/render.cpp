#include "render.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

Render::Render(SDL_Window* window) : m_Window(window) {}

Render::~Render() {
    if (m_LogicalDevice) {
        (void)m_LogicalDevice.waitIdle();
        destroyBloomTargets();
        destroyPresentTargets();
        if (m_Pipeline) m_LogicalDevice.destroyPipeline(m_Pipeline);
        if (m_BrightPipeline) m_LogicalDevice.destroyPipeline(m_BrightPipeline);
        if (m_BlurPipeline) m_LogicalDevice.destroyPipeline(m_BlurPipeline);
        if (m_PostLayout) m_LogicalDevice.destroyPipelineLayout(m_PostLayout);
        if (m_CompositeLayout) m_LogicalDevice.destroyPipelineLayout(m_CompositeLayout);
        if (m_SampleLayout) m_LogicalDevice.destroyDescriptorSetLayout(m_SampleLayout);
        if (m_CompositeSetLayout) m_LogicalDevice.destroyDescriptorSetLayout(m_CompositeSetLayout);
        if (m_Sampler) m_LogicalDevice.destroySampler(m_Sampler);
        if (m_LinearSampler) m_LogicalDevice.destroySampler(m_LinearSampler);
        if (m_VertexShader) m_LogicalDevice.destroyShaderModule(m_VertexShader);
        if (m_FragmentShader) m_LogicalDevice.destroyShaderModule(m_FragmentShader);
        if (m_BrightShader) m_LogicalDevice.destroyShaderModule(m_BrightShader);
        if (m_BlurShader) m_LogicalDevice.destroyShaderModule(m_BlurShader);
        destroyFramebuffersAndViews();
        if (m_Swapchain) m_LogicalDevice.destroySwapchainKHR(m_Swapchain);
        for (vk::Semaphore semaphore : m_ImageAvailableSemaphores) m_LogicalDevice.destroySemaphore(semaphore);
        for (vk::Semaphore semaphore : m_RenderFinishedSemaphores) m_LogicalDevice.destroySemaphore(semaphore);
        for (vk::Fence fence : m_InFlightFences) m_LogicalDevice.destroyFence(fence);
        if (m_CommandPool) m_LogicalDevice.destroyCommandPool(m_CommandPool);
        if (m_OffscreenPass) m_LogicalDevice.destroyRenderPass(m_OffscreenPass);
        if (m_RenderPass) m_LogicalDevice.destroyRenderPass(m_RenderPass);
        m_LogicalDevice.destroy();
    }

    if (!m_Instance) return;
    if (m_Surface) m_Instance.destroySurfaceKHR(m_Surface);
    if (m_DebugLayer && m_DebugMessenger) {
        m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, m_Dispatcher);
    }
    m_Instance.destroy();
}

void Render::init(vk::Extent2D& windowSize, uint32_t textureWidth, uint32_t textureHeight) {
    if (textureWidth == 0 || textureHeight == 0) {
        throw std::invalid_argument("texture size must be positive");
    }
    m_WindowExtent = windowSize;
    m_TexWidth = textureWidth;
    m_TexHeight = textureHeight;

    SPDLOG_INFO("Vulkan init");
    createInstance();
    createDebugMessenger();
    createSurface();
    selectPhysicalDevice();
    selectQueueFamilyIndexes();
    createLogicalDevice();
    createSwapchain(windowSize);
    selectSwapchainResources();
    createRenderPass();
    createOffscreenPass();
    createFrameBuffers();
    createShaderModules();
    createPipeline();
    createPresentTargets();
    createBloomTargets();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    SPDLOG_INFO("Presenting {}x{} simulation", m_TexWidth, m_TexHeight);
}

void Render::resize(vk::Extent2D& newWindowSize) {
    m_WindowExtent = newWindowSize;
    try {
        recreateSwapchain(newWindowSize);
    } catch (const std::runtime_error& error) {
        spdlog::error("Vulkan resize error: {}", error.what());
    }
}

void Render::draw(std::span<const uint8_t> rgba, int hudPixels) {
    if (m_SwapchainExtent.width == 0 || m_SwapchainExtent.height == 0) {
        SDL_Delay(16);
        return;
    }

    const size_t byteCount = static_cast<size_t>(m_TexWidth) * m_TexHeight * 4;
    if (rgba.size() < byteCount) {
        throw std::runtime_error("simulation buffer is smaller than the present texture");
    }

    const vk::Fence frameFence = m_InFlightFences[m_CurrentFrame];
    (void)m_LogicalDevice.waitForFences(1, &frameFence, vk::True, UINT64_MAX);

    const vk::ResultValue<uint32_t> acquired = m_LogicalDevice.acquireNextImageKHR(
        m_Swapchain,
        UINT64_MAX,
        m_ImageAvailableSemaphores[m_CurrentFrame],
        nullptr
    );
    if (acquired.result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapchain(m_WindowExtent);
        return;
    }
    if (acquired.result != vk::Result::eSuccess && acquired.result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    const uint32_t imageIndex = acquired.value;
    if (m_ImageInFlight[imageIndex]) {
        (void)m_LogicalDevice.waitForFences(1, &m_ImageInFlight[imageIndex], vk::True, UINT64_MAX);
    }
    m_ImageInFlight[imageIndex] = frameFence;
    (void)m_LogicalDevice.resetFences(1, &frameFence);

    FrameTarget& target = m_Targets[m_CurrentFrame];
    std::memcpy(target.mapped, rgba.data(), byteCount);

    vk::CommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
    commandBuffer.reset();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer.begin(beginInfo);

    transition(commandBuffer, target, vk::ImageLayout::eTransferDstOptimal);

    vk::BufferImageCopy region;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = vk::Extent3D{m_TexWidth, m_TexHeight, 1};
    commandBuffer.copyBufferToImage(
        target.staging,
        target.image,
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &region
    );
    transition(commandBuffer, target, vk::ImageLayout::eShaderReadOnlyOptimal);

    const float panelU = std::clamp(static_cast<float>(hudPixels) / static_cast<float>(m_TexWidth), 0.f, 1.f);
    const std::array<float, 4> black{0.f, 0.f, 0.f, 1.f};
    BloomFrame& bloom = m_Bloom[m_CurrentFrame];
    drawFullscreen(
        commandBuffer, m_BrightPipeline, m_PostLayout, m_OffscreenPass,
        bloom.bright.framebuffer, bloom.brightExtent, bloom.brightSet,
        &panelU, sizeof(panelU), black
    );

    const float vertical[2] = {0.f, 1.f / static_cast<float>(bloom.blurExtent.height)};
    drawFullscreen(
        commandBuffer, m_BlurPipeline, m_PostLayout, m_OffscreenPass,
        bloom.blurA.framebuffer, bloom.blurExtent, bloom.blurFromBright,
        vertical, sizeof(vertical), black
    );
    const float horizontal[2] = {1.f / static_cast<float>(bloom.blurExtent.width), 0.f};
    drawFullscreen(
        commandBuffer, m_BlurPipeline, m_PostLayout, m_OffscreenPass,
        bloom.blurB.framebuffer, bloom.blurExtent, bloom.blurFromA,
        horizontal, sizeof(horizontal), black
    );

    const std::array<float, 4> background{0.1f, 0.1f, 0.1f, 1.f};
    drawFullscreen(
        commandBuffer, m_Pipeline, m_CompositeLayout, m_RenderPass,
        m_FrameBuffers[imageIndex], m_SwapchainExtent, bloom.compositeSet,
        &panelU, sizeof(panelU), background
    );
    commandBuffer.end();

    const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame];
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[imageIndex];
    const vk::Result submitResult = m_GraphicQueue.submit(1, &submitInfo, frameFence);
    if (submitResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to submit frame");
    }

    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.pImageIndices = &imageIndex;
    const vk::Result presentResult = m_PresentQueue.presentKHR(presentInfo);
    if (presentResult == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapchain(m_WindowExtent);
    } else if (presentResult != vk::Result::eSuccess && presentResult != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("Failed to present frame");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Render::createPresentTargets() {
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.maxLod = 0.0f;
    m_Sampler = VK_ERROR_CHECK(m_LogicalDevice.createSampler(samplerInfo), "Sampler creating caused an error");
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    m_LinearSampler = VK_ERROR_CHECK(m_LogicalDevice.createSampler(samplerInfo), "Linear sampler creating caused an error");

    const vk::DeviceSize byteCount = static_cast<vk::DeviceSize>(m_TexWidth) * m_TexHeight * 4;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        FrameTarget& target = m_Targets[static_cast<size_t>(i)];

        createBuffer(
            byteCount,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            target.staging,
            target.stagingMemory
        );
        target.mapped = VK_ERROR_CHECK(
            m_LogicalDevice.mapMemory(target.stagingMemory, 0, byteCount),
            "Staging memory map caused an error"
        );

        vk::ImageCreateInfo imageInfo;
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = vk::Format::eR8G8B8A8Unorm;
        imageInfo.extent = vk::Extent3D{m_TexWidth, m_TexHeight, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        target.image = VK_ERROR_CHECK(m_LogicalDevice.createImage(imageInfo), "Simulation image creating caused an error");

        const vk::MemoryRequirements requirements = m_LogicalDevice.getImageMemoryRequirements(target.image);
        vk::MemoryAllocateInfo imageAlloc{
            requirements.size,
            findMemoryType(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        };
        target.imageMemory = VK_ERROR_CHECK(
            m_LogicalDevice.allocateMemory(imageAlloc),
            "Simulation image memory allocation caused an error"
        );
        m_LogicalDevice.bindImageMemory(target.image, target.imageMemory, 0);

        vk::ImageViewCreateInfo viewInfo;
        viewInfo.image = target.image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = vk::Format::eR8G8B8A8Unorm;
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        target.view = VK_ERROR_CHECK(
            m_LogicalDevice.createImageView(viewInfo),
            "Simulation image view creating caused an error"
        );
        target.layout = vk::ImageLayout::eUndefined;
    }
}

void Render::destroyColorTarget(ColorTarget& target) {
    if (target.framebuffer) m_LogicalDevice.destroyFramebuffer(target.framebuffer);
    if (target.view) m_LogicalDevice.destroyImageView(target.view);
    if (target.image) m_LogicalDevice.destroyImage(target.image);
    if (target.memory) m_LogicalDevice.freeMemory(target.memory);
    target.framebuffer = nullptr;
    target.view = nullptr;
    target.image = nullptr;
    target.memory = nullptr;
}

void Render::createColorTarget(ColorTarget& target, vk::Extent2D extent) {
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = vk::Format::eR16G16B16A16Sfloat;
    imageInfo.extent = vk::Extent3D{extent.width, extent.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    target.image = VK_ERROR_CHECK(m_LogicalDevice.createImage(imageInfo), "Bloom image creating caused an error");

    const vk::MemoryRequirements requirements = m_LogicalDevice.getImageMemoryRequirements(target.image);
    vk::MemoryAllocateInfo imageAlloc{
        requirements.size,
        findMemoryType(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };
    target.memory = VK_ERROR_CHECK(m_LogicalDevice.allocateMemory(imageAlloc), "Bloom image memory allocation caused an error");
    m_LogicalDevice.bindImageMemory(target.image, target.memory, 0);

    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = target.image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = vk::Format::eR16G16B16A16Sfloat;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    target.view = VK_ERROR_CHECK(m_LogicalDevice.createImageView(viewInfo), "Bloom image view creating caused an error");

    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = m_OffscreenPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &target.view;
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;
    target.framebuffer = VK_ERROR_CHECK(
        m_LogicalDevice.createFramebuffer(framebufferInfo),
        "Bloom framebuffer creating caused an error"
    );
}

void Render::writeSampler(vk::DescriptorSet set, uint32_t binding, vk::Sampler sampler, vk::ImageView view) {
    vk::DescriptorImageInfo info;
    info.sampler = sampler;
    info.imageView = view;
    info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    vk::WriteDescriptorSet write;
    write.dstSet = set;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    write.pImageInfo = &info;
    m_LogicalDevice.updateDescriptorSets(1, &write, 0, nullptr);
}

void Render::createBloomTargets() {
    if (m_SwapchainExtent.width == 0 || m_SwapchainExtent.height == 0) return;

    const vk::Extent2D brightExtent{
        std::max(m_SwapchainExtent.width / 8, 1u),
        std::max(m_SwapchainExtent.height / 8, 1u)
    };
    const vk::Extent2D blurExtent{
        std::max(m_SwapchainExtent.width / 16, 1u),
        std::max(m_SwapchainExtent.height / 16, 1u)
    };

    constexpr uint32_t kSampleSets = MAX_FRAMES_IN_FLIGHT * 3;
    vk::DescriptorPoolSize poolSize{vk::DescriptorType::eCombinedImageSampler, kSampleSets + MAX_FRAMES_IN_FLIGHT * 2};
    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.maxSets = kSampleSets + MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    m_DescriptorPool = VK_ERROR_CHECK(m_LogicalDevice.createDescriptorPool(poolInfo), "Descriptor pool creating caused an error");

    std::vector<vk::DescriptorSetLayout> sampleLayouts(kSampleSets, m_SampleLayout);
    vk::DescriptorSetAllocateInfo sampleAlloc;
    sampleAlloc.descriptorPool = m_DescriptorPool;
    sampleAlloc.descriptorSetCount = kSampleSets;
    sampleAlloc.pSetLayouts = sampleLayouts.data();
    const std::vector<vk::DescriptorSet> sampleSets = VK_ERROR_AND_EMPRY_CHECK(
        m_LogicalDevice.allocateDescriptorSets(sampleAlloc),
        "Bloom descriptor allocation caused an error",
        "Bloom descriptor allocation returned no results"
    );

    std::vector<vk::DescriptorSetLayout> compositeLayouts(MAX_FRAMES_IN_FLIGHT, m_CompositeSetLayout);
    vk::DescriptorSetAllocateInfo compositeAlloc;
    compositeAlloc.descriptorPool = m_DescriptorPool;
    compositeAlloc.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    compositeAlloc.pSetLayouts = compositeLayouts.data();
    const std::vector<vk::DescriptorSet> compositeSets = VK_ERROR_AND_EMPRY_CHECK(
        m_LogicalDevice.allocateDescriptorSets(compositeAlloc),
        "Composite descriptor allocation caused an error",
        "Composite descriptor allocation returned no results"
    );

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        BloomFrame& frame = m_Bloom[static_cast<size_t>(i)];
        frame.brightExtent = brightExtent;
        frame.blurExtent = blurExtent;
        createColorTarget(frame.bright, brightExtent);
        createColorTarget(frame.blurA, blurExtent);
        createColorTarget(frame.blurB, blurExtent);

        const size_t setIndex = static_cast<size_t>(i) * 3;
        frame.brightSet = sampleSets[setIndex];
        frame.blurFromBright = sampleSets[setIndex + 1];
        frame.blurFromA = sampleSets[setIndex + 2];
        frame.compositeSet = compositeSets[static_cast<size_t>(i)];

        const vk::ImageView scene = m_Targets[static_cast<size_t>(i)].view;
        writeSampler(frame.brightSet, 0, m_Sampler, scene);
        writeSampler(frame.blurFromBright, 0, m_LinearSampler, frame.bright.view);
        writeSampler(frame.blurFromA, 0, m_LinearSampler, frame.blurA.view);
        writeSampler(frame.compositeSet, 0, m_Sampler, scene);
        writeSampler(frame.compositeSet, 1, m_LinearSampler, frame.blurB.view);
    }
}

void Render::destroyBloomTargets() {
    if (!m_LogicalDevice) return;
    for (BloomFrame& frame : m_Bloom) {
        destroyColorTarget(frame.bright);
        destroyColorTarget(frame.blurA);
        destroyColorTarget(frame.blurB);
        frame.brightSet = nullptr;
        frame.blurFromBright = nullptr;
        frame.blurFromA = nullptr;
        frame.compositeSet = nullptr;
    }
    if (m_DescriptorPool) {
        m_LogicalDevice.destroyDescriptorPool(m_DescriptorPool);
        m_DescriptorPool = nullptr;
    }
}

void Render::drawFullscreen(vk::CommandBuffer commandBuffer, vk::Pipeline pipeline, vk::PipelineLayout layout,
                            vk::RenderPass renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent,
                            vk::DescriptorSet set, const void* push, uint32_t pushBytes, std::array<float, 4> clear) {
    vk::ClearValue clearValue;
    clearValue.color = vk::ClearColorValue(clear);

    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    vk::Viewport viewport;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &viewport);
    const vk::Rect2D scissor{{0, 0}, extent};
    commandBuffer.setScissor(0, 1, &scissor);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, 1, &set, 0, nullptr);
    commandBuffer.pushConstants(layout, vk::ShaderStageFlagBits::eFragment, 0, pushBytes, push);
    commandBuffer.draw(3, 1, 0, 0);
    commandBuffer.endRenderPass();
}

void Render::destroyPresentTargets() {
    for (FrameTarget& target : m_Targets) {
        if (target.mapped && target.stagingMemory) {
            m_LogicalDevice.unmapMemory(target.stagingMemory);
            target.mapped = nullptr;
        }
        if (target.view) m_LogicalDevice.destroyImageView(target.view);
        if (target.image) m_LogicalDevice.destroyImage(target.image);
        if (target.imageMemory) m_LogicalDevice.freeMemory(target.imageMemory);
        if (target.staging) m_LogicalDevice.destroyBuffer(target.staging);
        if (target.stagingMemory) m_LogicalDevice.freeMemory(target.stagingMemory);
        target = {};
    }
}

void Render::transition(vk::CommandBuffer commandBuffer, FrameTarget& target, vk::ImageLayout next) {
    if (target.layout == next) return;

    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout = target.layout;
    barrier.newLayout = next;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = target.image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;
    if (target.layout == vk::ImageLayout::eUndefined && next == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (target.layout == vk::ImageLayout::eTransferDstOptimal && next == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (target.layout == vk::ImageLayout::eShaderReadOnlyOptimal && next == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eFragmentShader;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else {
        throw std::invalid_argument("Unsupported simulation image layout transition");
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, 0, nullptr, 0, nullptr, 1, &barrier);
    target.layout = next;
}

uint32_t Render::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    const vk::PhysicalDeviceMemoryProperties memory = m_PhysicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
        const bool typeMatches = (typeFilter & (1u << i)) != 0;
        const bool flagsMatch = (memory.memoryTypes[i].propertyFlags & properties) == properties;
        if (typeMatches && flagsMatch) return i;
    }
    throw std::runtime_error("Failed to find a suitable memory type");
}
