#pragma once

#include "context.h"
#include "swapchain.h"
#include "pipeline.h"
#include "semaphore.h"
#include "fence.h"
#include "buffer.h"
#include "object.h"
#include "mesh.h"
#include "asset_manager.h"

#include <memory>

class Camera;
class MeshRenderer;
class Object;

struct FrameSync {
    Semaphore image_available;
    Semaphore render_finished;
    Fence in_flight_frame;

    FrameSync(Context &context) :
        image_available(Semaphore(context)),
        render_finished(Semaphore(context)),
        in_flight_frame(Fence(context, true)){}
};

struct CommandBufferSet {
    //one for each image in the swapchain
    std::vector<vk::CommandBuffer> commands;
    std::vector<bool> needs_rebuild;

    void close(Context& context) {
        context.device.freeCommandBuffers(context.command_pool, commands);
    }

    void mark_dirty() {
        for (size_t i = 0; i < needs_rebuild.size(); i++) {
            needs_rebuild[i] = true;
        }
    }
};

class Renderer
{
public:
    const int MAX_FRAMES_IN_FLIGHT = 2;

    Renderer(Context &ctx, AssetManager &assetmanager) : 
        context(ctx), 
        swapchain(Swapchain(ctx)), 
        pipeline(Pipeline(ctx)),
        asset_manager(assetmanager) {};
    
    void init();

    void close();
    void wait_for_idle();
    void render(Camera &camera, const std::vector<std::unique_ptr<Object>> &objects);

    void register_mesh(MeshRenderer* mr);

private:
    AssetManager& asset_manager;

    Context &context;
    Swapchain swapchain;
    vk::RenderPass renderpass;
    Pipeline pipeline;
    vk::DescriptorSetLayout descriptor_set_layout;
    std::vector<vk::Framebuffer> framebuffers;
    vk::DescriptorPool descriptor_pool;
    std::vector<vk::DescriptorSet> descriptor_sets;
    CommandBufferSet command_buffers;
    std::vector<FrameSync> sync;
    std::vector<Buffer> uniform_buffers;
    std::vector<MeshRenderer*> mesh_renderers;
    std::unique_ptr<Texture> depth_texture;

    size_t current_frame = 0;

    void init_sync_objects();
    void init_depth();
    void init_command_pool();
    void init_command_buffers();
    void init_framebuffers();
    void init_descriptor_set_layout();
    void init_render_pass();
    void init_physical_device();
    void init_logical_device();
    void init_uniform_buffers();
    void init_descriptor_pool();
    void init_descriptor_sets();
    void build_command_buffer(uint32_t image_index);

    void update_uniform_buffers(uint32_t current_image, const std::vector<std::unique_ptr<Object>> &objects, Camera& camera);

    void rebuild_swapchain();
    void close_swapchain();
};