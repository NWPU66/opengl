#include "glm/trigonometric.hpp"
#include <array>
#include <functional>
#include <limits>
#include <random>
#include <string>
#include <tuple>

#include <glad/glad.h>
#include <type_traits>
#include <vector>
// GLAD first

#define STB_IMAGE_IMPLEMENTATION
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ltc_matrix.hpp"
#include "util/class_camera.hpp"
#include "util/class_model.hpp"
#include "util/class_shader.hpp"
#include "util/debugTool.hpp"
#include "util/lightGroup.hpp"

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
GLuint createImageObjrct(const char* imagePath, const bool autoGammaCorrection = true, const bool flip_texture = true);
GLuint createSkyboxTexture(const char* imageFolder, const bool autoGammaCorrection = true);
void   createFBO(GLuint& fbo, GLuint& texAttachment, GLuint& rbo, const char* hint = "null");
void   createObjFromHardcode(GLuint&         vao,
                             GLuint&         vbo,
                             GLuint&         ebo,
                             vector<GLfloat> vertices,
                             vector<GLuint>  vertexIdx = {});
void   renderTextureToScreen(const GLuint screenVAO, const GLuint textureToShow, Shader& screenShader);
GLuint loadMTexture(float* matrixTable);

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
    // glClearColor(0.2F, 0.3F, 0.3F, 1.0F);  // 设置清空颜色
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_MULTISAMPLE);  // 启用多重采样
    /**NOTE - 文档中GL_MULTISAMPLE时默认启动的（true）
     */
    glEnable(GL_FRAMEBUFFER_SRGB);  // 自动Gamme矫正
    /**NOTE - gamma矫正关闭
    我们在pbrShader中手动矫正gamma
    */
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);  // 在立方体贴图的面之间进行正确过滤

    /**NOTE - 模型和着色器、纹理
     */
    // Model
    Model box("./box/box.obj");
    Model plane("./plane/plane.obj");
    Model sphere("./sphere/sphere.obj");
    Model geosphere("./geosphere/geosphere.obj");
    Model testScene("./testScene/scene.obj");
    // Shader
    Shader phongShader("./shader/stdVerShader.vs.glsl", "./shader/simpleWritePhongLighting.fs.glsl");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl", "./shader/skyboxShader.fs.glsl");
    Shader lightObjShader("./shader/stdVerShader.vs.glsl", "./shader/stdPureColor.fs.glsl");
    Shader areaLightShader("./shader/stdVerShader.vs.glsl", "./shader/areaLight2.fs.glsl");
    // Texture
    GLuint cubeTexture = createSkyboxTexture("./texture/", true);
    GLuint woodTexture = createImageObjrct("./texture/wood.jpg", true);
    GLuint wallTexture = createImageObjrct("./texture/wall.jpg", true);
    // LUT
    GLuint m1 = loadMTexture((float*)LTC1);
    GLuint m2 = loadMTexture((float*)LTC2);

    /**NOTE - 灯光组
     */
    LightGroup         lightGroup;
    std::vector<Light> lights = {
        Light(0, vec3(1, 1, 1), 1, vec3(0, 2, 2)),  // 1
        Light(0, vec3(1, 1, 1), 1, vec3(2, 2, 2)),  // 2
        Light(0, vec3(1, 1, 1), 1, vec3(0, 4, 2)),  // 3
        Light(0, vec3(1, 1, 1), 1, vec3(2, 4, 2)),  // 4
    };
    lightGroup.addLight(lights);
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);  // 清除颜色、深度和模板缓冲

        /**NOTE - 更新视图变换
         */
        mat4 view       = camera->GetViewMatrix();
        mat4 projection = perspective(radians(camera->Zoom), cameraAspect, 0.1f, 100.0f);

        /**NOTE - 渲染
         */
        // area light geometry
        static const vec3 areaLightColor = vec3(0.2, 0.6, 0.7);
        static const mat4 areaLightModel = rotate(
            rotate(translate(mat4(1), vec3(0, 1.5, 5)), radians(-90.0f), vec3(1, 0, 0)), radians(0.0f), vec3(0, 1, 0));
        static const vector<vec3> areaLightVertices = {
            vec3(1.0f, 0.0f, 1.0f),
            vec3(1.0f, 0.0f, -1.0f),
            vec3(-1.0f, 0.0f, -1.0f),
            vec3(-1.0f, 0.0f, 1.0f),
        };

        areaLightShader.use();
        // vertex shader
        areaLightShader.setParameter("model", mat4(1.0f));
        areaLightShader.setParameter("view", view);
        areaLightShader.setParameter("projection", projection);
        // fragment shader
        areaLightShader.setParameter("areaLight.intensity", 10.0f);
        areaLightShader.setParameter("areaLight.color", areaLightColor);
        for (int i : {0, 1, 2, 3})
        {
            vec3 globalPos = areaLightModel * vec4(areaLightVertices[i], 1);
            areaLightShader.setParameter("areaLight.points[" + to_string(i) + "]", globalPos);
        }
        areaLightShader.setParameter("areaLight.twoSided", false);
        areaLightShader.setParameter("areaLightTranslate", vec3(0));
        areaLightShader.setParameter("material.albedoRoughness", vec4(0.5f));
        areaLightShader.setParameter("viewPosition`", camera->Position);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m1);
        areaLightShader.setParameter("LTC1", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m2);
        areaLightShader.setParameter("LTC2", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, wallTexture);
        areaLightShader.setParameter("material.diffuse", 2);
        testScene.Draw(&areaLightShader);

        /**NOTE - 渲染灯光
         */
        // for (const auto& light : lightGroup.getLights())
        // {
        //     if (light.getLightType() != 1)  // 日光不渲染实体
        //     {
        //         lightObjShader.use();
        //         lightObjShader.setParameter("model", scale(translate(mat4(1), light.getPostion()), vec3(0.1)));
        //         lightObjShader.setParameter("view", view);
        //         lightObjShader.setParameter("projection", projection);
        //         lightObjShader.setParameter("lightColor", light.getColor());
        //         sphere.Draw(&lightObjShader);
        //         // FIXME - 常量对象只能调用它的常函数
        //     }
        // }
        lightObjShader.use();
        lightObjShader.setParameter("model", areaLightModel);
        lightObjShader.setParameter("view", view);
        lightObjShader.setParameter("projection", projection);
        lightObjShader.setParameter("lightColor", areaLightColor);
        plane.Draw(&lightObjShader);

        /**NOTE - skybox
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
GLuint createImageObjrct(const char* imagePath, const bool autoGammaCorrection, const bool flip_texture)
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
            glTexImage2D(GL_TEXTURE_2D, 0, (autoGammaCorrection) ? GL_SRGB : GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data);
            //  设置为GL_SRGB时，OpenGL回自动对图片进行重校
            // FIXME - 注意，对于在线性空间下创建的纹理，如法线贴图，不能设置SRGB重校。
        }
        else  // nrChannels == 4
        {
            glTexImage2D(GL_TEXTURE_2D, 0, (autoGammaCorrection) ? GL_SRGB_ALPHA : GL_SRGB_ALPHA, width, height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, data);
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
    string cubeTextureNames[6] = {"right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"};
    for (int i = 0; i < 6; i++)
    {
        string cubeTexturePath = basePath + cubeTextureNames[i];
        int    width, height, nrChannels;
        stbi_set_flip_vertically_on_load(false);  // 加载图片时翻转y轴
        GLubyte* data = stbi_load(cubeTexturePath.c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, (autoGammaCorrection) ? GL_SRGB : GL_RGB, width, height,
                         0, GL_RGB, GL_UNSIGNED_BYTE, data);
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
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, CAMERA_WIDTH, CAMERA_HEIGH, GL_TRUE);
        // 如果最后一个参数为GL_TRUE，图像将会对每个纹素使用相同的样本位置以及相同数量的子采样点个数。
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texAttachment, 0);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, texAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CAMERA_WIDTH, CAMERA_HEIGH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texAttachment, 0);
    }

    // 创建一个多重采样渲染缓冲对象
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    if (useMutiSampled)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, CAMERA_WIDTH, CAMERA_HEIGH);
    }
    else { glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, CAMERA_WIDTH, CAMERA_HEIGH); }
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
void createObjFromHardcode(GLuint& vao, GLuint& vbo, GLuint& ebo, vector<GLfloat> vertices, vector<GLuint> vertexIdx)
{
    bool useEBO = (vertexIdx.size() > 0);
    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // EBO
    if (useEBO)
    {
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIdx.size() * sizeof(GLuint), vertexIdx.data(), GL_STATIC_DRAW);
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

/**
 * @brief 载入预计算的LUT
 *
 * @param matrixTable
 * @return GLuint
 */
GLuint loadMTexture(float* matrixTable)
{
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_FLOAT, matrixTable);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

/**REVIEW - 区域光
光线无处不在。同样，合适的灯光在渲染中也十分重要。

目前为止我们已经在第三章中【投光物】这一节学习了三种截然不同的灯光类型：平行光、点光和聚光。但是在现实生活中的光源通常具有一定范围和面积，而以上三种光源要么是来自一点，要么光线在空间中各个位置相同（平行光）。

很显然我们无法通过以上三种光源去实现一种具有面积的光源。假设我们需要一个矩形光源，如果仅用点光或聚光实现，则需要成千上万的微小光源阵列在这个矩形区域中。又即使我们成功创建了这些光源，很明显，为了追求实时渲染的性能要求，渲染结果肯定不尽如人意（如果相机非常接近矩形光源，我们甚至可能还需要更密的灯光）。除此之外又可能出现的照明过度，所以为了能量守恒，每个光源的亮度又需要尽可能的小。很明显这种实现方式是不合理的。因此我们需要一种新的灯光类型：区域光。

这种具有区域的光源在离线渲染中，通过蒙特卡洛采样，使用光线追踪可以相对简单粗暴地解决这个问题。但是本篇教程主要面向实时渲染领域，上述离线渲染的实现方式在本文中不会被提及。

本篇教程描述了一种使用线性变换余弦分布集(Linearly Transformed Cosines)的方法从任意凸多面体中渲染区域光。该方法最初由Eric
Heitz、Jonathan Dupuy、Stephen Hill和David
Neubelt在SIGGRAPH2016年发表。后续人们又对该方法进行了拓展，除了矩形或多面体外又支持了圆形光源。

在本文中，我们将主要研究矩形区域光的具体实现方法。

* NOTE - 前置知识

假设我们有一个场景，并在场景中的某个位置放置一个矩形。我们希望这个矩形表示一个光源并发射光线。除此之外还有物理渲染中的BRDF函数，它描述了场景中表面如何散射光线（或者出射光和每条入射光间的比例）。

在渲染的时候，我们需要解决BRDF的积分，积分范围是自发光多面体（区域光源）在单位半球面围绕着着色点形成的立体角，以计算出射辐照度(Irradiance)。这个出射辐照度将反射回摄像机，从而完成渲染。

在论文中描述了如何将线性变换作用于球面分布函数，从而构造一系列可以拟合不同材质和摄像机视角下BRDF特性的球面分布函数。这些特性在Eric
Heitz的PPT[3]中有很好的表现。但由于本教程主要面向程序的实现，所以不会在这里画太多篇幅讲解理论推导。这些经过不同矩阵变换出的分布函数被统称为”线性变换余弦分布集”(Linearly
Transformed
Cosines)，之所以是“余弦”是因为所有的分布函数是来自一个变换后的受约余弦分布函数，同时具有近似的BRDF的重要特性。”受约”指的是余弦分布函数（或Lambertian分布）仅考虑正半球面。这很重要，因为单位球面固体角度测量值在区间[0,
4π]内，而光源可能永远不会照亮水平面以下的区域；因此我们将余弦分布函数约束到正半球面（上半球）（以入射着色点的表面法线为中心）。

在物理渲染(PBR)中我们学到，GGX微表面模型中的BRDF分布函数具有复杂的形状。比如在接近入射角度处的各向异性拉伸、偏斜度和不同程度的材质粗糙度等特性。正是由于这些特性，BRDF能够产生写实的渲染效果。所以如果我们能够设计一个线性变换，通过矩阵乘法将这些特性添加到受约余弦分布中，那么我们就可以获得媲美BRDF的效果。论文[1]中描述了如何实现这一点。余弦分布是一个非常好的选择，因为我们对其积分有一个闭合形式（Closed-Form）的表示：

* NOTE - 构建变换矩阵

粗糙度(roughness)直接从材质的粗糙度纹理中获取，各向异性(anisotropy)和偏斜度(skewness)来自
于摄像机和表面法线的夹角，从而实现逼真的渲染效果。如上图所示，我们可以根据需求和要表示的特
定BRDF模型选择任意程度的变换。在本教程中，我们将使用一个矩阵来近似GGX微表面模型。如前
文所述，我们需要为每个粗糙度和观察角度的组合计算一个矩阵M。不仅如此，我们还需要对矩阵
进行多次采样，从而最小化与目标BRDF的误差。显然，这对于实时渲染来说是不可行的，因此需
要预先计算在不同粗糙度/观察角度下的矩阵M。由于矩阵M及其逆矩阵都是稀疏矩阵，只有5个非
零值，我们可以通过归一化来将逆矩阵存储在一个4维向量中。这在实践中效果很好，并且可以将矩
阵信息以2D纹理的形式传入到shader中去查表。本教程中我们选择一个64x64的纹理来节省显存。

* NOTE - 计算积分

前文我们提到，计算区域光时我们需要在区域光所在立体角内积分辐射度(Radiance)，但是在实际操作中我们仅需要沿多面体光源的边缘进行积分。这个积分的等价转换过程十分神奇，不过由于教程篇幅的原因我仅引用一句ppt中的原话，希望读者进一步的了解：

“其中一种理解方法是斯托克斯定理(Stoke’s theorem)的应用，你可能在其他领域遇到过，比如流体模拟。”
斯托克斯定理讲的是：在向量场中，场中一个闭合曲线上的线积分等于其所围面积中散度(Curl)的通量(Flux)之和[11]。同理在对区域光光源的积分中，给定光源中任意两个顶点v1和v2，我们需要对下式进行积分求和：

对于具有N个顶点的整个区域光光源来说，我们可以根据上式将全部边缘求和来准确地计算出该区域光所构成的立体角：

* NOTE - 将多面体裁剪到正半球面

上述的IntegrateEdge函数虽然是一个很好的解决方案，但有待提升。为了获得正确的辐照度，我们需要将变换后的余弦分布约束到上半球。这个过程也可以被称为“水平面裁剪”，涉及到很多分支。如下图：

如果光源多面体在水平面（相对于物体表面）以下，或者部分在水平面以下，这个多面体则需要被修正
（裁剪）到上半球面。理论上来说，修正方法是将存在问题的每条边的较低的顶点上移，使其不再位于
水平面以下。如果一条边完全位于水平面以下，则需要将其整体上移。最后以上判断条件下，计算所得
到的边缘积分将小于修正前的值（甚至等于零），从而减少出射辐照度。但是，如果我们需要对所有边
的每个顶点进行上述判断，shader中会增加许多分支语句（if-else）进而影响渲染性能。因此我们需要
从公式入手，继续修改边缘积分的公式：删除与表面法向量的点积（从而不投影到平面上）。修正后的
公式如下

需要注意的是，现在积分的结果是一个向量（与法向量点乘的前一步），我们可以将其视为向量
形式因子或向量辐照度。我们将其称为向量F，向量F有一个比较明显的特性：向量F的模长为该光源
在F方向上的辐照度的大小[2]。此外，我们假想释放辐照度大小为||F||的光源来自一个球体（Proxy
Sphere），通过向量F我们可以得出该圆面相对余弦分布函数的张角和倾斜度。公式如下：

通过构造假想球体(Proxy
Sphere)我们可以完美解决上半球面修正的问题。不仅如此，通过预计算修正后的辐照度和修正前的
比例与其张角和倾斜度的关系，我们可以得到第二张纹理LUT。在渲染过程中通过查表可以直接获得
这一比例关系，从而高效地近似计算正半球修正。更多细节详见[1]
[2]。

向量F的模长表示着色点接受的总辐照度，从面积A传输到面积B。这一点类似于我们将多面体光
源的辐照度转换为多面体在经过半球面上立体角所的辐照度的过程。不过物体表面接收的辐照度
受正半球修正和光源倾斜度的影响，这个比例关系被存储在了一个64×64的LUT中，横纵坐标分
别为假想球体的张角和倾斜度。上文中之所以用“向量辐照度”作为向量F的名词，是因为作者想
强调入射光的能量。对于向量F更完备的表述详见[13]

* NOTE - 整体实现
我们还可以将它们作为DXT10压缩图像（.dds）加载。教程作者选择了C++头文件，
因为大多数图像加载库不支持DXT10压缩（文件头没有标准化！）。虽然在学习时使用头文件
存储数据很方便，但在发布游戏时最好使用.dds图像。

LUT是图形学经常遇到的概念，它是Look-Up
Table的简称，而且大多数情况以纹理贴图的形式在shader中发挥作用。结构体Material仅包含
三通道的颜色信息和一通道的粗糙度信息。如果你采用延迟渲染，这种压缩方式非常有用。虽
然颜色信息在本案例中未被使用，且物体的粗糙度是个定值，在大多数PBR渲染管线中粗糙度
可以是表面纹理的形式。
*/
