#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "mesh.h"

class Renderer;
class Engine;

class Transform {
public:
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	
	Transform() : position(glm::vec3(0)), rotation(glm::vec3(0)), scale(glm::vec3(1)) {};

	void set_rotation(const glm::vec3& euler);
	glm::mat4 matrix();
	glm::vec3 forward();
	
};

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
	MeshObject(Context &ctx) : Object(ctx), mesh_renderer(nullptr) {
		mesh_renderer = std::make_unique<MeshRenderer>(context);
	}
	MeshObject(MeshObject&& other): Object(std::move(other)), mesh_renderer(nullptr) {
		mesh_renderer = std::move(other.mesh_renderer);
	}
	void init(Engine& engine) override;

	void close() override;
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