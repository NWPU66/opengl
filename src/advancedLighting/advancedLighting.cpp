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
GLuint createImageObjrct(const char* imagePath);
GLuint createSkyboxTexture(const char* imageFolder);
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
    Shader phongShader("./shader/stdVerShader.vs.glsl",
                       "./shader/stdShadowedPhongLighting.fs.glsl");
    Shader lightObjShader("./shader/stdVerShader.vs.glsl", "./shader/stdPureColor.fs.glsl");
    GLuint cubeTexture      = createSkyboxTexture("./texture/");
    GLuint woodTexture      = createImageObjrct("./texture/wood.jpg");  // 创建立方体贴图
    GLuint containerTexture = createImageObjrct("./texture/container2.png");

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

    /**NOTE - 创建深度贴图（定向光源）
     */
    GLuint       depthMapFBO;
    GLuint       depthMapTexture;
    const GLuint SHADOW_WIDTH  = 1024;
    const GLuint SHADOW_HEIGHT = 1024;
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    // 设置texture属性
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = {1, 1, 1, 1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &borderColor[0]);
    // 深度纹理作为帧缓冲的深度缓冲
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    /**NOTE - 不包含颜色缓冲的帧缓冲对象是不完整的，所以我们需要显式告诉OpenGL
     * 我们不适用任何颜色数据进行渲染。我们通过将调用glDrawBuffer和glReadBuffer把
     * 读和绘制缓冲设置为GL_NONE来做这件事。
     */
    //  检查帧缓冲状态
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer is  complete!" << std::endl;
    }
    else { std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl; }
    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    /**NOTE - 生成深度贴图（定向光源）
     * 这里一定要记得调用glViewport。因为阴影贴图经常和我们原来渲染的场景（通常是
     * 窗口分辨率）有着不同的分辨率，我们需要改变视口（viewport）的参数以适应阴影贴图的尺寸。
     */
    // 光源的空间变换
    const GLfloat nearPlane = 0.1, farPlane = 15.0;
    mat4          lightView = lookAt(vec3(2, 3, 1), vec3(0), vec3(0, 1, 0));
    // mat4          lightProjection = ortho(-5.0f, 5.0f, -5.0f, 5.0f, nearPlane,
    //                                       farPlane);  // 正交投影变换
    mat4 lightProjection  = perspective(radians(45.0f), cameraAspect, nearPlane, farPlane);
    mat4 lightSpaceMatrix = lightProjection * lightView;
    // 渲染至深度贴图
    Shader stdNullShader("./shader/stdNullVShader.vs.glsl", "./shader/stdNullFShader.fs.glsl");
    stdNullShader.use();
    stdNullShader.setParameter("view", lightView);
    stdNullShader.setParameter("projection", lightProjection);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);  // FIXME - 写如深度值得时候，记得启动深度缓冲
    glEnable(GL_CULL_FACE);   // FIXME - 记得启动面剔除
    glCullFace(GL_FRONT);  // 正面剔除，深度会稍微大一些，但仍然再阴影得前面
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    // render scene
    stdNullShader.setParameter("model", scale(mat4(1), vec3(5)));
    plane.Draw(&stdNullShader);
    stdNullShader.setParameter("model", translate(scale(mat4(1), vec3(0.5)), vec3(0, 1, 0)));
    box.Draw(&stdNullShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCullFace(GL_BACK);

    /**NOTE - 创建深度立方体贴图（点光源）
     */
    GLuint depthCubemapTexture;
    glGenTextures(1, &depthCubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemapTexture);
    for (int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
                     SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // 将cubeMap绑定在FBO上
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    /**NOTE - 生成深度立方体贴图（点光源）
     */
    // 光空间的变换

    // 渲染循环
    while (!glfwWindowShouldClose(window))
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
        // 木地板
        phongShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        phongShader.setParameter("texture0", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        phongShader.setParameter("shadowMap", 1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        phongShader.setParameter("model", scale(mat4(1), vec3(5)));
        phongShader.setParameter("view", view);
        phongShader.setParameter("projection", projection);
        phongShader.setParameter("cameraPos", camera->Position);
        phongShader.setParameter("lightSpaceMatrix", lightSpaceMatrix);
        phongShader.setParameter("skybox", 0);
        plane.Draw(&phongShader);
        glBindTexture(GL_TEXTURE_2D, 0);
        // 木板箱子
        phongShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, containerTexture);
        phongShader.setParameter("texture0", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        phongShader.setParameter("shadowMap", 1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        phongShader.setParameter("model", translate(scale(mat4(1), vec3(0.5)), vec3(0, 1, 0)));
        phongShader.setParameter("view", view);
        phongShader.setParameter("projection", projection);
        phongShader.setParameter("cameraPos", camera->Position);
        phongShader.setParameter("lightSpaceMatrix", lightSpaceMatrix);
        phongShader.setParameter("skybox", 0);
        box.Draw(&phongShader);
        glBindTexture(GL_TEXTURE_2D, 0);  // TODO - 两个槽位都要解除

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
        // 阴影贴图等效灯光
        lightObjShader.setParameter("model", scale(translate(mat4(1), vec3(2, 3, 1)), vec3(0.1)));
        lightObjShader.setParameter("lightColor", vec3(1, 0, 0));
        sphere.Draw(&lightObjShader);

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
GLuint createImageObjrct(const char* imagePath)
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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         data);
            //  设置为GL_SRGB时，OpenGL回自动对图片进行重校
            // FIXME - 注意，对于在线性空间下创建的纹理，如法线贴图，不能设置SRGB重校。
        }
        else  // nrChannels == 4
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, data);
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
GLuint createSkyboxTexture(const char* imageFolder)
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
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width, height, 0, GL_RGB,
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

/**REVIEW - 阴影映射
 * 阴影是光线被阻挡的结果；当一个光源的光线由于其他物体的阻挡不能够达到一个物体的表面的时候，
 * 那么这个物体就在阴影中了。阴影能够使场景看起来真实得多，并且可以让观察者获得物体之间的
 * 空间位置关系。场景和物体的深度感因此能够得到极大提升，下图展示了有阴影和没有阴影
 * 的情况下的不同：
 * ![](https://learnopengl-cn.github.io/img/05/03/01/shadow_mapping_with_without.png)
 *
 * 你可以看到，有阴影的时候你能更容易地区分出物体之间的位置关系，
 * 例如，当使用阴影的时候浮在地板上的立方体的事实更加清晰。
 *
 * 阴影还是比较不好实现的，因为当前实时渲染领域还没找到一种完美的阴影算法。
 * 目前有几种近似阴影技术，但它们都有自己的弱点和不足，这点我们必须要考虑到。
 *
 * 视频游戏中较多使用的一种技术是阴影贴图（shadow mapping），效果不错，而且相对容易实现。
 * 阴影贴图并不难以理解，性能也不会太低，而且非常容易扩展成更高级的算法
 * （比如 Omnidirectional Shadow Maps和 Cascaded Shadow Maps）。
 *
 * NOTE - 阴影映射
 * 阴影映射(Shadow Mapping)背后的思路非常简单：我们以光的位置为视角进行渲染，
 * 我们能看到的东西都将被点亮，看不见的一定是在阴影之中了。假设有一个地板，
 * 在光源和它之间有一个大盒子。由于光源处向光线方向看去，可以看到这个盒子，
 * 但看不到地板的一部分，这部分就应该在阴影中了。
 * ![](https://learnopengl-cn.github.io/img/05/03/01/shadow_mapping_theory.png)
 *
 * 这里的所有蓝线代表光源可以看到的fragment。黑线代表被遮挡的fragment：它们应该渲染为
 * 带阴影的。如果我们绘制一条从光源出发，到达最右边盒子上的一个片段上的线段或射线，
 * 那么射线将先击中悬浮的盒子，随后才会到达最右侧的盒子。结果就是悬浮的盒子被照亮，
 * 而最右侧的盒子将处于阴影之中。
 *
 * 我们希望得到射线第一次击中的那个物体，然后用这个最近点和射线上其他点进行对比。
 * 然后我们将测试一下看看射线上的其他点是否比最近点更远，如果是的话，这个点就在阴影中。
 * 对从光源发出的射线上的成千上万个点进行遍历是个极端消耗性能的举措，实时渲染上基本不可取。
 * 我们可以采取相似举措，不用投射出光的射线。我们所使用的是非常熟悉的东西：深度缓冲。
 *
 * 你可能记得在深度测试教程中，在深度缓冲里的一个值是摄像机视角下，对应于一个片段的一个
 * 0到1之间的深度值。如果我们从光源的透视图来渲染场景，并把深度值的结果储存到纹理中会怎样？
 * 通过这种方式，我们就能对光源的透视图所见的最近的深度值进行采样。最终，深度值就会显示从
 * 光源的透视图下见到的第一个片段了。我们管储存在纹理中的所有这些深度值，叫做深度贴图
 * （depth map）或阴影贴图。
 * ![](https://learnopengl-cn.github.io/img/05/03/01/shadow_mapping_theory_spaces.png)
 *
 * 左侧的图片展示了一个定向光源（所有光线都是平行的）在立方体下的表面投射的阴影。
 * 通过储存到深度贴图中的深度值，我们就能找到最近点，用以决定片段是否在阴影中。
 * 我们使用一个来自光源的视图和投影矩阵来渲染场景就能创建一个深度贴图。这个投影和
 * 视图矩阵结合在一起成为一个T变换，它可以将任何三维位置转变到光源的可见坐标空间。
 *
 * 定向光并没有位置，因为它被规定为无穷远。然而，为了实现阴影贴图，
 * 我们得从一个光的透视图渲染场景，这样就得在光的方向的某一点上渲染场景。
 *
 * 在右边的图中我们显示出同样的平行光和观察者。我们渲染一个点P¯处的片段，需要决定它是
 * 否在阴影中。我们先得使用T把P¯变换到光源的坐标空间里。既然点P¯是从光的透视图中看到的，
 * 它的z坐标就对应于它的深度，例子中这个值是0.9。使用点P¯在光源的坐标空间的坐标，
 * 我们可以索引深度贴图，来获得从光的视角中最近的可见深度，结果是点C¯，最近的深度是0.4。
 * 因为索引深度贴图的结果是一个小于点P¯的深度，我们可以断定P¯被挡住了，它在阴影中了。
 *
 * 阴影映射由两个步骤组成：首先，我们渲染深度贴图，然后我们像往常一样渲染场景，
 * 使用生成的深度贴图来计算片段是否在阴影之中。听起来有点复杂，但随着我们一步一步
 * 地讲解这个技术，就能理解了。
 *
 * NOTE - 光源空间的变换
 *
 * NOTE - 渲染阴影
 *
 * NOTE - 改进阴影贴图
 * 我们试图让阴影映射工作，但是你也看到了，阴影映射还是有点不真实，我们修复它才
 * 能获得更好的效果，这是下面的部分所关注的焦点。
 *
 * 我们可以看到地板四边形渲染出很大一块交替黑线。
 * 这种阴影贴图的不真实感叫做阴影失真(Shadow Acne)，下图解释了成因：
 * ![](https://learnopengl-cn.github.io/img/05/03/01/shadow_mapping_acne_diagram.png)
 *
 * 因为阴影贴图受限于分辨率，在距离光源比较远的情况下，多个片段可能从深度贴图的同一个
 * 值中去采样。图片每个斜坡代表深度贴图一个单独的纹理像素。
 * 你可以看到，多个片段从同一个深度值进行采样。
 *
 * 虽然很多时候没问题，但是当光源以一个角度朝向表面的时候就会出问题，这种情况下深度贴图也
 * 是从一个角度下进行渲染的。多个片段就会从同一个斜坡的深度纹理像素中采样，有些在地板上面，
 * 有些在地板下面；这样我们所得到的阴影就有了差异。因为这个，有些片段被认为是在阴影之中，
 * 有些不在，由此产生了图片中的条纹样式。
 *
 * 我们可以用一个叫做阴影偏移（shadow bias）的技巧来解决这个问题，我们简单的对表面的深度
 * （或深度贴图）应用一个偏移量，这样片段就不会被错误地认为在表面之下了。
 *
 * 这里我们有一个偏移量的最大值0.05，和一个最小值0.005，它们是基于表面法线和光照方向的。
 * 这样像地板这样的表面几乎与光源垂直，得到的偏移就很小，而比如立方体的侧面这种表面得
 * 到的偏移就更大。下图展示了同一个场景，但使用了阴影偏移，效果的确更好：
 *
 * NOTE - 偏移
 * 使用阴影偏移的一个缺点是你对物体的实际深度应用了平移。偏移有可能足够大，
 * 以至于可以看出阴影相对实际物体位置的偏移，你可以从下图看到这个现象（这是一个夸张的偏移值）：
 * ![](https://learnopengl-cn.github.io/img/05/03/01/shadow_mapping_peter_panning.png)
 *
 * 这个阴影失真叫做悬浮(Peter Panning)，因为物体看起来轻轻悬浮在表面之上（译注Peter Pan
 * 就是童话彼得潘，而panning有平移、悬浮之意，而且彼得潘是个会飞的男孩…）。我们可以使用
 * 一个叫技巧解决大部分的Peter panning问题：当渲染深度贴图时候使用正面剔除（front face culling）
 * 你也许记得在面剔除教程中OpenGL默认是背面剔除。我们要告诉OpenGL我们要剔除正面。
 *
 * 因为我们只需要深度贴图的深度值，对于实体物体无论我们用它们的正面还是背面都没问题。
 * 使用背面深度不会有错误，因为阴影在物体内部有错误我们也看不见。
 * ![](https://learnopengl-cn.github.io/img/05/03/01/shadow_mapping_culling.png)
 *
 * 这十分有效地解决了peter panning的问题，但只对内部不会对外开口的实体物体有效。
 * 我们的场景中，在立方体上工作的很好，但在地板上无效，因为正面剔除完全移除了地板。
 * 地面是一个单独的平面，不会被完全剔除。如果有人打算使用这个技巧解决peter panning
 * 必须考虑到只有剔除物体的正面才有意义。
 *
 * 另一个要考虑到的地方是接近阴影的物体仍然会出现不正确的效果。
 * 必须考虑到何时使用正面剔除对物体才有意义。不过使用普通的偏移值通常就能避免peter panning。
 *
 * NOTE - 采样过多
 * 无论你喜不喜欢还有一个视觉差异，就是光的视锥不可见的区域一律被认为是处于阴影中，
 * 不管它真的处于阴影之中。出现这个状况是因为超出光的视锥的投影坐标比1.0大，
 * 这样采样的深度纹理就会超出他默认的0到1的范围。根据纹理环绕方式，我们将会得到
 * 不正确的深度结果，它不是基于真实的来自光源的深度值。
 *
 * 你可以在图中看到，光照有一个区域，超出该区域就成为了阴影；这个区域实际上代表着
 * 深度贴图的大小，这个贴图投影到了地板上。发生这种情况的原因是我们之前将深度贴图
 * 的环绕方式设置成了GL_REPEAT。
 *
 * 我们宁可让所有超出深度贴图的坐标的深度范围是1.0，这样超出的坐标将永远不在阴影之中。
 * 我们可以储存一个边框颜色，然后把深度贴图的纹理环绕选项设置为GL_CLAMP_TO_BORDER：
 *
 * 现在如果我们采样深度贴图0到1坐标范围以外的区域，
 * 纹理函数总会返回一个1.0的深度值，阴影值为0.0。结果看起来会更真实：
 *
 * 这些结果意味着，只有在深度贴图范围以内的被投影的fragment坐标才有阴影，
 * 所以任何超出范围的都将会没有阴影。由于在游戏中通常这只发生在远处，
 * 就会比我们之前的那个明显的黑色区域效果更真实。
 *
 * NOTE - PCF
 * 阴影现在已经附着到场景中了，不过这仍不是我们想要的。如果你放大看阴影，阴影映射对分辨率的依赖很快变得很明显。
 *
 * 因为深度贴图有一个固定的分辨率，多个片段对应于一个纹理像素。
 * 结果就是多个片段会从深度贴图的同一个深度值进行采样，这几个片段便得到的是同一个阴影，这就会产生锯齿边。
 *
 * 另一个（并不完整的）解决方案叫做PCF（percentage-closer
 * filtering），这是一种多个不同过滤方式的组合，
 * 它产生柔和阴影，使它们出现更少的锯齿块和硬边。核心思想是从深度贴图中多次采样，
 * 每一次采样的纹理坐标都稍有不同。每个独立的样本可能在也可能不再阴影中。
 * 所有的次生结果接着结合在一起，进行平均化，我们就得到了柔和阴影。
 *
 * 这个textureSize返回一个给定采样器纹理的0级mipmap的vec2类型的宽和高。用1除以它返回一个
 * 单独纹理像素的大小，我们用以对纹理坐标进行偏移，确保每个新样本，来自不同的深度值。
 * 这里我们采样得到9个值，它们在投影坐标的x和y值的周围，
 * 为阴影阻挡进行测试，并最终通过样本的总数目将结果平均化。
 *
 * NOTE - 正交 vs 投影
 * 在渲染深度贴图的时候，正交(Orthographic)和投影(Projection)矩阵之间有所不同。
 * 正交投影矩阵并不会将场景用透视图进行变形，所有视线/光线都是平行的，这使它
 * 对于定向光来说是个很好的投影矩阵。然而透视投影矩阵，会将所有顶点根据透视关系进行变形，
 * 结果因此而不同。下图展示了两种投影方式所产生的不同阴影区域：
 * ![](https://learnopengl-cn.github.io/img/05/03/01/shadow_mapping_projection.png)
 *
 * 透视投影对于光源来说更合理，不像定向光，它是有自己的位置的。
 * 透视投影因此更经常用在点光源和聚光灯上，而正交投影经常用在定向光上。
 *
 * 另一个细微差别是，透视投影矩阵，将深度缓冲视觉化经常会得到一个几乎全白的结果。
 * 发生这个是因为透视投影下，深度变成了非线性的深度值，它的大多数可辨范围都位于近平面附近。
 * 为了可以像使用正交投影一样合适地观察深度值，
 * 你必须先将非线性深度值转变为线性的，如我们在深度测试教程中已经讨论过的那样。
 *
 * 这个深度值与我们见到的用正交投影的很相似。需要注意的是，这个只适用于调试；
 * 正交或投影矩阵的深度检查仍然保持原样，因为相关的深度并没有改变。
 */

/**REVIEW - 点光源阴影
 * 上个教程我们学到了如何使用阴影映射技术创建动态阴影。效果不错，但它只适合定向光，
 * 因为阴影只是在单一定向光源下生成的。所以它也叫定向阴影映射，深度（阴影）
 * 贴图生成自定向光的视角。
 *
 * 本节我们的焦点是在各种方向生成动态阴影。这个技术可以适用于点光源，生成所有方向上的阴影。
 *
 * 这个技术叫做点光阴影，过去的名字是万向阴影贴图（omnidirectional shadow maps）技术。
 *
 * 本节代码基于前面的阴影映射教程，所以如果你对传统阴影映射不熟悉，还是建议先读一读
 * 阴影映射教程。 算法和定向阴影映射差不多：我们从光的透视图生成一个深度贴图，
 * 基于当前fragment位置来对深度贴图采样，然后用储存的深度值和每个fragment进行对比，
 * 看看它是否在阴影中。定向阴影映射和万向阴影映射的主要不同在于深度贴图的使用上。
 *
 * 对于深度贴图，我们需要从一个点光源的所有渲染场景，普通2D深度贴图不能工作；
 * 如果我们使用立方体贴图会怎样？因为立方体贴图可以储存6个面的环境数据，
 * 它可以将整个场景渲染到立方体贴图的每个面上，把它们当作点光源四周的深度值来采样。
 * ![](https://learnopengl-cn.github.io/img/05/03/02/point_shadows_diagram.png)
 *
 * 生成后的深度立方体贴图被传递到光照像素着色器，它会用一个方向向量来采样立方体贴图，
 * 从而得到当前的fragment的深度（从光的透视图）。
 * 大部分复杂的事情已经在阴影映射教程中讨论过了。算法只是在深度立方体贴图生成上稍微复杂一点。
 *
 * NOTE - 生成深度立方体贴图
 * 为创建一个光周围的深度值的立方体贴图，我们必须渲染场景6次：每次一个面。
 * 显然渲染场景6次需要6个不同的视图矩阵，每次把一个不同的立方体贴图面附加到帧缓冲对象上。
 *
 * 这会很耗费性能因为一个深度贴图下需要进行很多渲染调用。这个教程中我们将转而使用另外的
 * 一个小技巧来做这件事，几何着色器允许我们使用一次渲染过程来建立深度立方体贴图。
 *
 * 正常情况下，我们把立方体贴图纹理的一个面附加到帧缓冲对象上，渲染场景6次，
 * 每次将帧缓冲的深度缓冲目标改成不同立方体贴图面。由于我们将使用一个几何着色器，
 * 它允许我们把所有面在一个过程渲染，我们可以使用glFramebufferTexture直接把立方体
 * 贴图附加成帧缓冲的深度附件：
 *
 * 设置了帧缓冲和立方体贴图，我们需要一些方法来讲场景的所有几何体变换到6个光的方向
 * 中相应的光空间。与阴影映射教程类似，我们将需要一个光空间的变换矩阵T
 * ，但是这次是每个面都有一个。
 *
 * 每个光空间的变换矩阵包含了投影和视图矩阵。对于投影矩阵来说，我们将使用一个透视投影矩阵；
 * 光源代表一个空间中的点，所以透视投影矩阵更有意义。每个光空间变换矩阵使用同样的投影矩阵：
 */
