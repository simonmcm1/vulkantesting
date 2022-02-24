#pragma once

#include "context.h"
#include "transform.h"

class Light {
public:
	Light() : transform({}), color(0) {}
	Transform transform;
	glm::vec4 color;

};

class LightManager {
public:
	std::vector<Light> lights;
	Light& create_light();
	void assign_lightdata(UniformBufferObject& ubo);
};