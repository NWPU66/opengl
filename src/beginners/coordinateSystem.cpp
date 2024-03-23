#include <glad/glad.h>
// GLAD first
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
using namespace glm;
using namespace std;

#include "util/class_camera.hpp"
#include "util/class_shader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/**NOTE - 全局变量
 */
const GLint CAMERA_WIDTH = 800;
const GLint CAMERA_HEIGH = 600;

/**NOTE - 几何体源数据
 */
// float vertices[] = {
//     //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
//     0.8f,  0.8f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 右上
//     0.8f,  -0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // 右下
//     -0.8f, -0.8f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // 左下
//     -0.8f, 0.8f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // 左上
// };
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

/**NOTE - 摄像机和全局时钟
 */
Camera *camera =
    new Camera(vec3(0.0f, 0.0f, -3.0f), vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float mouseLastX = 0.0f, mouseLastY = 0.0f;
float lastFrame = 0.0f, deltaTime = 0.0f;

/// @brief 视窗回调函数
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

/// @brief 在每个周期处理来外部输入的事件和操作
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

/// @brief 创建一个GLFW窗口并初始化GLAD，成功返回1，不成功则返回0
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

/// @brief 从外部文件读取图像，返回一个OpenGL纹理对象的ID
int createImageObjrct(const char *imagePath)
{
    /*
    错题本："wall.jpg"是字符串常量，
    而char* imagePath声明imagePath所指向的字符串是变量
    所以要加上const，const char* imagePath申明imagePath的内容是常量
    但是imagePath指针本身是变量，可以改变
    */
    // 读取
    GLint width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // 加载图片时翻转y轴
    GLubyte *data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
    // 创建纹理对象
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    /*
    错题本：这里要用Texture系列的函数：glGenTextures() & glBindTexture()
    */
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
            /*
            glTexImage2D参数：
            纹理目标、MipMap级别、存储格式、纹理的宽度和高度、
            总为0、源图的格式和数据类型以及图像数据
            */
        }
        else
        {
            // nrChannels == 4
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

int main(int argc, char **argv)
{
    /**NOTE - 创建GLFW窗口
     */
    GLFWwindow *window;
    if (!initGLFWWindow(window))
        return -1;

    // SECTION - 准备数据
    /**NOTE - 创建着色器程序
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
    // VAO格式：[0] = 顶点位置；[1] = 顶点颜色；[2] = 顶点纹理坐标
    /*
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    /*
    错题本：(void*)(3 * sizeof(float))
    最后一个参数是void型的指针
    所以相对位置要向后推3个float型数据内存单元的地址，而不是3
    */
    glEnableVertexAttribArray(1);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    //~SECTION

    /**NOTE - 渲染循环
     */
    while (!glfwWindowShouldClose(window))
    {
        // 更新摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window); // 处理输入

        // SECTION - 渲染指令
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
        // 更新之前你必须先使用glUseProgram激活着色器程序中设置uniform。
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
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE,
                           value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE,
                           value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1,
                           GL_FALSE, value_ptr(projection));
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
    delete shader;

    return 0;
}

/**REVIEW - 坐标系统
 * OpenGL希望在每次顶点着色器运行后，我们可见的所有顶点都为标准化设备坐标
 * (Normalized Device Coordinate, NDC)。也就是说，每个顶点的x，y，z坐标都应该
 * 在-1.0到1.0之间，超出这个坐标范围的顶点都将不可见。我们通常会自己设定一个坐标的范围，
 * 之后再在顶点着色器中将这些坐标变换为标准化设备坐标。
 * 然后将这些标准化设备坐标传入光栅器(Rasterizer)，将它们变换为屏幕上的二维坐标或像素。
 *
 * 在流水线中，物体的顶点在最终转化为屏幕坐标之前还会被变换到多个坐标系统
 * (Coordinate System)。将物体的坐标变换到几个过渡坐标系
 * (Intermediate Coordinate System)的优点在于，在这些特定的坐标系统中，
 * 一些操作或运算更加方便和容易：
 * 局部空间(Local Space，或者称为物体空间(Object Space))
 * 世界空间(World Space)
 * 观察空间(View Space，或者称为视觉空间(Eye Space))
 * 裁剪空间(Clip Space)
 * 屏幕空间(Screen Space)
 *
 * NOTE - 概述
 * 我们的顶点坐标起始于局部空间(Local Space)，在这里它称为局部坐标(Local
 * Coordinate)， 它在之后会变为世界坐标(World Coordinate)，观察坐标(View
 * Coordinate)， 裁剪坐标(Clip Coordinate)，并最后以屏幕坐标(Screen
 * Coordinate)的形式结束。
 * ![](https://learnopengl-cn.github.io/img/01/08/coordinate_systems.png)
 *
 * NOTE - 视空间
 * 观察空间经常被人们称之OpenGL的摄像机(Camera)。
 * 观察空间是将世界空间坐标转化为用户视野前方的坐标而产生的结果。
 * 因此观察空间就是从摄像机的视角所观察到的空间。
 * 而这通常是由一系列的位移和旋转的组合来完成，
 * 平移/旋转场景从而使得特定的对象被变换到摄像机的前方。
 * 这些组合在一起的变换通常存储在一个观察矩阵(View Matrix)里，
 * 它被用来将世界坐标变换到观察空间。
 *
 * NOTE - 裁切空间
 * 为了将顶点坐标从观察变换到裁剪空间，我们需要定义一个投影矩阵(Projection
 * Matrix)， 它指定了一个范围的坐标，比如在每个维度上的-1000到1000。
 * 投影矩阵接着会将在这个指定的范围内的坐标变换为标准化设备坐标的范围(-1.0, 1.0)。
 * 所有在范围外的坐标不会被映射到在-1.0到1.0的范围之间，所以会被裁剪掉。
 *
 * # 如果只是图元(Primitive)，例如三角形，的一部分超出了裁剪体积
 * #则OpenGL会重新构建这个三角形为一个或多个三角形让其能够适合这个裁剪范围。
 *
 * 由投影矩阵创建的观察箱(Viewing
 * Box)被称为平截头体(Frustum)，每个出现在平截头体
 * 范围内的坐标都会最终出现在用户的屏幕上。将特定范围内的坐标转化到
 * 标准化设备坐标系的过程（而且它很容易被映射到2D观察空间坐标）被称之为投影(Projection)，
 * 因为使用投影矩阵能将3D坐标投影(Project)到很容易映射到2D的标准化设备坐标系中。
 *
 * 一旦所有顶点被变换到裁剪空间，最终的操作——透视除法(Perspective
 * Division)将会执行，
 * 在这个过程中我们将位置向量的x，y，z分量分别除以向量的齐次w分量；
 * 透视除法是将4D裁剪空间坐标变换为3D标准化设备坐标的过程。
 * 这一步会在每一个顶点着色器运行的最后被自动执行。
 *
 * 将观察坐标变换为裁剪坐标的投影矩阵可以为两种不同的形式，每种形式都定义了不同的平截头体。
 * 我们可以选择创建一个正射投影矩阵(Orthographic Projection Matrix)
 * 或一个透视投影矩阵(Perspective Projection Matrix)
 *
 * NOTE - 正射投影
 * 正射投影矩阵定义了一个类似立方体的平截头箱，它定义了一个裁剪空间，
 * 在这空间之外的顶点都会被裁剪掉。创建一个正射投影矩阵需要指定可见平截头体的
 * 宽、高和长度。在使用正射投影矩阵变换至裁剪空间之后处于这个平截头体内
 * 的所有坐标将不会被裁剪掉。它的平截头体看起来像一个容器：
 *
 * NOTE -透视投影
 * 这个投影矩阵将给定的平截头体范围映射到裁剪空间，除此之外还修改了每个顶点坐标的w值，
 * 从而使得离观察者越远的顶点坐标w分量越大。被变换到裁剪空间的坐标都会在-w到w的范围之间
 * 顶点坐标的每个分量都会除以它的w分量，距离观察者越远顶点坐标就会越小。
 * 这是也是w分量非常重要的另一个原因，它能够帮助我们进行透视投影。
 *
 * 同样，glm::perspective所做的其实就是创建了一个定义了可视空间的大平截头体，
 * 任何在这个平截头体以外的东西最后都不会出现在裁剪空间体积内，并且将会受到裁剪。
 * 一个透视平截头体可以被看作一个不均匀形状的箱子，在这个箱子内部的每个坐标
 * 都会被映射到裁剪空间上的一个点。
 * ![](https://learnopengl-cn.github.io/img/01/08/perspective_frustum.png)
 *
 * 注意矩阵运算的顺序是相反的（记住我们需要从右往左阅读矩阵的乘法）。
 * 最后的顶点应该被赋值到顶点着色器中的gl_Position，OpenGL将会自动进行透视除法和裁剪。
 *
 * 然后呢？
 * 顶点着色器的输出要求所有的顶点都在裁剪空间内，这正是我们刚才使用变换矩阵所做的。
 * OpenGL然后对裁剪坐标执行透视除法从而将它们变换到标准化设备坐标。
 * OpenGL会使用glViewPort内部的参数来将标准化设备坐标映射到屏幕坐标，
 * 每个坐标都关联了一个屏幕上的点（在我们的例子中是一个800x600的屏幕）。
 * 这个过程称为视口变换。
 *
 * NOTE - Z缓冲
 * OpenGL存储它的所有深度信息于一个Z缓冲(Z-buffer)中，也被称为深度缓冲(Depth
 * Buffer)。
 * GLFW会自动为你生成这样一个缓冲（就像它也有一个颜色缓冲来存储输出图像的颜色）。
 * 深度值存储在每个片段里面（作为片段的z值），当片段想要输出它的颜色时，
 * OpenGL会将它的深度值和z缓冲进行比较，如果当前的片段在其它片段之后，它将会被丢弃，
 * 否则将会覆盖。这个过程称为深度测试(Depth Testing)，它是由OpenGL自动完成的。
 */

/**REVIEW - 摄像机
 * OpenGL本身没有摄像机(Camera)的概念，但我们可以通过把场景中的所有物体往相
 * 反方向移动的方式来模拟出摄像机，产生一种我们在移动的感觉，而不是场景在移动。
 * 本节我们将会讨论如何在OpenGL中配置一个摄像机，并且将会讨论FPS风格的摄像机，
 * 让你能够在3D场景中自由移动。我们也会讨论键盘和鼠标输入，最终完成一个自定义的摄像机类。
 *
 * NOTE - 摄像机基本坐标轴
 * 摄像机位置：不要忘记正z轴是从屏幕指向你的，
 * 如果我们希望摄像机向后移动，我们就沿着z轴的正方向移动。
 *
 * 摄像机方向：我们这里希望它指向原点
 * 方向向量(Direction Vector)并不是最好的名字，
 * 因为它实际上指向从它到目标向量的相反方向
 *
 * 右轴：先定义一个上向量(Up Vector)。
 * 接下来把上向量和第二步得到的方向向量进行叉乘。
 *
 * 上轴：
 *
 * NOTE - Look At
 * 使用矩阵的好处之一是如果你使用3个相互垂直（或非线性）
 * 的轴定义了一个坐标空间，你可以用这3个轴外加一个平移向量来
 * 创建一个矩阵，并且你可以用这个矩阵乘以任何向量来将其变换
 * 到那个坐标空间。这正是LookAt矩阵所做的
 *
 * glm::LookAt函数需要一个位置、目标和上向量。
 * 它会创建一个和在上一节使用的一样的观察矩阵。
 *
 * NOTE - 视角移动 鼠标输入
 * 偏航角和俯仰角是通过鼠标（或手柄）移动获得的，水平的移动影响偏航角，竖直的移动影响俯仰角。
 * 它的原理就是，储存上一帧鼠标的位置，在当前帧中我们当前计算鼠标位置与上一帧的位置相差多少。
 * 如果水平/竖直差别越大那么俯仰角或偏航角就改变越大，也就是摄像机需要移动更多的距离。
 *
 * 首先我们要告诉GLFW，它应该隐藏光标，并捕捉(Capture)它。
 *
 *
 */
