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
GLuint loadCubemap();

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
                 GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    /*FIXME - 错题本
    这里一定要设置纹理属性，否则色彩帧缓冲无法使用
    文档中，这两个参数（GL_TEXTURE_MIN / MAG_FILTER）好像没有默认值*/
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           textureAttachment, 0);
    /*使用深度 / 模板混合的纹理附件
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, CAMERA_WIDTH,
                 CAMERA_HEIGH, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                 NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                           GL_TEXTURE_2D, textureAttachment, 0);*/
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
    // glCullFace(GL_BACK);                                // 剔除背面
    // glFrontFace(GL_CCW);                                // 逆时针为正面
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // 设置清空颜色
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //设置渲染模式（几何体 or
    // 线框）
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

        /**NOTE - 渲染天空盒
         */
        // glDepthMask(GL_FALSE);  // 禁用深度写入
        // 渲染天空盒
        // glDepthMask(GL_TRUE);  // 启用深度写入

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
        /**FIXME - 混合和深度测试一起使用的问题
         * 在这个程序中，橙球，玻璃还有线框都被正确的绘制了，
         * 但是透过玻璃看到的紫球没有被正确的绘制出来。
         * 原因是：绘制玻璃的时候更新了深度值，导致紫色球在深度测试中被丢弃
         *
         * 详细分析：
         * 1. 橙色球是首先被绘制的，没什么问题
         * 2. 绘制玻璃的时候，更新了深度值，现在玻璃区域的深度值更小了
         * 3.绘制紫色球时，紫色球位于玻璃之后，没能通过深度测试，被丢弃
         * 4. 绘制轮廓，深度测试是关闭的，所以能够正常绘制
         */

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
        /**FIXME -
         * 只修改通过深度测试的条件，而不禁用深度测试？
         * 不行，必须要更新线框的深度，不然天空盒总是会覆盖
         * （最好的方法应该是出模板Stencil）
         *
         * 解决方法：
         * 线框轮廓是除了天空盒，最后一个绘制的几何体
         * 总是通过深度测试，并让它更新深度就好了
         * 反正后面也没有其他几何体要渲染了。。。
         */
        glDepthFunc(GL_ALWAYS);  // 总是通过深度测试
        glDepthMask(GL_TRUE);    // 更新深度缓冲
        outlinerShader.use();
        outlinerShader.setParameter("view", view);
        outlinerShader.setParameter("projection", projection);
        outlinerShader.setParameter(
            "model",
            translate(scale(mat4(1.0f), vec3(1.01f)), vec3(0.5f, 0.0f, 0.0f)));
        outlinerShader.setParameter(
            "outlineShine", (float)(sin(4.0f * glfwGetTime()) / 4.0f + 0.75f));
        sphere.Draw(&outlinerShader);
        /**FIXME -
         * 白色的轮廓消失了，被天空盒覆盖了。。。
         * 禁用深度测试会阻止写入深度值吗？
         * “the depth buffer is not updated if the depth test is disabled.”
         * 文档中说它确实不会更新
         */
        // 恢复深度测试设置
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_TRUE);

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
        /**FIXME
         * 窗户的边缘（贴图alpha=0的位置）是纯色
         * 原因：窗户的深度比天空盒小，天空盒无法覆盖窗户的边缘
         */

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
    if (window == NULL)
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

/**REVIEW - 帧缓冲
 * 到目前为止，我们已经使用了很多屏幕缓冲了：用于写入颜色值的颜色缓冲、用于写入
 * 深度信息的深度缓冲和允许我们根据一些条件丢弃特定片段的模板缓冲。这些缓冲
 * 结合起来叫做帧缓冲(Framebuffer)，它被储存在GPU内存中的某处。
 * OpenGL允许我们定义我们自己的帧缓冲，也就是说我们能够定义我们自己的颜色缓冲，
 * 甚至是深度缓冲和模板缓冲。
 *
 * 我们目前所做的所有操作都是在默认帧缓冲的渲染缓冲上进行的。
 * 默认的帧缓冲是在你创建窗口的时候生成和配置的（GLFW帮我们做了这些）。
 * 通过创建我们自己的帧缓冲，我们可以获得额外的渲染目标(target)。
 *
 * 你可能不能很快理解帧缓冲的应用，但渲染你的场景到不同的帧缓冲能够让我们在场景中加入
 * 类似镜子的东西，或者做出很酷的后期处理效果。
 * 首先我们会讨论它是如何工作的，之后我们将来实现这些炫酷的后期处理效果。
 *
 * NOTE - 创建一个帧缓冲
 * 在绑定到GL_FRAMEBUFFER目标之后，所有的读取和写入帧缓冲的操作将会影响当前绑定的帧缓冲。
 * 我们也可以使用GL_READ_FRAMEBUFFER或GL_DRAW_FRAMEBUFFER，
 * 将一个帧缓冲分别绑定到读取目标或写入目标。绑定到GL_READ_FRAMEBUFFER的帧缓冲将
 * 会使用在所有像是glReadPixels的读取操作中，而绑定到GL_DRAW_FRAMEBUFFER的帧缓冲
 * 将会被用作渲染、清除等写入操作的目标。大部分情况你都不需要区分它们，
 * 通常都会使用GL_FRAMEBUFFER，绑定到两个上。
 *
 * 不幸的是，我们现在还不能使用我们的帧缓冲，因为它还不完整(Complete)，
 * 一个完整的帧缓冲需要满足以下的条件：
 * 1. 附加至少一个缓冲（颜色、深度或模板缓冲）。
 * 2. 至少有一个颜色附件(Attachment)。
 * 3. 所有的附件都必须是完整的（保留了内存）。
 * 4. 每个缓冲都应该有相同的样本数(sample)。
 *
 * 从上面的条件中可以知道，我们需要为帧缓冲创建一些附件，并将附件附加到帧缓冲上。
 * 在完成所有的条件之后，我们可以以GL_FRAMEBUFFER为参数调用glCheckFramebufferStatus，
 * 检查帧缓冲是否完整。它将会检测当前绑定的帧缓冲，并返回规范中这些值的其中之一。
 * 如果它返回的是GL_FRAMEBUFFER_COMPLETE，帧缓冲就是完整的了。
 *
 * 之后所有的渲染操作将会渲染到当前绑定帧缓冲的附件中。由于我们的帧缓冲不是默认帧缓冲，
 * 渲染指令将不会对窗口的视觉输出有任何影响。出于这个原因，渲染到一个不同的帧缓冲被叫
 * 做离屏渲染(Off-screen Rendering)。要保证所有的渲染操作在主窗口中有视觉效果，
 * 我们需要再次激活默认帧缓冲，将它绑定到0。
 * glBindFramebuffer(GL_FRAMEBUFFER, 0);
 *
 * 在完整性检查执行之前，我们需要给帧缓冲附加一个附件。附件是一个内存位置，
 * 它能够作为帧缓冲的一个缓冲，可以将它想象为一个图像。
 * 当创建一个附件的时候我们有两个选项：纹理或渲染缓冲对象(Renderbuffer Object)。
 *
 * NOTE - 纹理附件
 * 当把一个纹理附加到帧缓冲的时候，所有的渲染指令将会写入到这个纹理中，
 * 就像它是一个普通的颜色/深度或模板缓冲一样。使用纹理的优点是，
 * 所有渲染操作的结果将会被储存在一个纹理图像中，我们之后可以在着色器中很方便地使用它。
 *
 * 主要的区别就是，我们将维度设置为了屏幕大小（尽管这不是必须的），
 * 并且我们给纹理的data参数传递了NULL。对于这个纹理，我们仅仅分配了内存而没有填充它。
 * 填充这个纹理将会在我们渲染到帧缓冲之后来进行。
 * 同样注意我们并不关心环绕方式或多级渐远纹理，我们在大多数情况下都不会需要它们。
 *
 * 如果你想将你的屏幕渲染到一个更小或更大的纹理上，
 * 你需要（在渲染到你的帧缓冲之前）再次调用glViewport，
 * 使用纹理的新维度作为参数，否则只有一小部分的纹理或屏幕会被渲染到这个纹理上。
 *
 * 除了颜色附件之外，我们还可以附加一个深度和模板缓冲纹理到帧缓冲对象中。
 * 要附加深度缓冲的话，我们将附件类型设置为GL_DEPTH_ATTACHMENT。
 * 注意纹理的格式(Format)和内部格式(Internalformat)类型将变为GL_DEPTH_COMPONENT，
 * 来反映深度缓冲的储存格式。要附加模板缓冲的话，你要将第二个参数设置为
 * GL_STENCIL_ATTACHMENT，并将纹理的格式设定为GL_STENCIL_INDEX。
 *
 * 也可以将深度缓冲和模板缓冲附加为一个单独的纹理。
 * 纹理的每32位数值将包含24位的深度信息和8位的模板信息。要将深度和模板缓冲附加为一个纹理的话，
 * 我们使用GL_DEPTH_STENCIL_ATTACHMENT类型，并配置纹理的格式，
 * 让它包含合并的深度和模板值。将一个深度和模板缓冲附加为一个纹理到帧缓冲的例子可以在下面找到：
 *
 * NOTE - 渲染缓冲对象附件
 * 渲染缓冲对象(Renderbuffer Object)是在纹理之后引入到OpenGL中，
 * 作为一个可用的帧缓冲附件类型的，所以在过去纹理是唯一可用的附件。和纹理图像一样，
 * 渲染缓冲对象是一个真正的缓冲，即一系列的字节、整数、像素等。
 * 渲染缓冲对象附加的好处是，它会将数据储存为OpenGL原生的渲染格式，
 * 它是为离屏渲染到帧缓冲优化过的。
 *
 * 渲染缓冲对象直接将所有的渲染数据储存到它的缓冲中，不会做任何针对纹理格式的转换，
 * 让它变为一个更快的可写储存介质。然而，渲染缓冲对象通常都是只写的，
 * 所以你不能读取它们（比如使用纹理访问）。当然你仍然还是能够使用glReadPixels来读取它，
 * 这会从当前绑定的帧缓冲，而不是附件本身，中返回特定区域的像素。
 *
 * 因为它的数据已经是原生的格式了，当写入或者复制它的数据到其它缓冲中时是非常快的。
 * 所以，交换缓冲这样的操作在使用渲染缓冲对象时会非常快。我们在每个渲染迭代最后使用的
 * glfwSwapBuffers，也可以通过渲染缓冲对象实现：只需要写入一个渲染缓冲图像，
 * 并在最后交换到另外一个渲染缓冲就可以了。渲染缓冲对象对这种操作非常完美。
 *
 * 由于渲染缓冲对象通常都是只写的，它们会经常用于深度和模板附件，
 * 因为大部分时间我们都不需要从深度和模板缓冲中读取值，只关心深度和模板测试。
 * 我们需要深度和模板值用于测试，但不需要对它们进行采样，所以渲染缓冲对象非常适合它们。
 * 当我们不需要从这些缓冲中采样的时候，通常都会选择渲染缓冲对象，因为它会更优化一点。
 *
 * 创建一个渲染缓冲对象和纹理对象类似，不同的是这个对象是专门被设计作为帧缓冲附件使用的，
 * 而不是纹理那样的通用数据缓冲(General Purpose Data Buffer)。
 * 这里我们选择GL_DEPTH24_STENCIL8作为内部格式，它封装了24位的深度和8位的模板缓冲。
 *
 * 渲染缓冲对象能为你的帧缓冲对象提供一些优化，但知道什么时候使用渲染缓冲对象，
 * 什么时候使用纹理是很重要的。通常的规则是，如果你不需要从一个缓冲中采样数据，
 * 那么对这个缓冲使用渲染缓冲对象会是明智的选择。如果你需要从缓冲中采样颜色或深度值等数据，
 * 那么你应该选择纹理附件。性能方面它不会产生非常大的影响的。
 *
 * NOTE - 渲染到纹理
 * 既然我们已经知道帧缓冲（大概）是怎么工作的了，是时候实践它们了。
 * 我们将会将场景渲染到一个附加到帧缓冲对象上的颜色纹理中，
 * 之后将在一个横跨整个屏幕的四边形上绘制这个纹理。这样视觉输出和没使用帧缓冲时是完全一样的，
 * 但这次是打印到了一个四边形上。这为什么很有用呢？我们会在下一部分中知道原因。
 *
 * 现在这个帧缓冲就完整了，我们只需要绑定这个帧缓冲对象，让渲染到帧缓冲的缓冲中而不
 * 是默认的帧缓冲中。之后的渲染指令将会影响当前绑定的帧缓冲。所有的深度和模板操作都
 * 会从当前绑定的帧缓冲的深度和模板附件中（如果有的话）读取。如果你忽略了深度缓冲，
 * 那么所有的深度测试操作将不再工作，因为当前绑定的帧缓冲中不存在深度缓冲。
 *
 * 所以，要想绘制场景到一个纹理上，我们需要采取以下的步骤：
 * 1. 将新的帧缓冲绑定为激活的帧缓冲，和往常一样渲染场景
 * 2. 绑定默认的帧缓冲
 * 3. 绘制一个横跨整个屏幕的四边形，将帧缓冲的颜色缓冲作为它的纹理。
 *
 * NOTE - 后期处理
 * 既然整个场景都被渲染到了一个纹理上，我们可以简单地通过修改纹理数据创建出一些非常有意思的效果。
 * 在这一部分中，我们将会向你展示一些流行的后期处理效果，并告诉你改如何使用创造力创建你自己的效果。
 *
 * 反转色彩：我们现在能够访问渲染输出的每个颜色，所以在片段着色器中返回这些
 * 颜色的反相(Inversion)并不是很难。我们将会从屏幕纹理中取颜色值，
 * 然后用1.0减去它，对它进行反相
 *
 * 灰度化：另外一个很有趣的效果是，移除场景中除了黑白灰以外所有的颜色，让整个图像灰度化(Grayscale)。
 * 很简单的实现方式是，取所有的颜色分量，将它们平均化
 * 这已经能创造很好的结果了，但人眼会对绿色更加敏感一些，而对蓝色不那么敏感，
 * 所以为了获取物理上更精确的效果，我们需要使用加权的(Weighted)通道：
 *
 * NOTE - 核效果
 * 在一个纹理图像上做后期处理的另外一个好处是，我们可以从纹理的其它地方采样颜色值。
 * 比如说我们可以在当前纹理坐标的周围取一小块区域，对当前纹理值周围的多个纹理值进行采样。
 * 我们可以结合它们创建出很有意思的效果。
 *
 * 核(Kernel)（或卷积矩阵(Convolution
 * Matrix)）是一个类矩阵的数值数组，它的中心为当前的像素，
 * 它会用它的核值乘以周围的像素值，并将结果相加变成一个值。所以，基本上我们是在对当前像素
 * 周围的纹理坐标添加一个小的偏移量，并根据核将结果合并。
 *
 * 你在网上找到的大部分核将所有的权重加起来之后都应该会等于1，如果它们加起来不等于1，
 * 这就意味着最终的纹理颜色将会比原纹理值更亮或者更暗了。
 *
 * NOTE - 模糊
 * 这样的模糊效果创造了很多的可能性。我们可以随着时间修改模糊的量，创造出玩家醉酒时的效果，
 * 或者在主角没带眼镜的时候增加模糊。模糊也能够让我们来平滑颜色值，我们将在之后教程中使用到。
 *
 * 注意，核在对屏幕纹理的边缘进行采样的时候，由于还会对中心像素周围的8个像素进行采样，
 * 其实会取到纹理之外的像素。由于环绕方式默认是GL_REPEAT，所以在没有设置的情况下取到
 * 的是屏幕另一边的像素，而另一边的像素本不应该对中心像素产生影响，
 * 这就可能会在屏幕边缘产生很奇怪的条纹。
 * 为了消除这一问题，我们可以将屏幕纹理的环绕方式都设置为GL_CLAMP_TO_EDGE。
 * 这样子在取到纹理外的像素时，就能够重复边缘的像素来更精确地估计最终的值了。
 */

/**REVIEW - 立方体贴图
 * 我们已经使用2D纹理很长时间了，但除此之外仍有更多的纹理类型等着我们探索。
 * 我们将讨论的是将多个纹理组合起来映射到一张纹理上的一种纹理类型：立方体贴图(Cube
 * Map)。
 *
 * 简单来说，立方体贴图就是一个包含了6个2D纹理的纹理，每个2D纹理都组成了立方体的一个面：
 * 一个有纹理的立方体。你可能会奇怪，这样一个立方体有什么用途呢？为什么要把6张纹理合并到
 * 一张纹理中，而不是直接使用6个单独的纹理呢？立方体贴图有一个非常有用的特性，
 * 它可以通过一个方向向量来进行索引/采样。假设我们有一个1x1x1的单位立方体，
 * 方向向量的原点位于它的中心。使用一个橘黄色的方向向量来从立方体贴图上采样一个
 * 纹理值会像是这样：
 * ![](https://learnopengl-cn.github.io/img/04/06/cubemaps_sampling.png)
 *
 * 方向向量的大小并不重要，只要提供了方向，
 * OpenGL就会获取方向向量（最终）所击中的纹素，并返回对应的采样纹理值。
 *
 * 如果我们假设将这样的立方体贴图应用到一个立方体上，采样立方体贴图所使用的方向
 * 向量将和立方体（插值的）顶点位置非常相像。这样子，只要立方体的中心位于原点，
 * 我们就能使用立方体的实际位置向量来对立方体贴图进行采样了。接下来，我们可以将
 * 所有顶点的纹理坐标当做是立方体的顶点位置。最终得到的结果就是可以访问立方体贴
 * 图上正确面(Face)纹理的一个纹理坐标。
 *
 * NOTE - 创建立方体贴图
 * 立方体贴图是和其它纹理一样的，所以如果想创建一个立方体贴图的话，
 * 我们需要生成一个纹理，并将其绑定到纹理目标上，之后再做其它的纹理操作。
 * 这次要绑定到GL_TEXTURE_CUBE_MAP：
 *
 * 因为立方体贴图包含有6个纹理，每个面一个，我们需要调用glTexImage2D函数6次
 * 参数和之前教程中很类似。但这一次我们将纹理目标(target)参数设置为立方体贴图的一个特定的面，
 * 告诉OpenGL我们在对立方体贴图的哪一个面创建纹理。这就意味着我们需要对立方体贴图的
 * 每一个面都调用一次glTexImage2D。
 *
 * 由于我们有6个面，OpenGL给我们提供了6个特殊的纹理目标，
 * 专门对应立方体贴图的一个面。
 *
 * 和OpenGL的很多枚举(Enum)一样，它们背后的int值是线性递增的，所以如果我们有一个
 * 纹理位置的数组或者vector，我们就可以从GL_TEXTURE_CUBE_MAP_POSITIVE_X开始遍历它们，
 * 在每个迭代中对枚举值加1，遍历了整个纹理目标
 *
 * 不要被GL_TEXTURE_WRAP_R吓到，它仅仅是为纹理的R坐标设置了环绕方式，它对应的
 * 是纹理的第三个维度（和位置的z一样）。我们将环绕方式设置为GL_CLAMP_TO_EDGE，
 * 这是因为正好处于两个面之间的纹理坐标可能不能击中一个面（由于一些硬件限制），
 * 所以通过使用GL_CLAMP_TO_EDGE，OpenGL将在我们对两个面之间采样的时候，
 * 永远返回它们的边界值。
 *
 * 在绘制使用立方体贴图的物体之前，我们要先激活对应的纹理单元，并绑定立方体贴图，
 * 这和普通的2D纹理没什么区别。
 * 在片段着色器中，我们使用了一个不同类型的采样器，samplerCube，
 * 我们将使用texture函数使用它进行采样，但这次我们将使用一个vec3的方向向量而不是vec2。
 *
 * 看起来很棒，但为什么要用它呢？恰巧有一些很有意思的技术，使用立方体贴图来实现的话会简单多了。
 * 其中一个技术就是创建一个天空盒(Skybox)。
 *
 * NOTE - 天空盒
 * 天空盒是一个包含了整个场景的（大）立方体，它包含周围环境的6个图像，
 * 让玩家以为他处在一个比实际大得多的环境当中。游戏中使用天空盒的例子有群山、白云或星空。
 *
 * 如果你将这六个面折成一个立方体，你就会得到一个完全贴图的立方体，
 * 模拟一个巨大的场景。一些资源可能会提供了这样格式的天空盒，你必须手动提取六个面的图像，
 * 但在大部分情况下它们都是6张单独的纹理图像。
 *
 * NOTE - 显示天空盒
 * 用于贴图3D立方体的立方体贴图可以使用立方体的位置作为纹理坐标来采样。
 * 当立方体处于原点(0, 0, 0)时，它的每一个位置向量都是从原点出发的方向向量。
 * 这个方向向量正是获取立方体上特定位置的纹理值所需要的。正是因为这个，
 * 我们只需要提供位置向量而不用纹理坐标了。
 *
 * NOTE - 优化
 * 如果我们先渲染天空盒，我们就会对屏幕上的每一个像素运行一遍片段着色器，
 * 即便只有一小部分的天空盒最终是可见的。可以使用提前深度测试(Early Depth
 * Testing) 轻松丢弃掉的片段能够节省我们很多宝贵的带宽。
 *
 * 所以，我们将会最后渲染天空盒，以获得轻微的性能提升。这样子的话，深度缓冲就会填充
 * 满所有物体的深度值了，我们只需要在提前深度测试通过的地方渲染天空盒的片段就可以了，
 * 很大程度上减少了片段着色器的调用。问题是，天空盒很可能会渲染在所有其他对象之上，
 * 因为它只是一个1x1x1的立方体（译注：意味着距离摄像机的距离也只有1），会通过大部分
 * 的深度测试。不用深度测试来进行渲染不是解决方案，因为天空盒将会复写场景中的其它物体。
 * 我们需要欺骗深度缓冲，让它认为天空盒有着最大的深度值1.0，只要它前面有一个物体，
 * 深度测试就会失败。
 * 
 * 在坐标系统小节中我们说过，透视除法是在顶点着色器运行之后执行的，将gl_Position的
 * xyz坐标除以w分量。我们又从深度测试小节中知道，相除结果的z分量等于顶点的深度值。
 * 使用这些信息，我们可以将输出位置的z分量等于它的w分量，让z分量永远等于1.0，
 * 这样子的话，当透视除法执行之后，z分量会变为w / w = 1.0。
 * 
 * 最终的标准化设备坐标将永远会有一个等于1.0的z值：最大的深度值。结果就是天空盒只
 * 会在没有可见物体的地方渲染了（只有这样才能通过深度测试，其它所有的东西都在天空盒前面）。
 * 
 * 我们还要改变一下深度函数，将它从默认的GL_LESS改为GL_LEQUAL。深度缓冲将会填充上天空盒的1.0值，
 * 所以我们需要保证天空盒在值小于或等于深度缓冲而不是小于时通过深度测试。
 * 
 * NOTE - 环境映射
 * 我们现在将整个环境映射到了一个纹理对象上了，能利用这个信息的不仅仅只有天空盒。
 * 通过使用环境的立方体贴图，我们可以给物体反射和折射的属性。这样使用环境立方体贴图的
 * 技术叫做环境映射(Environment Mapping)，其中最流行的两个是反射(Reflection)和
 * 折射(Refraction)。
 *
 * NOTE - 反射
 * 反射这个属性表现为物体（或物体的一部分）反射它周围环境，即根据观察者的视角，物体的颜色
 * 或多或少等于它的环境。镜子就是一个反射性物体：它会根据观察者的视角反射它周围的环境。
 * 
 * 当反射应用到一整个物体上（像是箱子）时，这个物体看起来就像是钢或者铬这样的高反射性材质。
 * 如果我们加载模型加载小节中的纳米装模型，我们会得到一种整个套装都是使用铬做成的效果..
 * 
 * 这看起来非常棒，但在现实中大部分的模型都不具有完全反射性。
 * 我们可以引入反射贴图(Reflection Map)，来给模型更多的细节。
 * 与漫反射和镜面光贴图一样，反射贴图也是可以采样的纹理图像，
 * 它决定这片段的反射性。通过使用反射贴图，我们可以知道模型的哪些部分该以什么强度显示反射。
 * 在本节的练习中，将由你来为我们之前创建的模型加载器中引入反射贴图，显著提升纳米装模型的细节。
 *
 * NOTE - 折射
 * 环境映射的另一种形式是折射，它和反射很相似。折射是光线由于传播介质的改变而产生的方向变化。
 * 在常见的类水表面上所产生的现象就是折射，光线不是直直地传播，而是弯曲了一点。
 * 将你的半只胳膊伸进水里，观察出来的就是这种效果。
 * ![](https://learnopengl-cn.github.io/img/04/06/cubemaps_refraction_theory.png)
 * 
 * 折射可以使用GLSL的内建refract函数来轻松实现，
 * 它需要一个法向量、一个观察方向和两个材质之间的折射率(Refractive Index)。
 *
 * NOTE - 动态环境贴图
 * 现在我们使用的都是静态图像的组合来作为天空盒，看起来很不错，但它没有在场景中包括可移动
 * 的物体。我们一直都没有注意到这一点，因为我们只使用了一个物体。如果我们有一个镜子一样的物体，
 * 周围还有多个物体，镜子中可见的只有天空盒，看起来就像它是场景中唯一一个物体一样。
 *
 * 通过使用帧缓冲，我们能够为物体的6个不同角度创建出场景的纹理，并在每个渲染迭代中将它们
 * 储存到一个立方体贴图中。之后我们就可以使用这个（动态生成的）立方体贴图来创建出更真实的，
 * 包含其它物体的，反射和折射表面了。这就叫做动态环境映射(Dynamic Environment
 * Mapping)， 因为我们动态创建了物体周围的立方体贴图，并将其用作环境贴图。
 *
 * 虽然它看起来很棒，但它有一个很大的缺点：我们需要为使用环境贴图的物体渲染场景6次，
 * 这是对程序是非常大的性能开销。现代的程序通常会尽可能使用天空盒，并在可能的时候使用
 * 预编译的立方体贴图，只要它们能产生一点动态环境贴图的效果。虽然动态环境贴图是
 * 一个很棒的技术，但是要想在不降低性能的情况下让它工作还是需要非常多的技巧的。
 */
