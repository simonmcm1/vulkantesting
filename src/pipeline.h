#pragma once
#include "context.h"

class Pipeline {
public:
    vk::PipelineLayout layout;
    vk::Pipeline pipeline;
    std::string prefix;
    void init(vk::Extent2D viewport, vk::RenderPass renderpass, const std::vector<vk::DescriptorSetLayout> &descriptor_set_layouts);
    void close();
    Pipeline(Context &ctx, const std::string &shader_prefix) : 
        context(ctx),
        prefix(shader_prefix) {};

private:
    vk::ShaderModule load_shader(const std::vector<char> &bytes);
    Context &context;
};