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
vec3   generateRandomVec3(int upperBound);

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
    // glEnable(GL_BLEND);                                 // 启用混合
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数
    /**FIXME -
    在延迟渲染中关闭混合
     */
    // glEnable(GL_CULL_FACE);  // 启用面剔除
    // glClearColor(0.2F, 0.3F, 0.3F, 1.0F);               // 设置清空颜色
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_MULTISAMPLE);  // 启用多重采样
    /**NOTE - 文档中GL_MULTISAMPLE时默认启动的（true）
    延迟渲染现在用不了MSAA了
    */
    glEnable(GL_FRAMEBUFFER_SRGB);  // 自动Gamme矫正

    /**NOTE - 模型和着色器、纹理
     */
    Model  box("./box/box.obj");
    Model  plane("./plane/plane.obj");
    Model  sphere("./sphere/sphere.obj");
    Model  nanosuit("./nanosuit/nanosuit.obj");
    Shader phongShader("./shader/stdVerShader.vs.glsl", "./shader/stdPhongLighting.fs.glsl");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl", "./shader/skyboxShader.fs.glsl");
    Shader lightObjShader("./shader/stdVerShader.vs.glsl", "./shader/stdPureColor.fs.glsl");
    GLuint cubeTexture = createSkyboxTexture("./texture/", true);
    GLuint woodTexture = createImageObjrct("./texture/wood.jpg", true);
    // G-Buffer shader
    Shader gBuffer_geomProcess("./shader/stdVerShader.vs.glsl",
                               "./shader/gBuffer_geomProcess.fs.glsl");
    Shader gBuffer_lightingProcess("./shader/stdScreenShader.vs.glsl",
                                   "./shader/gBuffer_LightingProcess.fs.glsl");
    Shader lightVolumeShader("./shader/stdVerShader.vs.glsl", "./shader/lightVolume.fs.glsl");

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

    /**NOTE - G缓冲
     */
    GLuint gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    GLuint gPosition, gNormal, gAlbedoSpec;
    // position
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, CAMERA_WIDTH, CAMERA_HEIGH, 0, GL_RGB, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, CAMERA_WIDTH, CAMERA_HEIGH, 0, GL_RGB, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color specularity
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CAMERA_WIDTH, CAMERA_HEIGH, 0, GL_RGBA, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
    // draw buffer
    std::array<GLuint, 3> attachments = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
    };
    // depth buffer
    GLuint rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, CAMERA_WIDTH, CAMERA_HEIGH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              rboDepth);
    //  检查帧缓冲状态
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer is  complete!" << std::endl;
    }
    else { std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl; }
    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

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

        /**NOTE - 几何处理
         */
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glDrawBuffers(3, attachments.data());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        // nanosuit
        gBuffer_geomProcess.use();
        gBuffer_geomProcess.setParameter("view", view);
        gBuffer_geomProcess.setParameter("projection", projection);
        for (int i = 0; i < 20; i++)
        {
            auto [lx, ly] = std::make_tuple(float(i / 5), float(i % 5));
            gBuffer_geomProcess.setParameter(
                "model",
                scale(translate(mat4(1), lx * vec3(1, 0, 0) + ly * vec3(0, 0, 1)), vec3(0.1f)));
            nanosuit.Draw(&gBuffer_geomProcess);
        }

        /**NOTE - 光照处理
         */
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);           // 启用混合
        glBlendFunc(GL_ONE, GL_ONE);  // 设置混合函数，直接相加
        glStencilFunc(GL_NOTEQUAL, 1, std::numeric_limits<GLuint>::max());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        lightVolumeShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        lightVolumeShader.setParameter("gPosition", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        lightVolumeShader.setParameter("gNormal", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        lightVolumeShader.setParameter("gAlbedoSpec", 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        lightVolumeShader.setParameter("skybox", 3);
        lightVolumeShader.setParameter("view", view);
        lightVolumeShader.setParameter("projection", projection);
        lightVolumeShader.setParameter("viewPosition", camera->Position);
        lightVolumeShader.setParameter("screenWidth", float(CAMERA_WIDTH));
        lightVolumeShader.setParameter("screenHeight", float(CAMERA_HEIGH));
        int i = 0;
        for (const auto& light : lightGroup.getLights())
        {
            if (light.getLightType() == 1) { continue; }  // 跳过面光

            glClear(GL_STENCIL_BUFFER_BIT);  // 对每个光体积，清空模板缓冲
            lightVolumeShader.setParameter("lightIndex", i++);
            lightVolumeShader.setParameter("model",
                                           scale(translate(mat4(1), light.getPostion()), vec3(2)));
            sphere.Draw(&lightVolumeShader);
        }
        glEnable(GL_DEPTH_TEST);  // 启用深度缓冲
        glDisable(GL_BLEND);      // 启用混合

        // 复制FBO的深度到默认缓冲的深度上
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);  // 写入到默认帧缓冲
        glBlitFramebuffer(0, 0, CAMERA_WIDTH, CAMERA_HEIGH, 0, 0, CAMERA_WIDTH, CAMERA_HEIGH,
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        // 光体积范围
        glStencilFunc(GL_ALWAYS, 1, std::numeric_limits<GLuint>::max());
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_CULL_FACE);  // 启用面剔除
        lightObjShader.use();
        lightObjShader.setParameter("view", view);
        lightObjShader.setParameter("projection", projection);
        lightObjShader.setParameter("lightColor", vec3(1.0, 0, 0));
        for (const auto& light : lightGroup.getLights())
        {
            if (light.getLightType() == 1) { continue; }  // 跳过面光

            // 渲染光体积范围
            lightObjShader.setParameter("model",
                                        scale(translate(mat4(1), light.getPostion()), vec3(2)));
            sphere.Draw(&lightObjShader);
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        /**TODO - 光体积照明
        大致实现了，但是还有几个小问题要解决：
        1. 照明解结果要相加，现在因为启动了深度测试，只保留距离摄像机最近的照明结果
        2. 解决方案是关闭深度检测，并将混合模式改成直接相加，这样一来
        由于关闭了背面剔除，同一栈灯光的光效会计算两次（正面一次，背面一次）
        （PS：简单地使用背面剔除，则会在摄像机进入光体积时，没有任何灯光被计算）
        3. 解决方案是利用模板缓冲，对于同一栈灯光，计算光照时，向模板写值
        遇到第二个同像素位置的片元时，直接丢弃

        * TODO - 还有一个小问题
        光体积的背景需要稍微研究一下，背景颜色会跳
        */

        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // gBuffer_lightingProcess.use();
        // glBindVertexArray(debugTool.getScreenVAO());
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, gPosition);
        // gBuffer_lightingProcess.setParameter("gPosition", 0);
        // glActiveTexture(GL_TEXTURE1);
        // glBindTexture(GL_TEXTURE_2D, gNormal);
        // gBuffer_lightingProcess.setParameter("gNormal", 1);
        // glActiveTexture(GL_TEXTURE2);
        // glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        // gBuffer_lightingProcess.setParameter("gAlbedoSpec", 2);
        // glActiveTexture(GL_TEXTURE3);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        // gBuffer_lightingProcess.setParameter("skybox", 3);
        // gBuffer_lightingProcess.setParameter("viewPosition", camera->Position);
        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        // 木地板
        phongShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        phongShader.setParameter("texture0", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        phongShader.setParameter("skybox", 1);
        phongShader.setParameter("model", scale(mat4(1), vec3(5)));
        phongShader.setParameter("view", view);
        phongShader.setParameter("projection", projection);
        phongShader.setParameter("cameraPos", camera->Position);
        plane.Draw(&phongShader);

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

vec3 generateRandomVec3(int upperBound)
{
    static std::random_device rd;   // 用于获得一个随机数种子
    static std::mt19937 gen(rd());  // 以 rd() 作为种子初始化 Mersenne Twister 生成器
    std::uniform_int_distribution<int> dis(-upperBound, upperBound);
    // 定义一个在 [-upperBound, upperBound) 范围内的均匀分布
    return {dis(gen), dis(gen), dis(gen)};
}

/**REVIEW - 延迟渲染
我们现在一直使用的光照方式叫做正向渲染(Forward Rendering)或者正向着色法(Forward Shading)，
它是我们渲染物体的一种非常直接的方式，在场景中我们根据所有光源照亮一个物体，之后再渲染下
一个物体，以此类推。它非常容易理解，也很容易实现，但是同时它对程序性能的影响也很大，因为
对于每一个需要渲染的物体，程序都要对每一个光源每一个需要渲染的片段进行迭代，这是非常多的！
因为大部分片段着色器的输出都会被之后的输出覆盖，正向渲染还会在场景中因为高深的复杂度
(多个物体重合在一个像素上)浪费大量的片段着色器运行时间。

延迟着色法(Deferred Shading)，或者说是延迟渲染(Deferred Rendering)，为了解决上述问题而诞生
了，它大幅度地改变了我们渲染物体的方式。这给我们优化拥有大量光源的场景提供了很多的选择，
因为它能够在渲染上百甚至上千光源的同时还能够保持能让人接受的帧率。下面这张图片包含了一
共1874个点光源，它是使用延迟着色法来完成的，而这对于正向渲染几乎是不可能的\

延迟着色法基于我们延迟(Defer)或推迟(Postpone)大部分计算量非常大的渲染(像是光照)到后期进行
处理的想法。它包含两个处理阶段(Pass)：在第一个几何处理阶段(Geometry Pass)中，我们先渲染
场景一次，之后获取对象的各种几何信息，并储存在一系列叫做G缓冲(G-buffer)的纹理中；想想位
置向量(Position Vector)、颜色向量(Color Vector)、法向量(Normal Vector)和/或镜面值(Specular
 Value)。场景中这些储存在G缓冲中的几何信息将会在之后用来做(更复杂的)光照计算。下面是一帧
 中G缓冲的内容：
 ![](https://learnopengl-cn.github.io/img/05/08/deferred_g_buffer.png)

 我们会在第二个光照处理阶段(Lighting Pass)中使用G缓冲内的纹理数据。在光照处理阶段中，
 我们渲染一个屏幕大小的方形，并使用G缓冲中的几何数据对每一个片段计算场景的光照；
 在每个像素中我们都会对G缓冲进行迭代。我们对于渲染过程进行解耦，将它高级的片段处理挪
 到后期进行，而不是直接将每个对象从顶点着色器带到片段着色器。光照计算过程还是和我们以前一样，
 但是现在我们需要从对应的G缓冲而不是顶点着色器(和一些uniform变量)那里获取输入变量了。

 ![](https://learnopengl-cn.github.io/img/05/08/deferred_overview.png)

 这种渲染方法一个很大的好处就是能保证在G缓冲中的片段和在屏幕上呈现的像素所包含的片段信息是
 一样的，因为深度测试已经最终将这里的片段信息作为最顶层的片段。这样保证了对于在光照处理阶段
 中处理的每一个像素都只处理一次，所以我们能够省下很多无用的渲染调用。除此之外，延迟渲染还
 允许我们做更多的优化，从而渲染更多的光源。

 当然这种方法也带来几个缺陷， 由于G缓冲要求我们在纹理颜色缓冲中存储相对比较大的场景数据，
 这会消耗比较多的显存，尤其是类似位置向量之类的需要高精度的场景数据。 另外一个缺点就是他
 不支持混色(因为我们只有最前面的片段信息)， 因此也不能使用MSAA了。针对这几个问题我们可
 以做一些变通来克服这些缺点，这些我们留会在教程的最后讨论。

 在几何处理阶段中填充G缓冲非常高效，因为我们直接储存像素位置，颜色或者是法线等对象信息到
 帧缓冲中，而这几乎不会消耗处理时间。在此基础上使用多渲染目标(Multiple Render Targets,
 MRT)技术，我们甚至可以在一个渲染处理之内完成这所有的工作。

 * NOTE - G缓冲
 G缓冲(G-buffer)是对所有用来储存光照相关的数据，并在最后的光照处理阶段中使用的所有
 纹理的总称。趁此机会，让我们顺便复习一下在正向渲染中照亮一个片段所需要的所有数据：
    一个3D位置向量来计算(插值)片段位置变量供lightDir和viewDir使用
    一个RGB漫反射颜色向量，也就是反照率(Albedo)
    一个3D法向量来判断平面的斜率
    一个镜面强度(Specular Intensity)浮点值
    所有光源的位置和颜色向量
    玩家或者观察者的位置向量

有了这些(逐片段)变量的处置权，我们就能够计算我们很熟悉的(布林-)冯氏光照
(Blinn-Phong Lighting)了。光源的位置，颜色，和玩家的观察位置可以通过uniform变量来设置，但
是其它变量对于每个对象的片段都是不同的。如果我们能以某种方式传输完全相同的数据到最终的
延迟光照处理阶段中，我们就能计算与之前相同的光照效果了，尽管我们只是在渲染一个2D方形的片段。

OpenGL并没有限制我们能在纹理中能存储的东西，所以现在你应该清楚在一个或多个屏幕大小的
纹理中储存所有逐片段数据并在之后光照处理阶段中使用的可行性了。因为G缓冲纹理将会和光照
处理阶段中的2D方形一样大，我们会获得和正向渲染设置完全一样的片段数据，但在光照处理阶
段这里是一对一映射。

对于每一个片段我们需要储存的数据有：一个位置向量、一个法向量，一个颜色向量，一个镜面强度值。
所以我们在几何处理阶段中需要渲染场景中所有的对象并储存这些数据分量到G缓冲中。我们可以再次
使用多渲染目标(Multiple Render Targets)来在一个渲染处理之内渲染多个颜色缓冲，在之前的泛光
教程中我们也简单地提及了它。

对于几何渲染处理阶段，我们首先需要初始化一个帧缓冲对象，我们很直观的称它为gBuffer，它包含
了多个颜色缓冲和一个单独的深度渲染缓冲对象(Depth Renderbuffer Object)。对于位置和法向量的
纹理，我们希望使用高精度的纹理(每分量16或32位的浮点数)，而对于反照率和镜面值，使用默认的
纹理(每分量8位浮点数)就够了。

* NOTE - 延迟光照处理阶段
现在我们已经有了一大堆的片段数据储存在G缓冲中供我们处置，我们可以选择通过一个像素一个像素
地遍历各个G缓冲纹理，并将储存在它们里面的内容作为光照算法的输入，来完全计算场景最终的光照
颜色。由于所有的G缓冲纹理都代表的是最终变换的片段值，我们只需要对每一个像素执行一次昂贵的
光照运算就行了。这使得延迟光照非常高效，特别是在需要调用大量重型片段着色器的复杂场景中。

对于这个光照处理阶段，我们将会渲染一个2D全屏的方形(有一点像后期处理效果)并且在每个像素上
运行一个昂贵的光照片段着色器。


* NOTE - 结合延迟渲染与正向渲染
延迟着色法的其中一个缺点就是它不能进行混合(Blending)，因为G缓冲中所有的数据都是从一个单独
的片段中来的，而混合需要对多个片段的组合进行操作。延迟着色法另外一个缺点就是它迫使你对大
部分场景的光照使用相同的光照算法，你可以通过包含更多关于材质的数据到G缓冲中来减轻这一缺点。

为了克服这些缺点(特别是混合)，我们通常分割我们的渲染器为两个部分：一个是延迟渲染的部分，
另一个是专门为了混合或者其他不适合延迟渲染管线的着色器效果而设计的的正向渲染的部分。
为了展示这是如何工作的，我们将会使用正向渲染器渲染光源为一个小立方体，因为光照立方体会
需要一个特殊的着色器(会输出一个光照颜色)。

现在我们想要渲染每一个光源为一个3D立方体，并放置在光源的位置上随着延迟渲染器一起发出
光源的颜色。很明显，我们需要做的第一件事就是在延迟渲染方形之上正向渲染所有的光源，
它会在延迟渲染管线的最后进行。所以我们只需要像正常情况下渲染立方体，只是会在我们完成
延迟渲染操作之后进行。

我们需要做的就是首先复制出在几何渲染阶段中储存的深度信息，并输出到默认的帧缓冲的深度缓冲，
然后我们才渲染光立方体。这样之后只有当它在之前渲染过的几何体上方的时候，光立方体的片段才
会被渲染出来。我们可以使用glBlitFramebuffer复制一个帧缓冲的内容到另一个帧缓冲中，
这个函数我们也在抗锯齿的教程中使用过，用来还原多重采样的帧缓冲。glBlitFramebuffer这个
函数允许我们复制一个用户定义的帧缓冲区域到另一个用户定义的帧缓冲区域。

我们储存所有延迟渲染阶段中所有物体的深度信息在gBuffer这个FBO中。如果我们仅仅是简单复制
它的深度缓冲内容到默认帧缓冲的深度缓冲中，那么光立方体就会像是场景中所有的几何体都是
正向渲染出来的一样渲染出来。就像在抗锯齿教程中介绍的那样，我们需要指定一个帧缓冲为
读帧缓冲(Read Framebuffer)，并且类似地指定一个帧缓冲为写帧缓冲(Write Framebuffer)：

* NOTE - 更多的光源
延迟渲染一直被称赞的原因就是它能够渲染大量的光源而不消耗大量的性能。然而，延迟渲染它
本身并不能支持非常大量的光源，因为我们仍然必须要对场景中每一个光源计算每一个片段的光照分量。
真正让大量光源成为可能的是我们能够对延迟渲染管线引用的一个非常棒的优化：光体积(Light Volumes)

通常情况下，当我们渲染一个复杂光照场景下的片段着色器时，我们会计算场景中每一个光源的贡献，
不管它们离这个片段有多远。很大一部分的光源根本就不会到达这个片段，所以为什么我们还要浪费
这么多光照运算呢？

隐藏在光体积背后的想法就是计算光源的半径，或是体积，也就是光能够到达片段的范围。
由于大部分光源都使用了某种形式的衰减(Attenuation)，我们可以用它来计算光源能够到
达的最大路程，或者说是半径。我们接下来只需要对那些在一个或多个光体积内的片段进
行繁重的光照运算就行了。这可以给我们省下来很可观的计算量，因为我们现在只在需要
的情况下计算光照。

这个方法的难点基本就是找出一个光源光体积的大小，或者是半径。

* NOTE - 计算一个光源的体积或半径
为了获取一个光源的体积半径，我们需要解一个对于一个我们认为是黑暗(Dark)的亮度(Brightness)
的衰减方程，它可以是0.0，或者是更亮一点的但仍被认为黑暗的值，像是0.03。为了展示我们如何
计算光源的体积半径，我们将会使用一个在投光物这节中引入的一个更加复杂，但非常灵活的衰减方程

我们现在想要在Flight
等于0的前提下解这个方程，也就是说光在该距离完全是黑暗的。然而这个方程永远不会真正等于0.0，
所以它没有解。所以，我们不会求表达式等于0.0时候的解，相反我们会求当亮度值靠近于0.0的解，
这时候它还是能被看做是黑暗的。在这个教程的演示场景中，我们选择5/256
作为一个合适的光照值；除以256是因为默认的8-bit帧缓冲可以每个分量显示这么多强度值(Intensity)。

我们使用的衰减方程在它的可视范围内基本都是黑暗的，所以如果我们想要限制它为一个比5/256
更加黑暗的亮度，光体积就会变得太大从而变得低效。只要是用户不能在光体积边缘看到一个突兀
的截断，这个参数就没事了。当然它还是依赖于场景的类型，一个高的亮度阀值会产生更小的光体积，
从而获得更高的效率，然而它同样会产生一个很容易发现的副作用，那就是光会在光体积边界看起
来突然断掉。

对于场景中每一个光源，我们都计算它的半径，并仅在片段在光源的体积内部时才计算该光源的光照。
下面是更新过的光照处理阶段片段着色器，它考虑到了计算出来的光体积。注意这种方法仅仅用作教
学目的，在实际场景中是不可行的，我们会在后面讨论它：

* NOTE - 真正使用光体积
上面那个片段着色器在实际情况下不能真正地工作，并且它只演示了我们可以不知怎样能使用光体积
减少光照运算。然而事实上，你的GPU和GLSL并不擅长优化循环和分支。这一缺陷的原因是GPU中
着色器的运行是高度并行的，大部分的架构要求对于一个大的线程集合，GPU需要对它运行完全一样
的着色器代码从而获得高效率。这通常意味着一个着色器运行时总是执行一个if语句所有的分支从而
保证着色器运行都是一样的，这使得我们之前的半径检测优化完全变得无用，我们仍然在对所有光
源计算光照！

使用光体积更好的方法是渲染一个实际的球体，并根据光体积的半径缩放。这些球的中心放置在光源
的位置，由于它是根据光体积半径缩放的，这个球体正好覆盖了光的可视体积。这就是我们的技巧：
我们使用大体相同的延迟片段着色器来渲染球体。因为球体产生了完全匹配于受影响像素的着色器
调用，我们只渲染了受影响的像素而跳过其它的像素。下面这幅图展示了这一技巧：
![](https://learnopengl-cn.github.io/img/05/08/deferred_light_volume_rendered.png)

它被应用在场景中每个光源上，并且所得的片段相加混合在一起。这个结果和之前场景是一样的，
但这一次只渲染对于光源相关的片段。它有效地减少了从nr_objects * nr_lights到nr_objects +
nr_lights的计算量，这使得多光源场景的渲染变得无比高效。这正是为什么延迟渲染非常适合渲染
很大数量光源。

然而这个方法仍然有一个问题：面剔除(Face Culling)需要被启用(否则我们会渲染一个光效果两次)，
并且在它启用的时候用户可能进入一个光源的光体积，然而这样之后这个体积就不再被渲染了
(由于背面剔除)，这会使得光源的影响消失。这个问题可以通过一个模板缓冲技巧来解决。

渲染光体积确实会带来沉重的性能负担，虽然它通常比普通的延迟渲染更快，这仍然不是最好的优化。
另外两个基于延迟渲染的更流行(并且更高效)的拓展叫做延迟光照(Deferred Lighting)和切片式延迟
着色法(Tile-based Deferred Shading)。这些方法会很大程度上提高大量光源渲染的效率，并且也能
允许一个相对高效的多重采样抗锯齿(MSAA)。然而受制于这篇教程的长度，我将会在之后的教程中介
绍这些优化。

* NOTE - 延迟渲染 vs 正向渲染
仅仅是延迟着色法它本身(没有光体积)已经是一个很大的优化了，每个像素仅仅运行一个单独的片段
着色器，然而对于正向渲染，我们通常会对一个像素运行多次片段着色器。当然，延迟渲染确实带来
一些缺点：大内存开销，没有MSAA和混合(仍需要正向渲染的配合)。

当你有一个很小的场景并且没有很多的光源时候，延迟渲染并不一定会更快一点，甚至有些时候由
于开销超过了它的优点还会更慢。然而在一个更复杂的场景中，延迟渲染会快速变成一个重要的优化，
特别是有了更先进的优化拓展的时候。

最后我仍然想指出，基本上所有能通过正向渲染完成的效果能够同样在延迟渲染场景中实现，
这通常需要一些小的翻译步骤。举个例子，如果我们想要在延迟渲染器中使用法线贴图
(Normal Mapping)，我们需要改变几何渲染阶段着色器来输出一个世界空间法线
(World-space Normal)，它从法线贴图中提取出来(使用一个TBN矩阵)而不是表面法线，
光照渲染阶段中的光照运算一点都不需要变。如果你想要让视差贴图工作，首先你需要在
采样一个物体的漫反射，镜面，和法线纹理之前首先置换几何渲染阶段中的纹理坐标。
*/
