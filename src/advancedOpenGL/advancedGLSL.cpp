#include <glad/glad.h>
// GLAD first
#include "util/util.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
using namespace glm;

/**NOTE - 全局变量、摄像机、全局时钟以及函数
 */
const GLint CAMERA_WIDTH = 800;
const GLint CAMERA_HEIGH = 600;
const float cameraAspect = (float)CAMERA_WIDTH / (float)CAMERA_HEIGH;
Camera*     camera =
    new Camera(vec3(0.0f, 0.0f, 2.0f), vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float mouseLastX = 0.0f, mouseLastY = 0.0f;  // 记录鼠标的位置
float lastFrame = 0.0f, deltaTime = 0.0f;    // 全局时钟

void   framebuffer_size_callback(GLFWwindow* window, int w, int h);
void   mouse_callback(GLFWwindow* window, double xpos, double ypos);
void   scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void   processInput(GLFWwindow* window);
int    initGLFWWindow(GLFWwindow*& window);
GLuint createImageObjrct(const char* imagePath);
void   useUniformBuffer();

int main(int argc, char** argv)
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (!initGLFWWindow(window)) return -1;

    // SECTION - 准备数据
    /**NOTE - 模型和着色器、纹理
     */
    Model sphere("./sphere/sphere.obj"), plane("./plane/plane.obj"),
        monkey("./monkey/monkey.obj");
    Shader sphereShader("./shader/normalShader.vs.glsl",
                        "./shader/normalShader.fs.glsl"),
        outlinerShader("./shader/normalShader.vs.glsl",
                       "./shader/outlinerShader.fs.glsl"),
        grassShader("./shader/normalShader.vs.glsl",
                    "./shader/grassShader.fs.glsl");
    GLuint grassTexture = createImageObjrct("./texture/window.png");

    /**NOTE - 天空盒几何对象
     */
    float skyboxVertices[] = {
        // positions
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
    Shader skyboxShader("./shader/skyboxShader.vs.glsl",
                        "./shader/skyboxShader.fs.glsl");
    // VAO and VBO
    GLuint skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    // 向缓冲写入数据
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices) * sizeof(float),
                 skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /**NOTE - 屏幕几何对象
     */
    vector<GLfloat> screenVertices = {
        // 位置               // 纹理坐标
        -1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  // 左上
        1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // 右上
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 左下
        1.0f,  -1.0f, 0.0f, 1.0f, 0.0f   // 右下
    };
    vector<GLuint> screenVerticesIdx = {
        0, 2, 1,  // 第一个三角形
        1, 2, 3   // 第二个三角形
    };
    Shader screenShader("./shader/screenShader.vs.glsl",
                        "./shader/screenShader.fs.glsl");
    GLuint boxTexture = createImageObjrct("./texture/container2.png");
    // VBO
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, screenVertices.size() * sizeof(GLfloat),
                 screenVertices.data(), GL_STATIC_DRAW);
    // VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // EBO
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 screenVerticesIdx.size() * sizeof(GLuint),
                 screenVerticesIdx.data(), GL_STATIC_DRAW);
    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    /**NOTE - 几何着色器案例
     */
    float gsPoints[] = {
        -0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  // 左上
        0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  // 右上
        0.5f,  -0.5f, 0.0f, 0.0f, 1.0f,  // 右下
        -0.5f, -0.5f, 1.0f, 1.0f, 0.0f   // 左下
    };
    Shader gsShader("./shader/gsShader.vs.glsl", "./shader/gsShader.fs.glsl",
                    "./shader/gsShader.gs.glsl");
    // VAO and VBO
    GLuint gsVBO, gsVAO;
    glGenVertexArrays(1, &gsVAO);
    glGenBuffers(1, &gsVBO);
    glBindVertexArray(gsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gsVBO);
    // 向缓冲写入数据
    glBufferData(GL_ARRAY_BUFFER, sizeof(gsPoints), gsPoints, GL_STATIC_DRAW);
    // 设置顶点属性指针
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //~SECTION

    /**NOTE - 创建一个帧缓冲
     */
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // 创建一个纹理附件（Texture Attachment）
    GLuint textureAttachment;
    glGenTextures(1, &textureAttachment);
    glBindTexture(GL_TEXTURE_2D, textureAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CAMERA_WIDTH, CAMERA_HEIGH, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           textureAttachment, 0);
    // 渲染缓冲对象附件
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, CAMERA_WIDTH,
                          CAMERA_HEIGH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);
    //  检查帧缓冲状态
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer is  complete!" << std::endl;
    }
    else
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
                  << std::endl;
    }
    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    /**NOTE - 创建立方体贴图
     */
    GLuint cubeTexture;
    glGenTextures(1, &cubeTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
    string basePath            = "./texture/";
    string cubeTextureNames[6] = {"right.jpg",  "left.jpg",  "top.jpg",
                                  "bottom.jpg", "front.jpg", "back.jpg"};
    for (int i = 0; i < 6; i++)
    {
        string cubeTexturePath = basePath + cubeTextureNames[i];
        int    width, height, nrChannels;
        stbi_set_flip_vertically_on_load(false);  // 加载图片时翻转y轴
        GLubyte* data =
            stbi_load(cubeTexturePath.c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width,
                         height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            // 设置纹理属性
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                            GL_CLAMP_TO_EDGE);
        }
        else
        {
            std::cout << "Failed to load texture: " << cubeTexturePath
                      << std::endl;
        }
        stbi_image_free(data);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);  // 解绑

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);  // 启用深度缓冲
    glDepthFunc(GL_LEQUAL);   // 修改深度测试的标准

    glEnable(GL_STENCIL_TEST);  // 启用模板缓冲

    glEnable(GL_BLEND);                            // 启用混合
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);  // 设置模板缓冲的操作

    // glEnable(GL_CULL_FACE);                             // 启用面剔除
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // 设置清空颜色

    glEnable(GL_PROGRAM_POINT_SIZE);  // 启用逐个顶点的大小
    //~SECTION

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 更新时钟、摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window);

        // SECTION - 渲染循环
        // 启动帧缓冲对象
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glEnable(GL_DEPTH_TEST);
        /**NOTE - 清空屏幕
         */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);  // 清除颜色、深度和模板缓冲

        /**NOTE - 更新视图变换
         */
        mat4 view = camera->GetViewMatrix();
        mat4 projection =
            perspective(radians(camera->Zoom), cameraAspect, 0.1f, 100.0f);
        // projection应该是透视+投影，转换进标准体积空间：[-1,+1]^3

        /**NOTE - 绘制不需要轮廓的物体
         */
        glStencilMask(0x00);                // 禁用写入模板值
        glStencilFunc(GL_ALWAYS, 1, 0xFF);  // 无条件通过模板测试
        sphereShader.use();
        sphereShader.setParameter("view", view);
        sphereShader.setParameter("projection", projection);
        sphereShader.setParameter(
            "model", translate(mat4(1.0f), vec3(-0.5f, 0.0f, 0.0f)));
        sphereShader.setParameter("toneColor", vec3(1.0f, 0.5f, 0.31f));
        sphereShader.setParameter("cameraPos", camera->Position);
        sphere.Draw(&sphereShader);
        // 绘制grass
        grassShader.use();
        grassShader.setParameter("view", view);
        grassShader.setParameter("projection", projection);
        grassShader.setParameter(
            "model", rotate(translate(mat4(1.0f), vec3(0.0f, 0.0f, 1.0f)),
                            radians(-90.0f), vec3(1.0f, 0.0f, 0.0f)));
        glActiveTexture(GL_TEXTURE0);  // 激活纹理单元0
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        grassShader.setParameter("texture0", 0);  // 设置纹理槽位
        plane.Draw(&grassShader);

        /**NOTE - 绘制需要轮廓的物体
         */
        glStencilMask(0xFF);                // 启用写入模板值
        glStencilFunc(GL_ALWAYS, 1, 0xFF);  // 无条件通过模板测试
        sphereShader.use();
        sphereShader.setParameter("view", view);
        sphereShader.setParameter("projection", projection);
        sphereShader.setParameter(
            "model", translate(mat4(1.0f), vec3(0.5f, 0.0f, 0.0f)));
        sphereShader.setParameter("toneColor", vec3(0.5f, 0.0f, 0.31f));
        sphereShader.setParameter("cameraPos", camera->Position);
        sphere.Draw(&sphereShader);

        /**NOTE - 绘制轮廓
         */
        glStencilMask(0x00);                  // 禁用写入模板值
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);  // 模板值不等于1通过模板测试
        // glDisable(GL_DEPTH_TEST);             // 禁用深度测试
        glDepthFunc(GL_ALWAYS);  // 总是通过深度测试
        outlinerShader.use();
        outlinerShader.setParameter("view", view);
        outlinerShader.setParameter("projection", projection);
        outlinerShader.setParameter(
            "model",
            translate(scale(mat4(1.0f), vec3(1.01f)), vec3(0.5f, 0.0f, 0.0f)));
        outlinerShader.setParameter(
            "outlineShine", (float)(sin(4.0f * glfwGetTime()) / 4.0f + 0.75f));
        sphere.Draw(&outlinerShader);
        // 恢复深度测试设置
        glDepthFunc(GL_LEQUAL);

        /**NOTE - 恢复模板测试和深度测试
         */
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glEnable(GL_DEPTH_TEST);

        /**NOTE - 最后渲染天空盒
         * 他会填充没有物体的空间
         */
        skyboxShader.use();
        skyboxShader.setParameter("view",
                                  mat4(mat3(view)));  // 除去位移，相当于锁头
        skyboxShader.setParameter("projection", projection);
        glBindVertexArray(skyboxVAO);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        skyboxShader.setParameter("skybox", 0);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        /**NOTE - 将帧缓冲中的颜色缓冲渲染到屏幕上
         */
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);  // 清除颜色缓冲
        // 绘制屏幕几何对象
        glBindVertexArray(vao);
        screenShader.use();
        glBindTexture(GL_TEXTURE_2D, textureAttachment);
        screenShader.setParameter("screenTexture", 0);
        glDisable(GL_DEPTH_TEST);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        /**NOTE - 几何着色器案例
         */
        gsShader.use();
        glBindVertexArray(gsVAO);
        glDrawArrays(GL_POINTS, 0, 4);
        //~SECTION

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    /**NOTE - 释放内存
     */
    glfwTerminate();
    delete camera;
    return 0;
}

/**
 * 函数“framebuffer_size_callback”根据窗口大小设置视口尺寸。
 *
 * @param window `window` 参数是指向触发回调函数的 GLFW 窗口的指针。
 * @param w
 * “framebuffer_size_callback”函数中的“w”参数表示帧缓冲区的宽度，即OpenGL将渲染图形的区域的大小。
 * @param h
 * “framebuffer_size_callback”函数中的参数“h”表示帧缓冲区的高度（以像素为单位）。它用于设置在
 * OpenGL 上下文中渲染的视口高度。
 */
void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, w, h);
}

/**
 * 函数“mouse_callback”根据 GLFW 窗口中的鼠标移动更新相机的方向。
 *
 * @param window “window”参数是指向接收鼠标输入的 GLFW
 * 窗口的指针。它用于标识鼠标事件发生的窗口。
 * @param xpos mouse_callback 函数中的 xpos 参数表示鼠标光标位置的当前 x
 * 坐标。
 * @param ypos `mouse_callback` 函数中的 `ypos`
 * 参数表示鼠标光标在窗口内的当前 y
 * 坐标。它是一个双精度值，表示触发回调函数时鼠标光标的垂直位置。
 */
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    camera->ProcessMouseMovement(xpos - mouseLastX, ypos - mouseLastY, true);
    mouseLastX = xpos;
    mouseLastY = ypos;
}

/**
 * 函数“scroll_callback”处理鼠标滚动输入以调整相机位置。
 *
 * @param window `GLFWwindow* window`
 * 参数是指向接收滚动输入的窗口的指针。
 * @param xoffset `xoffset` 参数表示水平滚动偏移。
 * @param yoffset
 * `scroll_callback`函数中的`yoffset`参数表示鼠标滚轮的垂直滚动偏移量。正值表示向上滚动，负值表示向下滚动。
 */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera->ProcessMouseScroll(yoffset);
}

/**
 * C++ 中的函数“processInput”处理来自 GLFW
 * 窗口的用户输入，以控制相机的移动和速度。
 *
 * @param window processInput 函数中的 window 参数是一个指向 GLFWwindow
 * 对象的指针。该对象表示应用程序中用于渲染图形和处理用户输入的窗口。该函数使用此参数来检查按键并更新相机的移动和速度
 */
void processInput(GLFWwindow* window)
{
    // 当Esc按下时，窗口关闭
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);

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

/**
 * 该函数使用指定的 OpenGL 上下文版本和回调初始化 GLFW 窗口。
 *
 * @param window `initGLFWWindow` 函数中的 `window` 参数是指向
 * GLFWwindow 对象的指针的引用。此函数初始化
 * GLFW，使用指定参数创建窗口，设置回调，隐藏光标，并初始化 GLAD 以进行
 * OpenGL 加载。如果成功，则返回
 *
 * @return
 * 函数“initGLFWWindow”返回一个整数值。如果GLFW窗口初始化成功则返回1，如果创建窗口或加载GLAD失败则返回0。
 */
int initGLFWWindow(GLFWwindow*& window)
{
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

/**
 * 函数 createImageObject 加载图像文件，在 OpenGL
 * 中创建纹理对象，设置纹理参数，生成纹理，并返回纹理 ID。
 *
 * @param imagePath “createImageObject”函数中的“imagePath”参数是一个指向 C
 * 风格字符串的指针，该字符串表示要加载并从中创建纹理对象的图像文件的路径。该路径应指向文件系统上图像文件的位置。
 *
 * @return 函数 createImageObjrct 返回一个整数值，它是在 OpenGL
 * 中加载和创建的图像的纹理 ID。
 */
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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data);
        }
        else  // nrChannels == 4
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
        return -1;
    }
    stbi_image_free(data);  // 释放图像的内存，无论有没有data都释放
    glBindTexture(GL_TEXTURE_2D, 0);  // 解绑
    return texture;
}

void useUniformBuffer()
{
    /*
    layout(std140) uniform ExampleBlock
    {
                             // 基准对齐量       // 对齐偏移量
    float value;        // 4               // 0
    vec3 vector;     // 16              // 16  (必须是16的倍数，所以 4->16)
    mat4 matrix;     // 16              // 32  (列 0)
                            // 16              // 48  (列 1)
                            // 16              // 64  (列 2)
                            // 16              // 80  (列 3)
    float values[3]; // 16              // 96  (values[0])
                            // 16              // 112 (values[1])
                            // 16              // 128 (values[2])
    bool boolean;    // 4               // 144
    int integer;        // 4               // 148
    };
    */

    // 创建Uniform Buffer并向其写入数据
    GLuint uboExampleBlock;
    glGenBuffers(1, &uboExampleBlock);
    glBindBuffer(GL_UNIFORM_BUFFER, uboExampleBlock);
    glBufferData(GL_UNIFORM_BUFFER, 152, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // 将Shader链接到OpenGL的Uniform绑定点上
    Shader shaderA("", "");
    GLuint Matrices_idx = glGetUniformBlockIndex(shaderA.ID, "Matrices");
    glUniformBlockBinding(shaderA.ID, Matrices_idx, 2);

    // 将Uniform Buffer绑定到OpenGL的绑定点上
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboExampleBlock);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, uboExampleBlock, 0, 152);

    // 向Uniform Buffer填充或更新数据
    glBindBuffer(GL_UNIFORM_BUFFER, uboExampleBlock);
    int b = true;  // GLSL中的bool是4字节（4B）的，所以将它存为一个integer
    glBufferSubData(GL_UNIFORM_BUFFER, 144, 4, &b);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

/**REVIEW - 高级GLSL
 * 这一小节并不会向你展示非常先进非常酷的新特性，也不会对场景的视觉质量有显著的提高。
 * 但是，这一节会或多或少涉及GLSL的一些有趣的地方以及一些很棒的技巧，
 * 它们可能在今后会帮助到你。简单来说，它们就是在组合使用OpenGL和GLSL创建程序时的
 * 一些最好要知道的东西，和一些会让你生活更加轻松的特性。
 *
 * 我们将会讨论一些有趣的内建变量(Built-in
 * Variable)，管理着色器输入和输出的新方式以及 一个叫做Uniform缓冲对象(Uniform
 * Buffer Object)的有用工具。
 *
 * NOTE - GLSL的内建变量
 * 着色器都是最简化的，如果需要当前着色器以外地方的数据的话，我们必须要将数据传进来。
 * 我们已经学会使用顶点属性、uniform和采样器来完成这一任务了。然而，除此之外，
 * GLSL还定义了另外几个以gl_为前缀的变量，它们能提供给我们更多的方式来读取/写入数据。
 * 我们已经在前面教程中接触过其中的两个了：顶点着色器的输出向量gl_Position，
 * 和片段着色器的gl_FragCoord。
 *
 * NOTE - 顶点着色器变量
 * 我们已经见过gl_Position了，它是顶点着色器的裁剪空间输出位置向量。如果你想在屏幕上显示任何东西，
 * 在顶点着色器中设置gl_Position是必须的步骤。这已经是它的全部功能了。
 *
 * 1.
 * gl_PointSize：我们能够选用的其中一个图元是GL_POINTS，如果使用它的话，每一个顶点都是
 * 一个图元，都会被渲染为一个点。我们可以通过OpenGL的glPointSize函数来设置渲染出来的点
 * 的大小，但我们也可以在顶点着色器中修改这个值。
 *
 * GLSL定义了一个叫做gl_PointSize输出变量，它是一个float变量，你可以使用它来设置点的宽高
 * （像素）。在顶点着色器中修改点的大小的话，你就能对每个顶点设置不同的值了。
 *
 * 在顶点着色器中修改点大小的功能默认是禁用的，如果你需要启用它的话，
 * 你需要启用OpenGL的GL_PROGRAM_POINT_SIZE：
 *
 * 2.
 * gl_VertexID：gl_Position和gl_PointSize都是输出变量，因为它们的值是作为顶点着色器的
 * 输出被读取的。我们可以对它们进行写入，来改变结果。顶点着色器还为我们提供了一个有趣
 * 的输入变量，我们只能对它进行读取，它叫做gl_VertexID。
 *
 * 整型变量gl_VertexID储存了正在绘制顶点的当前ID。当（使用glDrawElements）
 * 进行索引渲染的时候，这个变量会存储正在绘制顶点的当前索引。当（使用glDrawArrays）
 * 不使用索引进行绘制的时候，这个变量会储存从渲染调用开始的已处理顶点数量。
 *
 * NOTE - 片段着色器变量
 * 1. gl_FragCoord：在讨论深度测试的时候，我们已经见过gl_FragCoord很多次了，
 * 因为gl_FragCoord的z分量等于对应片段的深度值。然而，我们也能使用它的x和y
 * 分量来实现一些有趣的效果。
 *
 * gl_FragCoord的x和y分量是片段的窗口空间(Window-space)坐标，其原点为窗口的左下角。
 * 我们已经使用glViewport设定了一个800x600的窗口了，所以片段窗口空间坐标的x分量将在
 * 0到800之间，y分量在0到600之间。
 *
 * 通过利用片段着色器，我们可以根据片段的窗口坐标，计算出不同的颜色。
 * gl_FragCoord的一个常见用处是用于对比不同片段计算的视觉输出效果，
 * 这在技术演示中可以经常看到。比如说，我们能够将屏幕分成两部分，在窗口的左侧渲染一种输出，
 * 在窗口的右侧渲染另一种输出。下面这个例子片段着色器会根据窗口坐标输出不同的颜色：
 * ![](https://learnopengl-cn.github.io/img/04/08/advanced_glsl_fragcoord.png)
 *
 * 2. gl_FrontFacing：片段着色器另外一个很有意思的输入变量是gl_FrontFacing。
 * 在面剔除教程中，我们提到OpenGL能够根据顶点的环绕顺序来决定一个面是正向还是背向面。
 * 如果我们不（启用GL_FACE_CULL来）使用面剔除，那么gl_FrontFacing将会告诉我们当前片
 * 段是属于正向面的一部分还是背向面的一部分。举例来说，我们能够对正向面计算出不同的颜色。
 *
 * gl_FrontFacing变量是一个bool，如果当前片段是正向面的一部分那么就是true，
 * 否则就是false。比如说，我们可以这样子创建一个立方体，在内部和外部使用不同的纹理：
 *
 * 注意，如果你开启了面剔除，你就看不到箱子内部的面了，
 * 所以现在再使用gl_FrontFacing就没有意义了。
 *
 * 3. gl_FragDepth：输入变量gl_FragCoord能让我们读取当前片段的窗口空间坐标，
 * 并获取它的深度值，但是它是一个只读(Read-only)变量。我们不能修改片段的窗口空间坐标，
 * 但实际上修改片段的深度值还是可能的。GLSL提供给我们一个叫做gl_FragDepth的输出变量，
 * 我们可以使用它来在着色器内设置片段的深度值。
 *
 * 要想设置深度值，我们直接写入一个0.0到1.0之间的float值到输出变量就可以了。
 * 如果着色器没有写入值到gl_FragDepth，它会自动取用gl_FragCoord.z的值。
 *
 * 然而，由我们自己设置深度值有一个很大的缺点，只要我们在片段着色器中对gl_FragDepth进行写入，
 * OpenGL就会（像深度测试小节中讨论的那样）禁用所有的提前深度测试(Early Depth
 * Testing)。
 * 它被禁用的原因是，OpenGL无法在片段着色器运行之前得知片段将拥有的深度值，
 * 因为片段着色器可能会完全修改这个深度值。
 *
 * 在写入gl_FragDepth时，你就需要考虑到它所带来的性能影响。然而，从OpenGL 4.2起，
 * 我们仍可以对两者进行一定的调和，在片段着色器的顶部使用深度条件(Depth
 * Condition) 重新声明gl_FragDepth变量：
 *
 * 通过将深度条件设置为greater或者less，
 * OpenGL就能假设你只会写入比当前片段深度值更大或者更小的值了。
 * 这样子的话，当深度值比片段的深度值要小的时候，OpenGL仍是能够进行提前深度测试的。
 *
 * NOTE - 接口块
 * 到目前为止，每当我们希望从顶点着色器向片段着色器发送数据时，我们都声明了几个对应的
 * 输入/输出变量。将它们一个一个声明是着色器间发送数据最简单的方式了，但当程序变得更大时，
 * 你希望发送的可能就不只是几个变量了，它还可能包括数组和结构体。
 *
 * 为了帮助我们管理这些变量，GLSL为我们提供了一个叫做接口块(Interface
 * Block)的东西， 来方便我们组合这些变量。接口块的声明和struct的声明有点相像，
 * 不同的是，现在根据它是一个输入还是输出块(Block)，使用in或out关键字来定义的。
 *
 * 这次我们声明了一个叫做vs_out的接口块，它打包了我们希望发送到下一个着色器中的所有输出变量。
 * 这只是一个很简单的例子，但你可以想象一下，它能够帮助你管理着色器的输入和输出。
 * 当我们希望将着色器的输入或输出打包为数组时，它也会非常有用，
 * 我们将在下一节讨论几何着色器(Geometry Shader)时见到。
 *
 * 之后，我们还需要在下一个着色器，即片段着色器，中定义一个输入接口块。
 * 块名(Block Name)应该是和着色器中一样的（VS_OUT），但实例名(Instance Name)
 * （顶点着色器中用的是vs_out）可以是随意的，但要避免使用误导性的名称，
 * 比如对实际上包含输入变量的接口块命名为vs_out。
 *
 * 只要两个接口块的名字一样，它们对应的输入和输出将会匹配起来。
 * 这是帮助你管理代码的又一个有用特性，它在几何着色器这样穿插特定着色器阶段的场景下会很有用。
 *
 * NOTE - Uniform缓冲对象
 * 我们已经使用OpenGL很长时间了，学会了一些很酷的技巧，但也遇到了一些很麻烦的地方。
 * 比如说，当使用多于一个的着色器时，尽管大部分的uniform变量都是相同的，
 * 我们还是需要不断地设置它们，所以为什么要这么麻烦地重复设置它们呢？
 *
 * OpenGL为我们提供了一个叫做Uniform缓冲对象(Uniform Buffer Object)的工具，
 * 它允许我们定义一系列在多个着色器程序中相同的全局Uniform变量。
 * 当使用Uniform缓冲对象的时候，我们只需要设置相关的uniform一次。
 * 当然，我们仍需要手动设置每个着色器中不同的uniform。
 * 并且创建和配置Uniform缓冲对象会有一点繁琐。
 *
 * 因为Uniform缓冲对象仍是一个缓冲，我们可以使用glGenBuffers来创建它，
 * 将它绑定到GL_UNIFORM_BUFFER缓冲目标，并将所有相关的uniform数据存入缓冲。
 * 在Uniform缓冲对象中储存数据是有一些规则的，我们将会在之后讨论它。
 * 首先，我们将使用一个简单的顶点着色器，
 * 将projection和view矩阵存储到所谓的Uniform块(Uniform Block)中：
 *
 * 在我们大多数的例子中，我们都会在每个渲染迭代中，对每个着色器设置projection和view
 * Uniform矩阵。这是利用Uniform缓冲对象的一个非常完美的例子，因为现在我们只需要
 * 存储这些矩阵一次就可以了。
 *
 * 这里，我们声明了一个叫做Matrices的Uniform块，它储存了两个4x4矩阵。
 * Uniform块中的变量可以直接访问，不需要加块名作为前缀。接下来，
 * 我们在OpenGL代码中将这些矩阵值存入缓冲中，每个声明了这个Uniform块
 * 的着色器都能够访问这些矩阵。
 *
 * 你现在可能会在想layout (std140)这个语句是什么意思。它的意思是说，当
 * 前定义的Uniform块对它的内容使用一个特定的内存布局。这个语句设置了
 * Uniform块布局(Uniform Block Layout)。
 *
 * NOTE - Uniform块布局
 * Uniform块的内容是储存在一个缓冲对象中的，它实际上只是一块预留内存。
 * 因为这块内存并不会保存它具体保存的是什么类型的数据，我们还需要告诉OpenGL
 * 内存的哪一部分对应着着色器中的哪一个uniform变量。
 *
 * 我们需要知道的是每个变量的大小（字节）和（从块起始位置的）偏移量，
 * 来让我们能够按顺序将它们放进缓冲中。每个元素的大小都是在OpenGL中有清楚地声明的，
 * 而且直接对应C++数据类型，其中向量和矩阵都是大的float数组。OpenGL没有声明的是这些
 * 变量间的间距(Spacing)。这允许硬件能够在它认为合适的位置放置变量。比如说，
 * 一些硬件可能会将一个vec3放置在float边上。不是所有的硬件都能这样处理，
 * 可能会在附加这个float之前，先将vec3填充(Pad)为一个4个float的数组。这个特性本身很棒，
 * 但是会对我们造成麻烦。
 *
 * 默认情况下，GLSL会使用一个叫做共享(Shared)布局的Uniform内存布局，共享是因为一旦
 * 硬件定义了偏移量，它们在多个程序中是共享并一致的。使用共享布局时，GLSL是可以为了
 * 优化而对uniform变量的位置进行变动的，只要变量的顺序保持不变。因为我们无法知道每个
 * uniform变量的偏移量，我们也就不知道如何准确地填充我们的Uniform缓冲了。
 * 我们能够使用像是glGetUniformIndices这样的函数来查询这个信息，但这超出本节的范围了。
 *
 * 虽然共享布局给了我们很多节省空间的优化，但是我们需要查询每个uniform变量的偏移量，
 * 这会产生非常多的工作量。通常的做法是，不使用共享布局，而是使用std140布局。
 * std140布局声明了每个变量的偏移量都是由一系列规则所决定的，
 * 这显式地声明了每个变量类型的内存布局。由于这是显式提及的，
 * 我们可以手动计算出每个变量的偏移量。
 *
 * 每个变量都有一个基准对齐量(Base
 * Alignment)，它等于一个变量在Uniform块中所占据的空间
 * （包括填充量(Padding)），这个基准对齐量是使用std140布局的规则计算出来的。接下来，
 * 对每个变量，我们再计算它的对齐偏移量(Aligned
 * Offset)，它是一个变量从块起始位置的字节偏移量。
 * 一个变量的对齐字节偏移量必须等于基准对齐量的倍数。
 *
 * 布局规则的原文可以在OpenGL的Uniform缓冲规范这里找到，但我们将会在下面列出最常见的规则。
 * GLSL中的每个变量，比如说int、float和bool，都被定义为4字节量。每4个字节将会用一个N来表示。
 *
 * 布局规则
 * 标量，比如int和bool	每个标量的基准对齐量为N。
 * 向量	2N或者4N。这意味着vec3的基准对齐量为4N。
 * 标量或向量的数组	每个元素的基准对齐量与vec4的相同。
 * 矩阵	储存为列向量的数组，每个向量的基准对齐量与vec4的相同。
 * 结构体	等于所有元素根据规则计算后的大小，但会填充到vec4大小的倍数。
 *
 * 通过在Uniform块定义之前添加layout
 * (std140)语句，我们告诉OpenGL这个Uniform块使用的是
 * std140布局。除此之外还可以选择两个布局，但它们都需要我们在填充缓冲之前先查询每个偏移量。
 * 我们已经见过shared布局了，剩下的一个布局是packed。当使用紧凑(Packed)布局时，
 * 是不能保证这个布局在每个程序中保持不变的（即非共享），因为它允许编译器去将uniform
 * 变量从Uniform块中优化掉，这在每个着色器中都可能是不同的。
 *
 * NOTE - 使用Uniform缓冲
 * 首先，我们需要调用glGenBuffers，创建一个Uniform缓冲对象。一旦我们有了一个缓冲对象，
 * 我们需要将它绑定到GL_UNIFORM_BUFFER目标，并调用glBufferData，分配足够的内存。
 *
 * 现在，每当我们需要对缓冲更新或者插入数据，我们都会绑定到uboExampleBlock，
 * 并使用glBufferSubData来更新它的内存。我们只需要更新这个Uniform缓冲一次，
 * 所有使用这个缓冲的着色器就都使用的是更新后的数据了。
 * 但是，如何才能让OpenGL知道哪个Uniform缓冲对应的是哪个Uniform块呢？
 *
 * 在OpenGL上下文中，定义了一些绑定点(Binding
 * Point)，我们可以将一个Uniform缓冲链接至它。
 * 在创建Uniform缓冲之后，我们将它绑定到其中一个绑定点上，并将着色器中的Uniform块绑定到
 * 相同的绑定点，把它们连接到一起。
 * ![](https://learnopengl-cn.github.io/img/04/08/advanced_glsl_binding_points.png)
 *
 * 你可以看到，我们可以绑定多个Uniform缓冲到不同的绑定点上。
 * 因为着色器A和着色器B都有一个链接到绑定点0的Uniform块，
 * 它们的Uniform块将会共享相同的uniform数据，uboMatrices，
 * 前提条件是两个着色器都定义了相同的Matrices Uniform块。
 *
 * 为了将Uniform块绑定到一个特定的绑定点中，我们需要调用glUniformBlockBinding函数，
 * 它的第一个参数是一个程序对象，之后是一个Uniform块索引和链接到的绑定点。
 * Uniform块索引(Uniform Block Index)是着色器中已定义Uniform块的位置值索引。
 * 这可以通过调用glGetUniformBlockIndex来获取，它接受一个程序对象和Uniform块的名称。
 *
 * 从OpenGL 4.2版本起，你也可以添加一个布局标识符，显式地将Uniform块的绑定点储存在着色器中，
 * 这样就不用再调用glGetUniformBlockIndex和glUniformBlockBinding了。
 * layout(std140, binding = 2) uniform Lights { ... };
 *
 * glBindbufferBase需要一个目标，一个绑定点索引和一个Uniform缓冲对象作为它的参数。
 * 这个函数将uboExampleBlock链接到绑定点2上，自此，绑定点的两端都链接上了。
 * 你也可以使用glBindBufferRange函数，它需要一个附加的偏移量和大小参数，
 * 这样子你可以绑定Uniform缓冲的特定一部分到绑定点中。通过使用glBindBufferRange函数，
 * 你可以让多个不同的Uniform块绑定到同一个Uniform缓冲对象上。
 *
 * 现在，所有的东西都配置完毕了，我们可以开始向Uniform缓冲中添加数据了。只要我们需要，
 * 就可以使用glBufferSubData函数，用一个字节数组添加所有的数据，或者更新缓冲的一部分。
 * 要想更新uniform变量boolean，我们可以用以下方式更新Uniform缓冲对象：
 *
 * NOTE - 总结
 * Uniform缓冲对象比起独立的uniform有很多好处。
 *
 * 第一，一次设置很多uniform会比一个一个设置多个uniform要快很多。
 *
 * 第二，比起在多个着色器中修改同样的uniform，在Uniform缓冲中修改一次会更容易一些。
 *
 * 最后一个好处可能不会立即显现，如果使用Uniform缓冲对象的话，
 * 你可以在着色器中使用更多的uniform。OpenGL限制了它能够处理的uniform数量，
 * 这可以通过GL_MAX_VERTEX_UNIFORM_COMPONENTS来查询。
 * 当使用Uniform缓冲对象时，最大的数量会更高。所以，当你达到了uniform的最大数量时（
 * 比如再做骨骼动画(Skeletal
 * Animation)的时候），你总是可以选择使用Uniform缓冲对象。
 */

/**REVIEW - 几何着色器
 * 在顶点和片段着色器之间有一个可选的几何着色器(Geometry Shader)，
 * 几何着色器的输入是一个图元（如点或三角形）的一组顶点。
 * 几何着色器可以在顶点发送到下一着色器阶段之前对它们随意变换。
 * 然而，几何着色器最有趣的地方在于，它能够将（这一组）顶点变换为完全不同的图元，
 * 并且还能生成比原来更多的顶点。
 *
 * 几何着色器同时希望我们设置一个它最大能够输出的顶点数量
 * （如果你超过了这个值，OpenGL将不会绘制多出的顶点），
 * 这个也可以在out关键字的布局修饰符中设置。在这个例子中，
 * 我们将输出一个line_strip，并将最大顶点数设置为2个。
 *
 * 如果你不知道什么是线条(Line Strip)：线条连接了一组点，形成一条连续的线，
 * 它最少要由两个点来组成。在渲染函数中每多加一个点，就会在这个点与前
 * 一个点之间形成一条新的线。
 *
 * 为了生成更有意义的结果，我们需要某种方式来获取前一着色器阶段的输出。
 * GLSL提供给我们一个内建(Built-in)变量，在内部看起来（可能）是这样的：
 * in gl_Vertex
 * {
 *     vec4  gl_Position;
 *     float gl_PointSize;
 *     float gl_ClipDistance[];
 * } gl_in[];
 *
 * 要注意的是，它被声明为一个数组，因为大多数的渲染图元包含多于1个的顶点，
 * 而几何着色器的输入是一个图元的所有顶点。
 *
 * 有了之前顶点着色器阶段的顶点数据，我们就可以使用2个几何着色器函数，
 * EmitVertex和EndPrimitive，来生成新的数据了。几何着色器希望你能够生成
 * 并输出至少一个定义为输出的图元。在我们的例子中，我们需要至少生成一个线条图元。
 *
 * 每次我们调用EmitVertex时，gl_Position中的向量会被添加到图元中来。当EndPrimitive被调用时，
 * 所有发射出的(Emitted)顶点都会合成为指定的输出渲染图元。在一个或多个EmitVertex调用之后
 * 重复调用EndPrimitive能够生成多个图元。在这个例子中，我们发射了两个顶点，它们从原始顶点
 * 位置平移了一段距离，之后调用了EndPrimitive，将这两个顶点合成为一个包含两个顶点的线条。
 *
 * 现在你（大概）了解了几何着色器的工作方式，你可能已经猜出这个几何着色器是做什么的了。
 * 它接受一个点图元作为输入，以这个点为中心，创建一条水平的线图元。如果我们渲染它，
 *
 * NOTE - 使用几何着色器
 *
 * NOTE - 造几个房子
 * 绘制点和线并没有那么有趣，所以我们会使用一点创造力，利用几何着色器在每个点的位置上绘制
 * 一个房子。要实现这个，我们可以将几何着色器的输出设置为triangle_strip，并绘制三个三角形：
 * 其中两个组成一个正方形，另一个用作房顶。
 *
 * OpenGL中，三角形带(Triangle Strip)是绘制三角形更高效的方式，它使用顶点更少。
 * 在第一个三角形绘制完之后，每个后续顶点将会在上一个三角形边上生成另一个三角形：
 * 每3个临近的顶点将会形成一个三角形。如果我们一共有6个构成三角形带的顶点，
 * 那么我们会得到这些三角形：(1, 2, 3)、(2, 3, 4)、(3, 4, 5)和(4, 5,
 * 6)，共形成4个三角形。
 * 一个三角形带至少需要3个顶点，并会生成N-2个三角形。使用6个顶点，我们创建了6-2
 * = 4个三角形。
 * ![](https://learnopengl-cn.github.io/img/04/09/geometry_shader_triangle_strip.png)
 *
 * 通过使用三角形带作为几何着色器的输出，我们可以很容易创建出需要的房子形状，
 * 只需要以正确的顺序生成3个相连的三角形就行了。
 * ![](https://learnopengl-cn.github.io/img/04/09/geometry_shader_house.png)
 *
 * 这个几何着色器生成了5个顶点，每个顶点都是原始点的位置加上一个偏移量，
 * 来组成一个大的三角形带。最终的图元会被光栅化，然后片段着色器会处理整个三角形带，
 * 最终在每个绘制的点处生成一个绿色房子
 *
 * 你可以看到，每个房子实际上是由3个三角形组成的——全部都是使用空间中一点来绘制的。
 * 这些绿房子看起来是有点无聊，所以我们会再给每个房子分配一个不同的颜色。为了实现这个，
 * 我们需要在顶点着色器中添加一个额外的顶点属性，表示颜色信息，将它传递至几何着色器，
 * 并再次发送到片段着色器中。
 *
 * 你可以看到，有了几何着色器，你甚至可以将最简单的图元变得十分有创意。
 * 因为这些形状是在GPU的超快硬件中动态生成的，这会比在顶点缓冲中手动定义图形要高效很多。
 * 因此，几何缓冲对简单而且经常重复的形状来说是一个很好的优化工具，
 * 比如体素(Voxel)世界中的方块和室外草地的每一根草。
 *
 * NOTE - 爆破物体
 * 尽管绘制房子非常有趣，但我们不会经常这么做。这也是为什么我们接下来要继续深入，来
 * 爆破(Explode)物体！虽然这也是一个不怎么常用的东西，但是它能向你展示几何着色器的强大之处。
 *
 * 当我们说爆破一个物体时，我们并不是指要将宝贵的顶点集给炸掉，我们是要将每个三角形沿着法向量的方向移动一小段时间。
 * 效果就是，整个物体看起来像是沿着每个三角形的法线向量爆炸一样。
 * ![](https://learnopengl-cn.github.io/img/04/09/geometry_shader_explosion.png)
 */
