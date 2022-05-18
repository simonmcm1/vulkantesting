#pragma once

#include "transform.h"
#include "mesh.h"
#include "model.h"

class Renderer;
class Engine;


class Object {
public:
	Transform transform;
	virtual void init(Engine &engine) {}
	virtual void close() {}

	Object(Object& other) = delete;
	Object(Object&& other) noexcept : context(other.context) {
		transform = std::move(other.transform);
	}
	Object(Context &ctx) : context(ctx) {}
protected:
	Context& context;
};

class MeshObject : public Object {
public:
	std::unique_ptr<MeshRenderer> mesh_renderer = nullptr;
	MeshObject(Context& ctx) : Object(ctx), mesh_renderer(nullptr) {
		mesh_renderer = std::make_unique<MeshRenderer>(context);
	}
	MeshObject(MeshObject&& other) : Object(std::move(other)), mesh_renderer(nullptr) {
		mesh_renderer = std::move(other.mesh_renderer);
	}
	void init(Engine& engine) override;

	void close() override;
};

class SceneObject : public Object {
public:
	SceneObject(Context& ctx) : Object(ctx) {

	}

	void init(Engine& engine) override;
	void load_model(Engine &engine, Model* model);

	std::unordered_map<std::string, MeshObject *> mesh_objects;
	std::vector<std::unique_ptr<Material>> materials;
};

class Camera {
public:	
	Transform transform;

	float fov;
	float aspect;
	float near_clip;
	float far_clip;

	glm::mat4 view();
	glm::mat4 projection();

};