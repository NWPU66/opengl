#include <glad/glad.h>
// GLAD first
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "util/class_camera.hpp"
#include "util/class_model.hpp"
#include "util/class_shader.hpp"
#include "util/debugTool.hpp"
#include "util/lightGroup.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

using namespace glm;
using namespace std;

#ifdef NDEBUG
#    define GL_CHECK()
#else
#    define GL_CHECK() glCheckError_(__FILE__, __LINE__)
#endif

/**NOTE - 全局变量、摄像机、全局时钟以及函数
 */
const GLint CAMERA_WIDTH = 1920;
const GLint CAMERA_HEIGH = 1080;
const float cameraAspect = static_cast<float>(CAMERA_WIDTH) / static_cast<float>(CAMERA_HEIGH);
Camera*     camera       = new Camera(vec3(0.0F, 1.0F, 2.0F), vec3(0.0F, 1.0F, 0.0F), -90.0F, 0.0F);
float       mouseLastX = 0.0f, mouseLastY = 0.0f;  // 记录鼠标的位置
float       lastFrame = 0.0f, deltaTime = 0.0f;    // 全局时钟

void          framebuffer_size_callback(GLFWwindow* window, int w, int h);
void          mouse_callback(GLFWwindow* window, double xpos, double ypos);
void          scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void          processInput(GLFWwindow* window);
int           initGLFWWindow(GLFWwindow*& window);
GLuint        createImageObjrct(const char* imagePath,
                                const bool  autoGammaCorrection = true,
                                const bool  flip_texture        = true);
GLuint        createSkyboxTexture(const char* imageFolder, const bool autoGammaCorrection = true);
void          createFBO(GLuint& fbo, GLuint& texAttachment, GLuint& rbo, const char* hint = "null");
void          createObjFromHardcode(GLuint&         vao,
                                    GLuint&         vbo,
                                    GLuint&         ebo,
                                    vector<GLfloat> vertices,
                                    vector<GLuint>  vertexIdx = {});
void          renderTextureToScreen(const GLuint screenVAO,
                                    const GLuint textureToShow,
                                    Shader&      screenShader);
void APIENTRY glDebugOutput(GLenum        source,
                            GLenum        type,
                            GLuint        id,
                            GLenum        severity,
                            GLsizei       length,
                            const GLchar* message,
                            const void*   userParam);
GLenum        glCheckError_(const char* file, int32_t line);

int main(int /*argc*/, char** /*argv*/)
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (initGLFWWindow(window) == 0) return -1;

        // glfw opengl debug
#ifdef NDEBUG
#else
    int32_t flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if ((flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0)
    {
        glDebugMessageCallback(glDebugOutput, nullptr);  // 向opengl注册调试回调函数
        glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR,
                              GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_TRUE);
    }
    else { printf("GL_CONTEXT_FLAG_DEBUG_BIT is not enabled."); }
#endif

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);                            // 启用深度缓冲
    glDepthFunc(GL_LEQUAL);                             // 修改深度测试的标准
    glEnable(GL_STENCIL_TEST);                          // 启用模板缓冲
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);       // 设置模板缓冲的操作
    glEnable(GL_BLEND);                                 // 启用混合
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数
    // glEnable(GL_CULL_FACE);                             // 启用面剔除
    glClearColor(0.2F, 0.3F, 0.3F, 1.0F);  // 设置清空颜色
    glEnable(GL_MULTISAMPLE);              // 启用多重采样
    glEnable(GL_FRAMEBUFFER_SRGB);         // 自动Gamme矫正

    /**NOTE - 模型和着色器、纹理
     */
    Model  box("./box/box.obj");
    Model  plane("./plane/plane.obj");
    Model  monkey("./monkey/monkey.obj");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl", "./shader/skyboxShader.fs.glsl");
    GLuint cubeTexture = createSkyboxTexture("./texture/", true);
    Shader phongShader("./shader/stdVerShader.vs.glsl", "./shader/stdPhongLighting.fs.glsl");
    GLuint woodTexture = createImageObjrct("./texture/wood.jpg");
    Model  sphere("./sphere/sphere.obj");
    Shader lightObjShader("./shader/stdVerShader.vs.glsl", "./shader/stdPureColor.fs.glsl");
    GLuint containerTexture = createImageObjrct("./texture/container2.png");
    Shader baking_shader("./shader/bake_it.vs.glsl", "./shader/bake_it.fs.glsl");
    Shader use_baked_texture_shader("./shader/stdVerShader.vs.glsl",
                                    "./shader/use_baked_texture.fs.glsl");
    GL_CHECK();

    /**NOTE - 灯光组
     */
    LightGroup lightGroup;
    lightGroup.addLight(Light(0, vec3(1, 1, 1), 2, vec3(1, 1.5, 1)));
    lightGroup.addLight(Light(1, vec3(1, 1, 1), 1.2, vec3(0, 0, 0), vec3(1, -1, 1)));
    lightGroup.addLight(Light(2, vec3(1, 1, 1), 1, vec3(0, 2.0, 0), vec3(0, -1, 0)));
    lightGroup.createLightUniformBuffer();
    lightGroup.bindingUniformBuffer(0);
    GL_CHECK();

    /**NOTE - ScreenTextureObject for debug
     */
    DebugTool debugTool;

    /**NOTE - frame buffer object
     */
    vector<unsigned char> image_data(1024 * 1024 * 3, 0);
    GLuint                fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // 创建一个纹理附件（Texture Attachment）
    GLuint bake_texture;
    glGenTextures(1, &bake_texture);
    glBindTexture(GL_TEXTURE_2D, bake_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 image_data.data());
    /**NOTE -
    这里出GL_R32F试一下单通道图，32位浮点数
    format 和 type 指的应该是从外部通过 data 指针传入的数据格式，按我的理解
    我不从外向内传数据，这两个值应该随便填都可以
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    // 设置frame buffer的颜色附件
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bake_texture, 0);
    //  检查帧缓冲状态
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer is  complete!" << std::endl;
    }
    else { std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl; }
    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_CHECK();

    /**NOTE - BAKE IT !
     */
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1024, 1024);
    glClear(GL_COLOR_BUFFER_BIT);
    // monkey
    baking_shader.use();
    baking_shader.setParameter("model", translate(scale(mat4(1), vec3(0.5f)), vec3(0, 1, 0)));
    monkey.Draw(&baking_shader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // FIXME - glMapBuffer不能绑定GL_TEXTURE_2D
    /**FIXME - model矩阵是空的
     * 原来是 baking_shader.use(); 没写, 啊这...
     */

    /**NOTE - 从OpenGL中读取图像数据
    单个PBO读取屏幕数据效率大概和 glReadPixels() 差不多，双PBO交换读取效率会很高。原因是
    使用PBO时，屏幕上的数据不是读取到内存，而是从显卡读到PBO中，或者如果内部机制是读取
    到内存中，但这也是由DMA 控制器来完成的，而不是cpu指令来做的，
    再加上两个PBO交换使用，所以读取效率很高。
    */
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    // FIXME - pbo缓冲还没有分配显存空间
    glBufferData(GL_PIXEL_PACK_BUFFER, 1024 * 1024 * 3, nullptr, GL_STREAM_READ);
    // 绑定read source buffer
    // FIXME - glBindFramebuffer()才可以绑定GL_READ_FRAMEBUFFER
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    // 如果glReadPixels的data参数为nullptr，且存在绑定到GL_PIXEL_PACK_BUFFER的缓冲
    // 则将数据从GL_READ_FRAMEBUFFER传送到GL_PIXEL_PACK_BUFFER上
    glReadPixels(0, 0, 1024, 1024, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    // 出一枚指向缓冲的指针
    unsigned char* ptr =
        static_cast<unsigned char*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
    stbi_write_png("baked_image.png", 1024, 1024, 3, ptr, 1024 * 3);
    // 解绑
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    /**NOTE - 双pbo缓冲请参考：
    https://www.jianshu.com/p/019a29a55352
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
        // monkey
        // phongShader.use();
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, containerTexture);
        // phongShader.setParameter("texture0", 0);
        // phongShader.setParameter("model", translate(scale(mat4(1), vec3(0.5F)), vec3(0, 1, 0)));
        // monkey.Draw(&phongShader);

        // NOTE - use baked texture to do something
        use_baked_texture_shader.use();
        use_baked_texture_shader.setParameter("model",
                                              translate(scale(mat4(1), vec3(0.5F)), vec3(0, 1, 0)));
        use_baked_texture_shader.setParameter("view", view);
        use_baked_texture_shader.setParameter("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bake_texture);
        use_baked_texture_shader.setParameter("baked_texture", 0);
        monkey.Draw(&use_baked_texture_shader);

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
        glViewport(0, 0, 1024, 1024);
        debugTool.renderTextureToScreen(bake_texture);

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

/// @brief opengl debug 回调函数
void APIENTRY glDebugOutput(GLenum        source,
                            GLenum        type,
                            GLuint        id,
                            GLenum        severity,
                            GLsizei       length,
                            const GLchar* message,
                            const void*   userParam)
{
    // 忽略一些不重要的错误/警告代码
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) { return; }

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API: std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION: std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER: std::cout << "Source: Other"; break;
    }
    std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR: std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY: std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER: std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP: std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP: std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER: std::cout << "Type: Other"; break;
    }
    std::cout << std::endl;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH: std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW: std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    }
    std::cout << "---------------" << std::endl;
    std::cout << std::endl;
}

GLenum glCheckError_(const char* file, int32_t line)
{
    GLenum errorCode = glGetError();
    if (errorCode == GL_NO_ERROR) { return GL_NO_ERROR; }

    std::string error;
    switch (errorCode)
    {
        case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW: error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW: error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        default: error = "Unknown error code: " + std::to_string(errorCode); break;
    }
    std::cout << "OpenGL error: " << error << " | " << file << " (" << line << ")" << std::endl;

    return errorCode;
}
