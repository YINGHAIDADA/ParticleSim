#include "render.h"
#include "render_utils.h"

void Render::createPipeline() {
    SPDLOG_INFO("Creating Pipeline");

    vk::DescriptorSetLayoutBinding binding;
    binding.binding = 0;
    binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    binding.descriptorCount = 1;
    binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    m_DescriptorSetLayout = VK_ERROR_CHECK(
        m_LogicalDevice.createDescriptorSetLayout(layoutInfo),
        "Descriptor set layout creating caused an error"
    );

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    m_PipelineLayout = VK_ERROR_CHECK(
        m_LogicalDevice.createPipelineLayout(pipelineLayoutInfo),
        "Pipeline layout creating caused an error"
    );

    vk::PipelineShaderStageCreateInfo vertexStage;
    vertexStage.stage = vk::ShaderStageFlagBits::eVertex;
    vertexStage.module = m_VertexShader;
    vertexStage.pName = "main";

    vk::PipelineShaderStageCreateInfo fragmentStage;
    fragmentStage.stage = vk::ShaderStageFlagBits::eFragment;
    fragmentStage.module = m_FragmentShader;
    fragmentStage.pName = "main";

    vk::PipelineShaderStageCreateInfo stages[] = {vertexStage, fragmentStage};

    vk::PipelineVertexInputStateCreateInfo vertexInput;
    vk::PipelineInputAssemblyStateCreateInfo assembly;
    assembly.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
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
    blend.blendEnable = vk::True;
    blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    blend.colorBlendOp = vk::BlendOp::eAdd;
    blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    blend.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    blend.alphaBlendOp = vk::BlendOp::eAdd;
    blend.colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;

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
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0;

    m_Pipeline = VK_ERROR_CHECK(
        m_LogicalDevice.createGraphicsPipeline(nullptr, pipelineInfo),
        "Pipeline creating caused an error"
    );

    SPDLOG_INFO("Pipeline was created successfully");
}
