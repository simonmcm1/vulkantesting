#include "renderer.h"
#include "log.h"
#include "geometry.h"
#include "object.h"

#include <iostream>
#include <set>

#include <chrono>

void Renderer::init_physical_device() {
    TRACE("initializing physical device")

    auto devices = context.instance.enumeratePhysicalDevices();

    for (const auto& device : devices) {
        if(Context::is_device_suitable(device, context.surface)) {
            context.physical_device = device;
            break;
        }
    }

    if(context.physical_device == vk::PhysicalDevice(nullptr)) {
        throw std::runtime_error("no physical device is suitable");
    }

    context.queue_families = Context::get_queue_families_from_device(context.physical_device, context.surface);
}

void Renderer::init_logical_device() {
    TRACE("initializing logical device")

    std::set<uint32_t> families = {
        context.queue_families.graphics.value(),
        context.queue_families.presentation.value()
    };
    
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    float priorities[] = {1.0f};
    for (uint32_t family : families) {
        vk::DeviceQueueCreateInfo queue_create_info{};    
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = priorities;

        queue_create_infos.push_back(queue_create_info);
    }

    vk::PhysicalDeviceFeatures features{};
    features.samplerAnisotropy = true;

    vk::DeviceCreateInfo create_info{};
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pEnabledFeatures = &features;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = device_extensions.data();

    if (context.enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    context.device = context.physical_device.createDevice(create_info, nullptr);
    context.device.getQueue(context.queue_families.graphics.value(), 0, &context.graphics_queue);
    context.device.getQueue(context.queue_families.presentation.value(), 0, &context.presentation_queue);
}

void Renderer::init_render_pass() {
    vk::AttachmentDescription color_attachment{};
    color_attachment.format = swapchain.format;
    color_attachment.samples = vk::SampleCountFlagBits::e1;
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    color_attachment.initialLayout = vk::ImageLayout::eUndefined;
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription depth_attachment{};
    depth_attachment.format = depth_texture->format;
    depth_attachment.samples = vk::SampleCountFlagBits::e1;
    depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
    depth_attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array<vk::AttachmentDescription, 2> attachments{ color_attachment, depth_attachment };
    vk::RenderPassCreateInfo renderpass_info{};
    renderpass_info.attachmentCount = attachments.size();
    renderpass_info.pAttachments = attachments.data();
    renderpass_info.subpassCount = 1;
    renderpass_info.pSubpasses = &subpass;
    renderpass_info.dependencyCount = 1;
    renderpass_info.pDependencies = &dependency;

    renderpass = context.device.createRenderPass(renderpass_info, nullptr);
}

void Renderer::init_framebuffers() {
    TRACE("initializing framebuffers")
    framebuffers.resize(swapchain.image_views.size());
    for(size_t i = 0; i < framebuffers.size(); i++) {
        std::array<vk::ImageView, 2> attachments{
            swapchain.image_views[i],
            depth_texture->image_view
        };

        vk::FramebufferCreateInfo create_info{};
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.renderPass = renderpass;
        create_info.width = swapchain.extent.width;
        create_info.height = swapchain.extent.height;
        create_info.layers = 1;

        framebuffers[i] = context.device.createFramebuffer(create_info);
    }
}

void Renderer::init_sync_objects() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        FrameSync framesync(context);
        sync.push_back(std::move(framesync));
    }
}

void Renderer::init_depth()
{
    auto depth_format = Texture::get_supported_format(
        context,
        {
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint
        },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );

    depth_texture = std::make_unique<Texture>(context);
    depth_texture->width = swapchain.extent.width;
    depth_texture->height = swapchain.extent.height;
    depth_texture->format = depth_format;
    depth_texture->aspect = vk::ImageAspectFlagBits::eDepth;
    depth_texture->tiling = vk::ImageTiling::eOptimal;
    depth_texture->usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    depth_texture->memory_flags = vk::MemoryPropertyFlagBits::eDeviceLocal;

    depth_texture->init();
    //depth_texture->transition_layout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

void Renderer::init_materials() {
    material_manager.init();

    for (auto& material_type : material_manager.material_types) {
        material_type.second.init(swapchain.extent, renderpass, descriptor_set_layout);
    }
}

void Renderer::init_descriptor_pool() {
    uint32_t max_descriptors = static_cast<uint32_t> (swapchain.images.size() * 30);
    std::array<vk::DescriptorPoolSize, 2> pool_sizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, max_descriptors),
vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, max_descriptors)
    };
    vk::DescriptorPoolCreateInfo create_info({},
        max_descriptors,
        static_cast<uint32_t>(pool_sizes.size()),
        pool_sizes.data());

    descriptor_pool = context.device.createDescriptorPool(create_info);
}

void Renderer::init_descriptor_set_layout() {
    vk::DescriptorSetLayoutBinding ubo_layout(0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex,
        nullptr);
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings{ ubo_layout };
    vk::DescriptorSetLayoutCreateInfo create_info({}, static_cast<uint32_t>(bindings.size()), bindings.data());
    descriptor_set_layout = context.device.createDescriptorSetLayout(create_info);
}

void Renderer::init_descriptor_sets() {
    std::cout << "init global descriptor set" << std::endl;
    std::vector<vk::DescriptorSetLayout> layouts(swapchain.images.size(), descriptor_set_layout);

    vk::DescriptorSetAllocateInfo allocate_info(descriptor_pool,
        static_cast<uint32_t>(layouts.size()),
        layouts.data());

    descriptor_sets = context.device.allocateDescriptorSets(allocate_info);

    for (size_t i = 0; i < layouts.size(); i++) {
        vk::DescriptorBufferInfo buffer_info(uniform_buffers[i].buffer, 0, sizeof(UniformBufferObject));

        std::array<vk::WriteDescriptorSet, 1> writes{
                vk::WriteDescriptorSet(descriptor_sets[i],
                                     0,
                                     0,
                                     1,
                                     vk::DescriptorType::eUniformBuffer,
                                     nullptr,
                                     &buffer_info,
                                     nullptr)
        };


        context.device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void Renderer::init_uniform_buffers() {
    vk::DeviceSize size = sizeof(UniformBufferObject);
    for (size_t i = 0; i < swapchain.images.size(); i++)
    {
        uniform_buffers.push_back(Buffer(context));
        uniform_buffers[i].init(size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }
}

void Renderer::update_uniform_buffers(uint32_t current_image, const std::vector<std::unique_ptr<Object>>& objects, Camera& camera) {
    UniformBufferObject ubo{};
    for (size_t i = 0; i < objects.size(); i++) {
        ubo.M[i] = objects[i]->transform.matrix();
    }
    ubo.V = camera.view();
    ubo.P = camera.projection();

    uniform_buffers[current_image].store(&ubo);
}


void Renderer::build_command_buffer(uint32_t image_index)
{
    vk::CommandBufferBeginInfo begin_info{};
    begin_info.flags = {};
    begin_info.pInheritanceInfo = nullptr;

    auto& command_buffer = command_buffers.commands[image_index];

    command_buffer.begin(begin_info);

    vk::RenderPassBeginInfo renderpass_begin_info{};
    renderpass_begin_info.framebuffer = framebuffers[image_index];
    renderpass_begin_info.renderPass = renderpass;
    renderpass_begin_info.renderArea.offset = vk::Offset2D{ 0,0 };
    renderpass_begin_info.renderArea.extent = swapchain.extent;
    std::array<vk::ClearValue, 2> clear_colors{
        vk::ClearColorValue{ std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f} },
        vk::ClearDepthStencilValue{ 1.0f, 0 }
    };
    renderpass_begin_info.clearValueCount = clear_colors.size();
    renderpass_begin_info.pClearValues = clear_colors.data();

    command_buffer.beginRenderPass(renderpass_begin_info, vk::SubpassContents::eInline);

    glm::uint32_t object_index = 0;
    for (auto mesh_renderer : mesh_renderers) {

        auto material = mesh_renderer->material;
        const auto& pipeline = material->get_pipeline();

        //build material descriptor sets if they weren't already
        //TODO: initialize explicitly somehwere?
        if (material->get_descriptor_set() == vk::DescriptorSet(nullptr)) {
            material->init_descriptor_set(descriptor_pool, asset_manager);
        }

        if (material->get_descriptor_set() == vk::DescriptorSet(nullptr)) {
            throw std::runtime_error("null descriptor set on " + material->material_type.name);
        }

        std::array<vk::DescriptorSet, 2> descriptors{ descriptor_sets[image_index], material->get_descriptor_set() };
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0, descriptors.size(), descriptors.data(), 0, nullptr);

        PushConstants push_constants{ object_index };
        object_index++;
        command_buffer.pushConstants(pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &push_constants);
        mesh_renderer->command_buffer(command_buffer);
    }

    command_buffer.endRenderPass();
    command_buffer.end();
}

void Renderer::init_command_buffers() {
    TRACE("initializing command buffers")

    vk::CommandBufferAllocateInfo allocate_info{};
    allocate_info.commandPool = context.command_pool;
    allocate_info.level = vk::CommandBufferLevel::ePrimary;
    allocate_info.commandBufferCount = (uint32_t)framebuffers.size();

    command_buffers.commands = context.device.allocateCommandBuffers(allocate_info);

    for(size_t i = 0; i < command_buffers.commands.size(); i++) {
        build_command_buffer(i);
    }

    command_buffers.needs_rebuild.resize(command_buffers.commands.size());
}

void Renderer::render(Camera &camera, const std::vector<std::unique_ptr<Object>> &objects) {
    context.device.waitForFences(1, &sync[current_frame].in_flight_frame.fence, VK_TRUE, UINT64_MAX);
    auto next_image_res = context.device.acquireNextImageKHR(swapchain.swapchain, UINT64_MAX, sync[current_frame].image_available.semaphore, nullptr);

    uint32_t next_image;
    switch (next_image_res.result)
    {
    case vk::Result::eErrorOutOfDateKHR:
        rebuild_swapchain();
        return;
    case vk::Result::eSuccess:
    case vk::Result::eSuboptimalKHR:
        next_image = next_image_res.value;
        break;
    default:
        throw std::runtime_error("failed to acquire next swapchain image");
    }

    //wait if this image is in use by a previous frame
    if(swapchain.image_fences[next_image] != vk::Fence(nullptr)) {
        context.device.waitForFences(1, &swapchain.image_fences[next_image], VK_TRUE, UINT64_MAX);
    }
    swapchain.image_fences[next_image] = sync[current_frame].in_flight_frame.fence;


    //update dirty materials
    for(auto &renderer : mesh_renderers) {
        renderer->material->update_if_dirty();
    }

    //rebuild command buffers if needed
    if (command_buffers.needs_rebuild[next_image] == true) {
        command_buffers.commands[next_image].reset({});
        build_command_buffer(next_image);
        command_buffers.needs_rebuild[next_image] = false;
   }

    update_uniform_buffers(next_image, objects, camera);


    vk::Semaphore wait_semaphores[] = {sync[current_frame].image_available.semaphore};
    vk::Semaphore signal_semaphores[] = {sync[current_frame].render_finished.semaphore};
    vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo submit_info{};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers.commands[next_image];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    context.device.resetFences(1, &sync[current_frame].in_flight_frame.fence);
    context.graphics_queue.submit(1, &submit_info, sync[current_frame].in_flight_frame.fence);

    vk::SwapchainKHR swapchains[] = {swapchain.swapchain};
    vk::PresentInfoKHR present_info{};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &next_image;
    present_info.pResults = nullptr;

    vk::Result present_result;
    try {
        present_result = context.presentation_queue.presentKHR(present_info);

        if (present_result == vk::Result::eErrorOutOfDateKHR || present_result == vk::Result::eSuboptimalKHR || context.framebuffer_resized) {
            context.framebuffer_resized = false;
            rebuild_swapchain();
        }
        else if (present_result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swapchain image");
        }
    }
    catch (vk::OutOfDateKHRError const &e) {
        //Out of date isn't defined as success in hpp wrapper so it throws an exception here :(
        context.framebuffer_resized = false;
        rebuild_swapchain();
    }
  
    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::register_mesh(MeshRenderer* mr)
{
    mesh_renderers.push_back(mr);
    command_buffers.mark_dirty();
}

void Renderer::init_command_pool() {
    vk::CommandPoolCreateInfo pool_create_info{};
    pool_create_info.queueFamilyIndex = context.queue_families.graphics.value();
    pool_create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    context.command_pool = context.device.createCommandPool(pool_create_info);
}

void Renderer::rebuild_swapchain() {
    TRACE("rebuilding swapchain");
    
    //framebuffer is 0x0 when application is minimized
    while (context.framebuffer_extent.width == 0 || context.framebuffer_extent.height == 0) {
        glfwWaitEvents();
    }
    context.device.waitIdle();


    close_swapchain();

    swapchain.init();
    init_depth();
    init_render_pass();

    for (auto& material_type : material_manager.material_types) {
        material_type.second.rebuild_pipeline(swapchain.extent, renderpass, descriptor_set_layout);
    }

    init_framebuffers();
    init_uniform_buffers();
    init_descriptor_pool();
    init_descriptor_sets();

    //must rebuild all the material descriptor sets, too
    //TODO: what about materials that aren't attached to a MeshRenderer?
    for (auto& renderer : mesh_renderers) {
        renderer->material->init_descriptor_set(descriptor_pool, asset_manager);
    }

    init_command_buffers();


}


void Renderer::init() {
    init_physical_device();
    init_logical_device();
    swapchain.init();
    init_depth();
    init_render_pass();
    init_descriptor_set_layout();
    init_materials();

    init_framebuffers();
    init_command_pool();
    init_uniform_buffers();
    init_descriptor_pool();
    init_descriptor_sets();
    init_command_buffers();
    init_sync_objects();
}

void Renderer::wait_for_idle() {
    context.device.waitIdle();
}

void Renderer::close_swapchain() {
    depth_texture.reset();

    for(auto framebuffer : framebuffers) {
        context.device.destroyFramebuffer(framebuffer);
    }
    for(auto &uniform_buffer: uniform_buffers) {
        uniform_buffer.close();
    }

    uniform_buffers.clear();
    context.device.destroyDescriptorPool(descriptor_pool);
    command_buffers.close(context);

    material_manager.close_pipelines();

    context.device.destroyRenderPass(renderpass);
    swapchain.close();
}

void Renderer::close() {
    close_swapchain();
    context.device.destroyDescriptorSetLayout(descriptor_set_layout);
    material_manager.close_layouts();
    for (auto &m : mesh_renderers) {
        m->material->close();
    }

    context.device.destroyCommandPool(context.command_pool);
    context.instance.destroySurfaceKHR(context.surface);
    sync.clear();
    context.device.destroy();
}