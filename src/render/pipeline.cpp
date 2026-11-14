#include "render.h"
#include "render_utils.h"

namespace {

vk::DescriptorSetLayout samplerLayout(vk::Device device, uint32_t bindings) {
    vk::DescriptorSetLayoutBinding binding[3];
    for (uint32_t i = 0; i < bindings; ++i) {
        binding[i].binding = i;
        binding[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        binding[i].descriptorCount = 1;
        binding[i].stageFlags = vk::ShaderStageFlagBits::eFragment;
    }

    vk::DescriptorSetLayoutCreateInfo info;
    info.bindingCount = bindings;
    info.pBindings = binding;
    return VK_ERROR_CHECK(device.createDescriptorSetLayout(info), "Descriptor set layout creating caused an error");
}

vk::PipelineLayout pipelineLayout(vk::Device device, vk::DescriptorSetLayout setLayout) {
    vk::PushConstantRange range;
    range.stageFlags = vk::ShaderStageFlagBits::eFragment;
    range.offset = 0;
    range.size = sizeof(float) * 5;

    vk::PipelineLayoutCreateInfo info;
    info.setLayoutCount = 1;
    info.pSetLayouts = &setLayout;
    info.pushConstantRangeCount = 1;
    info.pPushConstantRanges = &range;
    return VK_ERROR_CHECK(device.createPipelineLayout(info), "Pipeline layout creating caused an error");
}

}  // namespace

void Render::createPipeline() {
    SPDLOG_INFO("Creating Pipeline");

    m_SampleLayout = samplerLayout(m_LogicalDevice, 1);
    m_CompositeSetLayout = samplerLayout(m_LogicalDevice, 3);
    m_PostLayout = pipelineLayout(m_LogicalDevice, m_SampleLayout);
    m_CompositeLayout = pipelineLayout(m_LogicalDevice, m_CompositeSetLayout);

    const auto makePipeline = [&](vk::ShaderModule fragment, vk::PipelineLayout layout, vk::RenderPass pass) {
        vk::PipelineShaderStageCreateInfo vertexStage;
        vertexStage.stage = vk::ShaderStageFlagBits::eVertex;
        vertexStage.module = m_VertexShader;
        vertexStage.pName = "main";

        vk::PipelineShaderStageCreateInfo fragmentStage;
        fragmentStage.stage = vk::ShaderStageFlagBits::eFragment;
        fragmentStage.module = fragment;
        fragmentStage.pName = "main";

        vk::PipelineShaderStageCreateInfo stages[] = {vertexStage, fragmentStage};
        vk::PipelineVertexInputStateCreateInfo vertexInput;
        vk::PipelineInputAssemblyStateCreateInfo assembly;
        assembly.topology = vk::PrimitiveTopology::eTriangleList;

        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo dynamicState;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        vk::PipelineRasterizationStateCreateInfo raster;
        raster.polygonMode = vk::PolygonMode::eFill;
        raster.cullMode = vk::CullModeFlagBits::eNone;
        raster.frontFace = vk::FrontFace::eCounterClockwise;
        raster.lineWidth = 1.0f;

        vk::PipelineMultisampleStateCreateInfo multisample;
        multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

        vk::PipelineColorBlendAttachmentState blend;
        blend.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo blending;
        blending.attachmentCount = 1;
        blending.pAttachments = &blend;

        vk::GraphicsPipelineCreateInfo pipelineInfo;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &assembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &raster;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pColorBlendState = &blending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = layout;
        pipelineInfo.renderPass = pass;
        pipelineInfo.subpass = 0;
        return VK_ERROR_CHECK(
            m_LogicalDevice.createGraphicsPipeline(nullptr, pipelineInfo),
            "Pipeline creating caused an error"
        );
    };

    m_BrightPipeline = makePipeline(m_BrightShader, m_PostLayout, m_OffscreenPass);
    m_BlurPipeline = makePipeline(m_BlurShader, m_PostLayout, m_OffscreenPass);
    m_Pipeline = makePipeline(m_FragmentShader, m_CompositeLayout, m_RenderPass);
    SPDLOG_INFO("Pipeline was created successfully");
}
