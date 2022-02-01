#version 450

layout(set = 1, binding = 0) uniform sampler2D mainTex;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec4 vertexColor;
layout(location = 1) in vec2 uv;

void main() {
//    outColor = vec4(vertexColor.xyz, 1.0);
//    outColor = vec4(uv, 0.0, 1.0);
	outColor = texture(mainTex, uv);
}