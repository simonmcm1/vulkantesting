#include "object.h"
#include "log.h"
#include "engine.h"

glm::mat4 Camera::view()
{
	return glm::inverse(transform.matrix());
}

glm::mat4 Camera::projection()
{
	glm::mat4 p = glm::perspective(fov, aspect, near_clip, far_clip);
	
	//glm assumes opengl which has reversed depth
	p[1][1] *= -1;

	return p;
}

void MeshObject::init(Engine& engine) {
	mesh_renderer->init(engine.renderer);
}

void MeshObject::close() {
	mesh_renderer->close();
}

void SceneObject::init(Engine& engine) 
{
	auto& material_manager = engine.renderer.get_material_mangager();
	auto cmat = material_manager.get_instance<ColoredMaterial>("colored");
	auto fmat = cmat.get();
	materials.push_back(std::move(cmat));

	fmat->set_color(glm::vec4(0.81, 0.0, 0.0, 1.0));
	for (auto meshobj : mesh_objects) {
		meshobj.second->mesh_renderer->material = fmat;
	}
}


void SceneObject::load_model(Engine &engine, Model* model)
{
	for (const auto& node : model->nodes) {
		if (node.second.mesh == "") {
			continue;
		}
		auto &mo = engine.create_meshobject();
		mo.transform = node.second.transform;
		mo.mesh_renderer->load(model->meshes.at(node.second.mesh));
		
		mesh_objects[node.second.name] = &mo;
	}
}
