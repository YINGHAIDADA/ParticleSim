#include "render.h"
#include "render_utils.h"

void Render::createRenderPass() {
    SPDLOG_INFO("Creating Render Pass");

    vk::AttachmentDescription attachment {};
    attachment.loadOp = vk::AttachmentLoadOp::eClear;
    attachment.storeOp = vk::AttachmentStoreOp::eStore;
    attachment.initialLayout = vk::ImageLayout::eUndefined;
    attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    attachment.samples = vk::SampleCountFlagBits::e1;
    attachment.format = vk::Format::eB8G8R8A8Unorm;
    
    vk::AttachmentReference attachmentRef {};
    attachmentRef.attachment = 0;
    attachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass {};
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassCreateInfo {};
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;

    m_RenderPass = VK_ERROR_CHECK(
        m_LogicalDevice.createRenderPass(renderPassCreateInfo),
        "Render Pass creating caused an error"
    );

    SPDLOG_INFO("Render Pass was created successfully");
}

void Render::createOffscreenPass() {
    vk::AttachmentDescription attachment;
    attachment.format = vk::Format::eR16G16B16A16Sfloat;
    attachment.samples = vk::SampleCountFlagBits::e1;
    attachment.loadOp = vk::AttachmentLoadOp::eClear;
    attachment.storeOp = vk::AttachmentStoreOp::eStore;
    attachment.initialLayout = vk::ImageLayout::eUndefined;
    attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::AttachmentReference color;
    color.attachment = 0;
    color.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color;

    vk::SubpassDependency dependencies[2];
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[0].srcAccessMask = vk::AccessFlagBits::eShaderRead;
    dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[1].dstAccessMask = vk::AccessFlagBits::eShaderRead;

    vk::RenderPassCreateInfo info;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 2;
    info.pDependencies = dependencies;
    m_OffscreenPass = VK_ERROR_CHECK(
        m_LogicalDevice.createRenderPass(info),
        "Offscreen render pass creating caused an error"
    );
}