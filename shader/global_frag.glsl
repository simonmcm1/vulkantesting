#version 450

struct LightData {
	mat4 M;
	vec4 color;
}

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 V;
    mat4 P;
	mat4 M[16];
	LightData lights[16];
	uint num_lights;
	
} globals;

layout( push_constant ) uniform constants
{
	uint object_index;
} PushConstants;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUv;

layout(location = 0) out vec4 vertexColor;
layout(location = 1) out vec2 uv;

void main() {
    mat4 MVP = globals.P * globals.V * globals.M[PushConstants.object_index];
    gl_Position = MVP * vec4(inPosition, 1.0);
    vertexColor = inColor;
	uv = inUv;
}