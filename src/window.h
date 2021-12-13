#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "context.h"

class Window {
    public:
        int width;
        int height;
        int framebuffer_width;
        int framebuffer_height;

        Window(int w, int h, Context &ctx) : width(w), height(h), context(ctx) {
            open();
        }
        ~Window();

        bool should_close();
        void poll_events();
        void create_surface();

        void on_framebuffer_resized(int w, int h);

    private:
        Context &context;
        GLFWwindow *window;
        void open();
};