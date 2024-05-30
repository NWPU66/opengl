#pragma once
#include "glad/glad.h"
#include "util/class_shader.hpp"
#include <memory>
#include <string>
#include <vector>

// 屏幕几何数据的硬编码
std::vector<GLfloat> screenVertices = {
    // 位置               // 纹理坐标
    -1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  // 左上
    1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // 右上
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 左下
    1.0f,  -1.0f, 0.0f, 1.0f, 0.0f   // 右下
};
std::vector<GLuint> screenVerticesIdx = {
    0, 2, 1,  // 第一个三角形
    1, 2, 3   // 第二个三角形
};
// screenShader路径
std::vector<std::string> screenShaderPath = {"./shader/stdScreenShader.vs.glsl",
                                             "./shader/stdScreenShader.fs.glsl"};

class DebugTool {
private:
    GLuint                  screenVAO, screenVBO, screenEBO;
    std::unique_ptr<Shader> screenShader;

    /// @brief 从一组顶点的硬编码创建几何体
    void createObjFromHardcode();

public:
    DebugTool();
    ~DebugTool();

    /// @brief
    struct ShaderAdjustmentPropertys
    {
        GLfloat uvScaling, uvOffset;
    };

    // get method
    GLuint getScreenVAO() const;

    /// @brief 将指定texture绘制到屏幕上
    void renderTextureToScreen(const GLuint textureToShow) const;
};