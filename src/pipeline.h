#pragma once
#include "context.h"

class Pipeline {
public:
    vk::PipelineLayout layout;
    vk::Pipeline pipeline;
    void init(vk::Extent2D viewport, vk::RenderPass renderpass, vk::DescriptorSetLayout descriptor_set_layout);
    void close();
    Pipeline(Context &ctx) : context(ctx){};

private:
    vk::ShaderModule load_shader(const std::vector<char> &bytes);
    Context &context;
};