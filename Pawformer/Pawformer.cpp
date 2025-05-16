// Include standard headers
#include <glm/trigonometric.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <iostream>
#include <numeric>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define MINIAUDIO_IMPLEMENTATION
#include "../external/miniaudio/miniaudio.h"

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>



// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
glm::vec3 camera_position   = glm::vec3(-1.0f, 10.0f, 20.0f);
glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f,  0.0f);

// glm::vec3 orbital_camera_position = glm::vec3(0.0f, 10.0f, 10.0f);

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int sommets = 32;
const int MIN_SOMMETS = 4;
const int MAX_SOMMETS = 248;
bool scaleT = false;

// rotation
float angle = 0.;
float angle_perspective = 45.;
float zoom = 1.;
bool orbital = false;
float rotation_speed = 0.5f;

GLuint programID;
GLuint MatrixID;
GLuint ModelID;
glm::mat4 MVP;

bool debugFilaire = false;

bool cam_attache = true;
float yaw_ = -90.0f;
float pitch_ = 0.0f;
double _xCursorPos, _yCursorPos;
int nPreviousState = GLFW_RELEASE;
int fPreviousState = GLFW_RELEASE;
float cam_distance = 10.0f;

// float jump_height = 11.0f;
bool isJumping = false;
bool isFalling = false;
bool isGrabbing = false;

double gravite = 10.0f;
double masse_chat = 2.0f;
glm::vec3 acceleration = glm::vec3(0.0f,-gravite,0.0f);
glm::vec3 vitesse = glm::vec3(0.0f);

float rayon_chat = 0.5f;
double poids = masse_chat*gravite;

bool PBR_OnOff = false;
int p_preview_state = GLFW_RELEASE;

bool oiia = false;
int o_p_s = GLFW_RELEASE;

bool person = false;
int person_state = GLFW_RELEASE;

/*******************************************************************************/

void initImgui(GLFWwindow* window){
    std::cout << "[ImGui] Initialisation..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    std::cout << "[ImGui] Context créé." << std::endl;

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 3.f;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "[ImGui] Erreur Init GLFW backend!" << std::endl;
    } else {
        std::cout << "[ImGui] GLFW backend OK." << std::endl;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "[ImGui] Erreur Init OpenGL3 backend!" << std::endl;
    } else {
        std::cout << "[ImGui] OpenGL3 backend OK." << std::endl;
    }
}

ImGuiWindowFlags flags = ImGuiWindowFlags_NoInputs 
                       | ImGuiWindowFlags_NoTitleBar 
                       | ImGuiWindowFlags_NoResize 
                       | ImGuiWindowFlags_NoMove 
                       | ImGuiWindowFlags_NoScrollbar 
                       | ImGuiWindowFlags_NoCollapse 
                       | ImGuiWindowFlags_NoFocusOnAppearing;

// Chargeur de texture
GLuint loadTexture(const char* filename) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data) {
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Paramètres de texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cerr << "Failed to load texture: " << filename << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

void calculateUVSphere(std::vector<glm::vec3>& vertices,std::vector<glm::vec2>& uvs){
    uvs.clear();
    for(const auto& vertex : vertices){
        float theta = atan2(vertex.z, vertex.x);
        float phi = acos(vertex.y / glm::length(vertex));

        float u = (theta + M_PI) / (2.0f * M_PI);
        float v = phi / M_PI;

        uvs.push_back(glm::vec2(u, v));
    }
}

void cylindre(std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &uvs, std::vector<unsigned short> &indices, float hauteur = 30.0f, float rayon = 0.5f, int tranches = 32, int segments = 1){
    // Générer les vertices et UVs
    for(int i = 0; i <= segments; ++i){
        float y = ((float)i / segments) * hauteur;

        for(int j = 0; j <= tranches; ++j){
            float theta = ((float)j / tranches) * 2.0f * glm::pi<float>();
            float x = cos(theta);
            float z = sin(theta);

            vertices.emplace_back(glm::vec3(x, y, z));

            // float u = (float)j / (float)tranches;
            // float v = (float)i / (float)segments;

            float u = (float)j*hauteur / (float)tranches/hauteur;
            float v = (float)i*hauteur / (float)segments;
            uvs.emplace_back(glm::vec2(u, v));
        }
    }

    // Générer les indices
    for(int i = 0; i < segments; ++i){
        for(int j = 0; j < tranches; ++j){
            int current = i * (tranches + 1) + j;
            int next = current + tranches + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
}

void plateforme(std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &uvs, std::vector<unsigned short> &indices,
                float hauteur = 0.5f, float rayon = 5.f, int tranches = 32, int segments = 1){

    int baseIndex = vertices.size();

    // Génération du cylindre (latéral)
    for(int i = 0; i <= segments; ++i){
        float y = ((float)i / segments) * hauteur;

        for(int j = 0; j <= tranches; ++j){
            float theta = ((float)j / tranches) * 2.0f * glm::pi<float>();
            float x = cos(theta) * rayon;
            float z = sin(theta) * rayon;

            vertices.emplace_back(glm::vec3(x, y, z));

            float u = (float)j / (float)tranches;
            float v = (float)i / (float)segments;
            uvs.emplace_back(glm::vec2(u, v));
        }
    }

    // Indices du cylindre latéral
    for(int i = 0; i < segments; ++i){
        for(int j = 0; j < tranches; ++j){
            int current = baseIndex + i * (tranches + 1) + j;
            int next = current + tranches + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    // ===== Bouchon du bas =====
    int centerBasIndex = vertices.size();
    vertices.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f));
    uvs.emplace_back(glm::vec2(0.5f, 0.5f));

    for(int j = 0; j < tranches; ++j){
        int curr = baseIndex + j;
        int next = baseIndex + (j + 1) % (tranches + 1);
        indices.push_back(centerBasIndex);
        indices.push_back(next);
        indices.push_back(curr);
    }

    // ===== Bouchon du haut =====
    int centerHautIndex = vertices.size();
    vertices.emplace_back(glm::vec3(0.0f, hauteur, 0.0f));
    uvs.emplace_back(glm::vec2(0.5f, 0.5f));

    int ringStart = baseIndex + segments * (tranches + 1);
    for(int j = 0; j < tranches; ++j){
        int curr = ringStart + j;
        int next = ringStart + (j + 1) % (tranches + 1);
        indices.push_back(centerHautIndex);
        indices.push_back(curr);
        indices.push_back(next);
    }
}

void computeTangent(const std::vector<glm::vec3>& vertices,
                    const std::vector<glm::vec2>& uvs,
                    std::vector<glm::vec3>& tangents,
                    int i0, int i1, int i2)
{
    const glm::vec3 &v0 = vertices[i0];
    const glm::vec3 &v1 = vertices[i1];
    const glm::vec3 &v2 = vertices[i2];

    const glm::vec2 &uv0 = uvs[i0];
    const glm::vec2 &uv1 = uvs[i1];
    const glm::vec2 &uv2 = uvs[i2];

    glm::vec3 deltaPos1 = v1 - v0;
    glm::vec3 deltaPos2 = v2 - v0;

    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;

    float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;

    tangents[i0] += tangent;
    tangents[i1] += tangent;
    tangents[i2] += tangent;
}

void plan(std::vector<glm::vec3> &vertices,
          std::vector<glm::vec2> &uvs,
          std::vector<unsigned short> &indices,
          std::vector<glm::vec3> &normals,
          std::vector<glm::vec3> &tangents)
{
    float taille = 10.0f;
    float m = taille / 2.0f;
    float pas = taille / (float)sommets;

    int numVerts = (sommets + 1) * (sommets + 1);

    vertices.clear();
    normals.clear();
    uvs.clear();
    indices.clear();
    tangents.clear();

    vertices.reserve(numVerts);
    normals.reserve(numVerts);
    uvs.reserve(numVerts);
    tangents.resize(numVerts, glm::vec3(0.0f));

    for(int i = 0; i <= sommets; i++){
        for(int j = 0; j <= sommets; j++){
            float x = -m + j * pas;
            float z = -m + i * pas;

            vertices.emplace_back(glm::vec3(x, 0.0f, z));
            normals.emplace_back(glm::vec3(0.0f, 1.0f, 0.0f));

            float u = (float)j / (float)(sommets - 1);
            float v = (float)i / (float)(sommets - 1);

            uvs.emplace_back(glm::vec2(u, v));
        }
    }

    for(int i = 0; i < sommets - 1; i++){
        for(int j = 0; j < sommets - 1; j++){
            int topleft = i * (sommets + 1) + j;
            int topright = topleft + 1;
            int bottomleft = (i + 1) * (sommets + 1) + j;
            int bottomright = bottomleft + 1;

            // Triangle 1
            indices.push_back(topleft);
            indices.push_back(bottomleft);
            indices.push_back(topright);

            computeTangent(vertices, uvs, tangents, topleft, bottomleft, topright);

            // Triangle 2
            indices.push_back(topright);
            indices.push_back(bottomleft);
            indices.push_back(bottomright);

            computeTangent(vertices, uvs, tangents, topright, bottomleft, bottomright);
        }
    }

    // Optionnel : normaliser les tangentes finales
    for (auto &t : tangents) {
        t = glm::normalize(t);
    }
}

void cube(std::vector<glm::vec3> &vertices,std::vector<glm::vec2> &uvs, std::vector<unsigned short> &indices) {
    vertices = {
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f}, 
        { 0.5f,  0.5f,  0.5f}, 
        {-0.5f,  0.5f,  0.5f}
    };

    uvs = {
        {0.0f,0.0f},
        {1.0f,0.0f},
        {1.0f,1.0f},
        {0.0f,1.0f},
        {0.0f,0.0f},
        {1.0f,0.0f},
        {1.0f,1.0f},
        {0.0f,1.0f}
    };

    indices = {
        0, 1, 2,  2, 3, 0,
        1, 5, 6,  6, 2, 1,
        5, 4, 7,  7, 6, 5,
        4, 0, 3,  3, 7, 4, 
        3, 2, 6,  6, 7, 3,
        4, 5, 1,  1, 0, 4  
    };
}

void mur(std::vector<glm::vec3> &vertices,
         std::vector<glm::vec2> &uvs,
         std::vector<unsigned short> &indices,
         std::vector<glm::vec3> &normals,
         std::vector<glm::vec3> &tangents)
{
    float taille = 10.0f;
    float m = taille / 2.0f;
    float pas = taille / (float)sommets;

    int numVerts = (sommets + 1) * (sommets + 1);

    vertices.clear();
    normals.clear();
    uvs.clear();
    indices.clear();
    tangents.clear();

    vertices.reserve(numVerts);
    normals.reserve(numVerts);
    uvs.reserve(numVerts);
    tangents.resize(numVerts, glm::vec3(0.0f));

    for(int i = 0; i <= sommets; i++){
        for(int j = 0; j <= sommets; j++){
            float x = -m + j * pas;
            float y = i * pas;

            vertices.emplace_back(glm::vec3(x, y, 0.0f));
            normals.emplace_back(glm::vec3(0.0f, 0.0f, -1.0f)); // face vers -Z

            float u = (float)j / (float)(sommets - 1);
            float v = (float)i / (float)(sommets - 1);
            uvs.emplace_back(glm::vec2(u, v));
        }
    }

    for(int i = 0; i < sommets - 1; i++){
        for(int j = 0; j < sommets - 1; j++){
            int topleft = i * (sommets + 1) + j;
            int topright = topleft + 1;
            int bottomleft = (i + 1) * (sommets + 1) + j;
            int bottomright = bottomleft + 1;

            // Triangle 1
            indices.push_back(topleft);
            indices.push_back(bottomleft);
            indices.push_back(topright);
            computeTangent(vertices, uvs, tangents, topleft, bottomleft, topright);

            // Triangle 2
            indices.push_back(topright);
            indices.push_back(bottomleft);
            indices.push_back(bottomright);
            computeTangent(vertices, uvs, tangents, topright, bottomleft, bottomright);
        }
    }

    // Normalisation finale
    for (auto &t : tangents) {
        t = glm::normalize(t);
    }
}


float getTerrainHeight(float x, float z, const std::vector<glm::vec3>& vertices, const std::vector<unsigned short>& indices, const unsigned char* heightmapData, int heightmapWidth, int heightmapHeight) {
    float taille = 10.0f;
    float m = taille / 2.0f;
    float pas = taille / (float)sommets;

    int i = (z + m) / pas;
    int j = (x + m) / pas;

    int topleft = i * (sommets + 1) + j;
    int topright = topleft + 1;
    int bottomleft = (i + 1) * (sommets + 1) + j;
    int bottomright = bottomleft + 1;

    glm::vec3 v0 = vertices[topleft];
    glm::vec3 v1, v2;

    if ((x - v0.x) + (z - v0.z) < (bottomright - topleft)) {
        v1 = vertices[bottomleft];
        v2 = vertices[topright];
    } else {
        v1 = vertices[bottomright];
        v2 = vertices[topright];
    }

    // Barycentriques
    glm::vec3 v0v1 = v1 - v0;
    glm::vec3 v0v2 = v2 - v0;
    glm::vec3 p = glm::vec3(x, 0.0f, z);
    glm::vec3 v0p = p - v0;

    float d00 = glm::dot(v0v1,v0v1);
    float d01 = glm::dot(v0v1,v0v2);
    float d11 = glm::dot(v0v2,v0v2);
    float d20 = glm::dot(v0p,v0v1);
    float d21 = glm::dot(v0p,v0v2);

    float denom = d00*d11 - d01*d01;
    float v = (d11*d20 - d01*d21) / denom;
    float w = (d00*d21 - d01*d20) / denom;
    float u = 1.0f - v - w;

    // Interpolation de la hauteur avec la heightmap
    float height0 = heightmapData[((int)(v0.z + m) * heightmapWidth + (int)(v0.x + m))] / 255.0f;
    float height1 = heightmapData[((int)(v1.z + m) * heightmapWidth + (int)(v1.x + m))] / 255.0f;
    float height2 = heightmapData[((int)(v2.z + m) * heightmapWidth + (int)(v2.x + m))] / 255.0f;

    return u*(v0.y + height0*1.75) + v*(v1.y + height1*1.75) + w *(v2.y + height2*1.75);
}

/*******************************************************************************/

class Transform{
public:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Transform() : position(0.0f),rotation(0.0f),scale(1.0f){}
    glm::mat4 getMatrice() const {
        glm::mat4 mat = glm::mat4(1.0f);
        mat = glm::translate(mat, position);
        mat = glm::rotate(mat, rotation.x, glm::vec3(1, 0, 0));
        mat = glm::rotate(mat, rotation.y, glm::vec3(0, 1, 0));
        mat = glm::rotate(mat, rotation.z, glm::vec3(0, 0, 1));
        mat = glm::scale(mat, scale);
        return mat;
    }
};

class Light{
public:
    glm::vec3 position;
    float radius;
    float intensity;
    Light(glm::vec3 _position){
        this->position = _position;
        this->radius = 1.0;
        this->intensity = 1.0;
    }
};

void computeTangentsFromMesh(
    const std::vector<glm::vec3>& vertices,
    const std::vector<glm::vec2>& uvs,
    const std::vector<unsigned short>& indices,
    std::vector<glm::vec3> &tangents)
{
    tangents.resize(vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        const glm::vec3& v0 = vertices[i0];
        const glm::vec3& v1 = vertices[i1];
        const glm::vec3& v2 = vertices[i2];

        const glm::vec2& uv0 = uvs[i0];
        const glm::vec2& uv1 = uvs[i1];
        const glm::vec2& uv2 = uvs[i2];

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent = glm::normalize(tangent);

        tangents[i0] += tangent;
        tangents[i1] += tangent;
        tangents[i2] += tangent;

    }

    // Normalisation finale
    for (glm::vec3& t : tangents) {
        t = glm::normalize(t);
    }
}

class SNode : public std::enable_shared_from_this<SNode> {
public:
    Transform transform;
    std::vector<std::shared_ptr<SNode>> feuilles;
    std::shared_ptr<SNode> parent = nullptr;
    GLuint vao = 0, vbo = 0, ibo = 0, tangentVBO = 0, textureID = 0, normalsID = 0, roughnessID = 0, metalnessID = 0, aoID = 0;
    GLuint uvVBO = 0;
    size_t indexCPT = 0;
    glm::vec3 color;
    float rayon = 0.0f;
    float hauteur = 0.0f;
    int type_objet = 0;
    int isPBR = 0;
    bool grabable = false;

    const char* low;
    const char* high;
    const char* current_modele;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned short> indices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<std::vector<unsigned short>> triangles;

    glm::mat4 lastModelMatrix = glm::mat4(1.0f);

    // Constructeurs
    SNode(int obj,const char* texturePath){
        type_objet = obj;
        buffers();
        textureID = loadTexture(texturePath);
    }

    // Pour PBR
    SNode(int obj,const char* texturePathAlbedo,
        const char* texturePathNormals,
        const char* texturePathRoughness, 
        const char* texturePathMetalness,
        const char* texturePathAO
    ){
        type_objet = obj;
        buffers();
        isPBR = 1;
        textureID = loadTexture(texturePathAlbedo);
        normalsID = loadTexture(texturePathNormals);
        roughnessID = loadTexture(texturePathRoughness);
        metalnessID = loadTexture(texturePathMetalness);
        aoID = loadTexture(texturePathAO);
    }

    SNode(int obj,const char* texturePath,std::vector<const char*> modelesPath){
        type_objet = obj;
        low = modelesPath[1];
        high = modelesPath[0];
        current_modele = low;
        loadOFF(current_modele,vertices,indices,triangles);
        buffers();
        textureID = loadTexture(texturePath);
    }

    SNode(int obj,glm::vec3 nodeColor){
        type_objet = obj;
        color = nodeColor;
        buffers();
    }

    SNode(){}

    void buffers(){
        switch(type_objet){
            case 1:
                plan(vertices,uvs,indices, normals, tangents); // Plan
                break;
            case 2:
                calculateUVSphere(vertices,uvs);
                rayon = 1.0f;
                break;
            case 3:
                cube(vertices,uvs,indices);
                break;
            case 4:
                // loadOFF("modeles/kitten.off",vertices,indices,triangles);
                // loadOBJ("modeles/pitier.obj",vertices,uvs,normals);
                loadOBJ("modeles/chat.obj",vertices,uvs,normals);
                break;
            case 5:
                cylindre(vertices,uvs,indices);
                hauteur = 30.0f;
                rayon = 0.5f;
                break;
            case 6:
                mur(vertices,uvs,indices, normals, tangents);
                break;
            case 7:
                plateforme(vertices, uvs, indices);
                hauteur = 0.5f;
                rayon = 5.0f;
                break;
            case 8:
                loadOBJ("modeles/chat.obj",vertices,uvs,normals);
                break;
            default:
                loadOFF("modeles/sphere2.off",vertices,indices,triangles);
                calculateUVSphere(vertices,uvs);
                computeTangentsFromMesh(vertices, uvs, indices, tangents);
                rayon = 1.0f;
                break;
        }

        glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);

        glGenBuffers(1,&vbo);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(glm::vec3),&vertices[0],GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
        glEnableVertexAttribArray(0);

        glGenBuffers(1,&uvVBO);
        glBindBuffer(GL_ARRAY_BUFFER,uvVBO);
        glBufferData(GL_ARRAY_BUFFER,uvs.size()*sizeof(glm::vec2),&uvs[0],GL_STATIC_DRAW);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(glm::vec2),(void*)0);
        glEnableVertexAttribArray(1);

        if (hasNormals()) {
            GLuint normalVBO;
            glGenBuffers(1, &normalVBO);
            glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glEnableVertexAttribArray(2);
    
            // Si l'OBJ n’a pas d’indices, on les génère automatiquement
            if (!hasIndices()) {
                indices.resize(vertices.size());
                std::iota(indices.begin(), indices.end(), 0); // [0, 1, 2, ...]
            }

            //Si l'OBJ n'a pas de triangles
            if (triangles.empty()) {
                triangles.clear();
                for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                    triangles.push_back({
                        static_cast<unsigned short>(indices[i]),
                        static_cast<unsigned short>(indices[i + 1]),
                        static_cast<unsigned short>(indices[i + 2])
                    });
                }
            }

        }
    
        if (hasIndices()) {
            glGenBuffers(1, &ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
        }

        if (hasTangeants()) {
            glGenBuffers(1, &tangentVBO);
            glBindBuffer(GL_ARRAY_BUFFER, tangentVBO);
            glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(glm::vec3), tangents.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        } 

        indexCPT = indices.size();
        glBindVertexArray(0);
    }

    void addFeuille(std::shared_ptr<SNode> feuille){
        feuilles.push_back(feuille);
        feuille->parent = shared_from_this();
    }

    std::vector<std::shared_ptr<SNode>> getFeuilles(){
        std::vector<std::shared_ptr<SNode>> liste_feuilles;
        for(const std::shared_ptr<SNode>& feuille: feuilles){
            liste_feuilles.push_back(feuille);
            for(const std::shared_ptr<SNode>& f: feuille->getFeuilles()){
                liste_feuilles.push_back(f);
            }
        }
        return liste_feuilles;
    }

    virtual void update(float deltaTime){
        if(type_objet == 2){ // Si notre objet possede un LOD
            float distance = glm::distance(camera_position,transform.position);

            // Evite de recharger le modele si celui-ci est déjà chargé
            const char* nouveau_modele = (distance > 5.0f) ? low : high;

            if(current_modele != nouveau_modele){
                current_modele = nouveau_modele;
                loadOFF(current_modele,vertices,indices,triangles);
                buffers();
                std::cout << "Changement de modele : " << current_modele << std::endl;
            }
        }
        
        for(auto& feuille : feuilles){
            feuille->update(deltaTime);
        }
    }

    virtual void draw(GLuint shaderProgram, const glm::mat4& brancheMatrix, const glm::vec3& branchePos){
        glm::mat4 ModelMatrix = brancheMatrix * transform.getMatrice();
        
        glUseProgram(shaderProgram);

        glm::mat4 ViewMatrix = glm::lookAt(
            camera_position,
            camera_position + camera_target,
            camera_up
        );
        glm::mat4 ProjectionMatrix = glm::perspective(
            glm::radians(angle_perspective),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,
            200.0f
        );

        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(ModelID,1,GL_FALSE,&ModelMatrix[0][0]);
        glUniformMatrix4fv(MatrixID,1,GL_FALSE,&MVP[0][0]);

        GLuint isColorID = glGetUniformLocation(shaderProgram,"isColor");
        glUniform1i(isColorID,(type_objet == 4) ? 0 : 0);

        GLuint colorLocation = glGetUniformLocation(shaderProgram,"objColor");
        // std::cout << "couleur :" << color[0] << std::endl;
        glUniform3fv(colorLocation,1,&color[0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        GLuint textureLocation = glGetUniformLocation(shaderProgram,"texture1");
        if(textureLocation == -1){
            std::cerr << "Erreur : Uniform texture1 introuvable" << std::endl;
        }
        glUniform1i(textureLocation,0);

        GLuint isPBRID = glGetUniformLocation(shaderProgram,"isPBR");
        // std::cout<<"isPBR : "<<isPBR<<std::endl;
        glUniform1i(isPBRID, isPBR);

        if(isPBR == 1){
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, normalsID);
            GLuint normalsLocation = glGetUniformLocation(shaderProgram,"normalMap");
            if(normalsLocation == -1){
                std::cerr << "Erreur : Uniform normalMap introuvable" << std::endl;
            }
            glUniform1i(normalsLocation,1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, roughnessID);
            GLuint roughnessLocation = glGetUniformLocation(shaderProgram,"roughnessMap");
            if(roughnessLocation == -1){
                std::cerr << "Erreur : Uniform roughnessMap introuvable" << std::endl;
            }
            glUniform1i(roughnessLocation,2);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, metalnessID);
            GLuint metalnessLocation = glGetUniformLocation(shaderProgram,"metalnessMap");
            if(metalnessLocation == -1){
                std::cerr << "Erreur : Uniform metalnessMap introuvable" << std::endl;
            }
            glUniform1i(metalnessLocation,3);

            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, aoID);
            GLuint aoLocation = glGetUniformLocation(shaderProgram,"aoMap");
            if(aoLocation == -1){
                std::cerr << "Erreur : Uniform aoMap introuvable" << std::endl;
            }
            glUniform1i(aoLocation,4);
        }

        glBindVertexArray(vao);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo);
        
        if (!(type_objet == 4 && person)) {
            glDrawElements(GL_TRIANGLES,
                indexCPT,
                GL_UNSIGNED_SHORT,
                (void*)0
            );
        }
        

        glBindVertexArray(0);
        glDisableVertexAttribArray(0);

        for(auto &feuille: feuilles){
            glm::vec3 worldPos = branchePos + transform.position;
            feuille->draw(shaderProgram,ModelMatrix,worldPos);
        }
    }

    bool hasNormals() const {
        return !normals.empty();
    }
    
    bool hasIndices() const {
        return !indices.empty();
    }

    bool hasTangeants() const {
        std::cout<<!tangents.empty()<<std::endl;
        return !tangents.empty();
    }

    glm::vec3 getScale(){
        if(parent == nullptr) return transform.scale;
        else return parent->getScale()*transform.scale;
    }

    glm::mat4 getModelMatrix(){
        if(parent == nullptr) return transform.getMatrice();
        else return parent->getModelMatrix()*transform.getMatrice();
    }
}; 

class Scene{
public:
    std::shared_ptr<SNode> racine;
    std::vector<Light> lights;

    Scene(){racine = std::make_shared<SNode>();}

    void update(float deltaTime){
        racine->update(deltaTime);
    }

    void draw(GLuint shaderProgram){
        racine->draw(shaderProgram,glm::mat4(1.0f),glm::vec3(0.0f));
    }

    std::vector<glm::vec3> get_lights_pos(){
        std::vector<glm::vec3> lp;
        for(int i = 0; i<lights.size(); i++){
            lp.push_back(lights[i].position);
        }
        return lp;
    }

    void add_light(glm::vec3 light_position){

        if(lights.size() >= 10){
            std::cout<<"Trop de lights !"<<std::endl;
            return;
        }

        Light light(light_position);
        lights.push_back(light);
        std::vector<glm::vec3> l = get_lights_pos();

        GLuint nbLightsID = glGetUniformLocation(programID, "nbLights");
        glUniform1i(nbLightsID, lights.size());

        std::vector<GLfloat> flat_data;
        flat_data.reserve(l.size() * 3);
        for (const auto& vec : l) {
            flat_data.push_back(vec.x);
            flat_data.push_back(vec.y);
            flat_data.push_back(vec.z);
        }

        GLuint lightsPosID = glGetUniformLocation(programID, "lightsPos");
        glUniform3fv(lightsPosID, static_cast<GLsizei>(l.size()), flat_data.data());
    }
};

bool checkCollisionSpheres(glm::vec3 pos1, float radius1, glm::vec3 pos2, float radius2){
    float dist = glm::length(pos1-pos2);
    return dist < (radius1+radius2);
}

float gethauteur(std::shared_ptr<Scene> scene,std::shared_ptr<SNode> chat){
    float hauteurMax = -1.0f;
    std::shared_ptr<SNode> maxPlan;
    glm::vec3 position_chat = chat->transform.position;

    for(const std::shared_ptr<SNode>& plan: scene->racine->feuilles){
        if(plan->type_objet == 1 || plan->type_objet == 3){
            float planX = plan->transform.position.x;
            float planZ = plan->transform.position.z;
            float largeur;
            float longueur;
            if(plan->type_objet == 1){
                largeur = 5.0f * plan->transform.scale[0];
                longueur = 5.0f * plan->transform.scale[0];
            }
            // if(plan->type_objet == 3){
            //     largeur = plan->transform.scale[0];
            //     longueur = plan->transform.scale[0];
            // }

            if(position_chat.x >= planX - largeur && position_chat.x <= planX + largeur &&
                position_chat.z >= planZ - longueur && position_chat.z<= planZ + longueur
            ){

                if(plan->transform.position.y > hauteurMax && position_chat.y > plan->transform.position.y){
                    if(plan->type_objet == 1) hauteurMax = plan->transform.position.y - 0.15f;
                    // if(plan->type_objet == 3) hauteurMax = plan->transform.position.y + plan->transform.scale.y*rayon_chat-0.15f;
                }
            }
        }
        // if(plan->type_objet == 0 || plan->type_objet == 2){
        //     if(checkCollisionSpheres(position_chat, 1.0f, plan->transform.position, plan->transform.scale[0])){
        //         float cosChat = glm::dot(glm::vec3(0., 1., 0.), (position_chat - plan->transform.position));

        //     }
        // }
    }
    return hauteurMax;
}

void processGrabInput(std::shared_ptr<SNode> &chat, std::shared_ptr<SNode> grabable_object, float initialGrabAngle){
    if(grabable_object->type_objet == 5){
        static float angle = initialGrabAngle;

        // Matrices
        glm::mat4 model = grabable_object->getModelMatrix();

        // Position du chat dans le repère local du cylindre
        glm::vec3 worldPosChat = chat->transform.position;
        glm::vec3 localPosChat = glm::vec3(glm::inverse(model) * glm::vec4(worldPosChat, 1.0f));

        // Mouvement vertical
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
            localPosChat.y += 5.0f * deltaTime;
        }
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
            localPosChat.y -= 5.0f * deltaTime;
        }
        if(localPosChat.y > grabable_object->hauteur) localPosChat.y = grabable_object->hauteur;

        // Rotation autour du cylindre
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
            angle += 2.0f * deltaTime;
        }
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
            angle -= 2.0f * deltaTime;
        }

        // Nouvelle position locale autour du cylindre
        localPosChat.x = cos(angle) * grabable_object->rayon;
        localPosChat.z = sin(angle) * grabable_object->rayon;

        glm::vec3 center_local = glm::vec3(0.0f, localPosChat.y, 0.0f);
        glm::vec3 dir_local = glm::normalize(center_local - localPosChat);

        // Ajout du rayon du chat dans la direction radiale (non soumis au scale)
        localPosChat -= dir_local * rayon_chat;

        glm::vec3 newWorldPos = glm::vec3(model * glm::vec4(localPosChat, 1.0f));
        chat->transform.position = newWorldPos;

        glm::vec3 dir_world = glm::mat3(model) * dir_local;
        chat->transform.rotation.y = atan2(dir_world.x, dir_world.z);
    }
}

void processSphereCollision(glm::vec3 &pos_chat, std::shared_ptr<SNode> sphere, float &plan_hauteur) {
    if ((sphere->type_objet == 2 || sphere->type_objet == 0) &&
        checkCollisionSpheres(pos_chat, rayon_chat, sphere->transform.position, sphere->transform.scale[0])) {

        float rayon_sphere = sphere->transform.scale[0];
        glm::vec3 offset = pos_chat - sphere->transform.position;

        if (glm::length(offset) < 0.0001f)
            return;

        offset = glm::normalize(offset);
        float cosCollision = glm::dot(glm::vec3(0., 1., 0.), offset);

        // FOLLOW SPHERE ROTATION if on top
        if (cosCollision > 0.5f) {
            plan_hauteur = (sphere->transform.position + offset * rayon_sphere).y +rayon_chat;
            isFalling = false;
            pos_chat.y = plan_hauteur;

            // Suivi rotation sphère
            glm::mat4 currentModel = sphere->getModelMatrix();
            glm::mat4 delta = glm::inverse(sphere->lastModelMatrix) * currentModel;

            glm::vec4 localPos = glm::inverse(sphere->lastModelMatrix) * glm::vec4(pos_chat, 1.0f);
            pos_chat = glm::vec3(currentModel * localPos);

            sphere->lastModelMatrix = currentModel;
        }
        else {
            if (cosCollision < 0.0f) {
                isJumping = false;
            }
            glm::vec3 projectedOffset = (sphere->transform.position + offset * (rayon_sphere + rayon_chat));
            projectedOffset.y = pos_chat.y;
            pos_chat = projectedOffset;
        }
    }
}

bool checkCollisionCylindre(glm::vec3 &pos_chat, float rayon_chat, glm::vec3 pos_cylindre, float rayon_cylindre, float hauteur_cylindre){
    if(pos_chat.y+rayon_chat < pos_cylindre.y || pos_chat.y-rayon_chat > pos_cylindre.y+hauteur_cylindre) return false;
    if(glm::length(glm::vec3(pos_cylindre.x, pos_chat.y, pos_cylindre.z) - pos_chat) > rayon_chat+rayon_cylindre) return false;
    return true;
}

void processCylindreCollision(std::shared_ptr<SNode> &chat, std::shared_ptr<SNode> cylindre, float &plan_hauteur){
    if(cylindre->type_objet != 5 && cylindre->type_objet != 7) return;

    // Récupérer la matrice de transformation complète du cylindre
    glm::mat4 model = cylindre->getModelMatrix();
    glm::mat4 invModel = glm::inverse(model);

    // Passer la position du chat dans le repère local du cylindre
    glm::vec3 worldChatPos = chat->transform.position;
    glm::vec3 localChatPos = glm::vec3(invModel * glm::vec4(worldChatPos, 1.0f));

    // Extraire le scale global du cylindre (éventuellement non uniforme)
    glm::vec3 cylScale = glm::vec3(
        glm::length(glm::vec3(model[0])),  // X
        glm::length(glm::vec3(model[1])),  // Y
        glm::length(glm::vec3(model[2]))   // Z
    );

    float rayon_local = 0.5f * cylScale[0];
    float hauteur_cyl = cylindre->hauteur * cylScale.y;

    glm::vec3 adjustedLocalPos = localChatPos;
    adjustedLocalPos.y -= rayon_chat; // Décale vers les "pattes" du chat

    if (cylindre->type_objet == 5 && checkCollisionCylindre(adjustedLocalPos, rayon_chat, glm::vec3(0.0f), rayon_local, hauteur_cyl)) {

        float dist = glm::length(localChatPos - glm::vec3(0.0f, localChatPos.y, 0.0f));
        float overlap = rayon_chat + rayon_local - dist;

        if (overlap > 0.0f) {
            glm::vec3 dir_local = glm::normalize(localChatPos - glm::vec3(0.0f, localChatPos.y, 0.0f));
            glm::vec3 newLocalPos = localChatPos + dir_local * overlap;
            glm::vec3 newWorldPos = glm::vec3(model * glm::vec4(newLocalPos, 1.0));
            chat->transform.position = newWorldPos;
        }

        if (cylindre->grabable && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            isGrabbing = true;
            chat->transform.rotation = glm::vec3(glm::radians(90.0f), 0.0f, 0.0f);

            // Initialiser l'angle une seule fois
            glm::vec3 dir = glm::normalize(glm::vec3(localChatPos.x, 0.0f, localChatPos.z));
            float initialGrabAngle = atan2(dir.z, dir.x);  // ⚠️ atan2(y, x) → ici Z, X

            processGrabInput(chat, cylindre, initialGrabAngle);
        } else {
            chat->transform.rotation.x = 0.0f;
        }
    }

    if (cylindre->type_objet == 7 && checkCollisionCylindre(localChatPos, rayon_chat, glm::vec3(0.0f), cylindre->rayon, hauteur_cyl)) {
        if (glm::length(glm::vec3(0.0f, localChatPos.y, 0.0f) - localChatPos) <= rayon_chat + cylindre->rayon) {

            // Position dans le repère monde du sommet local touché
            glm::vec3 topWorld = glm::vec3(model * glm::vec4(localChatPos.x, hauteur_cyl, localChatPos.z, 1.0f));
            glm::vec3 bottomWorld = glm::vec3(model * glm::vec4(localChatPos.x, 0.0f, localChatPos.z, 1.0f));

            // Normale monde locale Y du cylindre
            glm::vec3 cylUp = glm::normalize(glm::vec3(model[1]));

            if (localChatPos.y >= hauteur_cyl - rayon_chat) {
                
                glm::vec3 contactPos = topWorld + cylUp * rayon_chat;
                plan_hauteur = contactPos.y - rayon_chat;
                chat->transform.position.y = contactPos.y;
                isFalling = false;
                glm::vec4 relativePos = glm::inverse(cylindre->lastModelMatrix) * glm::vec4(chat->transform.position, 1.0f);
                glm::vec3 rotatedPos = glm::vec3(model * relativePos);
                chat->transform.position = rotatedPos;
            }

            else if (localChatPos.y < 0.0f + rayon_chat) {
                // Collision par le dessous
                chat->transform.position = bottomWorld - cylUp * rayon_chat;
                isJumping = false;
                isFalling = true;
            }
            else{
                glm::vec3 offset = glm::normalize(localChatPos) * cylindre->rayon;
                chat->transform.position = glm::vec3(model * glm::vec4(offset, 1.0f));
            }
        }
    }
}

void processMurCollision(std::shared_ptr<SNode> &chat, std::shared_ptr<SNode> mur) {
    if (mur->type_objet != 6) return;

    glm::mat4 model = mur->getModelMatrix();
    glm::mat4 invModel = glm::inverse(model);

    glm::vec4 localPos = invModel * glm::vec4(chat->transform.position, 1.0f);

    // Taille de la box en espace local (avant scale)
    glm::vec3 minBox(-5.0f, 0.0f, 0.0f);
    glm::vec3 maxBox( 5.0f, 10.0f,  0.0f);

    // Rayon du chat exprimé en espace local du mur
    glm::vec3 scale = glm::vec3(
        glm::length(glm::vec3(model[0])),  // X axis
        glm::length(glm::vec3(model[1])),  // Y axis
        glm::length(glm::vec3(model[2]))   // Z axis
    );

    glm::vec3 rayon_local = glm::vec3(rayon_chat) / scale;

    // Collision AABB avec marge de rayon
    bool collision = (
        localPos.x <= maxBox.x + rayon_local.x && localPos.x >= minBox.x - rayon_local.x &&
        localPos.y <= maxBox.y + rayon_local.y && localPos.y >= minBox.y - rayon_local.y &&
        localPos.z <= maxBox.z + rayon_local.z && localPos.z >= minBox.z - rayon_local.z
    );

    if (collision) {
        // Repousse en Z local (normal du mur)
        float distFront = maxBox.z + rayon_local.z - localPos.z;
        float distBack  = localPos.z - (minBox.z - rayon_local.z);

        glm::vec3 correctionLocal(0.0f);
        if (distFront < distBack)
            correctionLocal.z = -distFront;
        else
            correctionLocal.z = distBack;

        glm::vec4 worldCorrection = model * glm::vec4(correctionLocal, 0.0f);
        chat->transform.position -= glm::vec3(worldCorrection);
    }
}

bool checkCubeCollision(glm::vec3 pos_chat, glm::vec3 rayon){
    float length_cube = 0.5f;
    if(pos_chat.x < -(length_cube + rayon.x) || pos_chat.x > length_cube + rayon.x) return false;
    if(pos_chat.y < -(length_cube + rayon.y) || pos_chat.y > length_cube + rayon.y) return false;
    if(pos_chat.z < -(length_cube + rayon.z) || pos_chat.z > length_cube + rayon.z) return false;
    return true;
}

void processCubeCollision(std::shared_ptr<SNode> &chat, std::shared_ptr<SNode> cube, float &plan_hauteur){
    if(cube->type_objet == 3){
        glm::mat4 model = cube->getModelMatrix();
        glm::mat4 invModel = glm::inverse(model);

        glm::vec3 localPos = glm::vec3(invModel * glm::vec4(chat->transform.position, 1.0f));

        glm::vec3 scale = glm::vec3(
            glm::length(glm::vec3(model[0])),  // X axis
            glm::length(glm::vec3(model[1])),  // Y axis
            glm::length(glm::vec3(model[2]))   // Z axis
        );

        glm::vec3 rayon_local = glm::vec3(rayon_chat) / scale;

        if(checkCubeCollision(localPos, rayon_local)){
            float maxCoord = localPos.x;
            unsigned int maxIndex = 0;
            if(abs(localPos.y) > abs(maxCoord)){
                maxCoord = localPos.y;
                maxIndex = 1;
            }
            if(abs(localPos.z) > abs(maxCoord)){
                maxCoord = localPos.z;
                maxIndex = 2;
            }

            glm::vec3 newPos = localPos;
            int signe = 1;
            if(maxCoord < 0) signe = -1;
            newPos[maxIndex] = signe * (0.5 + rayon_local[maxIndex]);
            float cosCollision = glm::dot(glm::normalize(newPos - glm::vec3(0.f)), glm::vec3(0.f, 1.f, 0.f));
            glm::vec3 newPosWorld = glm::vec3(model * glm::vec4(newPos, 1.0f));
            if(cosCollision > 0.5){
                plan_hauteur = newPosWorld.y;
                isFalling = false;
            }
            if(cosCollision < -0.5){
                isJumping = false;
                isFalling = true;
            }
            chat->transform.position = newPosWorld;
            glm::vec4 relativePos = glm::inverse(cube->lastModelMatrix) * glm::vec4(chat->transform.position, 1.0f);
            glm::vec3 rotatedPos = glm::vec3(model * relativePos);
            chat->transform.position = rotatedPos;
        }
    }
}

glm::vec3 ClosestPointOnTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    // Edges
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ap = p - a;

    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a; // Barycentric coords (1,0,0)

    glm::vec3 bp = p - b;
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b; // (0,1,0)

    glm::vec3 cp = p - c;
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c; // (0,0,1)

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        return a + v * ab;
    }

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float v = d2 / (d2 - d6);
        return a + v * ac;
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float v = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + v * (c - b);
    }

    // Inside face region
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

float SphereTriangleCollision(glm::vec3 sphereCenter, float sphereRadius, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 closest = ClosestPointOnTriangle(sphereCenter, a, b, c);
    float distSq = glm::length(closest - sphereCenter);
    return distSq;
}

void processMeshCollision(std::shared_ptr<SNode> &chat, std::shared_ptr<SNode> mesh, float &plan_hauteur){
    if(mesh->type_objet != 8) return;

    float min_dist = 100000.;
    int min_index = -1;

    glm::mat4 model = mesh->getModelMatrix();
    glm::mat4 invModel = glm::inverse(model);
    glm::mat4 delta = glm::inverse(mesh->lastModelMatrix) * model;

    glm::vec3 worldChatPos = chat->transform.position;
    glm::vec3 localChatPos = glm::vec3(invModel * glm::vec4(worldChatPos, 1.0f));

    // Approximation : on suppose un scale uniforme ou on prend la moyenne
    glm::vec3 scale = glm::vec3(
        glm::length(glm::vec3(model[0])),
        glm::length(glm::vec3(model[1])),
        glm::length(glm::vec3(model[2]))
    );
    float averageScale = (scale.x + scale.y + scale.z) / 3.0f;
    float rayon_chat_local = rayon_chat / averageScale;


    for(int i = 0; i<mesh->triangles.size(); i++){
        glm::vec3 a = mesh->vertices[mesh->triangles[i][0]];
        glm::vec3 b = mesh->vertices[mesh->triangles[i][1]];
        glm::vec3 c = mesh->vertices[mesh->triangles[i][2]];
        float distTriangle = SphereTriangleCollision(localChatPos, rayon_chat, a, b, c);
        if(min_dist > distTriangle){
            min_dist = distTriangle;
            min_index = i;
        }
    }

    if(min_dist <= rayon_chat_local){
        glm::vec3 a = mesh->vertices[mesh->triangles[min_index][0]];
        glm::vec3 b = mesh->vertices[mesh->triangles[min_index][1]];
        glm::vec3 c = mesh->vertices[mesh->triangles[min_index][2]];

        glm::vec3 n1 = mesh->normals[mesh->triangles[min_index][0]];
        glm::vec3 n2 = mesh->normals[mesh->triangles[min_index][1]];
        glm::vec3 n3 = mesh->normals[mesh->triangles[min_index][2]];
        glm::vec3 triangleNormal = glm::normalize((n1 + n2 + n3) / 3.0f);

        glm::vec3 closestPoint = ClosestPointOnTriangle(localChatPos, a, b, c);
        glm::vec3 correctionDir = glm::normalize(localChatPos - closestPoint);
        if (glm::dot(correctionDir, triangleNormal) < 0.0f) {
            triangleNormal = -triangleNormal;
        }
        float dist = rayon_chat_local - min_dist;
        glm::vec3 localCorrection = localChatPos + triangleNormal * dist;

        float cosCollision = glm::dot(glm::vec3(0., 1., 0.), triangleNormal);
        if(cosCollision >= 0.5f){
            plan_hauteur = glm::vec3(model * glm::vec4(localCorrection, 1.0f)).y;
            chat->transform.position.y = plan_hauteur;
            isFalling = false;
            glm::vec4 relativePos = glm::inverse(mesh->lastModelMatrix) * glm::vec4(chat->transform.position, 1.0f);
            glm::vec3 rotatedPos = glm::vec3(model * relativePos);
            chat->transform.position = rotatedPos;
        }
        else if(cosCollision < 0.0f){
            chat->transform.position = glm::vec3(model * glm::vec4(localCorrection, 1.0f));
            isJumping = false;
            isFalling = true;
        }
        else{
            chat->transform.position = glm::vec3(model * glm::vec4(localCorrection, 1.0f));
        }
        

    }
}

enum class MovementType {
    CARRE,
    LINEAIRE_X,
    LINEAIRE_Y,
    LINEAIRE_Z,
    CIRCULAIRE
};

struct PlateformeMobile {
    std::shared_ptr<SNode> node;

    MovementType movementType = MovementType::CARRE;
    float speed = 5.0f;

    float minX = -10.0f, maxX = 10.0f;
    float minY = 0.0f,  maxY = 100.0f;
    float minZ = -10.0f, maxZ = 10.0f;

    int move_state = 0;
    bool reverse = false;
    float angle = 0.0f;
    float radius = 5.0f;
    glm::vec3 center = {0.0f, 0.0f, 0.0f};
    float rotationSpeed = 1.0f;

    void update(float deltaTime) {
        switch (movementType) {
            case MovementType::CARRE:
                updateCarre(deltaTime);
                break;
            case MovementType::LINEAIRE_X:
                updateLineaire(deltaTime, 'x', minX, maxX);
                break;
            case MovementType::LINEAIRE_Y:
                updateLineaire(deltaTime, 'y', minY, maxY);
                break;
            case MovementType::LINEAIRE_Z:
                updateLineaire(deltaTime, 'z', minZ, maxZ);
                break;
            case MovementType::CIRCULAIRE:
                updateCirculaire(deltaTime);
                break;
        }
    }

private:
    void updateCarre(float deltaTime) {
        float step = deltaTime * speed;
        auto& pos = node->transform.position;

        switch (move_state) {
            case 0:
                pos.z += step;
                if (pos.z >= maxZ) {
                    pos.z = maxZ;
                    move_state = 1;
                }
                break;
            case 1:
                pos.x += step;
                if (pos.x >= maxX) {
                    pos.x = maxX;
                    move_state = 2;
                }
                break;
            case 2:
                pos.z -= step;
                if (pos.z <= minZ) {
                    pos.z = minZ;
                    move_state = 3;
                }
                break;
            case 3:
                pos.x -= step;
                if (pos.x <= minX) {
                    pos.x = minX;
                    move_state = 0;
                }
                break;
        }
    }

    void updateLineaire(float deltaTime, char axis, float minVal, float maxVal) {
        float step = deltaTime * speed;
        glm::vec3& pos = node->transform.position;
        float* value = nullptr;

        if (axis == 'x') value = &pos.x;
        else if (axis == 'y') value = &pos.y;
        else if (axis == 'z') value = &pos.z;

        if (!value) return;

        if (!reverse) {
            *value += step;
            if (*value >= maxVal) {
                *value = maxVal;
                reverse = true;
            }
        } else {
            *value -= step;
            if (*value <= minVal) {
                *value = minVal;
                reverse = false;
            }
        }
    }

    void updateCirculaire(float deltaTime) {
        angle += deltaTime * rotationSpeed;

        node->transform.position.x = center.x + radius * cos(angle);
        node->transform.position.z = center.z + radius * sin(angle);
        node->transform.position.y = center.y;
    }
};

/*******************************************************************************/

void processInput(GLFWwindow *window,std::shared_ptr<SNode> soleil,glm::vec3& vitesse,bool& isJumping,ma_engine engine);
bool input_toggle(int pressed_key, int &previous_state_key, bool &toggle_bool){
    bool is_toggled = glfwGetKey(window, pressed_key) == GLFW_PRESS && previous_state_key ==  GLFW_RELEASE;
    if(is_toggled){
		toggle_bool = !toggle_bool;
		previous_state_key = GLFW_PRESS;
	}
	else if(glfwGetKey(window, pressed_key) == GLFW_PRESS){
		previous_state_key = GLFW_PRESS;
	}
	else{
		previous_state_key = GLFW_RELEASE;
	}
    return is_toggled;
}

int main( void ){
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1800, 1400, "Pawformer", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Dark blue background
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
    // glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    //glEnable(GL_CULL_FACE);

    ma_engine engine;
    ma_engine_init(NULL, &engine);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "vertex_shader.glsl", "fragment_shader.glsl" );

    /****************************************/
    // Get a handle for our "Model View Projection" matrices uniforms
    MatrixID = glGetUniformLocation(programID,"MVP");
    ModelID = glGetUniformLocation(programID, "model");
    /****************************************/

    // Get a handle for our "LightPosition" uniform
    glUseProgram(programID);
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

    glActiveTexture(GL_TEXTURE6);
    GLuint heightmapTexture = loadTexture("textures/heightmap-1024x1024.png");
    glBindTexture(GL_TEXTURE_2D,heightmapTexture);
    GLuint heightmapID = glGetUniformLocation(programID,"heightmap");
    glUniform1i(heightmapID, 6);

    GLuint heightScaleID = glGetUniformLocation(programID,"heightScale");
    glUniform1f(heightScaleID,1.0f);

    // stbi_set_flip_vertically_on_load(true);
    // int width, height, nrComponents;
    // float *data = stbi_loadf("textures/brown_photostudio_02_4k.hdr", &width, &height, &nrComponents, 0);
    // unsigned int hdrTexture;
    // if (data)
    // {
    //     glGenTextures(1, &hdrTexture);
    //     glBindTexture(GL_TEXTURE_2D, hdrTexture);
    //     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); 

    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //     stbi_image_free(data);
    // }
    // else
    // {
    //     std::cout << "Failed to load HDR image." << std::endl;
    // }

    int heightmapWidth, heightmapHeight, nrChannels;
    unsigned char *heightmapData = stbi_load("textures/heightmap-1024x1024.png", &heightmapWidth, &heightmapHeight, &nrChannels, 1);

    std::cout << "width : " << heightmapWidth << " | height : " << heightmapHeight << std::endl;

    std::shared_ptr<Scene> scene = std::make_shared<Scene>();

    // std::shared_ptr<SNode> cube = std::make_shared<SNode>(3,glm::vec3(1.0,0.0,0.0));
    // std::shared_ptr<SNode> cube = std::make_shared<SNode>(3,"textures/rock.png");
    // std::shared_ptr<SNode> chat = std::make_shared<SNode>(4,vec3(1.0,0.1,0.2));
    std::shared_ptr<SNode> chat = std::make_shared<SNode>(4,"modeles/fure.png");
    // std::shared_ptr<SNode> cube = std::make_shared<SNode>(4,glm::vec3(1.0,0.0,0.0));
    std::shared_ptr<SNode> soleil = std::make_shared<SNode>(0,"pbr/rustediron2_basecolor.png",
        "pbr/rustediron2_normal.png", 
        "pbr/rustediron2_roughness.png", 
        "pbr/rustediron2_metallic.png",
        "pbr/test_sphere_ao.png"
    ); 
    std::shared_ptr<SNode> boule = std::make_shared<SNode>(
        0,
        "pbr/TCom_SolarCells_512_albedo.png",
        "pbr/TCom_SolarCells_512_normal.png",
        "pbr/TCom_SolarCells_512_roughness.png",
        "pbr/TCom_SolarCells_512_metallic.png",
        "pbr/TCom_SolarCells_512_ao.png"
    );
    std::shared_ptr<SNode> boule2 = std::make_shared<SNode>(
        0,
        "pbr/TCom_Scifi_Panel_512_albedo.png",
        "pbr/TCom_Scifi_Panel_512_normal.png",
        "pbr/TCom_Scifi_Panel_512_roughness.png",
        "pbr/TCom_Scifi_Panel_512_metallic.png",
        "pbr/TCom_Scifi_Panel_512_ao.png"
    );
    
    
    // Sans LOD
    // std::shared_ptr<SNode> soleil = std::make_shared<SNode>(
    //     2,
    //     "textures/s2.png",
    //     std::vector<const char*>{"modeles/sphere2.off","modeles/sphere.off"}
    // );

    std::shared_ptr<SNode> mur_p1 = std::make_shared<SNode>(6,"pbr/mur_albedo.png",
    "pbr/mur_normal.png",
    "pbr/mur_roughness.png",
    "pbr/metalness_noir.png",
    "pbr/mur_ao.png");
    std::shared_ptr<SNode> mur_p2 = std::make_shared<SNode>(6,"pbr/mur_albedo.png",
    "pbr/mur_normal.png",
    "pbr/mur_roughness.png",
    "pbr/metalness_noir.png",
    "pbr/mur_ao.png");
    std::shared_ptr<SNode> mur_p3 = std::make_shared<SNode>(6,"pbr/mur_albedo.png",
    "pbr/mur_normal.png",
    "pbr/mur_roughness.png",
    "pbr/metalness_noir.png",
    "pbr/mur_ao.png");
    std::shared_ptr<SNode> mur_p4 = std::make_shared<SNode>(6,"pbr/mur_albedo.png",
    "pbr/mur_normal.png",
    "pbr/mur_roughness.png",
    "pbr/metalness_noir.png",
    "pbr/mur_ao.png");
    std::shared_ptr<SNode> plafond = std::make_shared<SNode>(1,"pbr/mur_albedo.png",
    "pbr/mur_normal.png",
    "pbr/mur_roughness.png",
    "pbr/metalness_noir.png",
    "pbr/mur_ao.png");

    mur_p1->transform.position = glm::vec3(0.0,0.0,-5.0f);
    mur_p2->transform.position = glm::vec3(4.685,0.0,-0.315f);
    mur_p2->transform.rotation = glm::vec3(0.0,glm::radians(90.0f),0.0);
    mur_p3->transform.position = glm::vec3(-5.0f,0.0,-0.315f);
    mur_p3->transform.rotation = glm::vec3(0.0,glm::radians(90.0f),0.0);
    mur_p4->transform.position = glm::vec3(0.0,0.0,4.685f);
    plafond->transform.position = glm::vec3(0.0f,9.685f,0.0f);


    std::shared_ptr<SNode> jump;
    std::shared_ptr<SNode> plan = std::make_shared<SNode>(1,"pbr/parquet_albedo.png",
    "pbr/parquet_normal.png",
    "pbr/parquet_roughness.png",
    "pbr/metalness_noir.png",
    "pbr/parquet_ao.png");
    plan->transform.scale = glm::vec3(10.0f);
    // std::shared_ptr<SNode> plan2 = std::make_shared<SNode>(3,"textures/grass.png");
    // plan2->transform.scale = glm::vec3(2.0f);
    
    // std::shared_ptr<SNode> mur = std::make_shared<SNode>(6,"textures/rock.png");
    // std::shared_ptr<SNode> mesh_chat_test = std::make_shared<SNode>(8, glm::vec3(0.0f, 0.0f, 0.0f));

    scene->racine->addFeuille(chat);

    scene->racine->addFeuille(plan);
    plan->addFeuille(mur_p1);
    plan->addFeuille(mur_p2);
    plan->addFeuille(mur_p3);
    plan->addFeuille(mur_p4);
    plan->addFeuille(plafond);

    // ---------------------------------------------- ARBRE 1 ----------------------------------------------

    std::shared_ptr<SNode> tronc = std::make_shared<SNode>(5,"textures/corde_texture.png");
    tronc->transform.scale = glm::vec3(2.0f);
    tronc->transform.position = glm::vec3(-15.0f,0.0f, 0.0f);
    tronc->grabable = true;

    scene->racine->addFeuille(tronc);

    std::shared_ptr<SNode> socle = std::make_shared<SNode>(3,"textures/rock.png");
    socle->transform.scale = glm::vec3(2.0f, 1.0f, 2.0f);

    tronc->addFeuille(socle);

    std::shared_ptr<SNode> plateforme = std::make_shared<SNode>(7, "textures/corde_texture.png");
    std::shared_ptr<SNode> plateforme2 = std::make_shared<SNode>(7, "textures/corde_texture.png");
    std::shared_ptr<SNode> plateforme3 = std::make_shared<SNode>(7, "textures/corde_texture.png");
    plateforme->transform.position = glm::vec3(1.5, 2., 0.);
    plateforme->transform.scale = glm::vec3(0.5f);
    plateforme2->transform.position = glm::vec3(-1.5, 5., -1.5);
    plateforme2->transform.scale = glm::vec3(0.5f);
    plateforme3->transform.position = glm::vec3(-1.5, 8., 1.5);
    plateforme3->transform.rotation = glm::vec3(0.2f, 0.0f, 0.0f);
    plateforme3->transform.scale = glm::vec3(0.5f);

    tronc->addFeuille(plateforme);
    tronc->addFeuille(plateforme2);
    tronc->addFeuille(plateforme3);

    std::shared_ptr<SNode> cube = std::make_shared<SNode>(3,"textures/rock.png");
    cube->transform.position = glm::vec3(0.0f,1.5f,0.0f);
    cube->transform.scale = glm::vec3(2.0f);
    plateforme->addFeuille(cube);

    std::shared_ptr<SNode> chat_noir = std::make_shared<SNode>(8, "modeles/fure.png");
    chat_noir->transform.position = glm::vec3(1.5f,1.75f,-1.5f);
    chat_noir->transform.scale = glm::vec3(1.5f);
    plateforme2->addFeuille(chat_noir);
    
    std::shared_ptr<SNode> cube2 = std::make_shared<SNode>(3, "textures/snowrocks.png");
    cube2->transform.position = glm::vec3(10.f, 4.f, 10.f);
    cube2->transform.scale = glm::vec3(3.f);

    //tronc->transform.rotation = glm::vec3(0.0, 0.0, 5.0);
    
    soleil->transform.position = glm::vec3(-1.0f,15.0f,1.0f);
    boule->transform.position = glm::vec3(-4.0f,1.f,2.0f);
    boule->transform.scale = glm::vec3(2.0f);
    boule2->transform.position = glm::vec3(-4.0f,18.5f,-10.0f);
    boule2->transform.scale = glm::vec3(3.0f);

    scene->racine->addFeuille(soleil);
    scene->racine->addFeuille(boule);
    scene->racine->addFeuille(boule2);
    scene->racine->addFeuille(cube2);

    // ---------------------------------------------- ARBRE 2 ----------------------------------------------

    std::shared_ptr<SNode> tronc2 = std::make_shared<SNode>(5,"textures/corde_texture.png");
    tronc2->transform.scale = glm::vec3(2.0f);
    tronc2->transform.position = glm::vec3(15.0f, 0.0f, 0.0f);
    tronc2->grabable = true;

    scene->racine->addFeuille(tronc2);

    std::shared_ptr<SNode> socle2 = std::make_shared<SNode>(3,"textures/rock.png");
    socle2->transform.scale = glm::vec3(2.0f, 1.0f, 2.0f);

    tronc2->addFeuille(socle2);

    std::shared_ptr<SNode> plateforme_t2 = std::make_shared<SNode>(7, "textures/corde_texture.png");
    plateforme_t2->transform.position = glm::vec3(3.5f, 2.0f, 0.f);
    plateforme_t2->transform.scale = glm::vec3(0.5f);

    tronc2->addFeuille(plateforme_t2);

    PlateformeMobile pf;
    pf.node = plateforme_t2;
    pf.movementType = MovementType::LINEAIRE_Z;
    pf.minZ = -3.0f;
    pf.maxZ = 3.0f;

    std::shared_ptr<SNode> plateforme2_t2 = std::make_shared<SNode>(7, "textures/corde_texture.png");
    plateforme2_t2->transform.position = glm::vec3(0.0f, 3.5f, -3.f);
    plateforme2_t2->transform.scale = glm::vec3(0.5f);

    tronc2->addFeuille(plateforme2_t2);

    PlateformeMobile pf2;
    pf2.node = plateforme2_t2;
    pf2.movementType = MovementType::LINEAIRE_Y;
    pf2.minY = 2.0f;
    pf2.maxY = 20.0f;

    std::shared_ptr<SNode> plateforme3_t2 = std::make_shared<SNode>(7, "textures/corde_texture.png");
    plateforme3_t2->transform.position = glm::vec3(-3.0f, 5.0f, 0.f);
    plateforme3_t2->transform.scale = glm::vec3(0.5f);

    tronc2->addFeuille(plateforme3_t2);

    PlateformeMobile pf3;
    pf3.node = plateforme3_t2;
    pf3.movementType = MovementType::LINEAIRE_X;
    pf3.minX = -11.5f;
    pf3.maxX = -3.5f;

    std::shared_ptr<SNode> plateforme4_t2 = std::make_shared<SNode>(7, "textures/corde_texture.png");
    plateforme4_t2->transform.position = glm::vec3(3.0f, 15.0f, 0.f);
    plateforme4_t2->transform.scale = glm::vec3(0.5f);

    tronc2->addFeuille(plateforme4_t2);

    PlateformeMobile pf4;
    pf4.node = plateforme4_t2;
    pf4.movementType = MovementType::CARRE;
    pf4.minX = -18.5f;
    pf4.maxX = 3.5f;
    pf4.minZ = -3.5f;
    pf4.maxZ = 3.5f;

    // scene->racine->addFeuille(mur);
    
    // scene->racine->addFeuille(plan2);
    // scene->racine->addFeuille(plateforme);
    // scene->racine->addFeuille(plateforme2);
    // scene->racine->addFeuille(plateforme3);
    // scene->racine->addFeuille(mesh_chat_test);


    scene->add_light(glm::vec3(1., 1., 1.));
    scene->add_light(glm::vec3(0., 95., 0.));
    scene->add_light(glm::vec3(chat->transform.position));

    
    chat->transform.position = glm::vec3(-2.0f,5.0f,-2.0f);
    // plan2->transform.position = glm::vec3(0.0f,4.65f,-9.65f);
    // mur->transform.position = glm::vec3(0.0f,0.0f,-5.0f);
    // mesh_chat_test->transform.scale = glm::vec3(10.0f);
    // mesh_chat_test->transform.position = glm::vec3(20., 0., 20.);

    initImgui(window);

    std::cout<<"nb feuilles :"<<scene->racine->getFeuilles().size()<<std::endl;

    float time = 0.0f;

    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    std::vector<glm::vec3> terrainVertices = plan->vertices;
    std::vector<unsigned short> terrainIndices = plan->indices;


    float plan_hauteur = gethauteur(scene,chat);

    do{
        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        // std::cout << "d : " << deltaTime << std::endl;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window,chat,vitesse,isJumping,engine);

        pf.update(deltaTime);
        pf2.update(deltaTime);
        pf3.update(deltaTime);
        pf4.update(deltaTime*2);

        if(oiia){
            chat->transform.rotation += glm::vec3(0.0f,1.0f,0.0f) * (deltaTime*18);
            chat->transform.position.y += deltaTime*2;
        }

        cube2->transform.rotation += glm::vec3(0.0f, 0.01f, 0.0f);
        
        // std::cout<<isGrabbing<<std::endl;

        GLuint PBRID = glGetUniformLocation(programID,"PBR_OnOff");
        glUniform1i(PBRID,(PBR_OnOff) ? 1 : 0);

        scene->lights[1].position = chat->transform.position;

        // float hauteur_chat = chat->transform.scale.y;
        // if(!isJumping) plan_hauteur = gethauteur(scene,chat);

        float hauteur_chat = chat->transform.scale.y;
        // plan_hauteur = gethauteur(scene,chat);
        float seuil_sol = plan_hauteur + hauteur_chat;

        bool auSol = chat->transform.position.y <= seuil_sol;

        // std::cout << "seuil_sol : " << seuil_sol << std::endl;
        // std::cout << "chat pos : " << chat->transform.position.y << std::endl;
        // std::cout << auSol << std::endl;

        if(isJumping || isFalling){
            if(isGrabbing){
                isFalling = false;
                isJumping = false;
            }

            // chat->transform.position.y += deltaTime * 22.0f;
            vitesse += acceleration * deltaTime;
            chat->transform.position += vitesse * deltaTime;

            if (chat->transform.position.y <= seuil_sol) {
                plan_hauteur = gethauteur(scene,chat);
                chat->transform.position.y = seuil_sol;
                vitesse.y = 0.0f;
                isJumping = false;
                isFalling = false;
            }
        }else if(vitesse.y <= 0.0f){
            isFalling = true;
            isJumping = false;
            // if(!isGrabbing) chat->transform.position.y -= vitesse.length() * deltaTime * 11;

            // if (chat->transform.position.y < plan_hauteur + hauteur_chat) {
            //     chat->transform.position.y = plan_hauteur + hauteur_chat;
            //     isFalling = false;
            // }
        }
        // std::cout << "(pos.y | plan+jump)" << cube->transform.position.y << " | " << plan_hauteur+jump_height.y << std::endl;
        // if (chat->transform.position.y > plan_hauteur + jump_height){
        //     isJumping = false;
        //     isFalling = true;
        // }

        isGrabbing = false;
        if (debugFilaire) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // Vue filaire
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // Vue normale
        }

        for(const std::shared_ptr<SNode>& object: scene->racine->getFeuilles()){
            processSphereCollision(chat->transform.position, object, plan_hauteur);
            processCylindreCollision(chat, object, plan_hauteur);
            processMurCollision(chat, object);
            processMeshCollision(chat, object, plan_hauteur);
            processCubeCollision(chat, object, plan_hauteur);
            object->lastModelMatrix = object->getModelMatrix();
        }

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        // Use our shader
        glUseProgram(programID);

        scene->draw(programID);
        scene->update(deltaTime);

        GLuint camPosID = glGetUniformLocation(programID,"camPos");
        glUniform3fv(camPosID, 1, &camera_position[0]);

        // std::cout << "[Loop] Début frame" << std::endl;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // std::cout << "[Loop] ImGui frame commencée" << std::endl;

        ImGui::SetNextWindowSize(ImVec2(400, 75), ImGuiCond_Always);
        ImGui::Begin("Debug Info", nullptr, flags);
        ImGui::Text("Hauteur : %.2f m", chat->transform.position.y - 0.85f);
        ImGui::End();

        // std::cout << "[Loop] Fenêtre affichée" << std::endl;

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // std::cout << "[Loop] Render OK" << std::endl;

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        time += deltaTime;

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

    glDeleteProgram(programID);
    glDeleteVertexArrays(1,&VertexArrayID);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();
    return 0;
}

float ClipAngle180(float _angle){
	float _clippedAngle = _angle;
	if(_angle > 180.0f)
		_clippedAngle = _angle-360.0f;
	if(_angle < -180.0f)
		_clippedAngle = _angle+360.0f;
	return _clippedAngle;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, std::shared_ptr<SNode> chat,glm::vec3& vitesse,bool &isJumping,ma_engine engine){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 5.0 * deltaTime;


    input_toggle(GLFW_KEY_F, fPreviousState, debugFilaire);
    if(input_toggle(GLFW_KEY_N, nPreviousState, cam_attache) && !cam_attache){
        camera_position = glm::vec3(1.0, 1.0, 1.0);
        camera_target = glm::vec3(0.0, 0.0, 0.0);
        std::cout<<camera_position[0]<<", "<<camera_position[1]<<std::endl;
    }
        
    input_toggle(GLFW_KEY_P,p_preview_state,PBR_OnOff);

    input_toggle(GLFW_KEY_F5,person_state,person);

    if(!cam_attache){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            camera_position += cameraSpeed * camera_target;
        if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            camera_position -= cameraSpeed * camera_target;
        if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS)
            camera_position -= glm::normalize(glm::cross(camera_target,camera_up))*cameraSpeed;
        if(glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS)
            camera_position += cameraSpeed * camera_up;
        if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS)
            camera_position += glm::normalize(glm::cross(camera_target,camera_up))*cameraSpeed;
        if(glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS)
            camera_position -= cameraSpeed * camera_up;

        float sensitivity = 20.0f * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            yaw_ -= sensitivity;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            yaw_ += sensitivity;
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
            pitch_ += sensitivity;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            pitch_ -= sensitivity;

        if (pitch_ > 89.0f)
            pitch_ = 89.0f;
        if (pitch_ < -89.0f)
            pitch_ = -89.0f;

        yaw_ = ClipAngle180(yaw_);

        glm::vec3 front;
        front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        front.y = sin(glm::radians(pitch_));
        front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
        camera_target = glm::normalize(front);
    }else{
        glm::vec3 right = glm::normalize(glm::cross(camera_target, glm::vec3(0.0, 1.0, 0.0)));
        glm::vec3 forward = glm::normalize(glm::cross(right, glm::vec3(0.0, 1.0, 0.0)));

        glm::vec3 mouvement = glm::vec3(0.0f);

        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && !isGrabbing){
            // cube->transform.position -= forward * 10.0f * deltaTime;
            mouvement -= forward;
        }
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !isGrabbing){
            // cube->transform.position += forward * 10.0f * deltaTime;
            mouvement += forward;
        }
        if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS && !isGrabbing){
            // cube->transform.position -= right * 10.0f * deltaTime;
            mouvement -= right;
        }
        if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS && !isGrabbing){
            // cube->transform.position += right * 10.0f * deltaTime;
            mouvement += right;
        }

        if (glm::length(mouvement) > 0.0f) {
            // Normaliser le vecteur de mouvement
            mouvement = glm::normalize(mouvement);
        
            // Mettre à jour la position du chat
            chat->transform.position += mouvement * 10.0f * deltaTime;
        
            // Calculer l'angle de rotation autour de l'axe Y en fonction de la direction de mouvement
            float rotationY = atan2(mouvement.x, mouvement.z);
        
            // Appliquer la rotation au chat
            chat->transform.rotation.y = rotationY;
        }

        static float sonTimer = 0.0f;
        if(input_toggle(GLFW_KEY_O,o_p_s,oiia)){
            if(!oiia) sonTimer = 0.0f;
        };

        if (oiia) {
            sonTimer -= deltaTime;
            if(sonTimer <= 0.0f){
                ma_engine_play_sound(&engine,"assets/OIIAOIIA.wav",NULL);
                sonTimer = 1.35f;
            }
        }

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glfwGetCursorPos(window, &_xCursorPos, &_yCursorPos);

        float xDeltaPos = _xCursorPos - width / 2;
        float yDeltaPos = _yCursorPos - height / 2;

        float sensitivity = 0.1f;
        yaw_ += xDeltaPos * sensitivity;
        pitch_ += yDeltaPos * sensitivity;

        if (pitch_ > 89.0f)
            pitch_ = 89.0f;
        if (pitch_ < -89.0f)
            pitch_ = -89.0f;

        yaw_ = ClipAngle180(yaw_);

        
        if(glfwGetKey(window,GLFW_KEY_Q) == GLFW_PRESS)
            cam_distance += 3. * deltaTime;
        if(glfwGetKey(window,GLFW_KEY_E) == GLFW_PRESS)
            cam_distance -= 3. * deltaTime;

        if(!person){
            angle_perspective = 45.0;
            glm::vec3 offset;
            offset.x = cam_distance * cos(glm::radians(pitch_)) * cos(glm::radians(yaw_));
            offset.y = cam_distance * sin(glm::radians(pitch_));
            offset.z = cam_distance * cos(glm::radians(pitch_)) * sin(glm::radians(yaw_));

            camera_position = chat->transform.position + offset;

            camera_target = glm::normalize(chat->transform.position - camera_position);
        }else{
            angle_perspective = 70.0;
            camera_target.x = cos(glm::radians(pitch_)) * cos(glm::radians(yaw_));
            camera_target.y = sin(glm::radians(pitch_));
            camera_target.z = cos(glm::radians(pitch_)) * sin(glm::radians(yaw_));
            camera_target = -glm::normalize(camera_target);

            glm::vec3 forward = glm::normalize(glm::vec3(camera_target.x, 0.0f, camera_target.z));
            camera_position = chat->transform.position + glm::vec3(0.0f, 1.f, 0.0f);

            float rotationY = atan2(forward.x, forward.z);
            chat->transform.rotation.y = rotationY;
        }

        glfwSetCursorPos(window, width / 2, height / 2);
    }


    
    
    // if(glfwGetKey(window,GLFW_KEY_N) == GLFW_PRESS)
    //     cam_attache = !cam_attache;

    /****************************************/

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) 
        chat->transform.position -= glm::vec3(0.0f, 0.0f, 0.1f);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) 
        chat->transform.position += glm::vec3(0.0f, 0.0f, 0.1f);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) 
        chat->transform.position -= glm::vec3(0.1f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) 
        chat->transform.position += glm::vec3(0.1f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) 
        chat->transform.position -= glm::vec3(0.0f, 0.1f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) 
        chat->transform.position += glm::vec3(0.0f, 0.1f, 0.0f);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping && !isFalling && !isGrabbing){
        std::cout << "SAUT !" << std::endl;
        isJumping = true;
        vitesse.y = 10.0f;
    } 
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

