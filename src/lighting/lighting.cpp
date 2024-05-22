#include <glad/glad.h>
// GLAD first
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util/class_camera.hpp"
#include "util/class_shader.hpp"
// #include "util/structure.hpp"
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

/**NOTE - 高级渲染设置
 */
const Material material(vec3(0.05f, 0.1f, 0.5f), vec3(1.0f, 0.4f, 0.3f));
const Light    light(vec3(1.0f, 1.2f, 2.0f), 3.5f);

/**NOTE - 几何体元数据
 */
float vertices[] = {
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f,
    -0.5f, 1.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 0.5f,  0.5f,  -0.5f, 1.0f,
    1.0f,  0.0f,  0.0f,  -1.0f, 0.5f,  0.5f,  -0.5f, 1.0f,  1.0f,  0.0f,
    0.0f,  -1.0f, -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  -1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  0.0f,  0.0f,  -1.0f,

    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f,
    0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  1.0f,
    1.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  1.0f,  1.0f,  0.0f,
    0.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,

    -0.5f, 0.5f,  0.5f,  1.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,
    -0.5f, 1.0f,  1.0f,  -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,
    1.0f,  -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  1.0f,  -1.0f,
    0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,
    -0.5f, 0.5f,  0.5f,  1.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,
    -0.5f, 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 0.0f,
    1.0f,  1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 0.0f,  1.0f,  1.0f,
    0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f,  1.0f,  0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f,
    -0.5f, 1.0f,  1.0f,  0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, 0.5f,  1.0f,
    0.0f,  0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,
    -1.0f, 0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f,  1.0f,  0.0f,  -1.0f, 0.0f,

    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,
    -0.5f, 1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,
    0.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
    1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
};

vec3 positions[] = {vec3(0.5f, 0.5f, -1.0f), vec3(0.0f, 0.5f, -0.5f),
                    vec3(0.0f, 1.0f, -1.0f), vec3(-0.5f, 0.5f, -0.5f),
                    vec3(0.0f, .0f, 0.0f)};

void framebuffer_size_callback(GLFWwindow* window, int w, int h);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
int  initGLFWWindow(GLFWwindow*& window);
int  createImageObjrct(const char* imagePath);

int main(int argc, char** argv)
{
    GLFWwindow* window;  // 创建GLFW窗口
    if (!initGLFWWindow(window)) return -1;

    // SECTION - 准备数据
    /**NOTE - 创建着色器
     * 错题本：Shader 的构造函数中，use() 函数包含 gl 的基本函数，
     * 要在GLAD初始化后在调用
     */
    Shader* objShader   = new Shader("./shader/lighting_obj.vs.glsl",
                                     "./shader/lighting_obj.fs.glsl");
    Shader* lightShader = new Shader("./shader/lighting_light.vs.glsl",
                                     "./shader/lighting_light.fs.glsl");

    /**NOTE - 创建纹理对象
     * FIXME - 错题本
     * OpenGL一次只能启动一个Shader，我最后创建的Shader是lighting。
     * 因此lightingShager被激活了，想要修改ObjShader的属性还要手动激活ObjShader。
     */
    objShader->use();  // 启动物体着色器
    GLuint boxDiffTex = createImageObjrct("container2.png");
    GLuint boxSpecTex = createImageObjrct("container2_specular.png");
    // 设置纹理槽
    objShader->setParameter("material.diffuseMap", 0);
    objShader->setParameter("material.specularMap", 1);
    // 激活纹理单元
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, boxDiffTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, boxSpecTex);

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(lightVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)0);
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
        mat4 model_light = translate(mat4(1.0f), light.position);
        model_light      = scale(model_light, vec3(0.1f));
        mat4 view        = camera->GetViewMatrix();
        mat4 projection =
            perspective(radians(camera->Zoom), cameraAspect, 0.1f, 100.0f);
        /**FIXME - 错题本
         * OpenGL一次渲染只能选中一个Shader和一个VAO。
         * 所以 objShader 和 lightShader 要分别选中，分别绘制。
         */

        /**NOTE - 绘制立方体
         */
        objShader->use();
        objShader->setParameter("cameraPos", camera->Position);
        objShader->setParameter("material", material);
        objShader->setParameter("light", light);
        // objShader->setMat4("model", model_obj);
        objShader->setParameter("view", view);
        objShader->setParameter("projection", projection);
        glBindVertexArray(objVAO);
        for (int i = 0; i < 5; i++)
        {
            model_obj = translate(mat4(1.0f), 2.5f * positions[i]);
            objShader->setParameter("model", model_obj);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

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
 * @param xpos mouse_callback 函数中的 xpos 参数表示鼠标光标位置的当前 x 坐标。
 * @param ypos `mouse_callback` 函数中的 `ypos` 参数表示鼠标光标在窗口内的当前 y
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
 * @param window `GLFWwindow* window` 参数是指向接收滚动输入的窗口的指针。
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
 * @param window `initGLFWWindow` 函数中的 `window` 参数是指向 GLFWwindow
 * 对象的指针的引用。此函数初始化
 * GLFW，使用指定参数创建窗口，设置回调，隐藏光标，并初始化 GLAD 以进行 OpenGL
 * 加载。如果成功，则返回
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

/**REVIEW - 基础光照
 * 其中一个模型被称为冯氏光照模型(Phong Lighting Model)。冯氏光照模型的
 * 主要结构由3个分量组成：环境(Ambient)、漫反射(Diffuse)和镜面(Specular)光照。
 * ![](https://learnopengl-cn.github.io/img/02/02/basic_lighting_phong.png)
 *
 * 环境光照(Ambient Lighting)：即使在黑暗的情况下，世界上通常也仍然有一些光亮
 * （月亮、远处的光），所以物体几乎永远不会是完全黑暗的。为了模拟这个，
 * 我们会使用一个环境光照常量，它永远会给物体一些颜色。
 *
 * 漫反射光照(Diffuse Lighting)：模拟光源对物体的方向性影响(Directional
 * Impact)。
 * 它是冯氏光照模型中视觉上最显著的分量。物体的某一部分越是正对着光源，它就会越亮。
 *
 * 镜面光照(Specular Lighting)：模拟有光泽物体上面出现的亮点。
 * 镜面光照的颜色相比于物体的颜色会更倾向于光的颜色。
 *
 * NOTE - 环境光照
 * 光通常都不是来自于同一个光源，而是来自于我们周围分散的很多光源，即使它们可能
 * 并不是那么显而易见。光的一个属性是，它可以向很多方向发散并反弹，从而能够到达
 * 不是非常直接临近的点。所以，光能够在其它的表面上反射，对一个物体产生间接的影响。
 * 考虑到这种情况的算法叫做全局照明(Global
 * Illumination)，但它既开销高昂又极其复杂。
 *
 * 一个简化的全局照明模型，即环境光照。正如你在上一节所学到的，
 * 我们使用一个很小的常量（光照）颜色，添加到物体片段的最终颜色中，
 * 这样子的话即便场景中没有直接的光源也能看起来存在有一些发散的光。
 *
 * NOTE - 漫反射光照
 * 漫反射光照使物体上与光线方向越接近的片段能从光源处获得更多的亮度。
 *
 * 我们需要测量这个光线是以什么角度接触到这个片段的。如果光线垂直于物体表面，这束光对
 * 物体的影响会最大化（译注：更亮）。为了测量光线和片段的角度，我们使用一个叫做法向量
 * (Normal Vector)的东西，它是垂直于片段表面的一个向量（这里以黄色箭头表示），
 * 我们在后面再讲这个东西。这两个向量之间的角度很容易就能够通过点乘计算出来。
 *
 * NOTE - 法向量
 * 法向量是一个垂直于顶点表面的（单位）向量。由于顶点本身并没有表面
 * （它只是空间中一个独立的点），我们利用它周围的顶点来计算出这个顶点的表面。
 * 我们能够使用一个小技巧，使用叉乘对立方体所有的顶点计算法向量，
 * 但是由于3D立方体不是一个复杂的形状，所以我们可以简单地把法线数据手工添加到顶点数据中。
 *
 * 虽然对灯的着色器使用不能完全利用的顶点数据看起来不是那么高效，但这些顶点数据已经从
 * 箱子对象载入后开始就储存在GPU的内存里了，所以我们并不需要储存新数据到GPU内存中。
 * 这实际上比给灯专门分配一个新的VBO更高效了。
 *
 * NOTE - 关于法向量的变换
 * 首先，法向量只是一个方向向量，不能表达空间中的特定位置。同时，法向量没有齐次坐标
 * （顶点位置中的w分量）。这意味着，位移不应该影响到法向量。因此，如果我们打算把
 * 法向量乘以一个模型矩阵，我们就要从矩阵中移除位移部分，只选用模型矩阵左上角3×3的矩阵
 * （注意，我们也可以把法向量的w分量设置为0，再乘以4×4矩阵；这同样可以移除位移）。
 * 对于法向量，我们只希望对它实施缩放和旋转变换。
 * ![](https://learnopengl-cn.github.io/img/02/02/basic_lighting_normal_transformation.png)
 *
 * 其次，如果模型矩阵执行了不等比缩放，顶点的改变会导致法向量不再垂直于表面了。
 * 因此，我们不能用这样的模型矩阵来变换法向量。
 *
 * 每当我们应用一个不等比缩放时（注意：等比缩放不会破坏法线，因为法线的方向没被改变，仅仅改变了法线的长度，
 * 而这很容易通过标准化来修复），法向量就不会再垂直于对应的表面了，这样光照就会被破坏。
 * 修复这个行为的诀窍是使用一个为法向量专门定制的模型矩阵。这个矩阵称之为法线矩阵(Normal
 * Matrix)， 它使用了一些线性代数的操作来移除对法向量错误缩放的影响。
 *
 * 法线矩阵被定义为「模型矩阵左上角3x3部分的逆矩阵的转置矩阵」。
 *
 * 矩阵求逆是一项对于着色器开销很大的运算，因为它必须在场景中的每一个顶点上进行，
 * 所以应该尽可能地避免在着色器中进行求逆运算。以学习为目的的话这样做还好，
 * 但是对于一个高效的应用来说，你最好先在CPU上计算出法线矩阵，
 * 再通过uniform把它传递给着色器（就像模型矩阵一样）。
 *
 * NOTE - 镜面光照
 * 和漫反射光照一样，镜面光照也决定于光的方向向量和物体的法向量，但是它也决定于观察方向，
 * 例如玩家是从什么方向看向这个片段的。镜面光照决定于表面的反射特性。
 * 如果我们把物体表面设想为一面镜子，那么镜面光照最强的地方就是我们看到表面上反射光的地方。
 * ![](https://learnopengl-cn.github.io/img/02/02/basic_lighting_specular_theory.png)
 *
 * 我们通过根据法向量翻折入射光的方向来计算反射向量。然后我们计算反射向量与观察方向的角度差，
 * 它们之间夹角越小，镜面光的作用就越大。由此产生的效果就是，
 * 我们看向在入射光在表面的反射方向时，会看到一点高光。
 *
 * 观察向量是我们计算镜面光照时需要的一个额外变量，我们可以使用观察者的世界空间位置和片段的位置来计算它。
 * 之后我们计算出镜面光照强度，用它乘以光源的颜色，并将它与环境光照和漫反射光照部分加和。
 *
 * NOTE - 题外话
 * 在光照着色器的早期，开发者曾经在顶点着色器中实现冯氏光照模型。在顶点着色器中做光照的
 * 优势是，相比片段来说，顶点要少得多，因此会更高效，所以（开销大的）光照计算频率会更低。
 * 然而，顶点着色器中的最终颜色值是仅仅只是那个顶点的颜色值，片段的颜色值是由插值光照
 * 颜色所得来的。结果就是这种光照看起来不会非常真实，除非使用了大量顶点。
 *
 * 在顶点着色器中实现的冯氏光照模型叫做Gouraud着色(Gouraud Shading)，
 * 而不是冯氏着色(Phong Shading)。记住，由于插值，这种光照看起来有点逊色。
 * 冯氏着色能产生更平滑的光照效果。
 */

/**REVIEW - 材质
 * 在现实世界里，每个物体会对光产生不同的反应。比如，钢制物体看起来通常会比陶土花瓶更
 * 闪闪发光，一个木头箱子也不会与一个钢制箱子反射同样程度的光。有些物体反射光的时候\
 * 不会有太多的散射(Scatter)，因而产生较小的高光点，而有些物体则会散射很多，产生一个有
 * 着更大半径的高光点。如果我们想要在OpenGL中模拟多种类型的物体，
 * 我们必须针对每种表面定义不同的材质(Material)属性。
 *
 * 当描述一个表面时，我们可以分别为三个光照分量定义一个材质颜色(Material
 * Color)： 环境光照(Ambient Lighting)、漫反射光照(Diffuse
 * Lighting)和镜面光照(Specular Lighting)。
 * 通过为每个分量指定一个颜色，我们就能够对表面的颜色输出有细粒度的控制了。
 * 现在，我们再添加一个反光度(Shininess)分量，结合上述的三个颜色，
 * 我们就有了全部所需的材质属性了.
 *
 * ambient材质向量定义了在环境光照下这个表面反射的是什么颜色，通常与表面的颜色相同。
 * diffuse材质向量定义了在漫反射光照下表面的颜色。漫反射颜色（和环境光照一样）
 * 也被设置为我们期望的物体颜色。specular材质向量设置的是表面上镜面高光的颜色（
 * 或者甚至可能反映一个特定表面的颜色）。最后，shininess影响镜面高光的散射/半径。
 *
 * 物体过亮的原因是环境光、漫反射和镜面光这三个颜色对任何一个光源都全力反射。
 * 光源对环境光、漫反射和镜面光分量也分别具有不同的强度。前面的章节中，我们通过使用
 * 一个强度值改变环境光和镜面光强度的方式解决了这个问题。我们想做类似的事情，
 * 但是这次是要为每个光照分量分别指定一个强度向量。
 */

/**REVIEW - 光照贴图
 * 现实世界中的物体通常并不只包含有一种材质，而是由多种材质所组成。想想一辆汽车：
 * 它的外壳非常有光泽，车窗会部分反射周围的环境，轮胎不会那么有光泽，所以它没有镜面高光，
 * 轮毂非常闪亮（如果你洗车了的话）。汽车同样会有漫反射和环境光颜色，它们在整个物体上
 * 也不会是一样的，汽车有着许多种不同的环境光/漫反射颜色。
 * 总之，这样的物体在不同的部件上都有不同的材质属性。
 *
 * 引入漫反射和镜面光贴图(Map)。这允许我们对物体的漫反射分量
 * （以及间接地对环境光分量，它们几乎总是一样的）和镜面光分量有着更精确的控制。
 *
 * NOTE - 漫反射贴图
 * 在光照场景中，它通常叫做一个漫反射贴图(Diffuse
 * Map)（3D艺术家通常都这么叫它），
 * 它是一个表现了物体所有的漫反射颜色的纹理图像。
 *
 * 注意sampler2D是所谓的不透明类型(Opaque Type)，也就是说我们不能将它实例化，
 * 只能通过uniform来定义它。如果我们使用除uniform以外的方法（比如函数的参数）
 * 实例化这个结构体，GLSL会抛出一些奇怪的错误。这同样也适用于任何封装了不透明类型的结构体。
 *
 * NOTE - 镜面光贴图
 * 你可能会注意到，镜面高光看起来有些奇怪，因为我们的物体大部分都是木头，我们知道木头
 * 不应该有这么强的镜面高光的。我们可以将物体的镜面光材质设置为vec3(0.0)来解决这个问题，
 * 但这也意味着箱子钢制的边框将不再能够显示镜面高光了，我们知道钢铁应该是有一些镜面高光的。
 * 所以，我们想要让物体的某些部分以不同的强度显示镜面高光。
 * 这个问题看起来和漫反射贴图非常相似。是巧合吗？我想不是。
 */
