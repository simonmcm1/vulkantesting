

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
#include "texture.h"



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
    ColoredMaterial *colored_mat;
    glm::vec4 color{0.0f, 0.0f, 1.0f, 1.0f};
};

void Application::run() {
    engine.asset_manager.load_assets();
    plane = &engine.create_meshobject();

    //auto hydrant = engine.asset_manager.get_model("fire-hydrant");
    auto hydrant = engine.asset_manager.get_model("sphere");
    //plane->mesh_renderer->load(hydrant->meshes.at("Aset_street__S_uiuhbegfa_LOD0"));
    plane->mesh_renderer->load(hydrant->meshes.at("defaultobject"));
    //auto hydrant = engine.asset_manager.get_model("cube");
    //plane->mesh_renderer->load(hydrant->meshes.at("defaultobject"));
    //plane->transform.scale = glm::vec3(0.05, 0.05, 0.05);
    plane->transform.set_rotation(glm::vec3(glm::radians(90.0), 0, 0));
    plane->transform.position = glm::vec3(0, 0, -1.0f);

    //plane2 = &engine.create_meshobject();
    //plane2->mesh_renderer->load(QUAD);
    //plane2->transform.position = glm::vec3(0, 0, -0.3);

    init_window(800, 600);
    engine.init(*window);

    auto& material_manager = engine.renderer.get_material_mangager();
    //auto cmat = material_manager.get_instance<BasicMaterial>("basic");
    //auto fmat = cmat.get();
    //fmat->albedo_texture = "fire-hydrant-albedo";
    /*auto cmat = material_manager.get_instance<StandardMaterial>("standard");
    auto fmat = cmat.get();
    fmat->albedo_texture = "fire-hydrant-albedo";
    fmat->normal_map = "fire-hydrant-normal";
    fmat->pbr_map = "fire-hydrant-pbr";
    */
    auto cmat = material_manager.get_instance<ColoredMaterial>("colored");
    auto fmat = cmat.get();
    fmat->set_color(glm::vec4(0.81, 0.0, 0.0, 1.0));

    //auto mat2 = material_manager.get_instance<BasicMaterial>("basic");
    //mat2->albedo_texture = "smile";

    plane->mesh_renderer->material = fmat;

    //plane2->mesh_renderer->material = mat2.get();

    engine.camera = std::make_unique<Camera>();
    engine.camera->fov = glm::radians(45.0f);
    engine.camera->aspect = window->width / (float)window->height;
    engine.camera->near_clip = 0.1f;
    engine.camera->far_clip = 10.0;
    engine.camera->transform.position = glm::vec3(4.0f, 4.0f, 4.0f);
    engine.camera->transform.rotation = glm::quatLookAt(glm::normalize(glm::vec3(0) - glm::vec3(5.0)), glm::vec3(0, 0, 1));

    main_loop();
    
    close();
}

void Application::update() {
    plane_rot = plane_rot + glm::radians(90.0f) * engine.clock.delta_time;
    plane->transform.set_rotation(glm::vec3(glm::radians(90.0), 0, plane_rot));
    //plane2_rot = plane2_rot - glm::radians(90.0f) * engine.clock.delta_time;
    //plane2->transform.set_rotation(glm::vec3(0, 0, plane2_rot));

    //if(color.r <= 1.0f) {
    //    color.r = color.r + engine.clock.delta_time * 0.1f;
    //}
    //colored_mat->set_color(color);
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