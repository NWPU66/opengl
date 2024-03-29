#include <glad/glad.h>
// GLAD first
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util/util.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define MAX_NUM_LIGHTS_SPUUORT 16
using namespace glm;

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
Material material(vec3(0.05f, 0.1f, 0.2f), vec3(1.0f, 0.4f, 0.3f));
// lightGroup中的第一盏灯是绑定在摄像机上的手电筒
int n_light = 5;

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

vec3 cubePositions[] = {
    glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
    glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
    glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

Light lightGroup[] = {Light(2, vec3(0.0f), 1.0f, vec3(0.0f, 0.0f, 1.0f)),
                      Light(1, vec3(0.7f, 0.2f, 2.0f), 0.25f),
                      Light(1, vec3(2.3f, -3.3f, -4.0f), 0.25f),
                      Light(1, vec3(-4.0f, 2.0f, -12.0f), 0.25f),
                      Light(1, vec3(0.0f, 0.0f, -3.0f), 0.25f)};

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
     */
    Shader* objShader   = new Shader("./shader/lightCasting_obj.vs.glsl",
                                     "./shader/lightCasting_obj.fs.glsl");
    Shader* lightShader = new Shader("./shader/lightCasting_light.vs.glsl",
                                     "./shader/lightCasting_light.fs.glsl");

    /**NOTE - 创建纹理对象
     */
    objShader->use();  // 启动物体着色器
    GLuint boxDiffTex = createImageObjrct("container2.png");
    GLuint boxSpecTex = createImageObjrct("container2_specular.png");
    GLuint boxNormTex = createImageObjrct("container2_normal.png");
    // 设置纹理槽
    objShader->setParameter("material.diffuseMap", 0);
    objShader->setParameter("material.specularMap", 1);
    objShader->setParameter("material.normalMap", 2);
    // 激活纹理单元
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, boxDiffTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, boxSpecTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, boxNormTex);

    /**NOTE - 向Shader写入静态光的数据
     */
    int idx = 0;
    objShader->use();
    while (idx < MAX_NUM_LIGHTS_SPUUORT)
    {
        string shaderVarName = "light[" + to_string(idx) + "]";
        if (idx < n_light)
            objShader->setParameter(shaderVarName, lightGroup[idx]);
        else  // 向shader填充无效的灯光
            objShader->setParameter(shaderVarName, Light(-1, vec3(0.0f), 0.0f));
        idx++;
    }

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
        mat4 view = camera->GetViewMatrix();
        mat4 projection =
            perspective(radians(camera->Zoom), cameraAspect, 0.1f, 100.0f);

        /**NOTE - 将聚光灯绑定在摄像机上
         */
        objShader->use();
        lightGroup[0].position = camera->Position;
        lightGroup[0].lightDir = camera->Front;
        objShader->setParameter("light[0]", lightGroup[0]);

        /**NOTE - 绘制立方体
         */
        objShader->use();
        objShader->setParameter("cameraPos", camera->Position);
        objShader->setParameter("material", material);
        objShader->setParameter("view", view);
        objShader->setParameter("projection", projection);
        glBindVertexArray(objVAO);
        for (int i = 0; i < 10; i++)
        {
            mat4 model_obj = translate(mat4(1.0f), cubePositions[i]);
            objShader->setParameter("model", model_obj);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        /**NOTE - 绘制灯光
         */
        lightShader->use();
        // 第0盏灯是摄像机上的聚光灯，无需渲染实体
        for (int i = 1; i < n_light; i++)
        {
            mat4 model_light = translate(mat4(1.0f), lightGroup[i].position);
            model_light      = scale(model_light, vec3(0.1f));
            lightShader->setParameter("model", model_light);
            lightShader->setParameter("view", view);
            lightShader->setParameter("projection", projection);
            glBindVertexArray(lightVAO);
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

/**REVIEW - 投光物
 * 我们目前使用的光照都来自于空间中的一个点。它能给我们不错的效果，但现实世界中，
 * 我们有很多种类的光照，每种的表现都不同。将光投射(Cast)到物体的光源叫做投光物(Light
 * Caster)。
 *
 * NOTE - 平行光
 * 当一个光源处于很远的地方时，来自光源的每条光线就会近似于互相平行。不论物体和/或者
 * 观察者的位置，看起来好像所有的光都来自于同一个方向。当我们使用一个假设光源处于无限远处
 * 的模型时，它就被称为定向光，因为它的所有光线都有着相同的方向，它与光源的位置是没有关系的。
 *
 * 定向光非常好的一个例子就是太阳。太阳距离我们并不是无限远，但它已经远到在光照计算中
 * 可以把它视为无限远了。所以来自太阳的所有光线将被模拟为平行光线
 *
 * 因为所有的光线都是平行的，所以物体与光源的相对位置是不重要的，因为对场景中每一个物体光
 * 的方向都是一致的。由于光的位置向量保持一致，场景中每个物体的光照计算将会是类似的。
 *
 * 我们可以定义一个光线方向向量而不是位置向量来模拟一个定向光。着色器的计算基本保持不变，
 * 但这次我们将直接使用光的direction向量而不是通过position来计算lightDir向量。
 *
 * 我们一直将光的位置和位置向量定义为vec3，但一些人会喜欢将所有的向量都定义为vec4。
 * 当我们将位置向量定义为一个vec4时，很重要的一点是要将w分量设置为1.0，
 * 这样变换和投影才能正确应用。然而，当我们定义一个方向向量为vec4的时候，
 * 我们不想让位移有任何的效果（因为它仅仅代表的是方向），所以我们将w分量设置为0.0。
 *
 * 方向向量就会像这样来表示：vec4(0.2f, 1.0f, 0.3f,
 0.0f)。这也可以作为一个快速检测光照类型的工具：
 * 你可以检测w分量是否等于1.0，来检测它是否是光的位置向量；w分量等于0.0，
 * 则它是光的方向向量，这样就能根据这个来调整光照计算了：
 *
 * NOTE - 点光源
 * 点光源是处于世界中某一个位置的光源，它会朝着所有方向发光，但光线会随着距离逐渐衰减。
 * 想象作为投光物的灯泡和火把，它们都是点光源。
 *
 * 我们在给定位置有一个光源，它会从它的光源位置开始朝着所有方向散射光线。
 * 然而，我们定义的光源模拟的是永远不会衰减的光线，这看起来像是光源亮度非常的强。
 * 在大部分的3D模拟中，我们都希望模拟的光源仅照亮光源附近的区域而不是整个场景。
 *
 * NOTE - 衰减
 * 随着光线传播距离的增长逐渐削减光的强度通常叫做衰减(Attenuation)。
 * 随距离减少光强度的一种方式是使用一个线性方程。这样的方程能够随着距离的增长线性地减
 * 少光的强度，从而让远处的物体更暗。然而，这样的线性方程通常会看起来比较假。
 * 在现实世界中，灯在近处通常会非常亮，但随着距离的增加光源的亮度一开始会下降非常快，
 * 但在远处时剩余的光强度就会下降的非常缓慢了。
 *
 * (随着距离平方的反比下降)
 *
 * 在这里d代表了片段距光源的距离。接下来为了计算衰减值，
 * 我们定义3个（可配置的）项：常数项Kc、一次项Kl和二次项Kq。
 *
 * 常数项通常保持为1.0，它的主要作用是保证分母永远不会比1小，否则的话在某些距离上它反而会增加强度，这肯定不是我们想要的效果。
 * 一次项会与距离值相乘，以线性的方式减少强度。
 * 二次项会与距离的平方相乘，让光源以二次递减的方式减少强度。二次项在距离比较小的时候影响会比一次项小很多，但当距离值比较大的时候它就会比一次项更大了。
 *
 * ![](https://learnopengl-cn.github.io/img/02/05/attenuation.png)
 *
 * 我们可以将环境光分量保持不变，让环境光照不会随着距离减少，
 * 但是如果我们使用多于一个的光源，所有的环境光分量将会开始叠加，
 * 所以在这种情况下我们也希望衰减环境光照。
 *
 * NOTE - 聚光
 * 聚光是位于环境中某个位置的光源，它只朝一个特定方向而不是所有方向照射光线。
 * 这样的结果就是只有在聚光方向的特定半径内的物体才会被照亮，其它的物体都会保持黑暗。
 * 聚光很好的例子就是路灯或手电筒。
 *
 * OpenGL中聚光是用一个世界空间位置、一个方向和一个切光角(Cutoff
 Angle)来表示的，
 * 切光角指定了聚光的半径（译注：是圆锥的半径不是距光源距离那个半径）。对于每个片段，
 * 我们会计算片段是否位于聚光的切光方向之间（也就是在锥形内），
 * 如果是的话，我们就会相应地照亮片段。
 *
 * ![](https://learnopengl-cn.github.io/img/02/05/light_casters_spotlight_angles.png)
 *
 * 所以我们要做的就是计算LightDir向量和SpotDir向量之间的点积
 * （还记得它会返回两个单位向量夹角的余弦值吗？），并将它与切光角ϕ值对比。
 *
 * NOTE - 手电筒 Flashlight
 * 手电筒(Flashlight)是一个位于观察者位置的聚光，通常它都会瞄准玩家视角的正前方。
 * 基本上说，手电筒就是普通的聚光，但它的位置和方向会随着玩家的位置和朝向不断更新。
 *
 * 你可以看到，我们并没有给切光角设置一个角度值，反而是用角度值计算了一个余弦值，
 * 将余弦结果传递到片段着色器中。这样做的原因是在片段着色器中，我们会计算LightDir
 * 和SpotDir向量的点积，这个点积返回的将是一个余弦值而不是角度值，所以我们不能
 * 直接使用角度值和余弦值进行比较。为了获取角度值我们需要计算点积结果的反余弦，
 * 这是一个开销很大的计算。所以为了节约一点性能开销，我们将会计算切光角对应的余弦值，
 * 并将它的结果传入片段着色器中。由于这两个角度现在都由余弦角来表示了，
 * 我们可以直接对它们进行比较而不用进行任何开销高昂的计算。
 */

/**REVIEW - 多光源
 * 为了在场景中使用多个光源，我们希望将光照计算封装到GLSL函数中。
 * 这样做的原因是，每一种光源都需要一种不同的计算方法，而一旦我们想对多个光源进行光照计算时
 * ，代码很快就会变得非常复杂。如果我们只在main函数中进行所有的这些计算，
 * 代码很快就会变得难以理解。
 *
 * GLSL中的函数和C函数很相似，它有一个函数名、一个返回值类型，
 * 如果函数不是在main函数之前声明的，我们还必须在代码文件顶部声明一个原型。
 * 我们对每个光照类型都创建一个不同的函数：定向光、点光源和聚光。
 *
 * 当我们在场景中使用多个光源时，通常使用以下方法：我们需要有一个单独的颜色向量代表片段
 * 的输出颜色。对于每一个光源，它对片段的贡献颜色将会加到片段的输出颜色向量上。
 * 所以场景中的每个光源都会计算它们各自对片段的影响，并结合为一个最终的输出颜色。
 */
