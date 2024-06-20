#include "class_mesh.hpp"

/**
 * Mesh 构造函数使用提供的顶点、索引和纹理初始化 Mesh 对象，然后调用 setupMesh
 * 函数。
 *
 * @param Vertex
 * 包含有关网格中单个顶点的信息的结构，例如其位置、法线和纹理坐标。
 * @param indices
 * “Mesh”构造函数中的“indices”参数是一个无符号整数向量，表示网格中顶点的索引。这些索引用于定义顶点的连接性，指定它们如何连接以形成网格中的三角形或其他形状。
 * @param textures
 * “Mesh”构造函数中的“textures”参数是“Texture”对象的“std::vector”。它用于存储与网格相关的纹理。
 */
Mesh::Mesh(std::vector<Vertex>&       vertices,
           std::vector<unsigned int>& indices,
           std::vector<Texture>&      textures)
{
    this->vertices = vertices;
    this->indices  = indices;
    this->textures = textures;

    this->setupMesh();
}

Mesh::~Mesh() {}

/// @brief
/// 函数通过生成顶点数组和缓冲区、绑定它们、用数据填充它们、指定数据解释和取消绑定来创建和设置网格。
void Mesh::setupMesh()
{
    // 创建顶点对象
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // 绑定
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // 生成顶点数据，并填充
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(),
                 GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(),
                 GL_STATIC_DRAW);
    // 设置VBO中数据的解读方式
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Tangent));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Bitangent));
    glEnableVertexAttribArray(4);
    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::Draw(Shader* shader, GLuint instanceNum)
{
    GLuint diffuseNr  = 1;
    GLuint specularNr = 1;
    GLuint normalNr   = 1;
    for (int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);  // 在绑定纹理之前激活相应的纹理单元
        glBindTexture(GL_TEXTURE_2D,
                      textures[i].id);  // FIXME - 启动槽之后记得要绑定纹理啊

        string number;  //  获取纹理序号（diffuse_textureN 中的 N）
        string name = textures[i].type;
        if (name == "texture_diffuse")
            number = to_string(diffuseNr++);
        else if (name == "texture_specular")
            number = to_string(specularNr++);
        else if (name == "texture_normal")
            number = to_string(normalNr++);

        shader->setParameter("material." + name + number, i);
    }

    // 绘制
    glBindVertexArray(VAO);
    // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instanceNum);
    glBindVertexArray(0);

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
}

/// @brief 设置单个Mesh的实例数组
/// @param instanceVBO 实例数组的缓冲区索引
/// @param vaoSlot 要绑定的顶点属性指针的槽位
/// @param vecSize 实例数组每个元素的向量大小
/// @param updateFruq 更新频率，0表示每次绘制时更新，2表示每2个实例更新一次属性
void Mesh::SetInstanceArray(GLuint      instanceVBO,
                            GLuint      vaoSlot,
                            GLuint      vecSize,
                            GLsizei     stride,
                            const void* offset,
                            GLuint      updateFruq)
{
    glBindVertexArray(VAO);
    // 设置它的顶点属性指针，并启用顶点属性
    glVertexAttribPointer(vaoSlot, vecSize, GL_FLOAT, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(vaoSlot);
    glVertexAttribDivisor(vaoSlot, updateFruq);
    // NOTE - 这里不绑定/解绑VBO，因为整个model下的所有Mesh都共享一个instanceVBO
    // FIXME - 忘记链接vao了
    glBindVertexArray(0);
}
