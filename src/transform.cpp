#include "transform.h"

void Transform::set_rotation(const glm::vec3& euler)
{
	rotation = glm::quat(euler);
}

glm::mat4 Transform::matrix() const
{
	glm::mat4 m = glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);
	return m;

}

glm::vec3 Transform::forward() const
{
	//TODO: BROKEN
	return glm::vec4(0, 1, 0, 0) * matrix();
}