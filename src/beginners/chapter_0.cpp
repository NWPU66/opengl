#include <glad/glad.h>
// GLAD first
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include "util/class_camera.hpp"
#include "util/class_shader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace glm;
using namespace std;

/**NOTE - 全局变量、摄像机、全局时钟
 * 在设置任何uniform参数之前，记得启动着色器。
 */
const GLint CAMERA_WIDTH = 800;
const GLint CAMERA_HEIGH = 600;
Camera *camera =
    new Camera(vec3(0.0f, 0.0f, 3.0f), vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float mouseLastX = 0.0f, mouseLastY = 0.0f; // 记录鼠标的位置
float lastFrame = 0.0f, deltaTime = 0.0f;   // 全局时钟

/**NOTE - 几何体元数据
 */
float vertices[] = {
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,

    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f};
vec3 cubePositions[] = {vec3(0.0f, 0.0f, 0.0f), vec3(2.0f, 5.0f, -15.0f),
                        vec3(-1.5f, -2.2f, -2.5f), vec3(-3.8f, -2.0f, -12.3f),
                        vec3(2.4f, -0.4f, -3.5f), vec3(-1.7f, 3.0f, -7.5f),
                        vec3(1.3f, -2.0f, -2.5f), vec3(1.5f, 2.0f, -2.5f),
                        vec3(1.5f, 0.2f, -1.5f), vec3(-1.3f, 1.0f, -1.5f)};
GLuint idx[] = {0, 1, 3, 1, 2, 3}; // 绘制顺序

/// @brief 视窗回调函数
void framebuffer_size_callback(GLFWwindow *window, int w, int h);
/// @brief 鼠标回调函数
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
/// @brief 鼠标滚轮回调函数
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
/// @brief 在每个周期处理来外部输入的事件和操作
void processInput(GLFWwindow *window);
/// @brief 创建一个GLFW窗口并初始化GLAD，成功返回1，不成功则返回0
int initGLFWWindow(GLFWwindow *&window);
/// @brief 从外部文件读取图像，返回一个OpenGL纹理对象的ID
int createImageObjrct(const char *imagePath);

int main(int argc, char **argv)
{
    GLFWwindow *window; // 创建GLFW窗口
    if (!initGLFWWindow(window))
        return -1;

    // SECTION - 准备数据
    /**NOTE - 创建着色器
     * 错题本：Shader 的构造函数中，use() 函数包含 gl 的基本函数，要在 GLAD 初始化后在调用
     */
    Shader *shader =
        new Shader("./shader/coordinateSystem.vs.glsl", "./shader/coordinateSystem.fs.glsl");
    shader->use(); // 在设置任何uniform参数之前，记得启动着色器。

    /**NOTE - 读入并生成纹理
     */
    GLuint texture1 = createImageObjrct("wall.jpg");
    GLuint texture2 = createImageObjrct("awesomeface.png");
    // 设置纹理对应的采样器插槽号
    shader->setInt("myTexture1", 0);
    shader->setInt("myTexture2", 1);
    // 绑定时激活OpenGL的纹理槽
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    /**NOTE - 创建顶点数据
     */
    GLuint VBO, VAO, EBO;
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);
    // 绑定
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // 将顶点数组、绘制顺序送入VBO和EBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    // 设置顶点属性，描述VBO中数据的解读方式
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    //~SECTION

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 更新时钟、摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window);

        // SECTION - 渲染循环
        /**NOTE - 清空屏幕并激活着色器
         */
        glEnable(GL_DEPTH_TEST); // 启用深度缓冲
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader->use(); // 激活着色器程序

        // SECTION - Shader 参数更新
        /**NOTE -更新 outsideSetColor
         */
        float greenValue = (sin(glfwGetTime()) / 2.0f) + 0.5f;
        glUniform4f(glGetUniformLocation(shader->ID, "outsideSetColor"), 0.0f,
                    greenValue, 0.0f, 1.0f);

        /**NOTE -更新 DIY 变换矩阵
         */
        mat4 trans = scale(mat4(1.0f), vec3(.5f, .5f, .5f));
        // trans = rotate(trans, -2.0f * (float)glfwGetTime(), vec3(0.0f, 1.0f, 0.0f));
        // trans = translate(trans, vec3(0.0f, 0.0f, -1.0f));
        // trans = rotate(trans, 5.0f * (float)glfwGetTime(), vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "transform"), 1,
                           GL_FALSE, value_ptr(trans));

        /**NOTE -更新模型、视、投影变换
         */
        mat4 model = rotate(mat4(1.0f), radians(-55.0f), vec3(1.0f, 0.0f, 0.0f));
        mat4 view = camera->GetViewMatrix();
        mat4 projection =
            perspective(radians(camera->Zoom), (float)CAMERA_WIDTH / (float)CAMERA_HEIGH,
                        0.1f, 100.0f);
        shader->setMat4("model", model);
        shader->setMat4("view", view);
        shader->setMat4("projection", projection);
        //~SECTION

        /**NOTE -绘制图形
         */
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        for (int i = 0; i < 10; i++)
        {
            model = translate(mat4(1.0f), cubePositions[i]);
            model = rotate(model, radians(20.0f * i), vec3(1.0f, 0.3f, 0.5f));
            shader->setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        //~SECTION

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    /**NOTE - 删除OpenGL对象并释放内存
     */
    glfwTerminate();
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shader->ID);
    delete shader, camera;
    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int w, int h)
{
    glViewport(0, 0, w, h);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    camera->ProcessMouseMovement(xpos - mouseLastX, ypos - mouseLastY, true);
    mouseLastX = xpos;
    mouseLastY = ypos;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera->ProcessMouseScroll(yoffset);
}

void processInput(GLFWwindow *window)
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
    direction[0] = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) ? 1 : 0;
    direction[1] = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) ? 1 : 0;
    direction[2] = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) ? 1 : 0;
    direction[3] = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) ? 1 : 0;
    direction[4] = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) ? 1 : 0;
    direction[5] = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) ? 1 : 0;
    camera->ProcessKeyboard(direction, deltaTime);
}

int initGLFWWindow(GLFWwindow *&window)
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

int createImageObjrct(const char *imagePath)
{
    // 读取
    GLint width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // 加载图片时翻转y轴
    GLubyte *data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
    // 创建纹理对象
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    //
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
        else // nrChannels == 4
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
    stbi_image_free(data);           // 释放图像的内存，无论有没有data都释放
    glBindTexture(GL_TEXTURE_2D, 0); // 解绑
    return texture;
}
