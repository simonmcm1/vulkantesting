#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

class Transform {
public:
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	Transform() : position(glm::vec3(0)), rotation(glm::vec3(0)), scale(glm::vec3(1)) {};

	void set_rotation(const glm::vec3& euler);
	void set_matrix(const glm::mat4& mat);
	glm::mat4 matrix() const;
	glm::vec3 forward() const;
};
