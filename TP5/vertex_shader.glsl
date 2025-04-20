#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 vertexUV;

uniform mat4 MVP;

// Values that stay constant for the whole mesh.
out vec2 UV;
out vec3 normal;
out vec3 fragPos;
uniform sampler2D heightmap;
uniform sampler2D normalMap;
uniform int isTerrain;


void main(){
        normal = texture(normalMap,vertexUV).rgb;
        vec3 pos = vertices_position_modelspace;

        // if(isTerrain == 1){
        //         float height = texture(heightmap,vertexUV).r;
        //         pos.y += height;
        // }

        // gl_Position = MVP * vec4(relief,1);
        gl_Position = MVP * vec4(pos,1);
        fragPos = gl_Position.xyz;
        normal = (MVP * vec4(normal, 1)).xyz;
        UV = vertexUV;

}

