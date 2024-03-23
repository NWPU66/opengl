#include <glad/glad.h>
// GLAD first
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util/class_camera.hpp"
#include "util/class_shader.hpp"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace glm;
using namespace std;

/**NOTE - 全局变量、摄像机、全局时钟
 * 在设置任何uniform参数之前，记得启动着色器。
 */
const GLint CAMERA_WIDTH = 800;
const GLint CAMERA_HEIGH = 600;
const float cameraAspect = (float)CAMERA_WIDTH / (float)CAMERA_HEIGH;
Camera*     camera =
    new Camera(vec3(0.0f, 0.0f, 2.0f), vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float mouseLastX = 0.0f, mouseLastY = 0.0f;  // 记录鼠标的位置
float lastFrame = 0.0f, deltaTime = 0.0f;    // 全局时钟

/**NOTE - 几何体元数据
 */
float vertices[] = {
    -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f,
    0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f,

    -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,

    -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,

    0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f,
    0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,

    -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,
    0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f,

    -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f,
};
vec3 lightPos = vec3(0.3f, 0.4f, 1.2f);

/// @brief 视窗回调函数
void framebuffer_size_callback(GLFWwindow* window, int w, int h);
/// @brief 鼠标回调函数
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
/// @brief 鼠标滚轮回调函数
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
/// @brief 在每个周期处理来外部输入的事件和操作
void processInput(GLFWwindow* window);
/// @brief 创建一个GLFW窗口并初始化GLAD，成功返回1，不成功则返回0
int initGLFWWindow(GLFWwindow*& window);
/// @brief 从外部文件读取图像，返回一个OpenGL纹理对象的ID
int createImageObjrct(const char* imagePath);

int main(int argc, char** argv)
{
    GLFWwindow* window;  // 创建GLFW窗口
    if (!initGLFWWindow(window)) return -1;

    // SECTION - 准备数据
    /**NOTE - 创建着色器
     * 错题本：Shader 的构造函数中，use() 函数包含 gl 的基本函数，
     * 要在GLAD初始化后在调用
     */
    Shader* objShader =
        new Shader("./shader/color_obj.vs.glsl", "./shader/color_obj.fs.glsl");
    Shader* lightShader = new Shader("./shader/color_light.vs.glsl",
                                     "./shader/color_light.fs.glsl");
    // Shader 的构造函数已经把 Shader 启动了

    /**NOTE - 创建顶点数据
     */
    GLuint objVAO, lightVAO, VBO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &objVAO);
    glGenVertexArrays(1, &lightVAO);
    // 读取顶点数据至VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 绑定各VAO并设置数据的存储规则
    glBindVertexArray(objVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(lightVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    //~SECTION

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 更新时钟、摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window);

        // SECTION - 渲染循环：绘制灯光和立方体
        /**NOTE - 清空屏幕并激活着色器
         */
        glEnable(GL_DEPTH_TEST);  // 启用深度缓冲
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /**NOTE - 更新视图变换
         */
        mat4 model_obj   = mat4(1.0f);
        mat4 model_light = translate(mat4(1.0f), lightPos);
        model_light      = scale(model_light, vec3(0.1f));
        mat4 view        = camera->GetViewMatrix();
        mat4 projection =
            perspective(radians(camera->Zoom), cameraAspect, 0.1f, 100.0f);
        /**FIXME - 错题本
         * radians(camera->Zoom) 和 camera->Zoom 不一样，pi/4 和 45。
         *
         * OpenGL一次渲染只能选中一个Shader和一个VAO。
         * 所以 objShader 和 lightShader 要分别选中，分别绘制。
         */

        /**NOTE - 绘制立方体
         */
        objShader->use();
        objShader->setParameter("objectColor", vec3(1.0f, 0.5f, 0.31f));
        objShader->setParameter("lightColor", vec3(1.0f, 1.0f, 1.0f));
        objShader->setParameter("lightPos", lightPos);
        objShader->setParameter("model", model_obj);
        objShader->setParameter("view", view);
        objShader->setParameter("projection", projection);
        glBindVertexArray(objVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        /**FIXME - 错题本
         * setMat4()：设置着色器属性的时候看清楚shader内部变量的名字
         * 矩阵的类型是mat4，之前把它写成vec3了。
         */

        /**NOTE - 绘制灯光
         */
        lightShader->use();
        lightShader->setParameter("model", model_light);
        lightShader->setParameter("view", view);
        lightShader->setParameter("projection", projection);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        //~SECTION

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    /**NOTE - 删除OpenGL对象并释放内存
     */
    glfwTerminate();
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &objVAO);
    glDeleteBuffers(1, &lightVAO);
    glDeleteProgram(lightShader->ID);
    glDeleteProgram(objShader->ID);
    delete objShader, lightShader, camera;
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, w, h);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    camera->ProcessMouseMovement(xpos - mouseLastX, ypos - mouseLastY, true);
    mouseLastX = xpos;
    mouseLastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera->ProcessMouseScroll(yoffset);
}

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

int createImageObjrct(const char* imagePath)
{
    // 读取
    GLint width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);  // 加载图片时翻转y轴
    GLubyte* data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
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

/**REVIEW - 颜色
 * 我们在现实生活中看到某一物体的颜色并不是这个物体真正拥有
 * 的颜色，而是它所反射的
 * (Reflected)颜色。换句话说，那些不能被物体所吸收(Absorb)的颜色（被拒绝的颜色）
 * 就是我们能够感知到的物体的颜色。例如，太阳光能被看见的白光其实是由许多不同的
 * 颜色组合而成的（如下图所示）。如果我们将白光照在一个蓝色的玩具上，这个蓝色的
 * 玩具会吸收白光中除了蓝色以外的所有子颜色，不被吸收的蓝色光被反射到我们的眼中，
 * 让这个玩具看起来是蓝色的。下图显示的是一个珊瑚红的玩具，它以不同强度反射了多个颜色。
 * ![](https://learnopengl-cn.github.io/img/02/01/light_reflection.png)
 *
 * 你可以看到，白色的阳光实际上是所有可见颜色的集合，物体吸收了其中的大部分颜色。它
 * 仅反射了代表物体颜色的部分，被反射颜色的组合就是我们所感知到的颜色（此例中为珊瑚红）。
 *
 * 这些颜色反射的定律被直接地运用在图形领域。当我们在OpenGL中创建一个光源时，
 * 我们希望给光源一个颜色。在上一段中我们有一个白色的太阳，所以我们也将光源设置为白色。
 * 当我们把光源的颜色与物体的颜色值相乘，所得到的就是这个物体所反射的颜色
 * （也就是我们所感知到的颜色）。我们将这两个颜色向量作分量相乘，结果就是最终的颜色向量了：
 *
 * 我们可以看到，如果我们用绿色光源来照射玩具，那么只有绿色分量能被反射和感知到，
 * 红色和蓝色都不能被我们所感知到。这样做的结果是，
 * 1个珊瑚红的玩具突然变成了深绿色物体。
 *
 * NOTE - 创建一个光照场景
 * 首先我们需要一个物体来作为被投光(Cast the light)的对象，
 * 我们还需要一个物体来代表光源在3D场景中的位置。
 * 简单起见，我们依然使用一个立方体来代表光源
 *
 * 因为我们还要创建一个表示灯（光源）的立方体，所以我们还要为这个灯创建一个专门的VAO。
 * 当然我们也可以让这个灯和其它物体使用同一个VAO，简单地对它的model（模型）
 * 矩阵做一些变换就好了，然而接下来的教程中我们会频繁地对顶点数据和属性指针做出修改，
 * 我们并不想让这些修改影响到灯（我们只关心灯的顶点位置），
 * 因此我们有必要为灯创建一个新的VAO。
 */
