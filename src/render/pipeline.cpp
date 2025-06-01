#include "render.h"
#include "render_utils.h"

void Render::createPipeline() {
    SPDLOG_INFO("Creating Pipeline");

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

    SPDLOG_INFO("Pipeline was created successfully");
}

void Render::createUIPipeline() {
    SPDLOG_INFO("Creating UI Pipeline");
    // 着色器阶段
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = createShaderModule(readFile("shaders/ui.vert.spv"));
    vertShaderStageInfo.pName = "main";
    
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = createShaderModule(readFile("shaders/ui.frag.spv"));
    fragShaderStageInfo.pName = "main";
    
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
        vertShaderStageInfo, fragShaderStageInfo
    };
    
    // 顶点输入
    auto bindingDescription = TextVertex::getBindingDescription();
    auto attributeDescriptions = TextVertex::getAttributeDescriptions();
    
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    // 输入装配
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // 视口和裁剪
    vk::Viewport viewport(0.0f, 0.0f, 
                         static_cast<float>(g_texture_width), 
                         static_cast<float>(g_texture_height), 
                         0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, {g_texture_width, g_texture_height});
    
    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    // 光栅化
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // 多重采样
    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    
    // 颜色混合
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = 
        vk::ColorComponentFlagBits::eR | 
        vk::ColorComponentFlagBits::eG | 
        vk::ColorComponentFlagBits::eB | 
        vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
    
    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // 动态状态
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    
    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    
    // Push constant范围
    vk::PushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
    pushConstantRange.offset = 0;
    pushConstantRange.size = 4 * sizeof(float); // RGBA颜色
    
    // 管线布局
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptor_set_layout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    m_pipeline_layout = VK_ERROR_CHECK(m_LogicalDevice.createPipelineLayout(pipelineLayoutInfo),"createUIPipeline: Failed to create pipeline layout");
    
    // 创建UI管线
    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipeline_layout;
    pipelineInfo.renderPass = m_RenderPass; // 使用主渲染通道
    pipelineInfo.subpass = 0;
    
    m_ui_pipeline = m_LogicalDevice.createGraphicsPipeline(nullptr, pipelineInfo).value;
    
    // 清理着色器模块
    m_LogicalDevice.destroyShaderModule(vertShaderStageInfo.module);
    m_LogicalDevice.destroyShaderModule(fragShaderStageInfo.module);
}

void Render::createMainPipeline() {
    SPDLOG_INFO("Creating Main Pipeline");
    // 着色器阶段
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = m_VertexShader;
    vertShaderStageInfo.pName = "main";
    
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = m_FragmentShader;
    vertShaderStageInfo.pName = "main";
    
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // 顶点输入
    vk::VertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;
    bindingDescription.stride = 4 * sizeof(float); // position + UV
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;
    
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[0].offset = 0;
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[1].offset = 2 * sizeof(float);
    
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    // 输入装配
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // 视口和裁剪
    vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(g_texture_width), static_cast<float>(g_texture_height), 0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, {g_texture_width, g_texture_height});
    
    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    // 光栅化
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // 多重采样
    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    
    // 颜色混合
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = 
        vk::ColorComponentFlagBits::eR | 
        vk::ColorComponentFlagBits::eG | 
        vk::ColorComponentFlagBits::eB | 
        vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
    
    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // 动态状态
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    
    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    
    // 管线布局
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptor_set_layout;
    
    m_pipeline_layout = VK_ERROR_CHECK(m_LogicalDevice.createPipelineLayout(pipelineLayoutInfo),"createMainPipeline: Failed to create pipeline layout!");
    
    // 创建图形管线
    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipeline_layout;
    pipelineInfo.renderPass = m_offscreen_render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    
    m_Pipeline = VK_ERROR_CHECK(m_LogicalDevice.createGraphicsPipeline(nullptr, pipelineInfo),"createMainPipeline: Failed to create graphics pipeline");
}