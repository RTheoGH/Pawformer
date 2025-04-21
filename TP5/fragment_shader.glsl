#version 330 core

in vec2 UV;
in vec3 normal;
in vec3 fragPos;
// Ouput data
out vec3 color;

uniform sampler2D texture1;
uniform sampler2D roughnessMap;
uniform sampler2D metalnessMap;
uniform sampler2D aoMap;
uniform vec3 objColor;
uniform int isColor;
uniform int isPBR;
uniform vec3 camPos;
uniform int nbLights;
uniform vec3 lightsPos[10];
const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
} 

float DistributionBlinn(vec3 N, vec3 H, float roughness){
        return 1.0/(PI * roughness * roughness) * pow(dot(H, N), (2.0/(roughness*roughness) - 2));
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


float G1Beckmann(float NdotV, float roughness){
        float c = NdotV / (roughness * sqrt(1 - NdotV * NdotV));
        if(c<1.6) return 1.0;
        else return (3.535*c + 2.181*c*c) / (1 + 2.276*c + 2.577*c*c);
}

float G2Beckmann(vec3 N, vec3 L, vec3 V, float roughness){
        return G1Beckmann(dot(N, L), roughness) * G1Beckmann(dot(N, V), roughness);
}

void main(){
        vec3 color_temp;
        if(isColor == 1){
                color_temp = objColor;
        }else{
                color_temp = texture(texture1,UV).rgb;
        }

        if(isPBR == 1){
                vec3 N = normalize(normal);
                vec3 V = normalize(camPos - fragPos);

                vec3 albedo     = texture(texture1, UV).rgb;
                float metallic  = texture(metalnessMap, UV).r;
                float roughness = clamp(texture(roughnessMap, UV).r, 0.05, 1.0);
                float ao        = texture(aoMap, UV).r;

                vec3 F0 = vec3(0.04); 
                F0 = mix(F0, albedo, metallic);
                                
                // reflectance equation
                vec3 Lo = vec3(0.0);
                for(int i = 0; i < nbLights; ++i) 
                {
                        // calculate per-light radiance
                        vec3 L = normalize(lightsPos[i] - fragPos);
                        vec3 H = normalize(V + L);
                        float distance    = length(lightsPos[i] - fragPos);
                        float attenuation = 1.0 / (distance * distance);
                        vec3 radiance     = vec3(1.0) * attenuation;        
                        
                        // cook-torrance brdf
                        float NDF = DistributionGGX(N, H, roughness);        
                        float G   = GeometrySmith(N, V, L, roughness);      
                        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
                        
                        vec3 kS = F;
                        vec3 kD = vec3(1.0) - kS;
                        kD *= 1.0 - metallic;	  
                        
                        vec3 numerator    = NDF * G * F;
                        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
                        vec3 specular     = numerator / denominator;  
                        
                        // add to outgoing radiance Lo
                        float NdotL = max(dot(N, L), 0.0);                
                        Lo += (kD * albedo / PI + specular) * NdotL; 
                }   
                
                vec3 ambient = vec3(0.03) * albedo * ao;
                vec3 color = ambient + Lo;
                        
                color_temp = color_temp / (color_temp + vec3(1.0));
                color_temp = pow(color, vec3(1.0/2.2));  
                
        }

        color = color_temp;
        
        // DEBUG
        // color = objColor;
        // color =vec3(0.4,0.2,0.2);
        // color =vec3(UV,1.0);
}