#include <glad/glad.h>
// GLAD first
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "util/class_shader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// SECTION - 1 几何体源数据
float vertices[] = {
    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
    0.8f,  0.8f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 右上
    0.8f,  -0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // 右下
    -0.8f, -0.8f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // 左下
    -0.8f, 0.8f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // 左上
};

GLuint idx[] = {0, 1, 3, 1, 2, 3};  // 绘制顺序
//~SECTION

/// @brief 视窗回调函数
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
}

/// @brief 在每个周期处理来外部输入的事件和操作
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
}

/// @brief 创建一个GLFW窗口并初始化GLAD，成功返回1，不成功则返回0
int initGLFWWindow(GLFWwindow*& window) {
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    window = glfwCreateWindow(800, 600, "Window", NULL, NULL);
    if (window == NULL) {
        std::cout << "failed to create a window!" << std::endl;
        glfwTerminate();
        return 0;
    }
    glfwMakeContextCurrent(window);
    // 注册视窗回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "failed to load GLAD!" << std::endl;
        glfwTerminate();
        return 0;
    }

    return 1;
}

/// @brief 从外部文件读取图像，返回一个OpenGL纹理对象的ID
int createImageObjrct(const char* imagePath) {
    /*
    错题本："wall.jpg"是字符串常量，
    而char* imagePath声明imagePath所指向的字符串是变量
    所以要加上const，const char* imagePath申明imagePath的内容是常量
    但是imagePath指针本身是变量，可以改变
    */
    // 读取
    GLint width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);  // 加载图片时翻转y轴
    GLubyte* data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
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
    if (data) {
        if (nrChannels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data);
            /*
            glTexImage2D参数：
            纹理目标、MipMap级别、存储格式、纹理的宽度和高度、
            总为0、源图的格式和数据类型以及图像数据
            */
        } else {
            // nrChannels == 4
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
        return -1;
    }
    stbi_image_free(data);  // 释放图像的内存，无论有没有data都释放
    glBindTexture(GL_TEXTURE_2D, 0);  // 解绑

    return texture;
}

int main(int argc, char** argv) {
    GLFWwindow* window;  // ANCHOR - 2 创建GLFW窗口
    if (!initGLFWWindow(window)) return -1;

    // SECTION - 3 准备OpenGL对象
    // ANCHOR - 3.1 创建着色器程序
    Shader* shader =
        new Shader("coordinateSystem.vs.glsl", "coordinateSystem.fs.glsl");
    shader->use();  // 在设置任何uniform参数之前，记得启动着色器。

    // SECTION - 3.2 读入并生成纹理
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
    //~SECTION

    // SECTION - 3.3 创建顶点数据
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    //~SECTION
    //~SECTION

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        processInput(window);  // 处理输入

        // SECTION - 4 渲染指令
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);  // ANCHOR - 清空屏幕并激活着色器
        shader->use();                 // 激活着色器程序

        // SECTION - Shader 参数更新
        // ANCHOR - 更新 outsideSetColor
        float greenValue = (sin(glfwGetTime()) / 2.0f) + 0.5f;
        // 更新之前你必须先使用glUseProgram激活着色器程序中设置uniform。
        glUniform4f(glGetUniformLocation(shader->ID, "outsideSetColor"), 0.0f,
                    greenValue, 0.0f, 1.0f);

        // ANCHOR - 更新变换矩阵
        glm::mat4 trans = glm::scale(glm::mat4(1.0f), glm::vec3(.5f, .5f, .5f));
        trans = glm::rotate(trans, (float)glfwGetTime(),
                            glm::vec3(0.0f, 0.0f, 1.0f));
        trans = glm::translate(trans, glm::vec3(0.0f, 1.0f, 0.0f));
        trans = glm::rotate(trans, 2.5f * (float)glfwGetTime(),
                            glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "transform"), 1,
                           GL_FALSE, glm::value_ptr(trans));
        //~SECTION

        // ANCHOR - 绘制图形
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        //~SECTION

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ANCHOR - 5 删除OpenGL对象并释放内存
    glfwTerminate();
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shader->ID);
    delete shader;

    return 0;
}
