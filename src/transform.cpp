#include "transform.h"

void decompose(const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale)
{
    pos = m[3];
    for (int i = 0; i < 3; i++)
        scale[i] = glm::length(glm::vec3(m[i]));
    const glm::mat3 rot_mat(
        glm::vec3(m[0]) / scale[0],
        glm::vec3(m[1]) / scale[1],
        glm::vec3(m[2]) / scale[2]);
    rot = glm::quat_cast(rot_mat);
}

void Transform::set_rotation(const glm::vec3& euler)
{
	rotation = glm::quat(euler);
}

void Transform::set_matrix(const glm::mat4& mat)
{
    decompose(mat, position, rotation, scale);
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