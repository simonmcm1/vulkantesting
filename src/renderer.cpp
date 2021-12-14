#include "renderer.h"
#include "log.h"
#include <iostream>
#include <set>

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

void Renderer::init_command_buffers() {
    TRACE("initializing command buffers")

    //if this is a rebuild then we may already have a command pool
    if (command_pool == nullptr) {
        vk::CommandPoolCreateInfo pool_create_info{};
        pool_create_info.queueFamilyIndex = context.queue_families.graphics.value();
        pool_create_info.flags = {};

        command_pool = context.device.createCommandPool(pool_create_info);
    }

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
        command_buffers[i].draw(3, 1, 0, 0);

        command_buffers[i].endRenderPass();
        command_buffers[i].end();
    }
}

void Renderer::rebuild_swapchain() {
    TRACE("rebuilding swapchain");
    context.device.waitIdle();

    close_swapchain();

    swapchain.init();
    init_render_pass();
    pipeline.init(swapchain.extent, renderpass);
    init_framebuffers();
    init_command_buffers();
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

void Renderer::init() {
    init_physical_device();
    init_logical_device();
    swapchain.init();
    init_render_pass();
    pipeline.init(swapchain.extent, renderpass);
    init_framebuffers();
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
    context.device.freeCommandBuffers(command_pool, command_buffers);
    pipeline.close();
    context.device.destroyRenderPass(renderpass);
    swapchain.close();
}

void Renderer::close() {
    close_swapchain();
    context.device.destroyCommandPool(command_pool);
    context.instance.destroySurfaceKHR(context.surface);
    sync.clear();
    context.device.destroy();
}