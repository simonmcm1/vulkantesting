#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "context.h"
#include "swapchain.h"
#include "pipeline.h"
#include "semaphore.h"
#include "fence.h"
#include "buffer.h"

#include <memory>

struct FrameSync {
    Semaphore image_available;
    Semaphore render_finished;
    Fence in_flight_frame;

    FrameSync(Context &context) :
        image_available(Semaphore(context)),
        render_finished(Semaphore(context)),
        in_flight_frame(Fence(context, true)){}
};

struct UniformBufferObject {
    alignas(16) glm::mat4 M;
    alignas(16) glm::mat4 V;
    alignas(16) glm::mat4 P;
};

class Renderer
{
public:
    const int MAX_FRAMES_IN_FLIGHT = 2;

    Renderer(Context &ctx) : context(ctx), swapchain(Swapchain(ctx)), pipeline(Pipeline(ctx)){};

    void init();
    void close();
    void wait_for_idle();
    void render();

private:
    Context &context;
    Swapchain swapchain;
    vk::RenderPass renderpass;
    Pipeline pipeline;
    vk::DescriptorSetLayout descriptor_set_layout;
    std::vector<vk::Framebuffer> framebuffers;
    vk::CommandPool command_pool = nullptr;
    vk::DescriptorPool descriptor_pool;
    std::vector<vk::DescriptorSet> descriptor_sets;
    std::vector<vk::CommandBuffer> command_buffers;
    std::vector<FrameSync> sync;
    std::unique_ptr<Buffer> vertex_buffer;
    std::unique_ptr<Buffer> index_buffer;
    std::vector<Buffer> uniform_buffers;

    size_t current_frame = 0;

    void init_sync_objects();
    void init_command_pool();
    void init_command_buffers();
    void init_framebuffers();
    void init_descriptor_set_layout();
    void init_render_pass();
    void init_physical_device();
    void init_logical_device();
    void init_vertex_buffers();
    void init_uniform_buffers();
    void init_descriptor_pool();
    void init_descriptor_sets();

    void update_uniform_buffers(uint32_t current_image);

    void rebuild_swapchain();
    void close_swapchain();
};