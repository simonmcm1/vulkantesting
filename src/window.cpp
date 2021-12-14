#include "window.h"
#include "log.h"
#include <stdexcept>

void glfw_framebuffer_resize_callback(GLFWwindow *window, int w, int h) {
    auto instance = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
    instance->on_framebuffer_resized(w, h);
}

void Window::open() {
    if(glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("failed to initialized glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
 //   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(width, height, "vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_resize_callback);
}

Window::~Window() {
    if (window != nullptr) {
        glfwDestroyWindow(window);
    }

    glfwTerminate();
}

bool Window::should_close() {
    return glfwWindowShouldClose(window);
}

void Window::poll_events() {
    glfwPollEvents();
}

void Window::on_framebuffer_resized(int w, int h) {
    TRACE("framebuffer resized" << w << "x" << h)
    context.framebuffer_resized = true;
    framebuffer_height = h;
    framebuffer_width = w;

    context.framebuffer_extent = vk::Extent2D {
        static_cast<uint32_t>(framebuffer_width),
        static_cast<uint32_t>(framebuffer_height),
    };
}

void Window::create_surface() {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(context.instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface");
    }
    context.surface = vk::SurfaceKHR(surface);
    context.framebuffer_extent = vk::Extent2D{
        static_cast<uint32_t>(framebuffer_width),
        static_cast<uint32_t>(framebuffer_height),
    };
}