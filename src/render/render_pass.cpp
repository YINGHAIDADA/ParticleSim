#include "render.h"
#include "render_utils.h"

void Render::createRenderPass() {
    spdlog::info("Creating Render Pass");

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

    vk::RenderPassCreateInfo renderPassCreateInfo {};
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;

    m_RenderPass = VK_ERROR_CHECK(
        m_LogicalDevice.createRenderPass(renderPassCreateInfo),
        "Render Pass creating caused an error"
    );

    spdlog::info("Render Pass was created successfully");
}