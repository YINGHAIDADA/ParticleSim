#include "render.h"
#include "render_utils.h"

void Render::createPipeline() {
    spdlog::info("Creating Pipeline");

    vk::PipelineVertexInputStateCreateInfo vertexCreateInfo {};

    vk::PipelineColorBlendAttachmentState colorAttachment {};
    colorAttachment.blendEnable = vk::False;
    colorAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorCreateInfo {};
    colorCreateInfo.attachmentCount = 1;
    colorCreateInfo.pAttachments = &colorAttachment;

    vk::PipelineShaderStageCreateInfo vertexStageCreateInfo {};
    vertexStageCreateInfo.pName = "main";
    vertexStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertexStageCreateInfo.module = m_VertexShader;

    vk::PipelineShaderStageCreateInfo fragmentStageCreateInfo {};
    fragmentStageCreateInfo.pName = "main";
    fragmentStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragmentStageCreateInfo.module = m_FragmentShader;

    vk::PipelineShaderStageCreateInfo stages[] = {
        vertexStageCreateInfo,
        fragmentStageCreateInfo
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationStageCreateInfo {};
    rasterizationStageCreateInfo.frontFace = vk::FrontFace::eClockwise;
    rasterizationStageCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationStageCreateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationStageCreateInfo.lineWidth = 1.0f;

    vk::PipelineLayout pipelineLayout = VK_ERROR_CHECK(
        m_LogicalDevice.createPipelineLayout(vk::PipelineLayoutCreateInfo {}),
        "Pipeline Layout creating caused an error"
    );

    vk::Rect2D scissor {};
    vk::Viewport viewport {};

    vk::PipelineViewportStateCreateInfo viewportCreateInfo {};
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;

    vk::DynamicState dynamicStates[] {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    vk::PipelineMultisampleStateCreateInfo mulsisampleState {};
    mulsisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineInputAssemblyStateCreateInfo assemblyState {};
    assemblyState.topology = vk::PrimitiveTopology::eTriangleList;

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.pVertexInputState = &vertexCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorCreateInfo;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = stages;
    pipelineCreateInfo.pRasterizationState = &rasterizationStageCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = m_RenderPass;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &mulsisampleState;
    pipelineCreateInfo.pInputAssemblyState = &assemblyState;

    m_Pipeline = VK_ERROR_CHECK(
        m_LogicalDevice.createGraphicsPipeline(nullptr, pipelineCreateInfo),
        "Pipeline creating caused an error"
    );

    spdlog::info("Pipeline was created successfully");
}