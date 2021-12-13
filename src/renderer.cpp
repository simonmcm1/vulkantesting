#include "renderer.h"
#include "log.h"
#include <iostream>
#include <set>

void Renderer::init_physical_device() {
    TRACE("initializing physical device")

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(context.instance, &device_count, nullptr);
    if (device_count <= 0) {
        throw std::runtime_error("no physical device available");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(context.instance, &device_count, devices.data());

    for (const auto& device : devices) {
        if(Context::is_device_suitable(device, context.surface)) {
            context.physical_device = device;
            break;
        }
    }

    if(context.physical_device == VK_NULL_HANDLE) {
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
    
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    float priorities[] = {1.0f};
    for (uint32_t family : families) {
        VkDeviceQueueCreateInfo queue_create_info{};    
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = priorities;

        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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

    if(vkCreateDevice(context.physical_device, &create_info, nullptr, &context.device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(context.device, context.queue_families.graphics.value(), 0, &context.graphics_queue);
    vkGetDeviceQueue(context.device, context.queue_families.presentation.value(), 0, &context.presentation_queue);
}

void Renderer::init_render_pass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;\

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderpass_info{};
    renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_info.attachmentCount = 1;
    renderpass_info.pAttachments = &color_attachment;
    renderpass_info.subpassCount = 1;
    renderpass_info.pSubpasses = &subpass;
    renderpass_info.dependencyCount = 1;
    renderpass_info.pDependencies = &dependency; 

    if (vkCreateRenderPass(context.device, &renderpass_info, nullptr, &renderpass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }
}

void Renderer::init_framebuffers() {
    TRACE("initializing framebuffers")
    framebuffers.resize(swapchain.image_views.size());
    for(size_t i = 0; i < framebuffers.size(); i++) {
        VkImageView attachments[] = {
            swapchain.image_views[i]
        };

        VkFramebufferCreateInfo create_info {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
        create_info.renderPass = renderpass;
        create_info.width = swapchain.extent.width;
        create_info.height = swapchain.extent.height;
        create_info.layers = 1;

        if(vkCreateFramebuffer(context.device, &create_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer");
        }
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
        VkCommandPoolCreateInfo pool_create_info{};
        pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.queueFamilyIndex = context.queue_families.graphics.value();
        pool_create_info.flags = 0;

        if(vkCreateCommandPool(context.device, &pool_create_info, nullptr, &command_pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool");
        }
    }

    command_buffers.resize(framebuffers.size());
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = (uint32_t)command_buffers.size();

    if(vkAllocateCommandBuffers(context.device, &allocate_info, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }

    for(size_t i = 0; i < command_buffers.size(); i++) {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        if(vkBeginCommandBuffer(command_buffers[i], &begin_info) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin command buffer");
        }

        VkRenderPassBeginInfo renderpass_begin_info{};
        renderpass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderpass_begin_info.framebuffer = framebuffers[i];
        renderpass_begin_info.renderPass = renderpass;
        renderpass_begin_info.renderArea.offset = {0,0};
        renderpass_begin_info.renderArea.extent = swapchain.extent;
        VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderpass_begin_info.clearValueCount = 1;
        renderpass_begin_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(command_buffers[i], &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
        vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(command_buffers[i]);

        if(vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
            throw new std::runtime_error("failed to end command buffer");
        }
    }
}

void Renderer::rebuild_swapchain() {
    vkDeviceWaitIdle(context.device);
    close_swapchain();

    swapchain.init();
    init_render_pass();
    pipeline.init(swapchain.extent, renderpass);
    init_framebuffers();
    init_command_buffers();
}

void Renderer::render() {
    vkWaitForFences(context.device, 1, &sync[current_frame].in_flight_frame.fence, VK_TRUE, UINT64_MAX);
    
    uint32_t next_image;
    VkResult next_image_status = vkAcquireNextImageKHR(context.device, swapchain.swapchain, UINT64_MAX, sync[current_frame].image_available.semaphore, VK_NULL_HANDLE, &next_image);
    if(next_image_status == VK_ERROR_OUT_OF_DATE_KHR) {
        rebuild_swapchain();
        return;
    } else if(next_image_status != VK_SUCCESS && next_image_status != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire next swapchain image");
    }

    //wait if this image is in use by a previous frame
    if(swapchain.image_fences[next_image] != VK_NULL_HANDLE) {
        vkWaitForFences(context.device, 1, &swapchain.image_fences[next_image], VK_TRUE, UINT64_MAX);
    }
    swapchain.image_fences[next_image] = sync[current_frame].in_flight_frame.fence;

    VkSemaphore wait_semaphores[] = {sync[current_frame].image_available.semaphore};
    VkSemaphore signal_semaphores[] = {sync[current_frame].render_finished.semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[next_image];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkResetFences(context.device, 1, &sync[current_frame].in_flight_frame.fence);
    if(vkQueueSubmit(context.graphics_queue, 1, &submit_info, sync[current_frame].in_flight_frame.fence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkSwapchainKHR swapchains[] = {swapchain.swapchain};
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &next_image;
    present_info.pResults = nullptr;

    VkResult present_result = vkQueuePresentKHR(context.presentation_queue, &present_info);
    if(present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR || context.framebuffer_resized) {
        context.framebuffer_resized = false;
        rebuild_swapchain();
    } else if(present_result != VK_SUCCESS) {
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
    vkDeviceWaitIdle(context.device);
}

void Renderer::close_swapchain() {
    for(auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(context.device, framebuffer, nullptr);
    }

    vkFreeCommandBuffers(context.device, command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
    pipeline.close();
    vkDestroyRenderPass(context.device, renderpass, nullptr);
    swapchain.close();
}

void Renderer::close() {
    close_swapchain();
    vkDestroyCommandPool(context.device, command_pool, nullptr);
    sync.clear();
    vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
    vkDestroyDevice(context.device, nullptr);
}