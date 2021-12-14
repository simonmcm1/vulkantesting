#pragma once

#include <vector>
#include "context.h"

class Swapchain {
public:
    vk::SwapchainKHR swapchain;
    vk::Format format;
    vk::Extent2D extent;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> image_views;
    std::vector<vk::Fence> image_fences;

    Swapchain(Context &ctx) : context(ctx){};
    void init();
    void close();


    static SwapchainDetails get_swapchain_details_from_device(vk::PhysicalDevice device, vk::SurfaceKHR surface);

private:
    Context &context;
    void init_image_views();
};