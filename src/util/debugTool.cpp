#include "debugTool.hpp"

void DebugTool::createObjFromHardcode()
{
    // clean
    glDeleteVertexArrays(1, &screenVAO);
    glDeleteBuffers(1, &screenVBO);
    glDeleteBuffers(1, &screenEBO);

    // VBO
    glGenBuffers(1, &screenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, screenVertices.size() * sizeof(GLfloat), screenVertices.data(),
                 GL_STATIC_DRAW);
    // VAO
    glGenVertexArrays(1, &screenVAO);
    glBindVertexArray(screenVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // EBO

    glGenBuffers(1, &screenEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screenEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, screenVerticesIdx.size() * sizeof(GLuint),
                 screenVerticesIdx.data(), GL_STATIC_DRAW);

    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

DebugTool::DebugTool()
{
    createObjFromHardcode();
    screenShader.reset(new Shader(screenShaderPath[0].c_str(), screenShaderPath[1].c_str()));
}

DebugTool::~DebugTool() {}

GLuint DebugTool::getScreenVAO() const
{
    return screenVAO;
}

void DebugTool::renderTextureToScreen(const GLuint textureToShow) const
{
    if (!screenShader)
    {
        throw std::runtime_error("DebugTool::renderTextureToScreen: screenShader is nullptr");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);  // 清除颜色缓冲

    // 绘制屏幕几何对象
    glBindVertexArray(screenVAO);
    screenShader->use();
    glBindTexture(GL_TEXTURE_2D, textureToShow);
    screenShader->setParameter("screenTexture", 0);
    glDisable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // 解绑
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    // 恢复深度测试
    glEnable(GL_DEPTH_TEST);
}
