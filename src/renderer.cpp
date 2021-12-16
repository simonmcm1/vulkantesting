#include "renderer.h"
#include "log.h"
#include "geometry.h"

#include <iostream>
#include <set>
#include <glm/gtc/matrix_transform.hpp>
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

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderpass_info{};
    renderpass_info.attachmentCount = 1;
    renderpass_info.pAttachments = &color_attachment;
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
        vk::ImageView attachments[] = {
            swapchain.image_views[i]
        };

        vk::FramebufferCreateInfo create_info {};
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
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

void Renderer::init_vertex_buffers() {
    //vertex buffer
    vk::DeviceSize size = sizeof(Vertex) * QUAD.vertices.size();

    vertex_buffer = std::make_unique<Buffer>(context);
    vertex_buffer->init(size,
                        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                        vk::MemoryPropertyFlagBits::eDeviceLocal);

    Buffer vstaging = vertex_buffer->get_staging();
    vstaging.store(QUAD.vertices.data());
    vstaging.copy(command_pool, *vertex_buffer);
    vstaging.close();

    //index_buffer
    size = sizeof(QUAD.indices[0]) * QUAD.indices.size();

    index_buffer = std::make_unique<Buffer>(context);
    index_buffer->init(size,
                       vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                       vk::MemoryPropertyFlagBits::eDeviceLocal);

    Buffer istaging = index_buffer->get_staging();
    istaging.store(QUAD.indices.data());
    istaging.copy(command_pool, *index_buffer);
    istaging.close();

}

void Renderer::init_descriptor_set_layout() {
    vk::DescriptorSetLayoutBinding ubo_layout(0,
                                              vk::DescriptorType::eUniformBuffer,
                                              1,
                                              vk::ShaderStageFlagBits::eVertex,
                                              nullptr);
    vk::DescriptorSetLayoutCreateInfo create_info({}, 1, &ubo_layout);
    descriptor_set_layout = context.device.createDescriptorSetLayout(create_info);
}

void Renderer::init_descriptor_pool() {
    vk::DescriptorPoolSize pool_size(vk::DescriptorType::eUniformBuffer, static_cast<uint32_t> (swapchain.images.size()));

    vk::DescriptorPoolCreateInfo create_info({},
        static_cast<uint32_t>(swapchain.images.size()),
        1,
        &pool_size);
    
    descriptor_pool = context.device.createDescriptorPool(create_info);
}

void Renderer::init_descriptor_sets() {
    std::vector<vk::DescriptorSetLayout> layouts(swapchain.images.size(), descriptor_set_layout);

    vk::DescriptorSetAllocateInfo allocate_info(descriptor_pool,
                                                static_cast<uint32_t>(layouts.size()),
                                                layouts.data());

    descriptor_sets = context.device.allocateDescriptorSets(allocate_info);

    for (size_t i = 0; i < layouts.size(); i++) {
        vk::DescriptorBufferInfo buffer_info(uniform_buffers[i].buffer, 0, sizeof(UniformBufferObject));
        vk::WriteDescriptorSet write(descriptor_sets[i],
                                     0,
                                     0,
                                     1,
                                     vk::DescriptorType::eUniformBuffer,
                                     nullptr,
                                     &buffer_info,
                                     nullptr);

        context.device.updateDescriptorSets(1, &write, 0, nullptr);
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

void Renderer::update_uniform_buffers(uint32_t current_image) {
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    UniformBufferObject ubo{};
    ubo.M = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.V = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.P = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float) swapchain.extent.height, 0.1f, 10.0f);
    ubo.P[1][1] *= -1;

    uniform_buffers[current_image].store(&ubo);
}

void Renderer::init_command_buffers() {
    TRACE("initializing command buffers")

    vk::CommandBufferAllocateInfo allocate_info{};
    allocate_info.commandPool = command_pool;
    allocate_info.level = vk::CommandBufferLevel::ePrimary;
    allocate_info.commandBufferCount = (uint32_t)framebuffers.size();

    command_buffers = context.device.allocateCommandBuffers(allocate_info);

    for(size_t i = 0; i < command_buffers.size(); i++) {
        vk::CommandBufferBeginInfo begin_info{};
        begin_info.flags = {};
        begin_info.pInheritanceInfo = nullptr;

        command_buffers[i].begin(begin_info);

        vk::RenderPassBeginInfo renderpass_begin_info{};
        renderpass_begin_info.framebuffer = framebuffers[i];
        renderpass_begin_info.renderPass = renderpass;
        renderpass_begin_info.renderArea.offset = vk::Offset2D{0,0};
        renderpass_begin_info.renderArea.extent = swapchain.extent;
        vk::ClearValue clear_color(vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}});
        renderpass_begin_info.clearValueCount = 1;
        renderpass_begin_info.pClearValues = &clear_color;

        command_buffers[i].beginRenderPass(renderpass_begin_info, vk::SubpassContents::eInline);
        command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline);

        vk::Buffer vertex_buffers[] = {vertex_buffer->buffer};
        vk::DeviceSize offsets[] = {0};
        command_buffers[i].bindVertexBuffers(0, 1, vertex_buffers, offsets);
        command_buffers[i].bindIndexBuffer(index_buffer->buffer, 0, vk::IndexType::eUint16);
        command_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0, 1, &descriptor_sets[i], 0, nullptr);
        command_buffers[i].drawIndexed(static_cast<uint32_t>(QUAD.indices.size()), 1, 0, 0, 0);

        command_buffers[i].endRenderPass();
        command_buffers[i].end();
    }
}

void Renderer::render() {
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

    update_uniform_buffers(next_image);

    vk::Semaphore wait_semaphores[] = {sync[current_frame].image_available.semaphore};
    vk::Semaphore signal_semaphores[] = {sync[current_frame].render_finished.semaphore};
    vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo submit_info{};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[next_image];
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
    }
    catch (vk::OutOfDateKHRError const &e) {
        //Out of date isn't defined as success in hpp wrapper so it throw an exception here :(
        context.framebuffer_resized = false;
        rebuild_swapchain();
    }

    if(present_result == vk::Result::eErrorOutOfDateKHR || present_result == vk::Result::eSuboptimalKHR || context.framebuffer_resized) {
        context.framebuffer_resized = false;
        rebuild_swapchain();
    } else if(present_result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swapchain image");
    }
    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::init_command_pool() {
    vk::CommandPoolCreateInfo pool_create_info{};
    pool_create_info.queueFamilyIndex = context.queue_families.graphics.value();
    pool_create_info.flags = {};

    command_pool = context.device.createCommandPool(pool_create_info);
}

void Renderer::rebuild_swapchain() {
    TRACE("rebuilding swapchain");
    context.device.waitIdle();

    close_swapchain();

    swapchain.init();
    init_render_pass();
    pipeline.init(swapchain.extent, renderpass, descriptor_set_layout);
    init_framebuffers();
    init_uniform_buffers();
    init_descriptor_pool();
    init_descriptor_sets();
    init_command_buffers();
}


void Renderer::init() {
    init_physical_device();
    init_logical_device();
    swapchain.init();
    init_render_pass();
    init_descriptor_set_layout();
    pipeline.init(swapchain.extent, renderpass, descriptor_set_layout);
    init_framebuffers();
    init_command_pool();
    init_vertex_buffers();
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
    for(auto framebuffer : framebuffers) {
        context.device.destroyFramebuffer(framebuffer);
    }
    for(auto uniform_buffer: uniform_buffers) {
        uniform_buffer.close();
    }
    uniform_buffers.clear();
    context.device.destroyDescriptorPool(descriptor_pool);
    context.device.freeCommandBuffers(command_pool, command_buffers);
    pipeline.close();
    context.device.destroyRenderPass(renderpass);
    swapchain.close();
}

void Renderer::close() {
    close_swapchain();
    context.device.destroyDescriptorSetLayout(descriptor_set_layout);
    vertex_buffer->close();
    index_buffer->close();
    context.device.destroyCommandPool(command_pool);
    context.instance.destroySurfaceKHR(context.surface);
    sync.clear();
    context.device.destroy();
}