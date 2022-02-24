#version 450

#include "globals.glsl"
#include "lighting.glsl"

layout(set = 1, binding = 0) uniform MaterialUniformBufferObject {
    vec4 color;
} ubo;
layout(set = 1, binding = 1) uniform sampler2D albedoTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 1, binding = 3) uniform sampler2D prbMapTex;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec4 vertexColor;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 vertex_normal;
layout(location = 3) in vec3 world_position;
layout(location = 4) in mat3 TBN;

void main() {
	float ambientStrength = 0.002;
	vec3 ambient_color = vec3(1,1,1);
	vec3 albedo = ubo.color.xyz;
	vec3 normal = normalize(vertex_normal);

	//vec4 pbr = texture(prbMapTex, uv);
	float roughness = 0.5;

	LightingData light_data;
	light_data.world_pos = world_position;
	light_data.roughness = roughness;
	light_data.metallic = 0.5;
	light_data.albedo = albedo;
	light_data.normal = normal;
	

	/*vec3 lightDir = normalize(lightPos - world_position);  
	float diff = max(dot(normal, lightDir), 0.0);
	
	vec3 viewDir = normalize(camera_pos - world_position);
	vec3 reflectDir = reflect(-lightDir, normal); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
	vec3 specular = 0.5 * spec * lightColor;  	
	vec3 diffuse = diff * lightColor;
*/
	
	vec3 ambient = ambientStrength * ambient_color;
	vec3 direct = lighting_direct(light_data);
	outColor = vec4(albedo * ambient + direct, 1.0);
//	outColor = vec4(albedo * (ambient + diffuse + specular), 1.0);
	//outColor = vec4(diffuse, 1.0);
//	outColor = vec4(vertex_normal, 1.0);
	//outColor = vec4(albedo, 1.0);
}