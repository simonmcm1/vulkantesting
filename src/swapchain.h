#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "context.h"

class Swapchain {
public:
    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    std::vector<VkFence> image_fences;

    Swapchain(Context &ctx) : context(ctx){};
    void init();
    void close();


    static SwapchainDetails get_swapchain_details_from_device(VkPhysicalDevice device, VkSurfaceKHR surface);

private:
    Context &context;
    void init_image_views();
};