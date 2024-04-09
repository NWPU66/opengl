#include <glad/glad.h>
// GLAD first
#include "util/util.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
using namespace glm;

/**NOTE - 全局变量、摄像机、全局时钟
 */
const GLint CAMERA_WIDTH = 800;
const GLint CAMERA_HEIGH = 600;
const float cameraAspect = (float)CAMERA_WIDTH / (float)CAMERA_HEIGH;
Camera*     camera =
    new Camera(vec3(0.0f, 0.0f, 2.0f), vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float mouseLastX = 0.0f, mouseLastY = 0.0f;  // 记录鼠标的位置
float lastFrame = 0.0f, deltaTime = 0.0f;    // 全局时钟

/**NOTE - 函数
 */
void framebuffer_size_callback(GLFWwindow* window, int w, int h);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
int  initGLFWWindow(GLFWwindow*& window);
int  createImageObjrct(const char* imagePath);

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
    GLuint grassTexture = createImageObjrct("./grass.png");

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);                       // 启用深度缓冲
    glEnable(GL_STENCIL_TEST);                     // 启用模板缓冲
    glEnable(GL_BLEND);                            // 启用混合
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);          // 设置清空颜色
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);  // 设置模板缓冲的操作
    //~SECTION

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 更新时钟、摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window);

        // SECTION - 渲染循环
        /**NOTE - 清空屏幕
         */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);  // 清除颜色、深度和模板缓冲

        /**NOTE - 更新视图变换
         */
        mat4 view = camera->GetViewMatrix();
        mat4 projection =
            perspective(radians(camera->Zoom), cameraAspect, 0.1f, 100.0f);

        /**NOTE - 绘制不需要轮廓的物体
         */
        glStencilMask(0x00);                // 禁用写入模板值
        glStencilFunc(GL_ALWAYS, 1, 0xFF);  // 无条件通过模板测试
        sphereShader.use();
        sphereShader.setParameter("view", view);
        sphereShader.setParameter("projection", projection);
        sphereShader.setParameter(
            "model",
            translate(mat4(1.0f), vec3(-0.5f, 0.0f, 0.0f)));  // TODO - 设置平移
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
            "model",
            translate(mat4(1.0f), vec3(0.5f, 0.0f, 0.0f)));  // TODO - 设置平移
        sphereShader.setParameter("toneColor", vec3(0.5f, 0.0f, 0.31f));
        sphereShader.setParameter("cameraPos", camera->Position);
        sphere.Draw(&sphereShader);

        /**NOTE - 绘制轮廓
         */
        glStencilMask(0x00);                  // 禁用写入模板值
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);  // 模板值不等于1通过模板测试
        glDisable(GL_DEPTH_TEST);             // 禁用深度测试
        outlinerShader.use();
        outlinerShader.setParameter("view", view);
        outlinerShader.setParameter("projection", projection);
        outlinerShader.setParameter(
            "model",
            translate(scale(mat4(1.0f), vec3(1.01f)),
                      vec3(0.5f, 0.0f, 0.0f)));  // TODO - 设置平移和缩放
        sphere.Draw(&outlinerShader);

        /**NOTE - 恢复模板测试和深度测试
         */
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glEnable(GL_DEPTH_TEST);
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

/**REVIEW - 混合
 * OpenGL中，混合(Blending)通常是实现物体透明度(Transparency)的一种技术。
 * 透明就是说一个物体（或者其中的一部分）不是纯色(Solid Color)的，
 * 它的颜色是物体本身的颜色和它背后其它物体的颜色的不同强度结合。一
 * 个有色玻璃窗是一个透明的物体，玻璃有它自己的颜色，但它最终的颜色还包含了
 * 玻璃之后所有物体的颜色。这也是混合这一名字的出处，我们混合(Blend)
 * （不同物体的）多种颜色为一种颜色。所以透明度能让我们看穿物体。
 * ![](https://learnopengl-cn.github.io/img/04/03/blending_transparency.png)
 *
 * 透明的物体可以是完全透明的（让所有的颜色穿过），或者是半透明的（它让颜色通过，
 * 同时也会显示自身的颜色）。一个物体的透明度是通过它颜色的alpha值来决定的。
 * Alpha颜色值是颜色向量的第四个分量，你可能已经看到过它很多遍了。
 * 在这个教程之前我们都将这个第四个分量设置为1.0，让这个物体的透明度为0.0，
 * 而当alpha值为0.0时物体将会是完全透明的。当alpha值为0.5时，
 * 物体的颜色有50%是来自物体自身的颜色，50%来自背后物体的颜色。
 *
 * 我们目前一直使用的纹理有三个颜色分量：红、绿、蓝。但一些材质会有一个
 * 内嵌的alpha通道，对每个纹素(Texel)都包含了一个alpha值。这个alpha值精确地
 * 告诉我们纹理各个部分的透明度。比如说，下面这个窗户纹理中的玻璃部分
 * 的alpha值为0.25（它在一般情况下是完全的红色，但由于它有75%的透明度，
 * 能让很大一部分的网站背景颜色穿过，让它看起来不那么红了），角落的alpha值是0.0。
 *
 * NOTE - 丢弃片段
 * 有些图片并不需要半透明，只需要根据纹理颜色值，显示一部分，或者不显示一部分，
 * 没有中间情况。比如说草，如果想不太费劲地创建草这种东西，你需要将一个草的
 * 纹理贴在一个2D四边形(Quad)上，然后将这个四边形放到场景中。然而，
 * 草的形状和2D四边形的形状并不完全相同，所以你只想显示草纹理的某些部分，
 * 而忽略剩下的部分。
 *
 * 所以当添加像草这样的植被到场景中时，我们不希望看到草的方形图像，而是只显示草的部分，
 * 并能看透图像其余的部分。我们想要丢弃(Discard)显示纹理中透明部分的片段，
 * 不将这些片段存储到颜色缓冲中。在此之前，我们还要学习如何加载一个透明的纹理。
 *
 * OpenGL默认是不知道怎么处理alpha值的，更不知道什么时候应该丢弃片段。
 * 我们需要自己手动来弄。幸运的是，有了着色器，这还是非常容易的。
 * GLSL给了我们discard命令，一旦被调用，它就会保证片段不会被进一步处理，
 * 所以就不会进入颜色缓冲。有了这个指令，我们就能够在片段着色器中检测一个
 * 片段的alpha值是否低于某个阈值，如果是的话，则丢弃这个片段，就好像它不存在一样：
 *
 * 注意，当采样纹理的边缘的时候，OpenGL会对边缘的值和纹理下一个重复的值进行插值
 * （因为我们将它的环绕方式设置为了GL_REPEAT。这通常是没问题的，但是由于
 * 我们使用了透明值，纹理图像的顶部将会与底部边缘的纯色值进行插值。
 * 这样的结果是一个半透明的有色边框，你可能会看见它环绕着你的纹理四边形。
 * 要想避免这个，每当你alpha纹理的时候，请将纹理的环绕方式设置为GL_CLAMP_TO_EDGE：
 *
 * glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
 * glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
 *
 * NOTE - 混合
 * 虽然直接丢弃片段很好，但它不能让我们渲染半透明的图像。我们要么渲染一个片段，
 * 要么完全丢弃它。要想渲染有多个透明度级别的图像，我们需要启用混合(Blending)。
 * 和OpenGL大多数的功能一样，我们可以启用GL_BLEND来启用混合：
 */
