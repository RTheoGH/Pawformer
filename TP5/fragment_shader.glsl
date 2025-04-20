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

float DistributionGGX(vec3 N, vec3 H, float roughness); // à def plus tard

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

                vec3 Lo = vec3(0.0);

                for(int i = 0; i<nbLights; i++){
                        vec3 L = normalize(lightsPos[i] - fragPos);
                        vec3 H = normalize(V + L);

                        float distance = length(lightsPos[i] - fragPos);
                        float attenuation = 1.0 / (distance * distance);
                        vec3 radiance     = vec3(1.0) * attenuation;

                        float NDF = DistributionBlinn(N, H, roughness);        
                        float G   = G2Beckmann(N, L, V, roughness);      
                        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

                        vec3 numerator    = NDF * G * F;
                        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0)  + 0.0001;
                        vec3 specular     = numerator / denominator;

                        vec3 kS = F;
                        vec3 kD = vec3(1.0) - kS;
                        kD *= 1.0 - metallic;

                        float NdotL = max(dot(N, L), 0.0);                
                        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
                }
                
                vec3 ambient = vec3(0.03) * albedo * ao;
                color_temp = ambient + Lo;
                color_temp = color_temp / (color_temp + vec3(1.0));
                color_temp = pow(color_temp, vec3(1.0/2.2));
        }

        color = color_temp;
        
        // DEBUG
        // color = objColor;
        // color =vec3(0.4,0.2,0.2);
        // color =vec3(UV,1.0);
}