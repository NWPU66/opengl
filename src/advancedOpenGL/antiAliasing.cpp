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
mat4   makeRandomPosture(const float propotion,
                         const float radius = 50.0f,
                         const float offset = 2.5f);

int main(int argc, char** argv)
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (!initGLFWWindow(window)) return -1;

    /**NOTE - 模型和着色器、纹理
     */
    Model box("./box/box.obj"), planet("./planet/planet.obj"),
        rock("./rock/rock.obj");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl",
                        "./shader/skyboxShader.fs.glsl"),
        instanceBoxShader("./shader/instanceBoxShader.vs.glsl",
                          "./shader/instanceBoxShader.fs.glsl"),
        planetShader("./shader/planetShader.vs.glsl",
                     "./shader/generalFragShader.fs.glsl"),
        rockShader("./shader/rockShader.vs.glsl",
                   "./shader/generalFragShader.fs.glsl");
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

    /**NOTE - 实例化数组
     */
    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(offsetTranslate), offsetTranslate,
                 GL_STATIC_DRAW);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // 设置整个model对象下Mesh的实例数组
    box.SetInstanceArray(instanceVBO, 3, 2, 1);
    /**FIXME - updateFruq=0时出现几何体的撕扯现象
     * If divisor is zero, the attribute at slot index advances once per vertex.
     * divisor=0，每个顶点更新一次，这就不是实例的渲染了。
     *
     * If divisor is non-zero, the attribute advances once per divisor instances
     * of the set(s) of vertices being rendered.
     * 每‘divisor’个实例后更新一次。
     */

    /**NOTE - 设置陨石带的实例化数组
     */
    // 创建instanceModelMatrixs元数据
    rockShader.use();
    vector<mat4> instanceModelMatrixs;
    const GLuint rockAmount = 1000;
    const float  rockRadius = 10.0f, rockOffset = 1.0f;
    srand(glfwGetTime());
    for (GLuint i = 0; i < rockAmount; i++)
    {
        mat4 model = makeRandomPosture((float)i / (float)rockAmount, rockRadius,
                                       rockOffset);
        instanceModelMatrixs.push_back(model);
        /**FIXME -
         * (float)(i / rockAmount)永远是0，因为是整数除法
         * 应该使用浮点数除法：(float)i / (float)rockAmount
         */
    }
    // 创建VBO
    GLuint rockInstanceVBO;
    glGenBuffers(1, &rockInstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, rockInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mat4) * instanceModelMatrixs.size(),
                 instanceModelMatrixs.data(), GL_STATIC_DRAW);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // 设置整个model对象下Mesh的实例数组
    rock.SetInstanceArray(rockInstanceVBO, 3, 4, sizeof(mat4), (void*)0);
    rock.SetInstanceArray(rockInstanceVBO, 4, 4, sizeof(mat4),
                          (void*)(1 * sizeof(vec4)));
    rock.SetInstanceArray(rockInstanceVBO, 5, 4, sizeof(mat4),
                          (void*)(2 * sizeof(vec4)));
    rock.SetInstanceArray(rockInstanceVBO, 6, 4, sizeof(mat4),
                          (void*)(3 * sizeof(vec4)));
    // FIXME - 错题本：这里的偏移量是列向量vec4的大小，而不是mat4的大小。

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);  // 启用深度缓冲
    glDepthFunc(GL_LEQUAL);   // 修改深度测试的标准

    glEnable(GL_STENCIL_TEST);                     // 启用模板缓冲
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);  // 设置模板缓冲的操作

    glEnable(GL_BLEND);                                 // 启用混合
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数

    glEnable(GL_CULL_FACE);  // 启用面剔除

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

        /**NOTE - 绘制100个立方体实例
         */
        // instanceBoxShader.use();
        // instanceBoxShader.setParameter("view", view);
        // instanceBoxShader.setParameter("projection", projection);
        // box.Draw(&instanceBoxShader, 100);

        /**NOTE - 绘制行星
         */
        // 绘制行星
        planetShader.use();
        planetShader.setParameter("view", view);
        planetShader.setParameter("projection", projection);
        planetShader.setParameter(
            "model", translate(rotate(rotate(mat4(1.0f),
                                             radians((float)glfwGetTime() * 10),
                                             vec3(0.0f, 1.0f, 0.0f)),
                                      radians(90.0f), vec3(1.0f, 0.0f, 0.0f)),
                               vec3(0.0f, -1.0f, 0.0f)));
        planet.Draw(&planetShader);
        /**FIXME - 旋转的时候有点偏离原点
         * 小问题，修好了。。。
         */
        // 绘制行星的陨石带
        rockShader.use();
        rockShader.setParameter("view", view);
        rockShader.setParameter("projection", projection);
        rock.Draw(&rockShader, rockAmount);

        /**NOTE - 最后渲染天空盒
         */
        glFrontFace(GL_CW);  // 把顺时针的面设置为“正面”。
        skyboxShader.use();
        skyboxShader.setParameter("view",
                                  mat4(mat3(view)));  // 除去位移，相当于锁头
        skyboxShader.setParameter("projection", projection);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
        skyboxShader.setParameter("skybox", 0);
        box.Draw(&skyboxShader);
        glFrontFace(GL_CCW);
        /**FIXME - 错题本
         * 天空盒消失的原因：天空盒锁头，只能看到“背面”，然后我开了背面剔除。
         * 解决方案：渲染天空盒的时候，把顺时针的面设置为“正面”。
         * 或者开启“正面”剔除。
         */
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

/// @brief 函数“framebuffer_size_callback”根据窗口大小设置视口尺寸。
/// @param window `window` 参数是指向触发回调函数的 GLFW 窗口的指针。
/// @param w
/// “framebuffer_size_callback”函数中的“w”参数表示帧缓冲区的宽度，即OpenGL将渲染图形的区域的大小。
/// @param h
/// “framebuffer_size_callback”函数中的参数“h”表示帧缓冲区的高度（以像素为单位）。它用于设置在OpenGL
/// 上下文中渲染的视口高度。
void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, w, h);
}

/// @brief 函数“mouse_callback”根据 GLFW 窗口中的鼠标移动更新相机的方向。
/// @param window “window”参数是指向接收鼠标输入的
/// GLFW窗口的指针。它用于标识鼠标事件发生的窗口。
/// @param xpos mouse_callback 函数中的 xpos 参数表示鼠标光标位置的当前 x坐标。
/// @param ypos 'mouse_callback` 函数中的 `ypos`参数表示鼠标光标在窗口内的当前
/// y坐标。它是一个双精度值，表示触发回调函数时鼠标光标的垂直位置。
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    camera->ProcessMouseMovement(xpos - mouseLastX, ypos - mouseLastY, true);
    mouseLastX = xpos;
    mouseLastY = ypos;
}

/// @brief 函数“scroll_callback”处理鼠标滚动输入以调整相机位置。
/// @param window `GLFWwindow* window`参数是指向接收滚动输入的窗口的指针。
/// @param xoffset `xoffset` 参数表示水平滚动偏移。
/// @param yoffset
/// 'scroll_callback`函数中的`yoffset`参数表示鼠标滚轮的垂直滚动偏移量。正值表示向上滚动，负值表示向下滚动。
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera->ProcessMouseScroll(yoffset);
}

/// @brief C++ 中的函数“processInput”处理来自
/// GLFW窗口的用户输入，以控制相机的移动和速度。
/// @param window processInput 函数中的 window 参数是一个指向
/// GLFWwindow对象的指针。该对象表示应用程序中用于渲染图形和处理用户输入的窗口。该函数使用此参数来检查按键并更新相机的移动和速度
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

/// @brief 为陨石生成一个随机的姿态
/// @param propotion
/// @param radius 生成的目标半径
/// @param offset 位置的随机偏移量
/// @return 返回一个姿态矩阵mat4
mat4 makeRandomPosture(const float propotion,
                       const float radius,
                       const float offset)
{
    // TODO - 做陨石的旋转
    mat4 model(1.0f);
    // 1. 位移：分布在半径为 'radius' 的圆形上，偏移的范围是 [-offset, offset]
    float angle        = propotion * 360.0f;
    float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float x            = cos(angle) * radius + displacement;
    displacement       = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float y = displacement * 0.4f;  //// 让行星带的高度比x和z的宽度要小
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float z      = sin(angle) * radius + displacement;
    model        = translate(mat4(1.0f), vec3(x, y, z));

    // 2. 缩放：在 0.05 和 0.25f 之间缩放
    float scale = (rand() % 20) / 100.0f + 0.05;
    model       = glm::scale(model, vec3(scale * 0.5f));

    // 3. 旋转：绕着一个（半）随机选择的旋转轴向量进行随机的旋转
    float rotAngle = (rand() % 360);
    model          = rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

    return model;
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
 *
 * NOTE - 实例化数组
 * 虽然之前的实现在目前的情况下能够正常工作，但是如果我们要渲染远超过100个实例的时候
 * （这其实非常普遍），我们最终会超过最大能够发送至着色器的uniform数据大小上限。
 * 它的一个代替方案是实例化数组(Instanced Array)，它被定义为一个顶点属性
 * （能够让我们储存更多的数据），仅在顶点着色器渲染一个新的实例时才会更新。
 *
 * 使用顶点属性时，顶点着色器的每次运行都会让GLSL获取新一组适用于当前顶点的属性。
 * 而当我们将顶点属性定义为一个实例化数组时，顶点着色器就只需要对每个实例，
 * 而不是每个顶点，更新顶点属性的内容了。这允许我们对逐顶点的数据使用普通的顶点属性，
 * 而对逐实例的数据使用实例化数组。
 *
 * 这段代码很有意思的地方在于最后一行，我们调用了glVertexAttribDivisor。这个函数告诉了
 * OpenGL该什么时候更新顶点属性的内容至新一组数据。它的第一个参数是需要的顶点属性，
 * 第二个参数是属性除数(Attribute
 * Divisor)。默认情况下，属性除数是0，告诉OpenGL我们
 * 需要在顶点着色器的每次迭代时更新顶点属性。将它设置为1时，我们告诉OpenGL我们希望
 * 在渲染一个新实例的时候更新顶点属性。而设置为2时，我们希望每2个实例更新一次属性，
 * 以此类推。我们将属性除数设置为1，是在告诉OpenGL，处于位置值2的顶点属性是一个实例化数组。
 *
 * 虽然很有趣，但是这些例子并不是实例化的好例子。
 * 是的，它们的确让你知道实例化是怎么工作的，但是我们还没接触到它最有用的一点：
 * 绘制巨大数量的相似物体。出于这个原因，我们将会在下一部分进入太空探险，
 * 见识实例化渲染真正的威力。
 *
 * NOTE - 小行星带
 * 想象这样一个场景，在宇宙中有一个大的行星，它位于小行星带的中央。
 * 这样的小行星带可能包含成千上万的岩块，在很不错的显卡上也很难完成这样的渲染。
 * 实例化渲染正是适用于这样的场景，因为所有的小行星都可以使用一个模型来表示。
 * 每个小行星可以再使用不同的变换矩阵来进行少许的变化。
 *
 * 这里，我们绘制与之前相同数量amount的小行星，但是使用的是实例渲染。结果应该是非常相似的，
 * 但如果你开始增加amount变量，你就能看见实例化渲染的效果了。没有实例化渲染的时候，
 * 我们只能流畅渲染1000到1500个小行星。而使用了实例化渲染之后，我们可以将这个值设置为
 * 100000，每个岩石模型有576个顶点，每帧加起来大概要绘制5700万个顶点，但性能却没有
 * 受到任何影响！
 */

/**REVIEW - 抗锯齿
 * 在学习渲染的旅途中，你可能会时不时遇到模型边缘有锯齿的情况。
 * 这些锯齿边缘(Jagged Edges)的产生和光栅器将顶点数据转化为片段的方式有关。
 *
 * 可能不是非常明显，但如果你离近仔细观察立方体的边缘，
 * 你就应该能够看到锯齿状的图案。如果放大的话，你会看到下面的图案：
 * ![](https://learnopengl-cn.github.io/img/04/11/anti_aliasing_zoomed.png)
 *
 * 这很明显不是我们想要在最终程序中所实现的效果。你能够清楚看见形成边缘的像素。
 * 这种现象被称之为走样(Aliasing)。有很多种抗锯齿（Anti-aliasing，也被称为反走样）
 * 的技术能够帮助我们缓解这种现象，从而产生更平滑的边缘。
 *
 * 最开始我们有一种叫做超采样抗锯齿(Super Sample Anti-aliasing,
 * SSAA)的技术，它会使用
 * 比正常分辨率更高的分辨率（即超采样）来渲染场景，当图像输出在帧缓冲中更新时，分辨率
 * 会被下采样(Downsample)至正常的分辨率。这些额外的分辨率会被用来防止锯齿边缘的产生。
 * 虽然它确实能够解决走样的问题，但是由于这样比平时要绘制更多的片段，
 * 它也会带来很大的性能开销。所以这项技术只拥有了短暂的辉煌。
 *
 * 然而，在这项技术的基础上也诞生了更为现代的技术，叫做多重采样抗锯齿(Multisample
 *  Anti-aliasing,
 * MSAA)。它借鉴了SSAA背后的理念，但以更加高效的方式实现了抗锯齿。
 * 我们在这一节中会深度讨论OpenGL中内建的MSAA技术。
 *
 * NOTE - 多重采样
 * 为了理解什么是多重采样(Multisampling)，以及它是如何解决锯齿问题的，
 * 我们有必要更加深入地了解OpenGL光栅器的工作方式。
 */
