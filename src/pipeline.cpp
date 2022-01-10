#include "pipeline.h"
#include "log.h"
#include "geometry.h"

#include <fstream>
#include <iostream>

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

vk::ShaderModule Pipeline::load_shader(const std::vector<char> &bytes) {
    vk::ShaderModuleCreateInfo create_info{};
    create_info.codeSize = bytes.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(bytes.data());

    vk::ShaderModule shader = context.device.createShaderModule(create_info);
    return shader;
}

void Pipeline::init(vk::Extent2D viewport_extent, vk::RenderPass renderpass, vk::DescriptorSetLayout descriptor_set_layout) {
    TRACE("initializing pipeline");

    auto vert_code = readFile("shader/vert.spv");
    auto frag_code = readFile("shader/frag.spv");

    auto vert = load_shader(vert_code);
    auto frag = load_shader(frag_code);

    vk::PipelineShaderStageCreateInfo vert_info{};
    vert_info.stage = vk::ShaderStageFlagBits::eVertex;
    vert_info.module = vert;
    vert_info.pName = "main";

    vk::PipelineShaderStageCreateInfo frag_info{};
    frag_info.stage = vk::ShaderStageFlagBits::eFragment;
    frag_info.module = frag;
    frag_info.pName = "main";

    vk::PipelineShaderStageCreateInfo stages[] = {vert_info, frag_info};

    auto binding_description = Vertex::binding_description();
    auto attributes_description = Vertex::attribute_description();

    vk::PipelineVertexInputStateCreateInfo input_info{};
    input_info.vertexBindingDescriptionCount = 1;
    input_info.pVertexBindingDescriptions = &binding_description;
    input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes_description.size());
    input_info.pVertexAttributeDescriptions = attributes_description.data();

    vk::PipelineInputAssemblyStateCreateInfo assembly_info (
        {},
        vk::PrimitiveTopology::eTriangleList
    );

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)viewport_extent.width;
    viewport.height = (float)viewport_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = viewport_extent;

    vk::PipelineViewportStateCreateInfo viewport_info{};
    viewport_info.viewportCount = 1;
    viewport_info.pViewports = &viewport;
    viewport_info.scissorCount = 1;
    viewport_info.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer_info(
        {},
        {},
        {},
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise,
        {},
        0.0f,
        0.0f,
        0.0f,
        1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling_info{};
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling_info.minSampleShading = 1.0f; 
    multisampling_info.pSampleMask = nullptr; 
    multisampling_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_info.alphaToOneEnable = VK_FALSE;

    //per-framebuffer blending
    vk::PipelineColorBlendAttachmentState color_blend{};
    color_blend.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend.blendEnable = VK_TRUE;
    color_blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    color_blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    color_blend.colorBlendOp = vk::BlendOp::eAdd;
    color_blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    color_blend.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    color_blend.alphaBlendOp = vk::BlendOp::eAdd;

    //global blending
    vk::PipelineColorBlendStateCreateInfo color_blend_info{};
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = vk::LogicOp::eCopy;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend;
    color_blend_info.blendConstants[0] = 0.0f;
    color_blend_info.blendConstants[1] = 0.0f; 
    color_blend_info.blendConstants[2] = 0.0f; 
    color_blend_info.blendConstants[3] = 0.0f; 

    /*
    vk::DynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    */

    vk::PushConstantRange push_constant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants));

    vk::PipelineLayoutCreateInfo layout_info{};
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &descriptor_set_layout;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;

    layout = context.device.createPipelineLayout(layout_info);

    vk::GraphicsPipelineCreateInfo pipeline_info {};
    pipeline_info.stageCount =2;
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &input_info;
    pipeline_info.pInputAssemblyState = &assembly_info;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rasterizer_info;
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pDynamicState = nullptr;
    pipeline_info.layout = layout;
    pipeline_info.renderPass = renderpass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.basePipelineIndex = -1;

    pipeline = context.device.createGraphicsPipeline(nullptr, pipeline_info);

    context.device.destroyShaderModule(vert);
    context.device.destroyShaderModule(frag);
}

void Pipeline::close() {
    context.device.destroyPipeline(pipeline);
    context.device.destroyPipelineLayout(layout);
}