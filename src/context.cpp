#include "context.h"
#include "log.h"
#include <iostream>
#include <set>
#include <algorithm>
#include "swapchain.h"

void Context::init_validation_layers() {
    TRACE("initializing validation layers")
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const auto& layer : validation_layers) {
        bool found = false;
        for(const auto& layer_props : available_layers) {
            if (std::string(layer_props.layerName) == layer) {
                found = true;
                break;
            }
        }
        if(!found) {
            throw std::runtime_error("validation layers requested but not available");
        }
    }
}

bool Context::is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    QueueFamilies families = Context::get_queue_families_from_device(device, surface);
    if(!families.is_complete()) {
        return false;
    }

    //check device extensions
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

    for (const auto &required_ext : device_extensions) {
        bool found = false;
        for(const auto &ext : extensions) {
            if(std::string(ext.extensionName) == required_ext) {
                found = true;
                break;
            }
        }
        if(!found) {
            return false;
        }
    }

    //check swapchain capabilities
    SwapchainDetails swapchain = Swapchain::get_swapchain_details_from_device(device, surface);
    if (!swapchain.is_complete()) {
        TRACE("incomplete swapchain")
        return false;
    }

    return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.geometryShader;
}

QueueFamilies Context::get_queue_families_from_device(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, families.data());

    QueueFamilies result;
    for (uint32_t index = 0; index < families.size(); index++)
    {
        if(families.at(index).queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            result.graphics = index;
        }
        VkBool32 supports_present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &supports_present);
        if(supports_present) {
            result.presentation = index;
        }
    }

    return result;
}

void Context::init() {

    if(enable_validation_layers) {
        init_validation_layers();
    }

    TRACE("initializing vulkan instance")

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vulkan test";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = "SVE";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = VK_API_VERSION_1_0;

    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    if(glfw_extension_count <= 0) {
        throw std::runtime_error("failed to get glfw required extensions");
    }

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    DEBUG(extension_count << " extensions available")

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    create_info.enabledLayerCount = 0;

    if(enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    if(vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vulkan instance");
    }
}

void Context::close() {
    vkDestroyInstance(instance, nullptr);
}