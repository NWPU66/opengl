#ifndef SHADER_H

#define SHADER_H
#include <glad/glad.h>

#include "structure.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>

using namespace glm;

class Shader {
public:
    unsigned int ID;  // 程序ID

    // 构造函数和析构函数
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
    ~Shader();

    // 实用成员函数
    void use();  // 使用/激活程序
    void setParameter(const std::string& name, bool value) const;
    void setParameter(const std::string& name, int value) const;
    void setParameter(const std::string& name, float value) const;
    void setParameter(const std::string& name, vec3 value) const;
    void setParameter(const std::string& name, vec2 value) const;
    void setParameter(const std::string& name, mat4 value) const;
    void setParameter(const std::string& name, Material value) const;
    void setParameter(const std::string& name, Light value) const;
    // const表示该函数为常量成员函数，即不能修改成员变量的值，是只读函数。
    // 并且只读函数中只能调用只读函数

private:
    void checkShaderCompiling(GLuint shader);
    void checkShaderProgramCompiling(GLuint shaderProgram);
};

#endif