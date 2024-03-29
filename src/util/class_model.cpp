#include "class_model.hpp"

Model::Model(const string path)
{
    this->loadModel(path);
}

Model::~Model() {}

void Model::Draw(Shader* shader)
{
    for (int i = 0; i < meshes.size(); i++)
    {
        meshes[i].Draw(shader);
    }
}
