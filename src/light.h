#pragma once

#include "context.h"

class Light {
	glm::vec3 position;

};

class LightManager {
	std::vector<Light> lights;
};