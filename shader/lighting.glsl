#define PI 3.1415926538

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

struct LightingData {
	vec3 world_pos;
	
	float roughness;
	float metallic;
	vec3 albedo;
	vec3 normal;
};

vec3 lighting_direct(LightingData data) {
	vec3 camera_pos = globals.camera_position;

	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, data.albedo, data.metallic);
	vec3 V = normalize(camera_pos - data.world_pos);
	vec3 N = normalize(data.normal);
	
	vec3 Lo = vec3(0);
	//per-light from here
	for (int i = 0; i < globals.num_lights; i++) {
		vec3 light_color = globals.lights[i].color.xyz;
		vec3 light_pos = vec3(globals.lights[i].M * vec4(0,0,0,1.0));
	
		vec3 L = normalize(light_pos - data.world_pos);
		vec3 H = normalize(V + L);
		
		
		float distance    = length(light_pos - data.world_pos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance     = light_color * attenuation;        
		
		// cook-torrance brdf
		float NDF = DistributionGGX(N, H, data.roughness);        
		float G   = GeometrySmith(N, V, L, data.roughness);      
		vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
		
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - data.metallic;	  
		
		vec3 numerator    = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular     = numerator / denominator;  
			
		// add to outgoing radiance Lo
		float NdotL = max(dot(N, L), 0.0);                
		Lo += (kD * data.albedo / PI + specular) * radiance * NdotL; 
	}
	
	return Lo;
		
}