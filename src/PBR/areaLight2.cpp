// Implementing Areal Lights with Linearly Transformed Cosines.
//
// Inspiration:
// https://advances.realtimerendering.com/s2016/s2016_ltc_rnd.pdf
// https://eheitzresearch.wordpress.com/415-2/

#include <glad/glad.h>

// GLAD, GLFW, STB-IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <stb_image.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// LEARNOPENGL
#include <util/class_camera.hpp>
#include <util/class_model.hpp>
#include <util/class_shader.hpp>

// STANDARD
#include <filesystem>
#include <iostream>
#include <vector>

// CUSTOM
// #include "../colors.hpp"  // LOOK FOR DIFFERENT COLORS!
#include "glm/fwd.hpp"
#include "ltc_matrix.hpp"

struct VertexAL
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

const GLfloat psize                = 10.0f;
VertexAL      planeVertices[6]     = {{{-psize, 0.0f, -psize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                                      {{-psize, 0.0f, psize}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                                      {{psize, 0.0f, psize}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                                      {{-psize, 0.0f, -psize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                                      {{psize, 0.0f, psize}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                                      {{psize, 0.0f, -psize}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}};
VertexAL      areaLightVertices[6] = {
    {{-8.0f, 2.4f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // 0 1 5 4
    {{-8.0f, 2.4f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},   // 1
    {{-8.0f, 0.4f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},   //
    {{-8.0f, 2.4f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  //
    {{-8.0f, 0.4f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},   // 4
    {{-8.0f, 0.4f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // 5
};

GLuint planeVBO, planeVAO;
GLuint areaLightVBO, areaLightVAO;

void renderPlane()
{
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void renderAreaLight()
{
    glBindVertexArray(areaLightVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

/**NOTE - 全局变量、摄像机、全局时钟以及函数
 */
const GLint CAMERA_WIDTH = 1920;
const GLint CAMERA_HEIGH = 1080;
const float cameraAspect = static_cast<float>(CAMERA_WIDTH) / static_cast<float>(CAMERA_HEIGH);
Camera*     camera       = new Camera(vec3(0.0F, 0.0F, 2.0F), vec3(0.0F, 1.0F, 0.0F), -90.0F, 0.0F);
float       mouseLastX = 0.0f, mouseLastY = 0.0f;  // 记录鼠标的位置
float       lastFrame = 0.0f, deltaTime = 0.0f;    // 全局时钟

// arealight
const vec3 areaLightColor = vec3(0.2, 0.6, 0.7);
const mat4 areaLightModel =
    rotate(rotate(translate(mat4(1), vec3(0, 1.5, 5)), radians(-90.0f), vec3(1, 0, 0)), radians(0.0f), vec3(0, 1, 0));

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

void configureMockupData()
{
    // PLANE
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);

    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // AREA LIGHT
    glGenVertexArrays(1, &areaLightVAO);
    glBindVertexArray(areaLightVAO);

    glGenBuffers(1, &areaLightVBO);
    glBindBuffer(GL_ARRAY_BUFFER, areaLightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(areaLightVertices), areaLightVertices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    glBindVertexArray(0);
}

struct LTC_matrices
{
    GLuint mat1;
    GLuint mat2;
};

int main()
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (initGLFWWindow(window) == 0) return -1;

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // LUT textures
    LTC_matrices mLTC;
    mLTC.mat1 = loadMTexture((float*)LTC1);
    mLTC.mat2 = loadMTexture((float*)LTC2);

    // SHADERS
    Shader shaderLTC("./shader/7area_light.vs", "./shader/7area_light.fs");
    Shader shaderLightPlane("./shader/7light_plane.vs", "./shader/7light_plane.fs");

    // TEXTURES
    GLuint concreteTexture = createImageObjrct("./texture/wall.jpg", true);

    // SHADER CONFIGURATION
    shaderLTC.use();
    shaderLTC.setParameter("areaLight.points[0]", areaLightVertices[0].position);
    shaderLTC.setParameter("areaLight.points[1]", areaLightVertices[1].position);
    shaderLTC.setParameter("areaLight.points[2]", areaLightVertices[4].position);
    shaderLTC.setParameter("areaLight.points[3]", areaLightVertices[5].position);
    shaderLTC.setParameter("areaLight.color", areaLightColor);
    shaderLTC.setParameter("areaLight.intensity", 10.0f);
    shaderLTC.setParameter("LTC1", 0);
    shaderLTC.setParameter("LTC2", 1);
    shaderLTC.setParameter("material.diffuse", 2);
    shaderLTC.setParameter("material.albedoRoughness", glm::vec4(0.5f));
    glUseProgram(0);

    shaderLightPlane.use();
    {
        glm::mat4 model(1.0f);
        shaderLightPlane.setParameter("model", model);
    }
    shaderLightPlane.setParameter("lightColor", areaLightColor);
    glUseProgram(0);

    // 3D OBJECTS
    configureMockupData();
    auto areaLightTranslate = glm::vec3(0.0f, 0.0f, 0.0f);

    // RENDER LOOP
    while (!glfwWindowShouldClose(window))
    {
        // 更新时钟、摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderLTC.use();
        glm::mat4 model(1.0f);
        glm::mat3 normalMatrix = glm::mat3(model);
        shaderLTC.setParameter("model", model);
        shaderLTC.setParameter("normalMatrix", normalMatrix);
        glm::mat4 view = camera->GetViewMatrix();
        shaderLTC.setParameter("view", view);
        glm::mat4 projection =
            glm::perspective(glm::radians(camera->Zoom), (float)CAMERA_WIDTH / (float)CAMERA_HEIGH, 0.1f, 100.0f);
        shaderLTC.setParameter("projection", projection);
        shaderLTC.setParameter("viewPosition", camera->Position);
        shaderLTC.setParameter("areaLightTranslate", areaLightTranslate);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mLTC.mat1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mLTC.mat2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, concreteTexture);
        renderPlane();
        glUseProgram(0);

        shaderLightPlane.use();
        model = glm::translate(model, areaLightTranslate);
        shaderLightPlane.setParameter("model", model);
        shaderLightPlane.setParameter("view", view);
        shaderLightPlane.setParameter("projection", projection);
        renderAreaLight();
        glUseProgram(0);

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteVertexArrays(1, &areaLightVAO);
    glDeleteBuffers(1, &areaLightVBO);

    glfwTerminate();
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
