#pragma once

#include "light.h"
#include <iostream>

Light& LightManager::create_light()
{
	lights.push_back({});
	return lights.back();
}

void LightManager::assign_lightdata(UniformBufferObject& ubo)
{
	int i = 0;
	for (const auto& light : lights) {
		ubo.lights[i++] = {
			light.transform.matrix(),
			light.color
		};
	}
	ubo.num_lights = i;
}