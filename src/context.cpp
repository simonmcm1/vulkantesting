#include "context.h"
#include "log.h"
#include <iostream>
#include <set>
#include <algorithm>
#include "swapchain.h"

void Context::init_validation_layers() {
    TRACE("initializing validation layers")
    auto available_layers = vk::enumerateInstanceLayerProperties();

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

bool Context::is_device_suitable(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    vk::PhysicalDeviceProperties props = device.getProperties();
    vk::PhysicalDeviceFeatures features = device.getFeatures();

    QueueFamilies families = Context::get_queue_families_from_device(device, surface);
    if(!families.is_complete()) {
        return false;
    }

    //check device extensions
    std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties();

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

    return props.deviceType ==  vk::PhysicalDeviceType::eDiscreteGpu && features.geometryShader;
}

QueueFamilies Context::get_queue_families_from_device(vk::PhysicalDevice device, vk::SurfaceKHR surface) {

    std::vector<vk::QueueFamilyProperties> families = device.getQueueFamilyProperties();

    QueueFamilies result;
    for (uint32_t index = 0; index < families.size(); index++)
    {
        if(families.at(index).queueFlags & vk::QueueFlagBits::eGraphics) {
            result.graphics = index;
        }
        vk::Bool32 supports_present = device.getSurfaceSupportKHR(index, surface);
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

    vk::ApplicationInfo app_info(
        "vulkan test",
        VK_MAKE_VERSION(0, 0, 1),
        "SVE",
        VK_MAKE_VERSION(0, 0, 1),
        VK_API_VERSION_1_1);

    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    if(glfw_extension_count <= 0) {
        throw std::runtime_error("failed to get glfw required extensions");
    }

    std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();

    DEBUG(extensions.size() << " extensions available")

    vk::InstanceCreateInfo create_info{};
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

    if (vk::createInstance(&create_info, nullptr, &instance) != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create vulkan instance");
    }
}

void Context::close() {
    instance.destroy();
}