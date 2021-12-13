#include "swapchain.h"
#include "log.h"

#include <iostream>
#include <algorithm>

SwapchainDetails Swapchain::get_swapchain_details_from_device(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapchainDetails res;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &res.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    if(format_count > 0) {
        res.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, res.formats.data());
    } else {
        DEBUG("no surface formats")
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
    if(present_mode_count > 0) {
        res.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, res.present_modes.data());
    } else {
        DEBUG("no present modes")
    }

    return res;
}

VkSurfaceFormatKHR SwapchainDetails::choose_surface_format() {
    auto desired_format = VK_FORMAT_B8G8R8A8_SRGB;
    auto desired_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    for (const auto &format : formats)
    {
        if(format.format == desired_format && format.colorSpace == desired_color_space) {
            return format;
        }
    }

    //fallback to the first as default
    return formats[0];
}

VkPresentModeKHR SwapchainDetails::choose_present_mode() {
    auto desired_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    for(const auto &mode : present_modes) {
        if (mode == desired_mode) {
            return mode;
        }
    }

    //fallback to fifo
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainDetails::choose_swap_extent(VkExtent2D framebuffer_extent) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent {
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
    VkSurfaceFormatKHR format = details.choose_surface_format();
    VkPresentModeKHR present_mode = details.choose_present_mode();
    VkExtent2D extent = details.choose_swap_extent(context.framebuffer_extent);

    uint32_t image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
        image_count = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = context.surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_family_indices[] = {context.queue_families.graphics.value(), context.queue_families.presentation.value()};

    if(context.queue_families.graphics == context.queue_families.presentation) {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }

    create_info.preTransform = details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(context.device, &create_info, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }

    vkGetSwapchainImagesKHR(context.device, swapchain, &image_count, nullptr);
    this->images.resize(image_count);
    vkGetSwapchainImagesKHR(context.device, swapchain, &image_count, images.data());

    this->format = format.format;
    this->extent = extent;

    init_image_views();

    //init fences to null
    image_fences.resize(image_count, VK_NULL_HANDLE);
}

void Swapchain::init_image_views() {
    image_views.resize(images.size());

    for (size_t i; i < images.size(); i++) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

        create_info.format = this->format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if(vkCreateImageView(context.device, &create_info, nullptr, &image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain image views");
        }
    }
}

void Swapchain::close() {
    for(auto view : image_views) {
        vkDestroyImageView(context.device, view, nullptr);
    }
    vkDestroySwapchainKHR(context.device, swapchain, nullptr);
}