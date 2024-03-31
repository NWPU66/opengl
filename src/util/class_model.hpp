#ifndef MODEL_H
#define MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"

#include "class_mesh.hpp"
#include "class_shader.hpp"

#include <iostream>
#include <vector>

using namespace std;
using namespace glm;

class Model {
public:
    Model(const string path);
    ~Model();
    void Draw(Shader* shader);

private:
    // 模型数据
    vector<Mesh>    meshes;
    string          directory;
    vector<Texture> textures_loaded;  // 已经加载过的纹理不再加载
    // 函数
    void loadModel(const string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    vector<Texture>
    loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
};

#endif  // MODEL_H