#include "class_model.hpp"

GLuint TextureFromFile(const char* path, const string& directory, bool gamma);

Model::Model(const string path)
{
    this->loadModel(path);
}

Model::~Model() {}

void Model::Draw(Shader* shader, GLuint instanceNum)
{
    shader->use();
    for (int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader, instanceNum);
}

void Model::loadModel(const string path)
{
    Assimp::Importer import;
    // const aiScene*   scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    const aiScene* scene = import.ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
        return;
    }
    this->directory = path.substr(0, path.find_last_of('/'));  // 模型文件的目录

    this->processNode(scene->mRootNode, scene);
}

/// @brief 设置model对象下所有Mesh的实例数组
/// @param instanceVBO 实例数组的缓冲区索引
/// @param vaoSlot 要绑定的顶点属性指针的槽位
/// @param vecSize 实例数组每个元素的向量大小
/// @param updateFruq 更新频率，0表示每次绘制时更新，2表示每2个实例更新一次属性
void Model::SetInstanceArray(GLuint      instanceVBO,
                             GLuint      vaoSlot,
                             GLuint      vecSize,
                             GLsizei     stride,
                             const void* offset,
                             GLuint      updateFruq)
{
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    for (int i = 0; i < meshes.size(); i++)
        meshes[i].SetInstanceArray(instanceVBO, vaoSlot, vecSize, stride, offset, updateFruq);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // 处理节点所有的网格（如果有的话）
    for (int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // 接下来对节点的孩子节点重复以上过程
    for (int i = 0; i < node->mNumChildren; i++)
        this->processNode(node->mChildren[i], scene);
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    vector<Vertex>  vertices;
    vector<GLuint>  indices;
    vector<Texture> textures;

    for (int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        // 处理顶点位置、法线和纹理坐标
        vertex.Position = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.Normal   = vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        if (mesh->HasTextureCoords(0))
            vertex.TexCoords = vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.TexCoords = vec2(0.0f, 0.0f);

        // tangent and bitangent
        if (mesh->HasTangentsAndBitangents())
        {
            vertex.Tangent = vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertex.Bitangent =
                vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }

        vertices.push_back(vertex);
    }

    // 处理索引
    for (int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // 处理材质纹理
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // diffuse
        vector<Texture> diffuseMaps =
            loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // specular
        vector<Texture> specularMaps =
            loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // normal
        std::vector<Texture> normalMaps =
            loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // FIXME - 为什么法线贴图的类型是 aiTextureType_HEIGHT ？
    }

    return Mesh(vertices, indices, textures);
}

vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
{
    vector<Texture> textures;
    for (int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString texturePath;
        mat->GetTexture(type, i, &texturePath);
        bool skip = false;
        for (int j = 0; j < textures_loaded.size(); j++)
        {
            if (textures_loaded[j].path == string(texturePath.C_Str()))
            {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            GLuint  id = TextureFromFile(texturePath.C_Str(), this->directory, false);
            Texture texture(id, typeName, string(texturePath.C_Str()));
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
    }
    return textures;
}

GLuint TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    filename        = directory + '/' + filename;
    cout << "load texture: " << filename << endl;

    // 生成并绑定纹理对象
    GLuint textureID;
    glGenTextures(1, &textureID);             // gl函数要在GLAD加载之后调用
    glBindTexture(GL_TEXTURE_2D, textureID);  // 绑定

    // 读取图片
    int            width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        cout << "channel" << nrComponents << endl;

        // 创建纹理对象的数据
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 纹理设置
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
        cout << "Texture failed to load at path: " << path << endl;

    stbi_image_free(data);  // 释放图片的内存

    return textureID;
}
