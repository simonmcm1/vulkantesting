#pragma once

#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>


#include <vector>
#include <optional>

struct UniformBufferObject {
    alignas(16) glm::mat4 V;
    alignas(16) glm::mat4 P;
    alignas(16) glm::mat4 M[16];
};

struct PushConstants {
    glm::uint32_t object_index;
};

struct QueueFamilies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> presentation;

    bool is_complete() {
        return graphics.has_value() && presentation.has_value();
    }
};

struct SwapchainDetails {
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
    vk::SurfaceCapabilitiesKHR capabilities;

    bool is_complete()
    {
        return !formats.empty() && !present_modes.empty();
    }

    vk::SurfaceFormatKHR choose_surface_format();
    vk::PresentModeKHR choose_present_mode();
    vk::Extent2D choose_swap_extent(vk::Extent2D framebuffer_extent);
};

const std::vector<const char *>
    validation_layers = {
        "VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class Context {
public:
    vk::Instance instance;
    vk::PhysicalDevice physical_device;
    vk::Device device;
    QueueFamilies queue_families;
    vk::Queue graphics_queue;
    vk::Queue presentation_queue;
    vk::SurfaceKHR surface;
    vk::Extent2D framebuffer_extent;
    bool framebuffer_resized = false;
    vk::CommandPool command_pool = nullptr;

    void init();
    void close();


    uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags props);
    static QueueFamilies get_queue_families_from_device(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    static bool is_device_suitable(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

private:

    void init_validation_layers();
};

