#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
#include <array>
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
/**FIXME - 错题本：关于要不要反转图像的y轴
opengl纹理坐标uv(0, 0)位于图像的左下角，采样是（0，0）位置上的纹理会从图像的左下角读出
opengl要求纹理数据的第一个元素位于纹理的左下角，而DX要求纹理数据的第一个元素位于纹理的左上角
stb_image.h读取图像的规则是：
默认从图像的右上角开始读取，也就是说，它会将左上角当作data的第一个元素
换言之，data[0][0]将会是左上角的像素，而data[h-1][0]将会是图像左下角的像素
 */
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
    // glClearColor(0.2F, 0.3F, 0.3F, 1.0F);               // 设置清空颜色
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_MULTISAMPLE);  // 启用多重采样
    /**NOTE - 文档中GL_MULTISAMPLE时默认启动的（true） */
    // glEnable(GL_FRAMEBUFFER_SRGB);                      // 自动Gamme矫正
    /**NOTE - 关闭自动gamma矫正
    输入纹理的gamma矫正，还是OpenGL自动完成的
    gamma矫正由我们自己在hdr_shader中完成
    */

    /**NOTE - 模型和着色器、纹理
     */
    Model  box("./box/box.obj");
    Model  plane("./plane/plane.obj");
    Model  sphere("./sphere/sphere.obj");
    Shader phongShader("./shader/stdVerShader.vs.glsl", "./shader/stdPhongLighting.fs.glsl");
    Shader phongLightingWithNormal("./shader/stdVerShader.vs.glsl",
                                   "./shader/normalMapShader.fs.glsl");
    Shader phongLightingWithDepth("./shader/stdVerShader.vs.glsl",
                                  "./shader/ParallaxMapping.fs.glsl");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl", "./shader/skyboxShader.fs.glsl");
    Shader lightObjShader("./shader/stdVerShader.vs.glsl", "./shader/stdPureColor.fs.glsl");
    GLuint brickwallDiffuseTexture = createImageObjrct("./texture/brickwall.jpg", true, false);
    GLuint brickwallNormalTexture =
        createImageObjrct("./texture/brickwall_normal.jpg", false, false);
    GLuint cubeTexture = createSkyboxTexture("./texture/", true);
    GLuint woodTexture = createImageObjrct("./texture/wood.jpg", true);

    GLuint depthBrick_diffuseMap = createImageObjrct("./texture/bricks2.jpg", true, false);
    GLuint depthBrick_normalMap  = createImageObjrct("./texture/toy_box_normal.png", false, false);
    GLuint depthBrick_depthMap   = createImageObjrct("./texture/toy_box_disp.png", false, false);

    Shader hdr_shader("./shader/stdScreenShader.vs.glsl", "./shader/hdrScreenShader.fs.glsl");
    Shader blur("./shader/stdScreenShader.vs.glsl", "./shader/gaussianBlur.fs.glsl");

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

    /**NOTE - 帧缓冲对象
     */
    GLuint hdr_FBO;
    glGenFramebuffers(1, &hdr_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdr_FBO);
    // 创建一个纹理缓冲作为颜色缓冲
    std::array<GLuint, 2> hdr_frameTextures = {0, 0};  // 创建第二个纹理缓冲记录亮度大的片元
    glGenTextures(2, hdr_frameTextures.data());
    for (int i = 0; i < hdr_frameTextures.size(); i++)
    {
        glBindTexture(GL_TEXTURE_2D, hdr_frameTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, CAMERA_WIDTH, CAMERA_HEIGH, 0, GL_RGB, GL_FLOAT,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
                               hdr_frameTextures[i], 0);
    }
    // 显式告知OpenGL我们正在通过glDrawBuffers渲染到多个颜色缓冲
    std::array<GLuint, 2> attachments = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
    };
    glDrawBuffers(2, attachments.data());
    // 创建深度缓冲和模板缓冲
    GLuint hdr_RBO;
    glGenRenderbuffers(1, &hdr_RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, hdr_RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, CAMERA_WIDTH, CAMERA_HEIGH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              hdr_RBO);
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

    /**NOTE - 后期处理高斯模糊
     */
    std::array<GLuint, 2> pingpongFBO;
    std::array<GLuint, 2> pingpongBuffer;
    glGenFramebuffers(2, pingpongFBO.data());
    glGenTextures(2, pingpongBuffer.data());
    for (GLuint i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, CAMERA_WIDTH, CAMERA_HEIGH, 0, GL_RGB, GL_FLOAT,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               pingpongBuffer[i], 0);
        //  检查帧缓冲状态
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "Framebuffer is  complete!" << std::endl;
        }
        else { std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl; }
        // 解绑
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

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
        glBindFramebuffer(GL_FRAMEBUFFER, hdr_RBO);
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
        // 带法线的砖墙
        phongLightingWithNormal.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, brickwallDiffuseTexture);
        phongLightingWithNormal.setParameter("texture0", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, brickwallNormalTexture);
        phongLightingWithNormal.setParameter("texture_normal", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        phongLightingWithNormal.setParameter("skybox", 2);
        phongLightingWithNormal.setParameter(
            "model", rotate(translate(mat4(1), vec3(-1, 1, -2)), radians(90.0f), vec3(1, 0, 0)));
        phongLightingWithNormal.setParameter("view", view);
        phongLightingWithNormal.setParameter("projection", projection);
        phongLightingWithNormal.setParameter("cameraPos", camera->Position);
        plane.Draw(&phongLightingWithNormal);
        // 带由深度贴图的墙
        phongLightingWithDepth.use();
        phongLightingWithDepth.setParameter(
            "model", rotate(translate(mat4(1), vec3(1, 1, -2)), radians(90.0f), vec3(1, 0, 0)));
        phongLightingWithDepth.setParameter("view", view);
        phongLightingWithDepth.setParameter("projection", projection);
        phongLightingWithDepth.setParameter("cameraPos", camera->Position);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthBrick_diffuseMap);
        phongLightingWithDepth.setParameter("diffuseMap", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthBrick_normalMap);
        phongLightingWithDepth.setParameter("normalMap", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, depthBrick_depthMap);
        phongLightingWithDepth.setParameter("depthMap", 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        phongLightingWithDepth.setParameter("skybox", 3);
        plane.Draw(&phongLightingWithDepth);

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

        /**NOTE - 高斯模糊辉光
         */
        blur.use();
        glViewport(0, 0, CAMERA_WIDTH, CAMERA_HEIGH);
        glBindVertexArray(debugTool.getScreenVAO());
        bool horizontal = true, first_iteration = true;
        for (int i = 0; i < 10; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blur.setParameter("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D,
                          (first_iteration) ? hdr_frameTextures[1] : pingpongBuffer[!horizontal]);
            // draw
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                    GL_STENCIL_BUFFER_BIT);  // 清除颜色、深度和模板缓冲
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            horizontal      = !horizontal;
            first_iteration = false;
        }

        /**NOTE - 将RBO中的色彩绘制到屏幕上
         */
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, CAMERA_WIDTH, CAMERA_HEIGH);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);  // 清除颜色、深度和模板缓冲
        // render
        glBindVertexArray(debugTool.getScreenVAO());
        hdr_shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdr_frameTextures[0]);
        hdr_shader.setParameter("hdr_frameTexture", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[1]);
        hdr_shader.setParameter("bloomBlur", 1);
        hdr_shader.setParameter("exposure", 2.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // 解绑
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        // FIXME - renderTextureToScreen()这个函数默认设置的纹理名称不对（screenTexture）
        //~SECTION

        /**NOTE - 渲染阴影贴图的内容到屏幕上
         */
        // debugTool.renderTextureToScreen(hdr_frameTexture);

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
 *
 * 你可以看到所有法线都指向z方向，它们本该朝着表面法线指向y方向的。
 一个可行方案是为每个表面制作一个单独的法线贴图。如果是一个立方体的话我们就需要6个法线贴图，
 但是如果模型上有无数的朝向不同方向的表面，这就不可行了
 （译注：实际上对于复杂模型可以把朝向各个方向的法线储存在同一张贴图上，
 你可能看到过不只是蓝色的法线贴图，不过用那样的法线贴图有个问题是你必须记住模型的起始朝向，
 如果模型运动了还要记录模型的变换，这是非常不方便的；此外就像作者所说的，
 如果把一个diffuse纹理应用在同一个物体的不同表面上，就像立方体那样的，
 就需要做6个法线贴图，这也不可取）。
 *
 * 另一个稍微有点难的解决方案是，在一个不同的坐标空间中进行光照，
 这个坐标空间里，法线贴图向量总是指向这个坐标空间的正z方向；所有的光照向量都相对
 与这个正z方向进行变换。这样我们就能始终使用同样的法线贴图，不管朝向问题。
 这个坐标空间叫做切线空间（tangent space）。

 * NOTE - 切线空间
 法线贴图中的法线向量定义在切线空间中，在切线空间中，法线永远指着正z方向。
 切线空间是位于三角形表面之上的空间：法线相对于单个三角形的局部坐标系。
 它就像法线贴图向量的局部空间；它们都被定义为指向正z方向，无论最终变换到什么方向。
 使用一个特定的矩阵我们就能将本地/切线空间中的法线向量转成世界或视图空间下，
 使它们转向到最终的贴图表面的方向。

我们可以说，上个部分那个朝向正y的法线贴图错误的贴到了表面上。法线贴图被定义在切线空间中，
所以一种解决问题的方式是计算出一种矩阵，把法线从切线空间变换到一个不同的空间，
这样它们就能和表面法线方向对齐了：法线向量都会指向正y方向。切线空间的一大好处是
我们可以为任何类型的表面计算出一个这样的矩阵，由此我们可以把切线空间的z方向和表面的
法线方向对齐。

这种矩阵叫做TBN矩阵这三个字母分别代表tangent、bitangent和normal向量。
这是建构这个矩阵所需的向量。要建构这样一个把切线空间转变为不同空间的变异矩阵，
我们需要三个相互垂直的向量，它们沿一个表面的法线贴图对齐于：上、右、前；
这和我们在摄像机教程中做的类似。

* NOTE - 手工计算切线和副切线

* NOTE - 切线空间法线贴图
我们先将所有TBN向量变换到我们所操作的坐标系中，现在是世界空间，我们可以乘以
model矩阵。然后我们创建实际的TBN矩阵，直接把相应的向量应用到mat3构造器就行。
注意，如果我们希望更精确的话就不要将TBN向量乘以model矩阵，而是使用法线矩阵，
因为我们只关心向量的方向，不关心平移和缩放。

从技术上讲，顶点着色器中无需副切线。所有的这三个TBN向量都是相互垂直的所以我们
可以在顶点着色器中用T和N向量的叉乘，自己计算出副切线：vec3 B = cross(T, N);
 */

/**REVIEW - 视差贴图
视差贴图(Parallax Mapping)技术和法线贴图差不多，但它有着不同的原则。和法线贴图一样
视差贴图能够极大提升表面细节，使之具有深度感。它也是利用了视错觉，然而对深度有着更好的表达，
与法线贴图一起用能够产生难以置信的效果。视差贴图和光照无关，我在这里是作为法线贴图的
技术延续来讨论它的。需要注意的是在开始学习视差贴图之前强烈建议先对法线贴图，
特别是切线空间有较好的理解。

视差贴图属于位移贴图(Displacement Mapping)技术的一种，它对根据储存在纹理中的几何信息
对顶点进行位移或偏移。一种实现的方式是比如有1000个顶点，根据纹理中的数据对平面特定区域
的顶点的高度进行位移。这样的每个纹理像素包含了高度值纹理叫做高度贴图。一张简单的砖块
表面的高度贴图如下所示：

整个平面上的每个顶点都根据从高度贴图采样出来的高度值进行位移，根据材质的几何
属性平坦的平面变换成凹凸不平的表面。例如一个平坦的平面利用上面的高度贴图进行
置换能得到以下结果：
![](https://learnopengl-cn.github.io/img/05/05/parallax_mapping_plane_heightmap.png)

置换顶点有一个问题就是平面必须由很多顶点组成才能获得具有真实感的效果，
否则看起来效果并不会很好。一个平坦的表面上有1000个顶点计算量太大了。
我们能否不用这么多的顶点就能取得相似的效果呢？事实上，上面的表面就是用
6个顶点渲染出来的（两个三角形）。上面的那个表面使用视差贴图技术渲染，
位移贴图技术不需要额外的顶点数据来表达深度，它像法线贴图一样采用一种
聪明的手段欺骗用户的眼睛。

视差贴图背后的思想是修改纹理坐标使一个fragment的表面看起来比实际的更高或者更低，
所有这些都根据观察方向和高度贴图。为了理解它如何工作，看看下面砖块表面的图片：
![](https://learnopengl-cn.github.io/img/05/05/parallax_mapping_plane_height.png)

PS：这个问题好像没有合适的解析解，他用的是经验公式

* NOTE - 视差贴图
我们将使用一个简单的2D平面，在把它发送给GPU之前我们先计算它的切线和副切线向量；
和法线贴图教程做的差不多。我们将向平面贴diffuse纹理、法线贴图以及一个位移贴图，
你可以点击链接下载。这个例子中我们将视差贴图和法线贴图连用。因为视差贴图生成表面
位移了的幻觉，当光照不匹配时这种幻觉就被破坏了。法线贴图通常根据高度贴图生成，
法线贴图和高度贴图一起用能保证光照能和位移相匹配。

你可能已经注意到，上面链接上的那个位移贴图和教程一开始的那个高度贴图相比是
颜色是相反的。这是因为使用反色高度贴图（也叫深度贴图）去模拟深度比模拟高度更容易。
下图反映了这个轻微的改变：
![](https://learnopengl-cn.github.io/img/05/05/parallax_mapping_depth.png)

有一个地方需要注意，就是viewDir.xy除以viewDir.z那里。因为viewDir向量是经过了标准化的，
viewDir.z会在0.0到1.0之间的某处。当viewDir大致平行于表面时，它的z元素接近于0.0，
除法会返回比viewDir垂直于表面的时候更大的P¯
向量。所以，从本质上，相比正朝向表面，当带有角度地看向平面时，我们会更大程度地缩放P¯
的大小，从而增加纹理坐标的偏移；这样做在视角上会获得更大的真实度。

有些人更喜欢不在等式中使用viewDir.z，因为普通的视差贴图会在角度上产生不尽如人意的结果；
这个技术叫做有偏移量限制的视差贴图（Parallax Mapping with Offset Limiting）。
选择哪一个技术是个人偏好问题，但我倾向于普通的视差贴图。

问题的原因是这只是一个大致近似的视差映射。
还有一些技巧让我们在陡峭的高度上能够获得几乎完美的结果，
即使当以一定角度观看的时候。例如，我们不再使用单一样本，
取而代之使用多样本来找到最近点B
会得到怎样的结果？

* NOTE - 陡峭视差映射
陡峭视差映射(Steep Parallax Mapping)是视差映射的扩展，
原则是一样的，但不是使用一个样本而是多个样本来确定向量P¯
到B
。即使在陡峭的高度变化的情况下，它也能得到更好的结果，
原因在于该技术通过增加采样的数量提高了精确性。

陡峭视差映射的基本思想是将总深度范围划分为同一个深度/高度的多个层。从每个层中我们沿着P¯
方向移动采样纹理坐标，直到我们找到一个采样低于当前层的深度值。看看下面的图片：
![](https://learnopengl-cn.github.io/img/05/05/parallax_mapping_steep_parallax_mapping_diagram.png)

我们从上到下遍历深度层，我们把每个深度层和储存在深度贴图中的它的深度值进行对比。
如果这个层的深度值小于深度贴图的值，就意味着这一层的P¯
向量部分在表面之下。我们继续这个处理过程直到有一层的深度高于储存在深度贴图中的值：
这个点就在（经过位移的）表面下方。

这个例子中我们可以看到第二层(D(2) = 0.73)的深度贴图的值仍低于第二层的深度值0.4，
所以我们继续。下一次迭代，这一层的深度值0.6大于深度贴图中采样的深度值(D(3) = 0.37)。
我们便可以假设第三层向量P¯
是可用的位移几何位置。我们可以用从向量P3¯
的纹理坐标偏移T3
来对fragment的纹理坐标进行位移。你可以看到随着深度曾的增加精确度也在提高。

两种最流行的解决方法叫做Relief Parallax Mapping和Parallax Occlusion Mapping，
Relief Parallax Mapping更精确一些，但是比Parallax Occlusion Mapping性能开销更多。
因为Parallax Occlusion Mapping的效果和前者差不多但是效率更高，
因此这种方式更经常使用，所以我们将在下面讨论一下。

* NOTE - 视差遮蔽映射
视差遮蔽映射(Parallax Occlusion Mapping)和陡峭视差映射的原则相同，
但不是用触碰的第一个深度层的纹理坐标，而是在触碰之前和之后，在深度层之间进行线性插值。
我们根据表面的高度距离啷个深度层的深度层值的距离来确定线性插值的大小。
看看下面的图片就能了解它是如何工作的：
![](https://learnopengl-cn.github.io/img/05/05/parallax_mapping_parallax_occlusion_mapping_diagram.png)


在对（位移的）表面几何进行交叉，找到深度层之后，我们获取交叉前的纹理坐标。
然后我们计算来自相应深度层的几何之间的深度之间的距离，并在两个值之间进行插值。
线性插值的方式是在两个层的纹理坐标之间进行的基础插值。函数最后返回最终的经过插值的纹理坐标。

视差遮蔽映射的效果非常好，尽管有一些可以看到的轻微的不真实和锯齿的问题，
这仍是一个好交易，因为除非是放得非常大或者观察角度特别陡，否则也看不到。

视差贴图是提升场景细节非常好的技术，但是使用的时候还是要考虑到它会带来一点不自然。
大多数时候视差贴图用在地面和墙壁表面，这种情况下查明表面的轮廓并不容易，
同时观察角度往往趋向于垂直于表面。这样视差贴图的不自然也就很难能被注意到了，
对于提升物体的细节可以起到难以置信的效果。
*/

/**REVIEW - HDR
一般来说，当存储在帧缓冲(Framebuffer)中时，亮度和颜色的值是默认被限制在0.0到1.0之间的。
这个看起来无辜的语句使我们一直将亮度与颜色的值设置在这个范围内，尝试着与场景契合。
这样是能够运行的，也能给出还不错的效果。但是如果我们遇上了一个特定的区域，
其中有多个亮光源使这些数值总和超过了1.0，又会发生什么呢？答案是这些片段中超过1.0
的亮度或者颜色值会被约束在1.0，从而导致场景混成一片，难以分辨：
![](https://learnopengl-cn.github.io/img/05/06/hdr_clamped.png)

这是由于大量片段的颜色值都非常接近1.0，在很大一个区域内每一个亮的片段都有相同的白色。
这损失了很多的细节，使场景看起来非常假。

解决这个问题的一个方案是减小光源的强度从而保证场景内没有一个片段亮于1.0。
然而这并不是一个好的方案，因为你需要使用不切实际的光照参数。
一个更好的方案是让颜色暂时超过1.0，然后将其转换至0.0到1.0的区间内，从而防止损失细节。

显示器被限制为只能显示值为0.0到1.0间的颜色，但是在光照方程中却没有这个限制。
通过使片段的颜色超过1.0，我们有了一个更大的颜色范围，这也被称作HDR(High
Dynamic Range, 高动态范围)。有了HDR，亮的东西可以变得非常亮，暗的东西可以变得非常暗，
而且充满细节。

HDR原本只是被运用在摄影上，摄影师对同一个场景采取不同曝光拍多张照片，捕捉大范围的色彩值。
这些图片被合成为HDR图片，从而综合不同的曝光等级使得大范围的细节可见。看下面这个例子，
左边这张图片在被光照亮的区域充满细节，但是在黑暗的区域就什么都看不见了；
但是右边这张图的高曝光却可以让之前看不出来的黑暗区域显现出来。
![](https://learnopengl-cn.github.io/img/05/06/hdr_image.png)

这与我们眼睛工作的原理非常相似，也是HDR渲染的基础。当光线很弱的啥时候，
人眼会自动调整从而使过暗和过亮的部分变得更清晰，就像人眼有一个能自动根据场景亮度调整的自动曝光滑块。

HDR渲染和其很相似，我们允许用更大范围的颜色值渲染从而获取大范围的黑暗与明亮的场景细节，
最后将所有HDR值转换成在[0.0, 1.0]范围的LDR(Low Dynamic Range,低动态范围)。
转换HDR值到LDR值得过程叫做色调映射(Tone Mapping)，现在现存有很多的色调映射算法，
这些算法致力于在转换过程中保留尽可能多的HDR细节。这些色调映射算法经常会包含一个
选择性倾向黑暗或者明亮区域的参数。

在实时渲染中，HDR不仅允许我们超过LDR的范围[0.0, 1.0]与保留更多的细节，
同时还让我们能够根据光源的真实强度指定它的强度。比如太阳有比闪光灯之类的东西更高的强度，
那么我们为什么不这样子设置呢?(比如说设置一个10.0的漫亮度) 这允许我们用更现实的光照参数
恰当地配置一个场景的光照，而这在LDR渲染中是不能实现的，因为他们会被上限约束在1.0。

因为显示器只能显示在0.0到1.0范围之内的颜色，我们肯定要做一些转换从而使得当前的HDR
颜色值符合显示器的范围。简单地取平均值重新转换这些颜色值并不能很好的解决这个问题，
因为明亮的地方会显得更加显著。我们能做的是用一个不同的方程与/或曲线来转换这些HDR值到LDR值
从而给我们对于场景的亮度完全掌控，这就是之前说的色调变换，也是HDR渲染的最终步骤。

* NOTE - 浮点帧缓冲
在实现HDR渲染之前，我们首先需要一些防止颜色值在每一个片段着色器运行后被限制约束的方法。
当帧缓冲使用了一个标准化的定点格式(像GL_RGB)为其颜色缓冲的内部格式，
OpenGL会在将这些值存入帧缓冲前自动将其约束到0.0到1.0之间。
这一操作对大部分帧缓冲格式都是成立的，除了专门用来存放被拓展范围值的浮点格式。

当一个帧缓冲的颜色缓冲的内部格式被设定成了GL_RGB16F, GL_RGBA16F, GL_RGB32F
或者GL_RGBA32F时，这些帧缓冲被叫做浮点帧缓冲(Floating Point Framebuffer)，
浮点帧缓冲可以存储超过0.0到1.0范围的浮点值，所以非常适合HDR渲染。

想要创建一个浮点帧缓冲，我们只需要改变颜色缓冲的内部格式参数就行了（注意GL_FLOAT参数)：

很明显，在隧道尽头的强光的值被约束在1.0，因为一大块区域都是白色的，
过程中超过1.0的地方损失了所有细节。因为我们直接转换HDR值到LDR值，
这就像我们根本就没有应用HDR一样。为了修复这个问题我们需要做的是无
损转化所有浮点颜色值回0.0-1.0范围中。我们需要应用到色调映射。

* NOTE - 色调映射
色调映射(Tone Mapping)是一个损失很小的转换浮点颜色值至我们所需的LDR[0.0, 1.0]范围内的过程，
通常会伴有特定的风格的色平衡(Stylistic Color Balance)。

最简单的色调映射算法是Reinhard色调映射，它涉及到分散整个HDR颜色值到LDR颜色值上，
所有的值都有对应。Reinhard色调映射算法平均地将所有亮度值分散到LDR上。
我们将Reinhard色调映射应用到之前的片段着色器上，并且为了更好的测量加上一个
Gamma校正过滤(包括SRGB纹理的使用)：

现在你可以看到在隧道的尽头木头纹理变得可见了。用了这个非常简单地色调映射算法，
我们可以合适的看到存在浮点帧缓冲中整个范围的HDR值，使我们能在不丢失细节的前提下，
对场景光照有精确的控制。

另一个有趣的色调映射应用是曝光(Exposure)参数的使用。你可能还记得之前我们在介绍里讲到的，
HDR图片包含在不同曝光等级的细节。如果我们有一个场景要展现日夜交替，
我们当然会在白天使用低曝光，在夜间使用高曝光，就像人眼调节方式一样。有了这个曝光参数，
我们可以去设置可以同时在白天和夜晚不同光照条件工作的光照参数，我们只需要调整曝光参数就行了。

在这里我们将exposure定义为默认为1.0的uniform，从而允许我们更加精确设定我们是要注重黑暗
还是明亮的区域的HDR颜色值。举例来说，高曝光值会使隧道的黑暗部分显示更多的细节，
然而低曝光值会显著减少黑暗区域的细节，但允许我们看到更多明亮区域的细节。
下面这组图片展示了在不同曝光值下的通道：

* NOTE - HDR拓展
在这里展示的两个色调映射算法仅仅是大量(更先进)的色调映射算法中的一小部分，
这些算法各有长短。一些色调映射算法倾向于特定的某种颜色/强度，也有一些算法
同时显示低高曝光颜色从而能够显示更加多彩和精细的图像。也有一些技巧被称作
自动曝光调整(Automatic Exposure Adjustment)或者叫人眼适应(Eye Adaptation)技术，
它能够检测前一帧场景的亮度并且缓慢调整曝光参数模仿人眼使得场景在黑暗区域
逐渐变亮或者在明亮区域逐渐变暗。

HDR渲染的真正优点在庞大和复杂的场景中应用复杂光照算法会被显示出来，
但是出于教学目的创建这样复杂的演示场景是很困难的，这个教程用的场景是很小的，
而且缺乏细节。但是如此简单的演示也是能够显示出HDR渲染的一些优点：
在明亮和黑暗区域无细节损失，因为它们可以通过色调映射重新获得；
多个光照的叠加不会导致亮度被截断的区域的出现，光照可以被设定为它们原来的亮度值而
不是被LDR值限制。而且，HDR渲染也使一些有趣的效果更加可行和真实; 其
中一个效果叫做泛光(Bloom)，我们将在下一节讨论。
*/

/**REVIEW - bloom
明亮的光源和区域经常很难向观察者表达出来，因为显示器的亮度范围是有限的。
一种在显示器上区分明亮光源的方式是使它们发出光芒，光芒从光源向四周发散。
这有效地给观众一种这些光源或明亮的区域非常亮的错觉。
（译注：这个问题的提出简单来说是为了解决这样的问题：例如有一张在阳光下的白纸，
白纸在显示器上显示出是出白色，而前方的太阳也是纯白色的，所以基本上白纸和太阳就是一样的了，
给太阳加一个光晕，这样太阳看起来似乎就比白纸更亮了）

这种光流，或发光效果，是通过一种叫做泛光(Bloom)的后期处理效果来实现的。
泛光使场景中所有明亮的区域都具有类似发光的效果。
下面是带有或不带有辉光的场景示例(图片由Epic Games提供)：
![](https://learnopengl-cn.github.io/img/05/07/bloom_example.png)

泛光提供了一种针对物体明亮度的视觉效果。当用优雅微妙的方式实现泛光效果
(有些游戏完全没能做到)，将会显著增强您的场景光照并能提供更加有张力的效果。

泛光和HDR结合使用效果最好。很多人以为HDR和泛光是一样的，认为两种技术是可以互换的，
这是一种常见误解。它们是两种完全不同的技术，用于各自不同的目的。可以使用默认的8位
精确度的帧缓冲来实现泛光效果，也可以只使用HDR效果而不使用泛光效果。只不过在有了
HDR之后再实现泛光就更简单了(正如我们稍后会看到的)。

为实现泛光，我们像平时那样渲染一个有光场景，提取出场景的HDR颜色缓冲以及只有这个
场景明亮区域可见的图片。然后对提取的亮度图像进行模糊处理，并将结果添加到原始HDR
场景图像的上面。

我们来一步一步解释这个处理过程。我们在场景中渲染一个带有4个立方体形式不同颜色的
明亮的光源。带有颜色的发光立方体的亮度在1.5到15.0之间。如果我们将其渲染至HDR
颜色缓冲，场景看起来会是这样的：

我们得到这个HDR颜色缓冲纹理，提取所有超出一定亮度的fragment。
这样我们就会获得一个只有fragment超过了一定阈限的颜色区域：

我们将这个超过一定亮度阈限的纹理进行模糊处理。
泛光效果的强度很大程度上是由模糊过滤器的范围和强度决定的。

最终的被模糊化的纹理就是我们用来获得发出光晕效果的东西。
这个已模糊的纹理要添加到原来的HDR场景纹理之上。由于模糊滤镜的作用，
明亮的区域在宽度和高度上都得到了扩展，因此场景中的明亮区域看起来会发光或流光。

泛光本身并不是个复杂的技术，但很难获得正确的效果。
它的品质很大程度上取决于所用的模糊过滤器的质量和类型。
简单地改改模糊过滤器就会极大的改变泛光效果的品质。
![](https://learnopengl-cn.github.io/img/05/07/bloom_steps.png)

有颜色的立方体看起来仿佛更亮，它向外发射光芒，的确是一个更好的视觉效果。
这个场景比较简单，所以泛光效果不算十分令人瞩目，但在更充足照明的场景中
合理配置之后效果会有明显的不同。你可以在这里找到这个简单示例的源代码。

这个教程我们只是用了一个相对简单的高斯模糊过滤器，它在每个方向上只有5个样本。
通过沿着更大的半径或重复更多次数的模糊，进行采样我们就可以提升模糊的效果。
因为模糊的质量与泛光效果的质量正相关，提升模糊效果就能够提升泛光效果。
有些提升将模糊过滤器与不同大小的模糊kernel或采用多个高斯曲线来选择性
地结合权重结合起来使用。来自Kalogirou和EpicGames的附加资源讨论了
如何通过提升高斯模糊来显著提升泛光效果。
*/
