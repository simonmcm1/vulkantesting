
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 V;
    mat4 P;
	mat4 M[16];
} globals;

layout( push_constant ) uniform constants
{
	uint object_index;
} PushConstants;

mat4 get_mvp() {
	return globals.P * globals.V * globals.M[PushConstants.object_index];
}

mat4 get_m() {
	return globals.M[PushConstants.object_index];
}

mat3 get_tbn(vec3 norm, vec3 tan) {
	mat4 M = get_m();
	vec3 T = normalize(vec3(M * vec4(tan, 0.0)));
	vec3 N = normalize(vec3(M * vec4(norm, 0.0)));
	vec3 B = cross(N, T);
	return mat3(T, B, N);
}