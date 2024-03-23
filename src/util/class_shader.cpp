#include "class_shader.hpp"

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    // 1. 从文件路径中获取顶点/片元着色器源码
    //---------------------------------------------------------------------
    std::string   vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    // 保证ifstream对象可以抛出异常：
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        // 打开文件
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // 读取数据至StringStream
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // 关闭文件处理器
        vShaderFile.close();
        fShaderFile.close();
        // 转换数据流到String
        vertexCode   = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. 编译着色器程序
    //---------------------------------------------------------------------
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    // 设置着色器的源码
    glShaderSource(vs, 1, &vShaderCode, NULL);
    glShaderSource(fs, 1, &fShaderCode, NULL);
    glCompileShader(vs);
    glCompileShader(fs);
    // 检查着色器编译情况
    Shader::checkShaderCompiling(vs);
    Shader::checkShaderCompiling(fs);

    // 着色器程序
    Shader::ID = glCreateProgram();
    glAttachShader(ID, vs);
    glAttachShader(ID, fs);
    glLinkProgram(ID);
    // 检查链接情况
    checkShaderProgramCompiling(ID);
    // 删除已经不需要的着色器源码
    glDeleteShader(vs);
    glDeleteShader(fs);

    this->use();
}

Shader::~Shader()
{
    glDeleteProgram(this->ID);
}

void Shader::use()
{
    glUseProgram(this->ID);
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec3(const std::string& name, vec3 value) const
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, value_ptr(value));
}

void Shader::setMat4(const std::string& name, mat4 value) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE,
                       value_ptr(value));
}

void Shader::checkShaderCompiling(GLuint shader)
{
    int  success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    else { std::cout << "Shader Compile success!\n" << std::endl; }
}

void Shader::checkShaderProgramCompiling(GLuint shaderProgram)
{
    int  success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    else { std::cout << "Shader Program Compile success!\n" << std::endl; }
}