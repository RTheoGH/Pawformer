#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 3) in vec3 tangent;

uniform mat4 MVP;
uniform mat4 model;

// Values that stay constant for the whole mesh.
out vec2 UV;
out vec3 normal;
// out vec3 localPos;
out vec3 fragPos;
out mat3 TBN;
uniform sampler2D heightmap;
uniform sampler2D normalMap;
uniform int isTerrain;

void main(){
        normal = texture(normalMap,vertexUV).rgb;
        vec3 pos = vertices_position_modelspace;
        // localPos = pos;

        // if(isTerrain == 1){
        //         float height = texture(heightmap,vertexUV).r;
        //         pos.y += height;
        // }

        vec3 T = normalize(mat3(model) * tangent);   // tangent du mesh
        vec3 N = normalize(mat3(model) * normal);    // normale du mesh
        vec3 B = normalize(cross(N, T));             // bitangent, recalculé si besoin

        // gl_Position = MVP * vec4(relief,1);
        gl_Position = MVP * vec4(pos,1);
        fragPos = gl_Position.xyz;
        normal = (MVP * vec4(normal, 1)).xyz;
        UV = vertexUV;
        TBN = mat3(T, B, N);
}

