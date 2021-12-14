#include "swapchain.h"
#include "log.h"

#include <iostream>
#include <algorithm>

SwapchainDetails Swapchain::get_swapchain_details_from_device(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    SwapchainDetails res;
    
    device.getSurfaceCapabilitiesKHR(surface, &res.capabilities);

    res.formats = std::move(device.getSurfaceFormatsKHR(surface));
    res.present_modes = std::move(device.getSurfacePresentModesKHR(surface));

    return res;
}

vk::SurfaceFormatKHR SwapchainDetails::choose_surface_format() {
    auto desired_format = vk::Format::eB8G8R8A8Srgb;
    auto desired_color_space = vk::ColorSpaceKHR::eSrgbNonlinear;
    for (const auto &format : formats)
    {
        if(format.format == desired_format && format.colorSpace == desired_color_space) {
            return format;
        }
    }

    //fallback to the first as default
    return formats[0];
}

vk::PresentModeKHR SwapchainDetails::choose_present_mode() {
    auto desired_mode = vk::PresentModeKHR::eMailbox;
    for (const auto &mode : present_modes)
    {
        if (mode == desired_mode) {
            return mode;
        }
    }

    //fallback to fifo
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapchainDetails::choose_swap_extent(vk::Extent2D framebuffer_extent) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    vk::Extent2D actualExtent {
        framebuffer_extent.width,
        framebuffer_extent.height
    };
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void Swapchain::init() {
    TRACE("initializing swapchain")

    auto details = Swapchain::get_swapchain_details_from_device(context.physical_device, context.surface);
    vk::SurfaceFormatKHR format = details.choose_surface_format();
    vk::PresentModeKHR present_mode = details.choose_present_mode();
    vk::Extent2D extent = details.choose_swap_extent(context.framebuffer_extent);

    uint32_t image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
        image_count = details.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR create_info{};
    create_info.surface = context.surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    uint32_t queue_family_indices[] = {context.queue_families.graphics.value(), context.queue_families.presentation.value()};

    if(context.queue_families.graphics == context.queue_families.presentation) {
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    } else {
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }

    create_info.preTransform = details.capabilities.currentTransform;
    create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = nullptr;

    swapchain = context.device.createSwapchainKHR(create_info);

    images = context.device.getSwapchainImagesKHR(swapchain);

    this->format = format.format;
    this->extent = extent;

    init_image_views();

    //init fences to null
    image_fences.resize(image_count, vk::Fence(nullptr));
}

void Swapchain::init_image_views() {
    image_views.resize(images.size());

    for (size_t i; i < images.size(); i++) {
        vk::ImageViewCreateInfo create_info{};
        create_info.image = images[i];
        create_info.viewType = vk::ImageViewType::e2D;

        create_info.format = this->format;
        create_info.components.r = vk::ComponentSwizzle::eIdentity;
        create_info.components.g = vk::ComponentSwizzle::eIdentity;
        create_info.components.b = vk::ComponentSwizzle::eIdentity;
        create_info.components.a = vk::ComponentSwizzle::eIdentity;

        create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        image_views[i] = context.device.createImageView(create_info);
    }
}

void Swapchain::close() {
    for(auto view : image_views) {
        context.device.destroyImageView(view);
    }
    context.device.destroySwapchainKHR(swapchain);
}