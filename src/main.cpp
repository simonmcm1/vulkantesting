

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <memory>
#include <vector>

#include "window.h"
#include "context.h"
#include "renderer.h"

class Application
{
public:
    void run();

private:
    void init_window(int width, int height);
    void init_vulkan();
    void main_loop();
    void close();

    std::unique_ptr<Window> window;
    std::unique_ptr<Renderer> renderer;
    Context context;
};

void Application::run() {
    init_window(800, 600);
    init_vulkan();
    main_loop();
    close();
}

void Application::init_vulkan() {
    context.init();
    window->create_surface();
    renderer = std::make_unique<Renderer>(context);
    renderer->init();
}

void Application::init_window(int width, int height) {
    window = std::make_unique<Window>(width, height, context);
}

void Application::main_loop() {
    while (!window->should_close()) {
        window->poll_events();
        renderer->render();
    }
    renderer->wait_for_idle();
}

void Application::close() {
    renderer->close();
    renderer.reset();
    context.close();
    window.reset();
}

int main() {
    try {
        Application app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}