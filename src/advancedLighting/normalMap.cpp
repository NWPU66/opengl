#include <glad/glad.h>
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
const GLint CAMERA_WIDTH = 800;
const GLint CAMERA_HEIGH = 600;
const float cameraAspect = static_cast<float>(CAMERA_WIDTH) / static_cast<float>(CAMERA_HEIGH);
Camera*     camera       = new Camera(vec3(0.0F, 0.0F, 2.0F), vec3(0.0F, 1.0F, 0.0F), -90.0F, 0.0F);
float       mouseLastX = 0.0f, mouseLastY = 0.0f;  // 记录鼠标的位置
float       lastFrame = 0.0f, deltaTime = 0.0f;    // 全局时钟

void   framebuffer_size_callback(GLFWwindow* window, int w, int h);
void   mouse_callback(GLFWwindow* window, double xpos, double ypos);
void   scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void   processInput(GLFWwindow* window);
int    initGLFWWindow(GLFWwindow*& window);
GLuint createImageObjrct(const char* imagePath, const bool autoGammaCorrection = true);
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
    glEnable(GL_DEPTH_TEST);                            // 启用深度缓冲
    glDepthFunc(GL_LEQUAL);                             // 修改深度测试的标准
    glEnable(GL_STENCIL_TEST);                          // 启用模板缓冲
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);       // 设置模板缓冲的操作
    glEnable(GL_BLEND);                                 // 启用混合
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数
    glEnable(GL_CULL_FACE);                             // 启用面剔除
    glClearColor(0.2F, 0.3F, 0.3F, 1.0F);               // 设置清空颜色
    glEnable(GL_MULTISAMPLE);                           // 启用多重采样
    glEnable(GL_FRAMEBUFFER_SRGB);                      // 自动Gamme矫正

    /**NOTE - 模型和着色器、纹理
     */
    Model  box("./box/box.obj");
    Model  plane("./plane/plane.obj");
    Model  sphere("./sphere/sphere.obj");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl", "./shader/skyboxShader.fs.glsl");
    Shader lightObjShader("./shader/stdVerShader.vs.glsl", "./shader/stdPureColor.fs.glsl");
    GLuint brickwallDiffuseTexture = createImageObjrct("./texture/brickwall.jpg", true);
    GLuint brickwallNormalTexture  = createImageObjrct("./texture/brickwall_normal.jpg", false);
    GLuint cubeTexture             = createSkyboxTexture("./texture/", true);

    /**NOTE - 灯光组
     */
    LightGroup lightGroup;
    lightGroup.addLight(Light(0, vec3(1, 1, 1), 2, vec3(1, 1.5, 1)));
    lightGroup.addLight(Light(1, vec3(1, 1, 1), 1.2, vec3(0, 0, 0), vec3(1, -1, 1)));
    lightGroup.addLight(Light(2, vec3(1, 1, 1), 1, vec3(0, 1.5, 0), vec3(0, -1, 0)));
    lightGroup.createLightUniformBuffer();
    lightGroup.bindingUniformBuffer(0);

    /**NOTE - ScreenTextureObject for debug
     */
    DebugTool debugTool;

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
        // projection应该是透视+投影，转换进标准体积空间：[-1,+1]^3
        // FIXME - 标准化设备坐标的范围是[-1, 1]，但是OpenGL深度缓冲的范围是[0, 1], 转换是自动的
        // mat4的第一个索引是列向量（i.e. mat4[2]表示第三个列向量）

        /**NOTE - 渲染
         */

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
        //~SECTION

        /**NOTE - 渲染阴影贴图的内容到屏幕上
         */
        // debugTool.renderTextureToScreen(depthMapTexture);

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

    glfwWindowHint(GLFW_SAMPLES, 4);  // 多重采样缓冲

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
GLuint createImageObjrct(const char* imagePath, const bool autoGammaCorrection)
{
    // 读取
    GLint width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);  // 加载图片时翻转y轴
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

/**REVIEW - 法线贴图
 * 我们的场景中已经充满了多边形物体，其中每个都可能由成百上千平坦的三角形组成。
 * 我们以向三角形上附加纹理的方式来增加额外细节，提升真实感，隐藏多边形几何体是由
 * 无数三角形组成的事实。纹理确有助益，然而当你近看它们时，这个事实便隐藏不住了。
 * 现实中的物体表面并非是平坦的，而是表现出无数（凹凸不平的）细节。
 *
 * 光照并没有呈现出任何裂痕和孔洞，完全忽略了砖块之间凹进去的线条；表面看起来
 * 完全就是平的。我们可以使用specular贴图根据深度或其他细节阻止部分表面被照的
 * 更亮，以此部分地解决问题，但这并不是一个好方案。我们需要的是某种可以告知
 * 光照系统给所有有关物体表面类似深度这样的细节的方式。
 *
 * 如果我们以光的视角来看这个问题：是什么使表面被视为完全平坦的表面来照亮？
 * 答案会是表面的法线向量。以光照算法的视角考虑的话，只有一件事决定物体的形状，
 * 这就是垂直于它的法线向量。砖块表面只有一个法线向量，表面完全根据这个法线向量
 * 被以一致的方式照亮。如果每个fragment都是用自己的不同的法线会怎样？
 * 这样我们就可以根据表面细微的细节对法线向量进行改变；
 * 这样就会获得一种表面看起来要复杂得多的幻觉
 * ![](https://learnopengl-cn.github.io/img/05/04/normal_mapping_surfaces.png)
 *
 * 每个fragment使用了自己的法线，我们就可以让光照相信一个表面由很多微小的
 * （垂直于法线向量的）平面所组成，物体表面的细节将会得到极大提升。
 * 这种每个fragment使用各自的法线，替代一个面上所有fragment使用同一个法线
 * 的技术叫做法线贴图（normal mapping）或凹凸贴图（bump mapping）。
 * 应用到砖墙上，效果像这样：
 * ![](https://learnopengl-cn.github.io/img/05/04/normal_mapping_compare.png)
 * 你可以看到细节获得了极大提升，开销却不大。因为我们只需要改变每个fragment的法线向量，
 * 并不需要改变所有光照公式。现在我们是为每个fragment传递一个法线，不再使用插值表面法线。
 * 这样光照使表面拥有了自己的细节。
 *
 * NOTE - 法线贴图
 * 为使法线贴图工作，我们需要为每个fragment提供一个法线。像diffuse贴图和
 * specular贴图一样，我们可以使用一个2D纹理来储存法线数据。
 * 2D纹理不仅可以储存颜色和光照数据，还可以储存法线向量。这样我们可以从2D纹理
 * 中采样得到特定纹理的法线向量。
 *
 * 由于法线向量是个几何工具，而纹理通常只用于储存颜色信息，
 * 用纹理储存法线向量不是非常直接。如果你想一想，就会知道纹理中的
 * 颜色向量用r、g、b元素代表一个3D向量。类似的我们也可以将法线向量
 * 的x、y、z元素储存到纹理中，代替颜色的r、g、b元素。法线向量的范围
 * 在-1到1之间，所以我们先要将其映射到0到1的范围：
 * 
 * 这会是一种偏蓝色调的纹理（你在网上找到的几乎所有法线贴图都是这样的）。
 * 这是因为所有法线的指向都偏向z轴（0, 0, 1）这是一种偏蓝的颜色。法线向量从
 * z轴方向也向其他方向轻微偏移，颜色也就发生了轻微变化，这样看起来便有了
 * 一种深度。例如，你可以看到在每个砖块的顶部，颜色倾向于偏绿，这是因为
 * 砖块的顶部的法线偏向于指向正y轴方向（0, 1, 0），这样它就是绿色的了。
 * 
 * 在一个简单的朝向正z轴的平面上，我们可以用这个diffuse纹理和这个法线贴图
 * 来渲染前面部分的图片。要注意的是这个链接里的法线贴图和上面展示的那个
 * 不一样。原因是OpenGL读取的纹理的y（或V）坐标和纹理通常被创建的方式相反。
 * 链接里的法线贴图的y（或绿色）元素是相反的（你可以看到绿色现在在下边）；
 * 如果你没考虑这个，光照就不正确了（译注：如果你现在不再使用SOIL了，
 * 那就不要用链接里的那个法线贴图，这个问题是SOIL载入纹理上下颠倒所致，
 * 它也会把法线在y方向上颠倒）。
 */
