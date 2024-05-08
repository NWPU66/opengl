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

int main(int argc, char** argv)
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (!initGLFWWindow(window)) return -1;

    // SECTION - 准备数据
    /**NOTE - 模型和着色器、纹理
     */
    Model  sphere("./sphere/sphere.obj"), plane("./plane/plane.obj");
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
 */
