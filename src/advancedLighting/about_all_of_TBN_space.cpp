#include "glm/geometric.hpp"
#include "glm/matrix.hpp"
#include <cstdint>
#include <cstdlib>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>

static const std::string model_path = "./ico_sphere/ico_sphere.obj";

struct Mesh
{
    std::vector<glm::vec3> position;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec2> texcoord;
    std::vector<glm::vec3> tangent;
    std::vector<glm::vec3> bitangent;
    unsigned int           num_vertices;

    // global
    std::vector<glm::vec3> global_position;
    std::vector<glm::vec3> global_normal;
    std::vector<glm::vec3> global_tangent;
    std::vector<glm::vec3> global_bitangent;
    std::vector<glm::mat3> TBN;
    std::vector<glm::mat3> inverse_TBN;

    // tangent
    std::vector<glm::vec3> tangent_position;
    std::vector<glm::vec3> tangent_normal;
    std::vector<glm::vec3> tangent_tangent;
    std::vector<glm::vec3> tangent_bitangent;

    explicit Mesh(const aiMesh* const _mesh, const bool print_info = false)
        : num_vertices(_mesh->mNumVertices)
    {
        position.reserve(num_vertices);
        normal.reserve(num_vertices);
        texcoord.reserve(num_vertices);
        tangent.reserve(num_vertices);
        bitangent.reserve(num_vertices);

        // global
        global_position.reserve(num_vertices);
        global_normal.reserve(num_vertices);
        global_tangent.reserve(num_vertices);
        global_bitangent.reserve(num_vertices);
        TBN.reserve(num_vertices);
        inverse_TBN.reserve(num_vertices);

        // tangent
        tangent_position.reserve(num_vertices);
        tangent_normal.reserve(num_vertices);
        tangent_tangent.reserve(num_vertices);
        tangent_bitangent.reserve(num_vertices);

        for (int i = 0; i < num_vertices; ++i)
        {
            position.emplace_back(_mesh->mVertices[i].x, _mesh->mVertices[i].y,
                                  _mesh->mVertices[i].z);
            normal.emplace_back(_mesh->mNormals[i].x, _mesh->mNormals[i].y, _mesh->mNormals[i].z);
            texcoord.emplace_back(_mesh->mTextureCoords[0][i].x, _mesh->mTextureCoords[0][i].y);
            tangent.emplace_back(_mesh->mTangents[i].x, _mesh->mTangents[i].y,
                                 _mesh->mTangents[i].z);
            bitangent.emplace_back(_mesh->mBitangents[i].x, _mesh->mBitangents[i].y,
                                   _mesh->mBitangents[i].z);
        }

        if (print_info) { print_vertex_info(); }
    }

    void print_vertex_info()
    {
        for (int i = 0; i < num_vertices; ++i)
        {
            std::cout << "vertex[" << ((i < 10) ? "0" : "") << i << "]";
            // 打印位置坐标
            std::cout << " position= " << position[i].x << ", " << position[i].y << ", "
                      << position[i].z;
            // 打印法线
            std::cout << " normal= " << normal[i].x << ", " << normal[i].y << ", " << normal[i].z;
            // 打印纹理坐标
            std::cout << " texcoord= " << texcoord[i].x << ", " << texcoord[i].y;
            // 打印切线和副切线
            std::cout << " tangent= " << tangent[i].x << ", " << tangent[i].y << ", "
                      << tangent[i].z;
            std::cout << " bitangent= " << bitangent[i].x << ", " << bitangent[i].y << ", "
                      << bitangent[i].z;
            std::cout << std::endl;
        }
        std::cout << "\n\n\n";
    }
};

int main(int argc, char** argv)
{
    // ANCHOR -  import model
    Assimp::Importer import;
    const aiScene*   scene = import.ReadFile(model_path, aiProcessPreset_TargetRealtime_MaxQuality);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return EXIT_FAILURE;
    }

    // ANCHOR - 将ico_sphere放到场景中
    auto      mesh_ptr = std::make_shared<Mesh>(scene->mMeshes[0], false);
    glm::mat4 model(1);
    for (int i = 0; i < mesh_ptr->num_vertices; ++i)
    {
        // globbal参数
        mesh_ptr->global_position[i] = glm::vec3(model * glm::vec4(mesh_ptr->position[i], 1));
        mesh_ptr->global_normal[i]   = glm::normalize(
            glm::vec3(glm::transpose(glm::inverse(model)) * glm::vec4(mesh_ptr->normal[i], 0)));
        mesh_ptr->global_tangent[i] = glm::normalize(
            glm::vec3(glm::transpose(glm::inverse(model)) * glm::vec4(mesh_ptr->tangent[i], 0)));
        mesh_ptr->global_bitangent[i] = glm::normalize(
            glm::vec3(glm::transpose(glm::inverse(model)) * glm::vec4(mesh_ptr->bitangent[i], 0)));
        mesh_ptr->TBN[i] = glm::mat3(mesh_ptr->global_tangent[i], mesh_ptr->global_bitangent[i],
                                     mesh_ptr->global_normal[i]);
        mesh_ptr->inverse_TBN[i] = glm::inverse(mesh_ptr->TBN[i]);

        // tangent参数
        mesh_ptr->tangent_position[i]  = mesh_ptr->inverse_TBN[i] * mesh_ptr->global_position[i];
        mesh_ptr->tangent_normal[i]    = mesh_ptr->inverse_TBN[i] * mesh_ptr->global_normal[i];
        mesh_ptr->tangent_tangent[i]   = mesh_ptr->inverse_TBN[i] * mesh_ptr->global_tangent[i];
        mesh_ptr->tangent_bitangent[i] = mesh_ptr->inverse_TBN[i] * mesh_ptr->global_bitangent[i];

        // print data
        std::cout << "vertex[" << ((i < 10) ? "0" : "") << i << "]";
        std::cout << " global_position= " << mesh_ptr->global_position[i].x << ", "
                  << mesh_ptr->global_position[i].y << ", " << mesh_ptr->global_position[i].z;
        std::cout << " tangent_position= " << mesh_ptr->tangent_position[i].x << ", "
                  << mesh_ptr->tangent_position[i].y << ", " << mesh_ptr->tangent_position[i].z;
        std::cout << " texcoord= " << mesh_ptr->texcoord[i].x << ", " << mesh_ptr->texcoord[i].y;
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}