#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "class_shader.hpp"

#include <iostream>
#include <vector>

using namespace std;
using namespace glm;

struct Vertex
{
    vec3 Position;
    vec3 Normal;
    vec2 TexCoords;
    Vertex(){};
    Vertex(vec3 Position, vec3 Normal, vec2 TexCoords)
        : Position(Position), Normal(Normal), TexCoords(TexCoords)
    {
    }
};

struct Texture
{
    GLuint id;
    string type;
    string path;
    Texture(){};
    Texture(GLuint id, string type, string path)
        : id(id), type(type), path(path)
    {
    }
};

class Mesh {
public:
    // 网格数据
    vector<Vertex>  vertices;
    vector<GLuint>  indices;
    vector<Texture> textures;
    // 函数
    Mesh(vector<Vertex>&  vertices,
         vector<GLuint>&  indices,
         vector<Texture>& textures);
    ~Mesh();
    void Draw(Shader* shader, GLuint instanceNum = 1);
    void SetInstanceArray(GLuint instanceVBO,
                          GLuint vaoSlot,
                          GLuint vecSize,
                          GLuint updateFruq = 1);

private:
    // 渲染数据
    GLuint VAO, VBO, EBO;
    // 函数
    void setupMesh();
};

#endif  // MESH_H