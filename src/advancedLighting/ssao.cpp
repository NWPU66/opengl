#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include <array>
#include <glad/glad.h>
#include <limits>
#include <random>
#include <tuple>
// GLAD first
#define STB_IMAGE_IMPLEMENTATION
#include "util/class_camera.hpp"
#include "util/class_model.hpp"
#include "util/class_shader.hpp"
#include "util/debugTool.hpp"
#include "util/lightGroup.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using namespace glm;
using namespace std;

/**NOTE - 全局变量、摄像机、全局时钟以及函数
 */
const GLint CAMERA_WIDTH = 1920;
const GLint CAMERA_HEIGH = 1080;
const float cameraAspect = static_cast<float>(CAMERA_WIDTH) / static_cast<float>(CAMERA_HEIGH);
Camera*     camera       = new Camera(vec3(0.0F, 0.0F, 2.0F), vec3(0.0F, 1.0F, 0.0F), -90.0F, 0.0F);
float       mouseLastX = 0.0f, mouseLastY = 0.0f;  // 记录鼠标的位置
float       lastFrame = 0.0f, deltaTime = 0.0f;    // 全局时钟

void   framebuffer_size_callback(GLFWwindow* window, int w, int h);
void   mouse_callback(GLFWwindow* window, double xpos, double ypos);
void   scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void   processInput(GLFWwindow* window);
int    initGLFWWindow(GLFWwindow*& window);
GLuint createImageObjrct(const char* imagePath,
                         const bool  autoGammaCorrection = true,
                         const bool  flip_texture        = true);
GLuint createSkyboxTexture(const char* imageFolder, const bool autoGammaCorrection = true);
void   createFBO(GLuint& fbo, GLuint& texAttachment, GLuint& rbo, const char* hint = "null");
void   createObjFromHardcode(GLuint&         vao,
                             GLuint&         vbo,
                             GLuint&         ebo,
                             vector<GLfloat> vertices,
                             vector<GLuint>  vertexIdx = {});
void   renderTextureToScreen(const GLuint screenVAO,
                             const GLuint textureToShow,
                             Shader&      screenShader);

int main(int /*argc*/, char** /*argv*/)
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (initGLFWWindow(window) == 0) return -1;

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);    // 启用深度缓冲
    glDepthFunc(GL_LEQUAL);     // 修改深度测试的标准
    glEnable(GL_STENCIL_TEST);  // 启用模板缓冲
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glEnable(GL_BLEND);                                 // 启用混合
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数
    glEnable(GL_CULL_FACE);                             // 启用面剔除
    glClearColor(0.2F, 0.3F, 0.3F, 1.0F);               // 设置清空颜色
    glDisable(GL_MULTISAMPLE);                          // 启用多重采样
    /**NOTE - 文档中GL_MULTISAMPLE时默认启动的（true）
     */
    glEnable(GL_FRAMEBUFFER_SRGB);  // 自动Gamme矫正

    /**NOTE - 模型和着色器、纹理
     */
    Model  box("./box/box.obj");
    Model  plane("./plane/plane.obj");
    Model  sphere("./sphere/sphere.obj");
    Model  nanosuit("./nanosuit/nanosuit.obj");
    Model  testScene("./testScene/scene.obj");
    Shader phongShader("./shader/stdVerShader.vs.glsl",
                       "./shader/simpleWritePhongLighting.fs.glsl");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl", "./shader/skyboxShader.fs.glsl");
    Shader lightObjShader("./shader/stdVerShader.vs.glsl", "./shader/stdPureColor.fs.glsl");
    GLuint cubeTexture = createSkyboxTexture("./texture/", true);
    GLuint woodTexture = createImageObjrct("./texture/wood.jpg", true);

    /**NOTE - 灯光组
     */
    LightGroup lightGroup;
    lightGroup.addLight(Light(0, vec3(1, 1, 1), 1, vec3(1, 2.5, 1)));
    lightGroup.addLight(Light(1, vec3(1, 1, 1), 1, vec3(0, 0, 0), vec3(1, -1, 1)));
    lightGroup.addLight(Light(2, vec3(1, 1, 1), 1, vec3(0, 3, -2), vec3(0, -1, 0)));
    lightGroup.createLightUniformBuffer();
    lightGroup.bindingUniformBuffer(0);

    /**NOTE - ScreenTextureObject for debug
     */
    DebugTool debugTool;

    /**NOTE - 创建G缓冲
     */

    // 渲染循环
    while (glfwWindowShouldClose(window) == 0)
    {
        // 更新时钟、摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window);

        // SECTION - 渲染循环
        /**NOTE - 清空屏幕
         */
        glViewport(0, 0, CAMERA_WIDTH, CAMERA_HEIGH);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);  // 清除颜色、深度和模板缓冲

        /**NOTE - 更新视图变换
         */
        mat4 view       = camera->GetViewMatrix();
        mat4 projection = perspective(radians(camera->Zoom), cameraAspect, 0.1f, 100.0f);

        /**NOTE - 渲染
         */
        phongShader.use();
        phongShader.setParameter("model", mat4(1));
        phongShader.setParameter("view", view);
        phongShader.setParameter("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        phongShader.setParameter("skybox", 0);
        phongShader.setParameter("cameraPos", camera->Position);
        phongShader.setParameter("albedoMap", vec3(1));
        testScene.Draw(&phongShader, 1, GL_TEXTURE1);

        /**NOTE - 渲染灯光
         */
        for (const auto& light : lightGroup.getLights())
        {
            if (light.getLightType() != 1)  // 日光不渲染实体
            {
                lightObjShader.use();
                lightObjShader.setParameter(
                    "model", scale(translate(mat4(1), light.getPostion()), vec3(0.1)));
                lightObjShader.setParameter("view", view);
                lightObjShader.setParameter("projection", projection);
                lightObjShader.setParameter("lightColor", light.getColor());
                sphere.Draw(&lightObjShader);
                // FIXME - 常量对象只能调用它的常函数
            }
        }

        /**NOTE - 最后渲染天空盒
         */
        glFrontFace(GL_CW);  // 把顺时针的面设置为“正面”。
        skyboxShader.use();
        skyboxShader.setParameter("view",
                                  mat4(mat3(view)));  // 除去位移，相当于锁头
        skyboxShader.setParameter("projection", projection);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        skyboxShader.setParameter("skybox", 0);
        box.Draw(&skyboxShader);
        glFrontFace(GL_CCW);
        // ~SECTION

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // 释放内存
    glfwTerminate();
    delete camera;
    return 0;
}

/// @brief 函数“framebuffer_size_callback”根据窗口大小设置视口尺寸。
/// @param window `window` 参数是指向触发回调函数的 GLFW 窗口的指针。
/// @param w
/// “framebuffer_size_callback”函数中的“w”参数表示帧缓冲区的宽度，即OpenGL将渲染图形的区域的大小。
/// @param h
/// “framebuffer_size_callback”函数中的参数“h”表示帧缓冲区的高度（以像素为单位）。它用于设置在OpenGL
/// 上下文中渲染的视口高度。
void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, w, h);
}

/// @brief 函数“mouse_callback”根据 GLFW 窗口中的鼠标移动更新相机的方向。
/// @param window “window”参数是指向接收鼠标输入的
/// GLFW窗口的指针。它用于标识鼠标事件发生的窗口。
/// @param xpos mouse_callback 函数中的 xpos 参数表示鼠标光标位置的当前 x坐标。
/// @param ypos 'mouse_callback` 函数中的 `ypos`参数表示鼠标光标在窗口内的当前
/// y坐标。它是一个双精度值，表示触发回调函数时鼠标光标的垂直位置。
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    camera->ProcessMouseMovement(xpos - mouseLastX, ypos - mouseLastY, true);
    mouseLastX = xpos;
    mouseLastY = ypos;
}

/// @brief 函数“scroll_callback”处理鼠标滚动输入以调整相机位置。
/// @param window `GLFWwindow* window`参数是指向接收滚动输入的窗口的指针。
/// @param xoffset `xoffset` 参数表示水平滚动偏移。
/// @param yoffset
/// 'scroll_callback`函数中的`yoffset`参数表示鼠标滚轮的垂直滚动偏移量。正值表示向上滚动，负值表示向下滚动。
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera->ProcessMouseScroll(yoffset);
}

/// @brief C++ 中的函数“processInput”处理来自
/// GLFW窗口的用户输入，以控制相机的移动和速度。
/// @param window processInput 函数中的 window 参数是一个指向
/// GLFWwindow对象的指针。该对象表示应用程序中用于渲染图形和处理用户输入的窗口。该函数使用此参数来检查按键并更新相机的移动和速度
void processInput(GLFWwindow* window)
{
    // 当Esc按下时，窗口关闭
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, 1);

    // 按下Shift时，飞行加速
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera->SpeedUp(true);
    else
        camera->SpeedUp(false);

    // 处理摄像机移动
    GLint direction[6] = {0, 0, 0, 0, 0, 0};
    direction[0]       = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) ? 1 : 0;
    direction[1]       = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) ? 1 : 0;
    direction[2]       = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) ? 1 : 0;
    direction[3]       = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) ? 1 : 0;
    direction[4]       = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) ? 1 : 0;
    direction[5]       = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) ? 1 : 0;
    camera->ProcessKeyboard(direction, deltaTime);
}

/// @brief 该函数使用指定的 OpenGL 上下文版本和回调初始化 GLFW 窗口。
/// @param window `initGLFWWindow` 函数中的 `window` 参数是指向GLFWwindow
/// 对象的指针的引用。此函数初始化GLFW，使用指定参数创建窗口，设置回调，隐藏光标，并初始化
/// GLAD 以进行OpenGL 加载。如果成功，则返回
/// @return
/// 函数“initGLFWWindow”返回一个整数值。如果GLFW窗口初始化成功则返回1，如果创建窗口或加载GLAD失败则返回0。
int initGLFWWindow(GLFWwindow*& window)
{
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfwWindowHint(GLFW_SAMPLES, 4);  // 多重采样缓冲

    // 创建窗口
    window = glfwCreateWindow(CAMERA_WIDTH, CAMERA_HEIGH, "Window", NULL, NULL);
    if (window == nullptr)
    {
        std::cout << "failed to create a window!" << std::endl;
        glfwTerminate();
        return 0;
    }
    glfwMakeContextCurrent(window);
    // 注册回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // 隐藏光标
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "failed to load GLAD!" << std::endl;
        glfwTerminate();
        return 0;
    }
    return 1;
}

/// @brief 函数 createImageObject 加载图像文件，在
/// OpenGL中创建纹理对象，设置纹理参数，生成纹理，并返回纹理 ID。
/// @param imagePath “createImageObject”函数中的“imagePath”参数是一个指向
/// C风格字符串的指针，该字符串表示要加载并从中创建纹理对象的图像文件的路径。该路径应指向文件系统上图像文件的位置。
/// @return 函数 createImageObjrct 返回一个整数值，它是在
/// OpenGL中加载和创建的图像的纹理 ID。
GLuint
createImageObjrct(const char* imagePath, const bool autoGammaCorrection, const bool flip_texture)
{
    // 读取
    GLint width, height, nrChannels;
    stbi_set_flip_vertically_on_load(flip_texture);  // 加载图片时翻转y轴
    GLubyte* data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
    // 创建纹理对象
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // 设置纹理属性
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 生成纹理
    if (data)
    {
        if (nrChannels == 3)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, (autoGammaCorrection) ? GL_SRGB : GL_RGB, width, height,
                         0, GL_RGB, GL_UNSIGNED_BYTE, data);
            //  设置为GL_SRGB时，OpenGL回自动对图片进行重校
            // FIXME - 注意，对于在线性空间下创建的纹理，如法线贴图，不能设置SRGB重校。
        }
        else  // nrChannels == 4
        {
            glTexImage2D(GL_TEXTURE_2D, 0, (autoGammaCorrection) ? GL_SRGB_ALPHA : GL_SRGB_ALPHA,
                         width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
        return -1;
    }
    stbi_image_free(data);            // 释放图像的内存，无论有没有data都释放
    glBindTexture(GL_TEXTURE_2D, 0);  // 解绑
    return texture;
}

/// @brief 加载一个天空盒贴图
/// @param imageFolder 纹理集所在文件夹路径
/// @return 函数 createImageObjrct
/// 返回一个整数值，它是在OpenGL中加载和创建的图像的纹理 ID。
GLuint createSkyboxTexture(const char* imageFolder, const bool autoGammaCorrection)
{
    GLuint cubeTexture;
    glGenTextures(1, &cubeTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
    string basePath            = string(imageFolder);
    string cubeTextureNames[6] = {"right.jpg",  "left.jpg",  "top.jpg",
                                  "bottom.jpg", "front.jpg", "back.jpg"};
    for (int i = 0; i < 6; i++)
    {
        string cubeTexturePath = basePath + cubeTextureNames[i];
        int    width, height, nrChannels;
        stbi_set_flip_vertically_on_load(false);  // 加载图片时翻转y轴
        GLubyte* data = stbi_load(cubeTexturePath.c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                         (autoGammaCorrection) ? GL_SRGB : GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data);
            // 设置纹理属性
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        else { std::cout << "Failed to load texture: " << cubeTexturePath << std::endl; }
        stbi_image_free(data);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);  // 解绑
    return cubeTexture;
}

/// @brief 创建一个FBO（帧缓冲对象）
/// @param hint 如果hint为"ms"，则使用多重采样
void createFBO(GLuint& fbo, GLuint& texAttachment, GLuint& rbo, const char* hint)
{
    bool useMutiSampled = (strcmp(hint, "ms") == 0);
    // FIXME - 使用strcmp()的时候，不可以出空指针

    // 创建一个帧缓冲
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    //  创建一个纹理附件
    glGenTextures(1, &texAttachment);
    if (useMutiSampled)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texAttachment);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, CAMERA_WIDTH, CAMERA_HEIGH,
                                GL_TRUE);
        // 如果最后一个参数为GL_TRUE，图像将会对每个纹素使用相同的样本位置以及相同数量的子采样点个数。
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                               texAttachment, 0);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, texAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CAMERA_WIDTH, CAMERA_HEIGH, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texAttachment,
                               0);
    }

    // 创建一个多重采样渲染缓冲对象
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    if (useMutiSampled)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, CAMERA_WIDTH,
                                         CAMERA_HEIGH);
    }
    else
    {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, CAMERA_WIDTH, CAMERA_HEIGH);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    // FIXME - GL_DEPTH_STENCIL_ATTACHMENT写错了，导致深度缓冲没有初始化成功。

    //  检查帧缓冲状态
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "Framebuffer is  complete!" << endl;
    }
    else { cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl; }

    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // FIXME - 函数写错了，fbo没有解绑。导致默认的fbo为空。
    if (useMutiSampled) { glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0); }
    else { glBindTexture(GL_TEXTURE_2D, 0); }
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

/// @brief 从一组顶点的硬编码创建几何体
void createObjFromHardcode(GLuint&         vao,
                           GLuint&         vbo,
                           GLuint&         ebo,
                           vector<GLfloat> vertices,
                           vector<GLuint>  vertexIdx)
{
    bool useEBO = (vertexIdx.size() > 0);
    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(),
                 GL_STATIC_DRAW);
    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // EBO
    if (useEBO)
    {
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIdx.size() * sizeof(GLuint), vertexIdx.data(),
                     GL_STATIC_DRAW);
    }
    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (useEBO) { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
}

/// @brief 将指定texture绘制到屏幕上
void renderTextureToScreen(const GLuint screenVAO, const GLuint textureToShow, Shader& screenShader)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);  // 清除颜色缓冲

    // 绘制屏幕几何对象
    glBindVertexArray(screenVAO);
    screenShader.use();
    glBindTexture(GL_TEXTURE_2D, textureToShow);
    screenShader.setParameter("screenTexture", 0);
    glDisable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    // 解绑
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    // 恢复深度测试
    glEnable(GL_DEPTH_TEST);
}

/**REVIEW - SSAO
我们已经在前面的基础教程中简单介绍到了这部分内容：环境光照(Ambient Lighting)。
环境光照是我们加入场景总体光照中的一个固定光照常量，它被用来模拟光的散射(Scattering)。
在现实中，光线会以任意方向散射，它的强度是会一直改变的，所以间接被照到的那部分场景
也应该有变化的强度，而不是一成不变的环境光。其中一种间接光照的模拟叫做环境光遮蔽
(Ambient Occlusion)，它的原理是通过将褶皱、孔洞和非常靠近的墙面变暗的方法近似模拟
出间接光照。这些区域很大程度上是被周围的几何体遮蔽的，光线会很难流失，所以这些地方
看起来会更暗一些。站起来看一看你房间的拐角或者是褶皱，是不是这些地方会看起来有一点暗？

下面这幅图展示了在使用和不使用SSAO时场景的不同。
特别注意对比褶皱部分，你会发现(环境)光被遮蔽了许多：
![](https://learnopengl-cn.github.io/img/05/09/ssao_example.png)

尽管这不是一个非常明显的效果，启用SSAO的图像确实给我们更真实的感觉，
这些小的遮蔽细节给整个场景带来了更强的深度感。

环境光遮蔽这一技术会带来很大的性能开销，因为它还需要考虑周围的几何体。
我们可以对空间中每一点发射大量光线来确定其遮蔽量，但是这在实时运算中会很快变成大问题。
在2007年，Crytek公司发布了一款叫做屏幕空间环境光遮蔽(Screen-Space Ambient Occlusion,
SSAO)的技术，并用在了他们的看家作孤岛危机上。这一技术使用了屏幕空间场景的深度而不是
真实的几何体数据来确定遮蔽量。这一做法相对于真正的环境光遮蔽不但速度快，而且还能获得
很好的效果，使得它成为近似实时环境光遮蔽的标准。

SSAO背后的原理很简单：对于铺屏四边形(Screen-filled Quad)上的每一个片段，我们都会根据
周边深度值计算一个遮蔽因子(Occlusion Factor)。这个遮蔽因子之后会被用来减少或者抵消片段
的环境光照分量。遮蔽因子是通过采集片段周围球型核心(Kernel)的多个深度样本，并和当前片
段深度值对比而得到的。高于片段深度值样本的个数就是我们想要的遮蔽因子。
![](https://learnopengl-cn.github.io/img/05/09/ssao_crysis_circle.png)

上图中在几何体内灰色的深度样本都是高于片段深度值的，他们会增加遮蔽因子；
几何体内样本个数越多，片段获得的环境光照也就越少。

很明显，渲染效果的质量和精度与我们采样的样本数量有直接关系。如果样本数量太低，
渲染的精度会急剧减少，我们会得到一种叫做波纹(Banding)的效果；如果它太高了，
反而会影响性能。我们可以通过引入随机性到采样核心(Sample Kernel)的采样中从而
减少样本的数目。通过随机旋转采样核心，我们能在有限样本数量中得到高质量的结果。
然而这仍然会有一定的麻烦，因为随机性引入了一个很明显的噪声图案，我们将需要通
过模糊结果来修复这一问题。下面这幅图片(John Chapman的佛像)展示了波纹效果还
有随机性造成的效果：
![](https://learnopengl-cn.github.io/img/05/09/ssao_banding_noise.jpg)

你可以看到，尽管我们在低样本数的情况下得到了很明显的波纹效果，
引入随机性之后这些波纹效果就完全消失了。

Crytek公司开发的SSAO技术会产生一种特殊的视觉风格。因为使用的采样核心是一个球体，
它导致平整的墙面也会显得灰蒙蒙的，因为核心中一半的样本都会在墙这个几何体上。
下面这幅图展示了孤岛危机的SSAO，它清晰地展示了这种灰蒙蒙的感觉：
![](https://learnopengl-cn.github.io/img/05/09/ssao_crysis.jpg)

由于这个原因，我们将不会使用球体的采样核心，而使用一个沿着表面法向量的半球体采样核心。
![](https://learnopengl-cn.github.io/img/05/09/ssao_hemisphere.png)

通过在法向半球体(Normal-oriented Hemisphere)周围采样，
我们将不会考虑到片段底部的几何体.它消除了环境光遮蔽灰蒙蒙的感觉，
从而产生更真实的结果。这个SSAO教程将会基于法向半球法和John Chapman出色的SSAO教程。
[](https://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html)

* NOTE - 样本缓冲
SSAO需要获取几何体的信息，因为我们需要一些方式来确定一个片段的遮蔽因子。
对于每一个片段，我们将需要这些数据：
    逐片段位置向量
    逐片段的法线向量
    逐片段的反射颜色
    采样核心
    用来旋转采样核心的随机旋转矢量

通过使用一个逐片段观察空间位置，我们可以将一个采样半球核心对准片段的观察空间表面法线。
对于每一个核心样本我们会采样线性深度纹理来比较结果。采样核心会根据旋转矢量稍微偏转一点；
我们所获得的遮蔽因子将会之后用来限制最终的环境光照分量。
![](https://learnopengl-cn.github.io/img/05/09/ssao_overview.png)

由于SSAO是一种屏幕空间技巧，我们对铺屏2D四边形上每一个片段计算这一效果；
也就是说我们没有场景中几何体的信息。我们能做的只是渲染几何体数据到屏幕空间纹理中，
我们之后再会将此数据发送到SSAO着色器中，之后我们就能访问到这些几何体数据了。如果你
看了前面一篇教程，你会发现这和延迟渲染很相似。这也就是说SSAO和延迟渲染能完美地兼容，
因为我们已经存位置和法线向量到G缓冲中了。
*/
