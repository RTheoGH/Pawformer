// Include standard headers
#include <glm/trigonometric.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <memory>
#include <iostream>
#include <numeric>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
glm::mat4 MVP;

bool debugFilaire = false;

bool cam_attache = true;
float yaw_ = -90.0f;
float pitch_ = 0.0f;
double _xCursorPos, _yCursorPos;
int nPreviousState = GLFW_RELEASE;
int fPreviousState = GLFW_RELEASE;
float cam_distance = 10.0f;

glm::vec3 jump_limit = glm::vec3(0.0f,11.0f,0.0f);
bool isJumping = false;
bool isFalling = false;

double gravite = 10.0f;
double masse_cube = 2.0f;

double poids = masse_cube*gravite;

/*******************************************************************************/

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
            float x = cos(theta) * rayon;
            float z = sin(theta) * rayon;

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


void plan(std::vector<glm::vec3> &vertices,std::vector<glm::vec2> &uvs,std::vector<unsigned short> &indices){
    float taille = 10.0f;
    float m = taille / 2.0f;
    float pas = taille / (float)sommets;

    for(int i = 0; i <= sommets; i++){
        for(int j = 0; j <= sommets; j++){
            float x = -m + j * pas;
            float z = -m + i * pas;

            vertices.emplace_back(glm::vec3(x,0.0f,z));

            float u = (float)j / (float)(sommets-1);
            float v = (float)i / (float)(sommets-1);

            uvs.emplace_back(glm::vec2(u,v));
        }
    }

    for(int i = 0; i < sommets-1; i++){
        for(int j = 0; j < sommets-1; j++){
            int topleft = i * (sommets+1) + j;
            int topright = topleft + 1;
            int bottomleft = (i+1) * (sommets+1) + j;
            int bottomright = bottomleft + 1;

            indices.push_back(topleft);
            indices.push_back(bottomleft);
            indices.push_back(topright);

            indices.push_back(topright);
            indices.push_back(bottomleft);
            indices.push_back(bottomright);
        }
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

void mur(std::vector<glm::vec3> &vertices, std::vector<glm::vec2> &uvs, std::vector<unsigned short> &indices){
    float taille = 10.0f;
    float m = taille / 2.0f;
    float pas = taille / (float)sommets;

    for(int i = 0; i <= sommets; i++){
        for(int j = 0; j <= sommets; j++){
            float x = -m + j * pas;
            float y = i * pas;

            vertices.emplace_back(glm::vec3(x, y, 0.0f));

            float u = (float)j / (float)(sommets-1);
            float v = (float)i / (float)(sommets-1);

            uvs.emplace_back(glm::vec2(u, v));
        }
    }

    for(int i = 0; i < sommets-1; i++){
        for(int j = 0; j < sommets-1; j++){
            int topleft = i * (sommets+1) + j;
            int topright = topleft + 1;
            int bottomleft = (i+1) * (sommets+1) + j;
            int bottomright = bottomleft + 1;

            indices.push_back(topleft);
            indices.push_back(bottomleft);
            indices.push_back(topright);

            indices.push_back(topright);
            indices.push_back(bottomleft);
            indices.push_back(bottomright);
        }
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

class SNode{
public:
    Transform transform;
    std::vector<std::shared_ptr<SNode>> feuilles;
    GLuint vao = 0, vbo = 0, ibo = 0, textureID = 0, normalsID = 0, roughnessID = 0, metalnessID = 0, aoID = 0;
    GLuint uvVBO = 0;
    size_t indexCPT = 0;
    glm::vec3 color;
    int type_objet = 0;
    int isPBR = 0;

    const char* low;
    const char* high;
    const char* current_modele;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned short> indices;
    std::vector<glm::vec3> normals;
    std::vector<std::vector<unsigned short>> triangles;

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

    SNode(int obj,glm::vec3 nodeColor): color(nodeColor){
        type_objet = obj;
        buffers();
    }

    SNode(){}

    void buffers(){
        switch(type_objet){
            case 1:
                plan(vertices,uvs,indices); // Plan
                break;
            case 2:
                calculateUVSphere(vertices,uvs);
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
                break;
            case 6:
                mur(vertices,uvs,indices);
                break;
            default:
                loadOFF("modeles/sphere2.off",vertices,indices,triangles);
                calculateUVSphere(vertices,uvs);
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
        }
    
        if (hasIndices()) {
            glGenBuffers(1, &ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
        }

        indexCPT = indices.size();
        glBindVertexArray(0);
    }

    void addFeuille(std::shared_ptr<SNode> feuille){
        feuilles.push_back(feuille);
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
            100.0f
        );

        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID,1,GL_FALSE,&MVP[0][0]);

        GLuint isColorID = glGetUniformLocation(shaderProgram,"isColor");
        glUniform1i(isColorID,(type_objet == 4) ? 1 : 0);

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
        
        glDrawElements(GL_TRIANGLES,
            indexCPT,
            GL_UNSIGNED_SHORT,
            (void*)0
        );

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

float gethauteur(std::shared_ptr<Scene> scene,glm::vec3 cube_pos){
    float hauteurMax = -1000.0f;
    std::shared_ptr<SNode> maxPlan;
    for(const std::shared_ptr<SNode>& plan: scene->racine->feuilles){
        if(plan->type_objet == 1){
            float planX = plan->transform.position.x;
            float planZ = plan->transform.position.z;
            float largeur = 5.0f;
            float longueur = 5.0f;

            if(cube_pos.x >= planX - largeur && cube_pos.x <= planX + largeur &&
                cube_pos.z >= planZ - longueur && cube_pos.z<= planZ + longueur
            ){

                if(plan->transform.position.y > hauteurMax && cube_pos.y > plan->transform.position.y){
                    hauteurMax = plan->transform.position.y+0.5;
                }
            }
        }
    }
    return hauteurMax;
}

/*******************************************************************************/

void processInput(GLFWwindow *window,std::shared_ptr<SNode> soleil);
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

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "TP5 - GLFW", NULL, NULL);
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

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "vertex_shader.glsl", "fragment_shader.glsl" );

    /****************************************/
    // Get a handle for our "Model View Projection" matrices uniforms
    MatrixID = glGetUniformLocation(programID,"MVP");

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

    int heightmapWidth, heightmapHeight, nrChannels;
    unsigned char *heightmapData = stbi_load("textures/heightmap-1024x1024.png", &heightmapWidth, &heightmapHeight, &nrChannels, 1);

    std::cout << "width : " << heightmapWidth << " | height : " << heightmapHeight << std::endl;

    std::shared_ptr<Scene> scene = std::make_shared<Scene>();

    // std::shared_ptr<SNode> cube = std::make_shared<SNode>(3,glm::vec3(1.0,0.0,0.0));
    // std::shared_ptr<SNode> cube = std::make_shared<SNode>(3,"textures/rock.png");
    std::shared_ptr<SNode> cube = std::make_shared<SNode>(4,vec3(1.0,0.1,0.2));
    // std::shared_ptr<SNode> cube = std::make_shared<SNode>(4,glm::vec3(1.0,0.0,0.0));
    std::shared_ptr<SNode> soleil = std::make_shared<SNode>(0,"textures/rustediron2_basecolor.png",
    "textures/rustediron2_normal.png", 
    "textures/rustediron2_roughness.png", 
    "textures/rustediron2_metallic.png",
    "textures/test_sphere_ao.png"); // Sans LOD
    // std::shared_ptr<SNode> soleil = std::make_shared<SNode>(
    //     2,
    //     "textures/s2.png",
    //     std::vector<const char*>{"modeles/sphere2.off","modeles/sphere.off"}
    // );

    std::shared_ptr<SNode> jump;
    std::shared_ptr<SNode> plan = std::make_shared<SNode>(1,"textures/grass.png");
    std::shared_ptr<SNode> plan2 = std::make_shared<SNode>(1,"textures/grass.png");
    std::shared_ptr<SNode> tronc = std::make_shared<SNode>(5,"textures/corde_texture.png");
    std::shared_ptr<SNode> mur = std::make_shared<SNode>(6,"textures/rock.png");

    scene->racine->addFeuille(cube);
    scene->racine->addFeuille(soleil);
    scene->racine->addFeuille(tronc);
    scene->racine->addFeuille(mur);
    scene->racine->addFeuille(plan);
    scene->racine->addFeuille(plan2);
    scene->add_light(glm::vec3(1., 1., 1.));
    scene->add_light(glm::vec3(cube->transform.position));

    soleil->transform.position = glm::vec3(-1.0f,5.0f,1.0f);
    tronc->transform.position = glm::vec3(0.0f,0.0f, 0.0f);
    cube->transform.position = glm::vec3(-1.0f,0.75f,-1.0f);
    plan2->transform.position = glm::vec3(0.0f,9.65f,-9.65f);
    mur->transform.position = glm::vec3(0.0f,0.0f,-5.0f);

    float time = 0.0f;

    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    std::vector<glm::vec3> terrainVertices = plan->vertices;
    std::vector<unsigned short> terrainIndices = plan->indices;

    glm::vec3 acceleration = glm::vec3(0.0f,-gravite,0.0f);
    glm::vec3 vitesse = glm::vec3(0.0f,0.0f,0.0f);

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
        processInput(window,cube);

        scene->lights[1].position = cube->transform.position;

        float plan_hauteur = gethauteur(scene,cube->transform.position);
        // std::cout << "hauteur :" << plan_hauteur << std::endl;

        if(isJumping){
            cube->transform.position.y += deltaTime * 22.0f;
            vitesse += acceleration * deltaTime;
        }else{
            isFalling = true;
            cube->transform.position.y -= vitesse.length() * deltaTime * 11;

            if (cube->transform.position.y < plan_hauteur + 0.35) {
                cube->transform.position.y = plan_hauteur + 0.35;
                isFalling = false;
            }
        }
        // std::cout << "(pos.y | plan+jump)" << cube->transform.position.y << " | " << plan_hauteur+jump_limit.y << std::endl;
        if (cube->transform.position.y > plan_hauteur + jump_limit.y){
            isJumping = false;
            isFalling = true;
        }

        if (debugFilaire) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // Vue filaire
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // Vue normale
        }

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        scene->draw(programID);
        scene->update(deltaTime);

        GLuint camPosID = glGetUniformLocation(programID,"camPos");
        glUniform3fv(camPosID, 1, &camera_position[0]);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        time += deltaTime;

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

    glDeleteProgram(programID);
    glDeleteVertexArrays(1,&VertexArrayID);

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
void processInput(GLFWwindow *window, std::shared_ptr<SNode> cube){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 5.0 * deltaTime;


    input_toggle(GLFW_KEY_F, fPreviousState, debugFilaire);
    if(input_toggle(GLFW_KEY_N, nPreviousState, cam_attache) && !cam_attache){
        camera_position = glm::vec3(1.0, 1.0, 1.0);
        camera_target = glm::vec3(0.0, 0.0, 0.0);
        std::cout<<camera_position[0]<<", "<<camera_position[1]<<std::endl;
    }
        

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

        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
            // cube->transform.position -= forward * 10.0f * deltaTime;
            mouvement -= forward;
        }
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
            // cube->transform.position += forward * 10.0f * deltaTime;
            mouvement += forward;
        }
        if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS){
            // cube->transform.position -= right * 10.0f * deltaTime;
            mouvement -= right;
        }
        if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS){
            // cube->transform.position += right * 10.0f * deltaTime;
            mouvement += right;
        }

        if (glm::length(mouvement) > 0.0f) {
            // Normaliser le vecteur de mouvement
            mouvement = glm::normalize(mouvement);
        
            // Mettre à jour la position du cube
            cube->transform.position += mouvement * 10.0f * deltaTime;
        
            // Calculer l'angle de rotation autour de l'axe Y en fonction de la direction de mouvement
            float rotationY = atan2(mouvement.x, mouvement.z);
        
            // Appliquer la rotation au cube
            cube->transform.rotation.y = rotationY;
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
            cam_distance += 1. * deltaTime;
        if(glfwGetKey(window,GLFW_KEY_E) == GLFW_PRESS)
            cam_distance -= 1. * deltaTime;


        glm::vec3 offset;
        offset.x = cam_distance * cos(glm::radians(pitch_)) * cos(glm::radians(yaw_));
        offset.y = cam_distance * sin(glm::radians(pitch_));
        offset.z = cam_distance * cos(glm::radians(pitch_)) * sin(glm::radians(yaw_));

        camera_position = cube->transform.position + offset;

        camera_target = glm::normalize(cube->transform.position - camera_position);

        glfwSetCursorPos(window, width / 2, height / 2);
    }


    
    
    // if(glfwGetKey(window,GLFW_KEY_N) == GLFW_PRESS)
    //     cam_attache = !cam_attache;

    /****************************************/

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) 
        cube->transform.position -= glm::vec3(0.0f, 0.0f, 0.1f);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) 
        cube->transform.position += glm::vec3(0.0f, 0.0f, 0.1f);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) 
        cube->transform.position -= glm::vec3(0.1f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) 
        cube->transform.position += glm::vec3(0.1f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) 
        cube->transform.position -= glm::vec3(0.0f, 0.1f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) 
        cube->transform.position += glm::vec3(0.0f, 0.1f, 0.0f);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping && !isFalling){
        isJumping = true;
        cube->transform.position.y += deltaTime;
        jump_limit = cube->transform.position + glm::vec3(0., 11., 0.);
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

