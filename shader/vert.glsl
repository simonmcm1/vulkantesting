#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 M;
    mat4 V;
    mat4 P;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 vertexColor;

void main() {
    mat4 MVP = ubo.P * ubo.V * ubo.M;
    gl_Position = MVP * vec4(inPosition, 0.0, 1.0);
    vertexColor = inColor;
}