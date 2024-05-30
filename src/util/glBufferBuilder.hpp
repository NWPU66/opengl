#pragma once
#include "glad/glad.h"
#include <memory>
#include <string>
#include <vector>

class GlBufferBuilder {
private:
public:
    GlBufferBuilder();
    ~GlBufferBuilder();

    /// @brief 函数 createImageObject 加载图像文件，在
    /// OpenGL中创建纹理对象，设置纹理参数，生成纹理，并返回纹理 ID。
    /// @param imagePath “createImageObject”函数中的“imagePath”参数是一个指向
    /// C风格字符串的指针，该字符串表示要加载并从中创建纹理对象的图像文件的路径。该路径应指向文件系统上图像文件的位置。
    /// @return 函数 createImageObjrct 返回一个整数值，它是在
    /// OpenGL中加载和创建的图像的纹理 ID。
    static void buildImageBO(const char* imagePath);

    /// @brief 加载一个天空盒贴图
    /// @param imageFolder 纹理集所在文件夹路径
    /// @return 函数 createImageObjrct
    /// 返回一个整数值，它是在OpenGL中加载和创建的图像的纹理 ID。
    static void buildCubemapBO(const char* imageFolder);

    /// @brief 创建一个FBO（帧缓冲对象）
    /// @param hint 如果hint为"ms"，则使用多重采样
    static void buildFrameBO(GLuint& fbo, GLuint& texAttachment, GLuint& rbo, const char* hint);

    /// @brief 从一组顶点的硬编码创建几何体
    static void buildVAOFromHardcode(GLuint&              vao,
                                     GLuint&              vbo,
                                     GLuint&              ebo,
                                     std::vector<GLfloat> vertices,
                                     std::vector<GLuint>  vertexIdx);
};