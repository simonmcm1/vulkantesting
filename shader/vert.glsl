#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 V;
    mat4 P;
	mat4 M[16];
} ubo;

layout( push_constant ) uniform constants
{
	uint object_index;
} PushConstants;


layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUv;

layout(location = 0) out vec4 vertexColor;
layout(location = 1) out vec2 uv;

void main() {
    mat4 MVP = ubo.P * ubo.V * ubo.M[PushConstants.object_index];
    gl_Position = MVP * vec4(inPosition, 0.0, 1.0);
    vertexColor = inColor;
	uv = inUv;
}