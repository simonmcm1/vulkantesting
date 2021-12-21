

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <memory>
#include <vector>
#include <glm/gtc/quaternion.hpp>

#include "window.h"
#include "context.h"
#include "engine.h"
#include "geometry.h"



class Application
{
public:
    void run();

private:
    void init_window(int width, int height);
    void main_loop();
    void update();
    void close();
    Engine engine;
    std::unique_ptr<Window> window;
    MeshObject* plane;
    float plane_rot = 0.0f;
    MeshObject* plane2;
    float plane2_rot = 0.0f;
};

void Application::run() {
    plane = &engine.create_meshobject();
    plane->mesh_renderer->load(QUAD);

    plane2 = &engine.create_meshobject();
    plane2->mesh_renderer->load(QUAD);
    plane2->transform.position = glm::vec3(0, 0, 0.3);

    init_window(800, 600);
    engine.init(*window);

    engine.camera = std::make_unique<Camera>();
    engine.camera->fov = glm::radians(45.0f);
    engine.camera->aspect = window->width / (float)window->height;
    engine.camera->near_clip = 0.1f;
    engine.camera->far_clip = 10.0;
    engine.camera->transform.position = glm::vec3(2.0f, 2.0f, 2.0f);
    engine.camera->transform.rotation = glm::quatLookAt(glm::normalize(glm::vec3(0) - glm::vec3(2.0)), glm::vec3(0, 0, 1));

    main_loop();
    close();
}

void Application::update() {
    plane_rot = plane_rot + glm::radians(90.0f) * engine.clock.delta_time;
    plane->transform.set_rotation(glm::vec3(0, 0, plane_rot));
    plane2_rot = plane2_rot - glm::radians(90.0f) * engine.clock.delta_time;
    plane2->transform.set_rotation(glm::vec3(0, 0, plane2_rot));
}

void Application::init_window(int width, int height) {
    window = std::make_unique<Window>(width, height, engine.get_context());
}

void Application::main_loop() {
    while (!window->should_close()) {
        window->poll_events();
        update();
        engine.execute_frame();
    }
    engine.close();
}

void Application::close() {
    window.reset();
}

int main() {
    try {
        Application app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Caught Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}