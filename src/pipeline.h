#pragma once
#include "context.h"

class Pipeline {
public:
    VkPipelineLayout layout;
    VkPipeline pipeline;
    void init(VkExtent2D viewport, VkRenderPass renderpass);
    void close();
    Pipeline(Context &ctx) : context(ctx){};

private:
    VkShaderModule load_shader(const std::vector<char> &bytes);
    Context &context;
};