#include "object.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "log.h"
#include "engine.h"


void Transform::set_rotation(const glm::vec3& euler)
{
	rotation = glm::quat(euler);
}

glm::mat4 Transform::matrix()
{
	glm::mat4 m = glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);
	return m;
	
}

glm::vec3 Transform::forward()
{
	//TODO: BROKEN
	return glm::vec4(0, 1, 0, 0) * matrix();
}

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
