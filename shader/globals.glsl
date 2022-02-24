struct LightData {
	mat4 M;
	vec4 color;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 V;
    mat4 P;
	mat4 M[256];
	vec3 camera_position;
	LightData lights[16];
	uint num_lights;
	
} globals;