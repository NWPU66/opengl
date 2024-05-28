#include <glad/glad.h>
// GLAD first
#define STB_IMAGE_IMPLEMENTATION
#include "util/class_camera.hpp"
#include "util/class_model.hpp"
#include "util/class_shader.hpp"
#include "util/lightGroup.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
using namespace glm;
using namespace std;

/**NOTE - 全局变量、摄像机、全局时钟以及函数
 */
const GLint CAMERA_WIDTH = 800;
const GLint CAMERA_HEIGH = 600;
const float cameraAspect = (float)CAMERA_WIDTH / (float)CAMERA_HEIGH;
Camera*     camera       = new Camera(vec3(0.0f, 0.0f, 2.0f), vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float       mouseLastX = 0.0f, mouseLastY = 0.0f;  // 记录鼠标的位置
float       lastFrame = 0.0f, deltaTime = 0.0f;    // 全局时钟

void   framebuffer_size_callback(GLFWwindow* window, int w, int h);
void   mouse_callback(GLFWwindow* window, double xpos, double ypos);
void   scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void   processInput(GLFWwindow* window);
int    initGLFWWindow(GLFWwindow*& window);
GLuint createImageObjrct(const char* imagePath);
GLuint createSkyboxTexture(const char* imageFolder);
mat4   makeRandomPosture(const float propotion,
                         const float radius = 50.0f,
                         const float offset = 2.5f);
void   createFBO(GLuint& fbo, GLuint& texAttachment, GLuint& rbo, const char* hint = "null");
void   createObjFromHardcode(GLuint&         vao,
                             GLuint&         vbo,
                             GLuint&         ebo,
                             vector<GLfloat> vertices,
                             vector<GLuint>  vertexIdx = {});

int main(int argc, char** argv)
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (!initGLFWWindow(window)) return -1;

    /**NOTE - 模型和着色器、纹理
     */
    Model  box("./box/box.obj"), plane("./plane/plane.obj"), sphere("./sphere/sphere.obj");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl", "./shader/skyboxShader.fs.glsl"),
        phongShader("./shader/stdVerShader.vs.glsl", "./shader/stdPhongLighting.fs.glsl"),
        lightObjShader("./shader/stdVerShader.vs.glsl", "./shader/stdPureColor.fs.glsl");
    GLuint cubeTexture      = createSkyboxTexture("./texture/"),  // 创建立方体贴图
        woodTexture         = createImageObjrct("./texture/wood.jpg"),
           containerTexture = createImageObjrct("./texture/container2.png");

    /**NOTE - 灯光组
     */
    LightGroup lightGroup;
    lightGroup.addLight(Light(0, vec3(1, 1, 1), 2, vec3(1, 1.5, 1)));
    lightGroup.addLight(Light(1, vec3(1, 1, 1), 1.2, vec3(0, 0, 0), vec3(1, -1, 1)));
    lightGroup.addLight(Light(2, vec3(1, 1, 1), 1, vec3(0, 1.5, 0), vec3(0, -1, 0)));
    lightGroup.createLightUniformBuffer();
    lightGroup.bindingUniformBuffer(0);

    /**NOTE - 创建深度贴图
     */
    GLuint       depthMapFBO, depthMapTexture;
    const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    glGenBuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    // 设置texture属性
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    // 深度纹理作为帧缓冲的深度缓冲
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    /**NOTE - 不包含颜色缓冲的帧缓冲对象是不完整的，所以我们需要显式告诉OpenGL
     * 我们不适用任何颜色数据进行渲染。我们通过将调用glDrawBuffer和glReadBuffer把
     * 读和绘制缓冲设置为GL_NONE来做这件事。
     */
    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    /**NOTE - 生成深度贴图
     */
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    // rendering
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    /**NOTE - 这里一定要记得调用glViewport。因为阴影贴图经常和我们原来渲染的场景（通常是
     * 窗口分辨率）有着不同的分辨率，我们需要改变视口（viewport）的参数以适应阴影贴图的尺寸。
     */

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);                            // 启用深度缓冲
    glDepthFunc(GL_LEQUAL);                             // 修改深度测试的标准
    glEnable(GL_STENCIL_TEST);                          // 启用模板缓冲
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);       // 设置模板缓冲的操作
    glEnable(GL_BLEND);                                 // 启用混合
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数
    glEnable(GL_CULL_FACE);                             // 启用面剔除
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);               // 设置清空颜色
    glEnable(GL_MULTISAMPLE);                           // 启用多重采样
    glEnable(GL_FRAMEBUFFER_SRGB);                      // 自动Gamme矫正

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
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        phongShader.setParameter("model", scale(mat4(1), vec3(5)));
        phongShader.setParameter("view", view);
        phongShader.setParameter("projection", projection);
        phongShader.setParameter("cameraPos", camera->Position);
        phongShader.setParameter("skybox", 0);
        plane.Draw(&phongShader);
        glBindTexture(GL_TEXTURE_2D, 0);
        // 木板箱子
        phongShader.use();
        glBindTexture(GL_TEXTURE_2D, containerTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        phongShader.setParameter("model", translate(scale(mat4(1), vec3(0.5)), vec3(0, 1, 0)));
        phongShader.setParameter("view", view);
        phongShader.setParameter("projection", projection);
        phongShader.setParameter("cameraPos", camera->Position);
        phongShader.setParameter("skybox", 0);
        box.Draw(&phongShader);
        glBindTexture(GL_TEXTURE_2D, 0);

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

/// @brief 为陨石生成一个随机的姿态
/// @param propotion
/// @param radius 生成的目标半径
/// @param offset 位置的随机偏移量
/// @return 返回一个姿态矩阵mat4
mat4 makeRandomPosture(const float propotion, const float radius, const float offset)
{
    // TODO - 做陨石的旋转
    mat4 model(1.0f);
    // 1. 位移：分布在半径为 'radius' 的圆形上，偏移的范围是 [-offset, offset]
    float angle        = propotion * 360.0f;
    float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float x            = cos(angle) * radius + displacement;
    displacement       = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float y            = displacement * 0.4f;  //// 让行星带的高度比x和z的宽度要小
    displacement       = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float z            = sin(angle) * radius + displacement;
    model              = translate(mat4(1.0f), vec3(x, y, z));

    // 2. 缩放：在 0.05 和 0.25f 之间缩放
    float scale = (rand() % 20) / 100.0f + 0.05;
    model       = glm::scale(model, vec3(scale * 0.5f));

    // 3. 旋转：绕着一个（半）随机选择的旋转轴向量进行随机的旋转
    float rotAngle = (rand() % 360);
    model          = rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

    return model;
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

    // TODO - 从一组顶点的硬编码创建几何体
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
 */
