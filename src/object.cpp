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
