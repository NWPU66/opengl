#include <glad/glad.h>
// GLAD should be include first
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

void printOpenglInfo() {
    const GLubyte* name =
        glGetString(GL_VENDOR);  // 返回负责当前OpenGL实现厂商的名字
    const GLubyte* token =
        glGetString(GL_RENDERER);  // 返回一个渲染器标识符，通常是个硬件平台
    const GLubyte* OpenGLVersion =
        glGetString(GL_VERSION);  // 返回当前OpenGL实现的版本号
    // const GLubyte* gluVersion = gluGetString(GLU_VERSION);
    // //返回当前GLU工具库版本

    std::cout << "OpenGL Provider = " << name << std::endl;
    std::cout << "Render Token = " << token << std::endl;
    std::cout << "OpenGL Version = " << OpenGLVersion << std::endl;
    // printf("OGLU工具库版本=%s\n", gluVersion);
}

/// @brief 窗口的回调函数：每次窗口大小被调整的时候被调用
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    /*
    当用户改变窗口的大小的时候，视口也应该被调整。
    我们可以对窗口注册一个回调函数(Callback Function)，
    它会在每次窗口大小被调整的时候被调用。

    这个帧缓冲大小函数需要一个GLFWwindow指针，以及两个整数表示窗口的新维度。
    每当窗口改变大小，GLFW会调用这个函数并填充相应的参数供你处理。
    */
    glViewport(0, 0, w, h);
}

/// @brief  在渲染循环中处理外界输入
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }
}

int main(int argc, char** argv) {
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);  // OpenGL主版本号
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);  // OpenGL次版本号
    glfwWindowHint(GLFW_OPENGL_PROFILE,
                   GLFW_OPENGL_CORE_PROFILE);  // 使用核心模式

    // 创建窗口对象
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "failed to create a GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);  // 将窗口的上下文设置为当前线程的主上下文

    /*
    GLAD是用来管理OpenGL的函数指针的，
    所以在调用任何OpenGL的函数之前我们需要初始化GLAD
    */
    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "failed to initialize GLAD!" << std::endl;
        return -1;
    }

    // 打印OpenGL版本
    printOpenglInfo();

    glViewport(0, 0, 800, 600);  // 声明渲染窗口的大小，即视口(Viewport)大小
    /*
    对应光栅化的最后一个变换：视口变换或视窗变换
    glViewport参数：左下角坐标以及视窗像素大小

    实际上也可以将视口的维度设置为比GLFW的维度小，
    这样子之后所有的OpenGL渲染将会在一个更小的窗口中显示，
    这样子的话我们也可以将一些其它元素显示在OpenGL视口之外。
    */

    // 注册窗口的回调函数，告诉GLFW每当窗口调整大小的时候调用这个函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    /*
    当窗口被第一次显示的时候framebuffer_size_callback也会被调用。
    在创建窗口之后，渲染循环初始化之前注册这些回调函数。
    */

    /*
    渲染循环：
    glfwWindowShouldClose在我们每次循环的开始前检查一次GLFW是否被要求退出
    glfwPollEvents检查有没有触发什么事件、更新窗口状态，并调用对应的回调函数。
    glfwSwapBuffers会交换颜色缓冲（它是一个储存着窗口每一个像素颜色值的大缓冲）
    它在这一迭代中被用来绘制，并且将会作为输出显示在屏幕上。
    */
    while (!glfwWindowShouldClose(window)) {
        // 输入
        processInput(window);

        // 渲染指令
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        /*
         在每个新的迭代开始的时候我们总是希望清屏，否则我们仍能看见上一次迭代的渲染结果。
         我们可以通过调用glClear来清空屏幕的颜色缓冲，
         它接受一个缓冲位(Buffer Bit)来指定要清空的缓冲，
         可能的缓冲位有COLOR_BUFFER，DEPTH_BUFFER和STENCIL_BUFFER。
         由于现在我们只关心颜色值，所以我们只清空颜色缓冲。

         glClearColor函数是一个状态设置函数，
         而glClear函数则是一个状态使用的函数，它使用了当前的状态来获取应该清除为的颜色。
         */

        // 检查并调用事件，交换缓冲
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    /*
    双缓冲(Double Buffer)：
    因为生成的图像不是一下子被绘制出来的，
    而是按照从左到右，由上而下逐像素地绘制而成的。
    最终图像不是在瞬间显示给用户，而是通过一步一步生成的，这会导致渲染的结果闪烁。

    为了规避这些问题，我们应用双缓冲渲染窗口应用程序。
    前缓冲保存着最终输出的图像，它会在屏幕上显示；
    而所有的的渲染指令都会在后缓冲上绘制。
    当所有的渲染指令执行完毕后，我们交换(Swap)前缓冲和后缓冲。
    */
    glfwTerminate();  // 释放/删除之前的分配的所有资源
    return 0;
}