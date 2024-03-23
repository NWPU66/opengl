#ifndef SHADER_H

#define SHADER_H
#include <glad/glad.h>

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>

using namespace glm;

class Shader
{
public:
    unsigned int ID; // 程序ID

    // 构造函数和析构函数
    Shader(const char *vertexPath, const char *fragmentPath);
    ~Shader();

    // 实用成员函数
    void use(); // 使用/激活程序
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, vec3 value) const;
    void setMat4(const std::string &name, mat4 value) const;

private:
    void checkShaderCompiling(GLuint shader);
    void checkShaderProgramCompiling(GLuint shaderProgram);
};

#endif