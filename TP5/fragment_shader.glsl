#version 330 core

in vec2 UV;
// Ouput data
out vec3 color;

uniform sampler2D texture1;
uniform vec3 objColor;

void main(){

        color = texture(texture1,UV).rgb;
        
        // DEBUG
        // color = objColor;
        // color =vec3(0.4,0.2,0.2);
        // color =vec3(UV,1.0);
}