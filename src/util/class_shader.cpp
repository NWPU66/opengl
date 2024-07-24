#include "class_shader.hpp"

/**
 * C++中的Shader构造函数从文件中读取并编译顶点和片段着色器源代码，创建着色器对象，将它们链接到着色器程序中，然后激活着色器程序。
 *
 * @param vertexPath “Shader::Shader”构造函数中的“vertexPath”参数是一个 C
 * 风格的字符串，表示顶点着色器源代码文件的文件路径。该文件包含定义图形管道中顶点着色器行为的
 * GLSL 代码。构造函数读取该文件的内容
 * @param fragmentPath “Shader::Shader”构造函数中的“fragmentPath”参数是一个
 * const char
 * 指针，表示片段着色器源代码文件的文件路径。该文件包含定义图形管道中片段着色器行为的
 * GLSL 代码。构造函数读取该文件的内容
 */
Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    /**FIXME - 关于带有默认值的函数
     * 只能在定义或声明时指出默认参数，同时指定会报错。
     * 如果函数有声明就会以声明的为准，定义时指出的默认参数就会无效，所以最好在声明时指定。
     */
    bool useGeometryShader = (geometryPath != nullptr);
    std::cout << "useGeometryShader: " << useGeometryShader << std::endl;

    // 1. 从文件路径中获取顶点/片元着色器源码
    //---------------------------------------------------------------------
    std::string   vertexCode, fragmentCode, geometryCode;
    std::ifstream vShaderFile, fShaderFile, gShaderFile;
    // 保证ifstream对象可以抛出异常：
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

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
        if (geometryPath != nullptr)
        {
            gShaderFile.open(geometryPath);
            std::stringstream gShaderStream;
            gShaderStream << gShaderFile.rdbuf();
            gShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    }
    catch (std::ifstream::failure e)
    {
        // std::cout << vertexPath << std::endl;
        // std::cout << fragmentPath << std::endl;
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. 编译着色器程序
    //---------------------------------------------------------------------
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
    // 设置着色器的源码
    glShaderSource(vs, 1, &vShaderCode, NULL);
    glShaderSource(fs, 1, &fShaderCode, NULL);
    glCompileShader(vs);
    glCompileShader(fs);
    // 检查着色器编译情况
    Shader::checkShaderCompiling(vs);
    Shader::checkShaderCompiling(fs);
    if (geometryPath != nullptr)
    {
        const char* gShaderCode = geometryCode.c_str();
        glShaderSource(gs, 1, &gShaderCode, NULL);
        glCompileShader(gs);
        Shader::checkShaderCompiling(gs);
    }

    // 着色器程序
    Shader::ID = glCreateProgram();
    glAttachShader(ID, vs);
    glAttachShader(ID, fs);
    if (geometryPath != nullptr) { glAttachShader(ID, gs); }
    glLinkProgram(ID);
    // 检查链接情况
    checkShaderProgramCompiling(ID);
    // 删除已经不需要的着色器源码
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (geometryPath != nullptr) { glDeleteShader(gs); }

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

void Shader::setParameter(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setParameter(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setParameter(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setParameter(const std::string& name, vec3 value) const
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, value_ptr(value));
}

void Shader::setParameter(const std::string& name, vec2 value) const
{
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, value_ptr(value));
}

void Shader::setParameter(const std::string& name, mat4 value) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, value_ptr(value));
}

void Shader::setParameter(const std::string& name, vec4 value) const
{
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, value_ptr(value));
}

/**
 * 该函数设置着色器中材质的环境光、漫反射、镜面反射和光泽度值的参数。
 *
 * @param name “name”参数是一个字符串，表示着色器程序中设置的参数名称。
 * @param value 环境光、漫反射、镜面反射、光泽度
 */
// void Shader::setParameter(const std::string& name, Material value) const
// {
//     Shader::setParameter(name + ".ambient", value.ambient);
//     Shader::setParameter(name + ".diffuse", value.diffuse);
//     Shader::setParameter(name + ".specular", value.specular);
//     Shader::setParameter(name + ".shininess", value.shininess);
// }

/**
 * 该函数为着色器程序中的 Light 对象设置参数。
 *
 * @param name “name”参数是一个字符串，表示为着色器设置的参数名称。
 * @param value 位置、强度、漫反射、环境光、镜面反射
 */
// void Shader::setParameter(const std::string& name, Light value) const
// {
//     Shader::setParameter(name + ".type", value.type);  // 灯光类型

//     // 基本属性
//     Shader::setParameter(name + ".position", value.position);
//     Shader::setParameter(name + ".intensity", value.intensity);
//     Shader::setParameter(name + ".diffuse", value.diffuse);
//     Shader::setParameter(name + ".ambient", value.ambient);
//     Shader::setParameter(name + ".specular", value.specular);
//     Shader::setParameter(name + ".dropOffFac", value.dropOffFac);

//     // 方向光和聚光灯设置
//     Shader::setParameter(name + ".lightDir", value.lightDir);
//     Shader::setParameter(name + ".innerCutOff", value.innerCutOff);
//     Shader::setParameter(name + ".outerCutOff", value.outerCutOff);
// }

/**
 * 函数“checkShaderCompiling”检查着色器是否已成功编译，如果编译失败则打印错误消息。
 *
 * @param shader
 * checkShaderCompiling函数中的shader参数为GLuint类型，表示需要检查编译状态的shader对象的句柄。
 */
void Shader::checkShaderCompiling(GLuint shader)
{
    int  success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED" << infoLog << std::endl;
    }
    else { std::cout << "Shader Compile success!" << std::endl; }
}

/**
 * 函数“checkShaderProgramCompiling”检查着色器程序是否已成功编译，如果失败则打印错误消息。
 *
 * @param shaderProgram `shaderProgram`
 * 参数是要检查编译是否成功的着色器程序对象的
 * ID。该函数检索着色器程序的链接状态，并在编译失败时打印出任何相关的错误信息。
 */
void Shader::checkShaderProgramCompiling(GLuint shaderProgram)
{
    int  success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED" << infoLog << std::endl;
    }
    else { std::cout << "Shader Program Compile success!" << std::endl; }
}
