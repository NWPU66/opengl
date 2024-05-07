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
    GLuint grassTexture = createImageObjrct("./texture/window.png");

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);                            // 启用深度缓冲
    glEnable(GL_STENCIL_TEST);                          // 启用模板缓冲
    glEnable(GL_BLEND);                                 // 启用混合
    // glEnable(GL_CULL_FACE);                             // 启用面剔除
    // glCullFace(GL_BACK);                                // 剔除背面
    // glFrontFace(GL_CCW);                                // 逆时针为正面
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);               // 设置清空颜色
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
 *
 * 片段着色器运行完成后，并且所有的测试都通过之后，这个混合方程(Blend Equation)
 * 才会应用到片段颜色输出与当前颜色缓冲中的值（当前片段之前储存的之前片段的颜色）
 * 上。源颜色和目标颜色将会由OpenGL自动设定，但源因子和目标因子的值可以由我们来决定。
 *
 * glBlendFunc(GLenum sfactor, GLenum dfactor)函数接受两个参数，
 * 来设置源和目标因子。OpenGL为我们定义了很多个选项，我们将在下面列出大部分最常用的选项。
 * 注意常数颜色向量C¯constant可以通过glBlendColor函数来另外设置。
 *
 * 为了获得之前两个方形的混合结果，我们需要使用源颜色向量的alpha作为源因子，
 * 使用1−alpha作为目标因子。这将会产生以下的glBlendFunc：
 * glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 * 也可以使用glBlendFuncSeparate为RGB和alpha通道分别设置不同的选项：
 * glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
 *
 * 这个函数和我们之前设置的那样设置了RGB分量，
 * 但这样只能让最终的alpha分量被源颜色向量的alpha值所影响到。
 *
 * OpenGL甚至给了我们更多的灵活性，允许我们改变方程中源和目标部分的运算符。
 * 当前源和目标是相加的，但如果愿意的话，我们也可以让它们相减。
 * glBlendEquation(GLenum mode)允许我们设置运算符，它提供了三个选项：
 * 1. GL_FUNC_ADD：默认选项，将两个分量相加:
 * 2. GL_FUNC_SUBTRACT：将两个分量相减：
 * 3. GL_FUNC_REVERSE_SUBTRACT：将两个分量相减，但顺序相反：
 * 通常我们都可以省略调用glBlendEquation，因为GL_FUNC_ADD对大部分的操作
 * 来说都是我们希望的混合方程，但如果你真的想打破主流，其它的方程也可能符合你的要求。
 *
 * NOTE - 渲染半透明纹理
 * 如果你仔细看的话，你可能会注意到有些不对劲。
 * 最前面窗户的透明部分遮蔽了背后的窗户？这为什么会发生呢？
 *
 * 发生这一现象的原因是，深度测试和混合一起使用的话会产生一些麻烦。
 * 当写入深度缓冲时，深度缓冲不会检查片段是否是透明的，
 * 所以透明的部分会和其它值一样写入到深度缓冲中。
 * 结果就是窗户的整个四边形不论透明度都会进行深度测试。
 * 即使透明的部分应该显示背后的窗户，深度测试仍然丢弃了它们。
 *
 * 所以我们不能随意地决定如何渲染窗户，让深度缓冲解决所有的问题了。
 * 这也是混合变得有些麻烦的部分。要想保证窗户中能够显示它们背后的窗户，
 * 我们需要首先绘制背后的这部分窗户。这也就是说在绘制的时候，
 * 我们必须先手动将窗户按照最远到最近来排序，再按照顺序渲染。
 *
 * 排序透明物体的一种方法是，从观察者视角获取物体的距离。
 * 这可以通过计算摄像机位置向量和物体的位置向量之间的距离所获得。
 * 接下来我们把距离和它对应的位置向量存储到一个STL库的map数据结构中。
 * map会自动根据键值(Key)对它的值排序，所以只要我们添加了所有的位置，
 * 并以它的距离作为键，它们就会自动根据距离值排序了。
 *
 * 在场景中排序物体是一个很困难的技术，很大程度上由你场景的类型所决定，
 * 更别说它额外需要消耗的处理能力了。完整渲染一个包含不透明和透明物体的
 * 场景并不是那么容易。更高级的技术还有次序无关透明度
 * (Order Independent Transparency, OIT)，但这超出本教程的范围了。
 * 现在，你还是必须要普通地混合你的物体，但如果你很小心，并且知道目前方法的限制的话，
 * 你仍然能够获得一个比较不错的混合实现。
 */

/**REVIEW - 面剔除
 * 这正是面剔除(Face Culling)所做的。OpenGL能够检查所有面向(Front
 * Facing)观察者的面， 并渲染它们，而丢弃那些背向(Back
 * Facing)的面，节省我们很多的片段着色器调用
 * （它们的开销很大！）。但我们仍要告诉OpenGL哪些面是正向面(Front Face)，
 * 哪些面是背向面(Back Face)。
 * OpenGL使用了一个很聪明的技巧，分析顶点数据的环绕顺序(Winding Order)。
 *
 * NOTE - 环绕顺序
 * 当我们定义一组三角形顶点时，我们会以特定的环绕顺序来定义它们，可能是顺时针(Clockwise)的，
 * 也可能是逆时针(Counter-clockwise)的。每个三角形由3个顶点所组成，我们会从三角形中间来看，
 * 为这3个顶点设定一个环绕顺序。
 * ![](https://learnopengl-cn.github.io/img/04/04/faceculling_windingorder.png)
 *
 * 每组组成三角形图元的三个顶点就包含了一个环绕顺序。OpenGL在渲染图元的时候将使用
 * 这个信息来决定一个三角形是一个正向三角形还是背向三角形。
 * 默认情况下，逆时针顶点所定义的三角形将会被处理为正向三角形。
 *
 * 当你定义顶点顺序的时候，你应该想象对应的三角形是面向你的，
 * 所以你定义的三角形从正面看去应该是逆时针的。这样定义顶点很棒的一点是，
 * 实际的环绕顺序是在光栅化阶段进行的，也就是顶点着色器运行之后。
 * 这些顶点就是从观察者视角所见的了。
 *
 * 观察者所面向的所有三角形顶点就是我们所指定的正确环绕顺序了，
 * 而立方体另一面的三角形顶点则是以相反的环绕顺序所渲染的。
 * 这样的结果就是，我们所面向的三角形将会是正向三角形，而背面的三角形则是背向三角形。
 * ![](https://learnopengl-cn.github.io/img/04/04/faceculling_frontback.png)
 *
 * NOTE - 面剔除
 */
