#version 450
#extension GL_GOOGLE_include_directive : enable

#include "global_vert.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec4 vertexColor;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 world_position;
layout(location = 4) out mat3 TBN;

void main() {
    mat4 MVP = get_mvp();
	mat4 M = get_m();
	
    gl_Position = MVP * vec4(inPosition, 1.0);
	world_position = vec3(M * vec4(inPosition, 1.0));
    vertexColor = inColor;
	uv = inUv;
	
	normal = mat3(transpose(inverse(M))) * inNormal;

	TBN = get_tbn(inNormal, inTangent);
}