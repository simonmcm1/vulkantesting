#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>

struct QueueFamilies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> presentation;

    bool is_complete() {
        return graphics.has_value() && presentation.has_value();
    }
};

struct SwapchainDetails {
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
    VkSurfaceCapabilitiesKHR capabilities;

    bool is_complete()
    {
        return !formats.empty() && !present_modes.empty();
    }

    VkSurfaceFormatKHR choose_surface_format();
    VkPresentModeKHR choose_present_mode();
    VkExtent2D choose_swap_extent(VkExtent2D framebuffer_extent);
};

const std::vector<const char *>
    validation_layers = {
        "VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class Context {
public:
    VkInstance instance;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device;
    QueueFamilies queue_families;
    VkQueue graphics_queue;
    VkQueue presentation_queue;
    VkSurfaceKHR surface;
    VkExtent2D framebuffer_extent;
    bool framebuffer_resized = false;
    
    void init();
    void close();


    static QueueFamilies get_queue_families_from_device(VkPhysicalDevice device, VkSurfaceKHR surface);
    static bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface);

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

private:

    void init_validation_layers();
};

