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
GLuint createSkyboxTexture(const char* imageFolder);

int main(int argc, char** argv)
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (!initGLFWWindow(window)) return -1;

    /**NOTE - 模型和着色器、纹理
     */
    Model  box("./box/box.obj");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl",
                        "./shader/skyboxShader.fs.glsl");
    Shader instanceBoxShader("./shader/instanceBoxShader.vs.glsl",
                             "./shader/instanceBoxShader.fs.glsl");
    GLuint cubeTexture = createSkyboxTexture("./texture/");  // 创建立方体贴图

    /**NOTE - 计算instance的offset矩阵
     */
    instanceBoxShader.use();
    vec2  offsetTranslate[100];
    int   index  = 0;
    float offset = 0.1f;
    for (int y = -9; y < 10; y += 2)
    {
        for (int x = -9; x < 10; x += 2)
        {
            offsetTranslate[index] = vec2((float)x + offset, (float)y + offset);
            // set shader parameter
            string paraName = "offsets[" + to_string(index) + "]";
            instanceBoxShader.setParameter(paraName, offsetTranslate[index]);
            index++;
        }
    }

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);  // 启用深度缓冲
    glDepthFunc(GL_LEQUAL);   // 修改深度测试的标准

    glEnable(GL_STENCIL_TEST);                     // 启用模板缓冲
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);  // 设置模板缓冲的操作

    glEnable(GL_BLEND);                                 // 启用混合
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数

    // glEnable(GL_CULL_FACE);  // 启用面剔除

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // 设置清空颜色

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
        // projection应该是透视+投影，转换进标准体积空间：[-1,+1]^3

        /**NOTE - 绘制
         */
        // 绘制100个立方体实例
        instanceBoxShader.use();
        instanceBoxShader.setParameter("view", view);
        instanceBoxShader.setParameter("projection", projection);
        // TODO - 设置offsets矩阵
        box.Draw(&instanceBoxShader, 100);

        /**NOTE - 最后渲染天空盒
         */
        skyboxShader.use();
        skyboxShader.setParameter("view",
                                  mat4(mat3(view)));  // 除去位移，相当于锁头
        skyboxShader.setParameter("projection", projection);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        skyboxShader.setParameter("skybox", 0);
        box.Draw(&skyboxShader);
        //~SECTION

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // 释放内存
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

/// @brief 该函数使用指定的 OpenGL 上下文版本和回调初始化 GLFW 窗口。
/// @param window `initGLFWWindow` 函数中的 `window` 参数是指向GLFWwindow
/// 对象的指针的引用。此函数初始化GLFW，使用指定参数创建窗口，设置回调，隐藏光标，并初始化
/// GLAD 以进行OpenGL 加载。如果成功，则返回
/// @return
/// 函数“initGLFWWindow”返回一个整数值。如果GLFW窗口初始化成功则返回1，如果创建窗口或加载GLAD失败则返回0。
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

/// @brief 函数 createImageObject 加载图像文件，在
/// OpenGL中创建纹理对象，设置纹理参数，生成纹理，并返回纹理 ID。
/// @param imagePath “createImageObject”函数中的“imagePath”参数是一个指向
/// C风格字符串的指针，该字符串表示要加载并从中创建纹理对象的图像文件的路径。该路径应指向文件系统上图像文件的位置。
/// @return 函数 createImageObjrct 返回一个整数值，它是在
/// OpenGL中加载和创建的图像的纹理 ID。
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

/// @brief 加载一个天空盒贴图
/// @param imageFolder 纹理集所在文件夹路径
/// @return 函数 createImageObjrct
/// 返回一个整数值，它是在OpenGL中加载和创建的图像的纹理 ID。
GLuint createSkyboxTexture(const char* imageFolder)
{
    GLuint cubeTexture;
    glGenTextures(1, &cubeTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
    string basePath            = string(imageFolder);
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
    return cubeTexture;
}

/**REVIEW - 实例化
 * 假设你有一个绘制了很多模型的场景，而大部分的模型包含的是同一组顶点数据，只不过进行的是
 * 不同的世界空间变换。想象一个充满草的场景：每根草都是一个包含几个三角形的小模型。
 * 你可能会需要绘制很多根草，最终在每帧中你可能会需要渲染上千或者上万根草。
 * 因为每一根草仅仅是由几个三角形构成，渲染几乎是瞬间完成的，但上千个渲染函数调用却会极大地
 * 影响性能。
 *
 * 如果像这样绘制模型的大量实例(Instance)，你很快就会因为绘制调用过多而达到性能瓶颈。
 * 与绘制顶点本身相比，使用glDrawArrays或glDrawElements函数告诉GPU去绘制你的顶点数据
 * 会消耗更多的性能，因为OpenGL在绘制顶点数据之前需要做很多准备工作（比如告诉GPU该从
 * 哪个缓冲读取数据，从哪寻找顶点属性，而且这些都是在相对缓慢的CPU到GPU总线
 * (CPU to GPU Bus)上进行的）。所以，即便渲染顶点非常快，命令GPU去渲染却未必。
 *
 * 如果我们能够将数据一次性发送给GPU，然后使用一个绘制函数让OpenGL
 * 利用这些数据绘制多个物体，就会更方便了。这就是实例化(Instancing)。
 *
 * 实例化这项技术能够让我们使用一个渲染调用来绘制多个物体，来节省每次绘制物体时
 * CPU -> GPU的通信，它只需要一次即可。如果想使用实例化渲染，我们只需要将
 * glDrawArrays和glDrawElements的渲染调用分别改为glDrawArraysInstanced和
 * glDrawElementsInstanced就可以了。这些渲染函数的实例化版本需要一个额外的参数，
 * 叫做实例数量(Instance Count)，它能够设置我们需要渲染的实例个数。
 * 这样我们只需要将必须的数据发送到GPU一次，然后使用一次函数调用告诉GPU它应该如何
 * 绘制这些实例。GPU将会直接渲染这些实例，而不用不断地与CPU进行通信。
 *
 * 这个函数本身并没有什么用。渲染同一个物体一千次对我们并没有什么用处，
 * 每个物体都是完全相同的，而且还在同一个位置。我们只能看见一个物体！出于这个原因，
 * GLSL在顶点着色器中嵌入了另一个内建变量，gl_InstanceID。
 *
 * 在使用实例化渲染调用时，gl_InstanceID会从0开始，在每个实例被渲染时递增1。
 * 比如说，我们正在渲染第43个实例，那么顶点着色器中它的gl_InstanceID将会是42。
 * 因为每个实例都有唯一的ID，我们可以建立一个数组，将ID与位置值对应起来，
 * 将每个实例放置在世界的不同位置。
 *
 * 为了体验一下实例化绘制，我们将会在标准化设备坐标系中使用一个渲染调用，
 * 绘制100个2D四边形。我们会索引一个包含100个偏移向量的uniform数组，
 * 将偏移值加到每个实例化的四边形上。最终的结果是一个排列整齐的四边形网格：
 */
