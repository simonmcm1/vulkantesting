#pragma once

#include "context.h"
#include "swapchain.h"
#include "pipeline.h"
#include "semaphore.h"
#include "fence.h"

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

class Renderer {
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
    std::vector<vk::Framebuffer> framebuffers;
    VkCommandPool command_pool = nullptr;
    std::vector<vk::CommandBuffer> command_buffers;
    std::vector<FrameSync> sync;

    size_t current_frame = 0;

    void init_sync_objects();
    void init_command_buffers();
    void init_framebuffers();
    void init_render_pass();
    void init_physical_device();
    void init_logical_device();

    void rebuild_swapchain();
    void close_swapchain();
};