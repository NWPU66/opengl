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
void   createFBO(GLuint&     fbo,
                 GLuint&     texAttachment,
                 GLuint&     rbo,
                 const char* hint = "null");
void   createObjFromHardcode(vector<GLfloat> vertices,
                             vector<GLuint>  vertexIdx = {},
                             GLuint&         vao,
                             GLuint&         vbo,
                             GLuint& ebo = 0);  // TODO - createObjFromHardcode

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

    /**NOTE - 多重采样缓冲
     */
    GLuint msFbo, msTexAttachment, msRbo, fbo, texAttchment, rbo;
    createFBO(msFbo, msTexAttachment, msRbo, "ms");
    createFBO(fbo, texAttchment, rbo);

    /**NOTE - 屏幕几何对象
     */
    vector<GLfloat> screenVertices = {
        // 位置               // 纹理坐标
        -1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  // 左上
        1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // 右上
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 左下
        1.0f,  -1.0f, 0.0f, 1.0f, 0.0f   // 右下
    };
    vector<GLuint> screenVerticesIdx = {
        0, 2, 1,  // 第一个三角形
        1, 2, 3   // 第二个三角形
    };
    Shader screenShader("./shader/screenShader.vs.glsl",
                        "./shader/screenShader.fs.glsl");
    // VBO
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, screenVertices.size() * sizeof(GLfloat),
                 screenVertices.data(), GL_STATIC_DRAW);
    // VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // EBO
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 screenVerticesIdx.size() * sizeof(GLuint),
                 screenVerticesIdx.data(), GL_STATIC_DRAW);
    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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

    glEnable(GL_MULTISAMPLE);  // 启用多重采样

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 更新时钟、摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window);

        // SECTION - 渲染循环
        glBindFramebuffer(GL_FRAMEBUFFER, msFbo);
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

        /**NOTE - 从msFbo多重采样
         */
        // 将多重采样缓冲还原到中介FBO上
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glBlitFramebuffer(0, 0, CAMERA_WIDTH, CAMERA_HEIGH, 0, 0, CAMERA_WIDTH,
                          CAMERA_HEIGH, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        // 绘制ScreenTexture
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);
        glBindVertexArray(vao);
        screenShader.use();
        glBindTexture(GL_TEXTURE_2D, texAttchment);
        screenShader.setParameter("screenTexture", 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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

    glfwWindowHint(GLFW_SAMPLES, 4);  // 多重采样缓冲

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

/// @brief 创建一个FBO（帧缓冲对象）
/// @param hint 如果hint为"ms"，则使用多重采样
void createFBO(GLuint&     fbo,
               GLuint&     texAttachment,
               GLuint&     rbo,
               const char* hint)
{
    bool useMutiSampled = (strcmp(hint, "ms") == 0);
    // FIXME - 使用strcmp()的时候，不可以出空指针

    // 创建一个帧缓冲
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    //  创建一个纹理附件
    glGenTextures(1, &texAttachment);
    if (useMutiSampled)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texAttachment);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB,
                                CAMERA_WIDTH, CAMERA_HEIGH, GL_TRUE);
        // 如果最后一个参数为GL_TRUE，图像将会对每个纹素使用相同的样本位置以及相同数量的子采样点个数。
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER,
                        GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D_MULTISAMPLE, texAttachment, 0);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, texAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CAMERA_WIDTH, CAMERA_HEIGH, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, texAttachment, 0);
    }

    // 创建一个多重采样渲染缓冲对象
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    if (useMutiSampled)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4,
                                         GL_DEPTH24_STENCIL8, CAMERA_WIDTH,
                                         CAMERA_HEIGH);
    }
    else
    {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                              CAMERA_WIDTH, CAMERA_HEIGH);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);
    // FIXME - GL_DEPTH_STENCIL_ATTACHMENT写错了，导致深度缓冲没有初始化成功。

    //  检查帧缓冲状态
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "Framebuffer is  complete!" << endl;
    }
    else
    {
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    }

    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // FIXME - 函数写错了，fbo没有解绑。导致默认的fbo为空。
    if (useMutiSampled) { glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0); }
    else { glBindTexture(GL_TEXTURE_2D, 0); }
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

/// @brief 从一组顶点的硬编码创建几何体
void createObjFromHardcode(vector<GLfloat> vertices,
                           vector<GLuint>  vertexIdx,
                           GLuint&         vao,
                           GLuint&         vbo,
                           GLuint&         ebo)
{
    if (vertexIdx.size() == 0 && ebo == 0)
    {
        cout << "No EBO used to create a simple object!" << endl;
    }

    // TODO -
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
 *
 * 光栅器是位于最终处理过的顶点之后到片段着色器之前所经过的所有的算法与过程的总和。
 * 光栅器会将一个图元的所有顶点作为输入，并将它转换为一系列的片段。
 * 顶点坐标理论上可以取任意值，但片段不行，因为它们受限于你窗口的分辨率。
 * 顶点坐标与片段之间几乎永远也不会有一对一的映射，所以光栅器必须以某种方式来决
 * 定每个顶点最终所在的片段/屏幕坐标。
 *
 * 这里我们可以看到一个屏幕像素的网格，每个像素的中心包含有一个采样点(Sample
 * Point)，
 * 它会被用来决定这个三角形是否遮盖了某个像素。图中红色的采样点被三角形所遮盖，
 * 在每一个遮住的像素处都会生成一个片段。虽然三角形边缘的一些部分也遮住了某些屏幕像素，
 * 但是这些像素的采样点并没有被三角形内部所遮盖，所以它们不会受到片段着色器的影响。
 *
 * 由于屏幕像素总量的限制，有些边缘的像素能够被渲染出来，而有些则不会。
 * 结果就是我们使用了不光滑的边缘来渲染图元，导致之前讨论到的锯齿边缘。
 *
 * 多重采样所做的正是将单一的采样点变为多个采样点（这也是它名称的由来）。
 * 我们不再使用像素中心的单一采样点，取而代之的是以特定图案排列的4个子采样点(Subsample)。
 * 我们将用这些子采样点来决定像素的遮盖度。当然，这也意味着颜色缓冲的大小会随着子
 * 采样点的增加而增加。
 * ![](https://learnopengl-cn.github.io/img/04/11/anti_aliasing_sample_points.png)
 *
 * 采样点的数量可以是任意的，更多的采样点能带来更精确的遮盖率。
 *
 * 从这里开始多重采样就变得有趣起来了。我们知道三角形只遮盖了2个子采样点，所以下一步是
 * 决定这个像素的颜色。你的猜想可能是，我们对每个被遮盖住的子采样点运行一次片段着色器，
 * 最后将每个像素所有子采样点的颜色平均一下。在这个例子中，我们需要在两个子采样点上对
 * 被插值的顶点数据运行两次片段着色器，并将结果的颜色储存在这些采样点中。（幸运的是）
 * 这并不是它工作的方式，因为这本质上说还是需要运行更多次的片段着色器，会显著地降低性能。
 *
 * MSAA真正的工作方式是，无论三角形遮盖了多少个子采样点，（每个图元中）每个像素只运行
 * 一次片段着色器。片段着色器所使用的顶点数据会插值到每个像素的中心，所得到的结果颜色会
 * 被储存在每个被遮盖住的子采样点中。当颜色缓冲的子样本被图元的所有颜色填满时，
 * 所有的这些颜色将会在每个像素内部平均化。因为上图的4个采样点中只有2个被遮盖住了，
 * 这个像素的颜色将会是三角形颜色与其他两个采样点的颜色（在这里是无色）的平均值，
 * 最终形成一种淡蓝色。
 *
 * 这样子做之后，颜色缓冲中所有的图元边缘将会产生一种更平滑的图形。
 * ![](https://learnopengl-cn.github.io/img/04/11/anti_aliasing_rasterization_samples.png)
 *
 * 这里，每个像素包含4个子采样点（不相关的采样点都没有标注），蓝色的采样点被三角形所遮盖，
 * 而灰色的则没有。对于三角形的内部的像素，片段着色器只会运行一次，颜色输出会被存储到
 * 全部的4个子样本中。而在三角形的边缘，并不是所有的子采样点都被遮盖，
 * 所以片段着色器的结果将只会储存到部分的子样本中。根据被遮盖的子样本的数量，
 * 最终的像素颜色将由三角形的颜色与其它子样本中所储存的颜色来决定。
 *
 * 简单来说，一个像素中如果有更多的采样点被三角形遮盖，
 * 那么这个像素的颜色就会更接近于三角形的颜色。如果我们给上面的三角形填充颜色
 * ![](https://learnopengl-cn.github.io/img/04/11/anti_aliasing_rasterization_samples_filled.png)
 *
 * 对于每个像素来说，越少的子采样点被三角形所覆盖，那么它受到三角形的影响就越小。
 * 三角形的不平滑边缘被稍浅的颜色所包围后，从远处观察时就会显得更加平滑了。
 *
 * 不仅仅是颜色值会受到多重采样的影响，深度和模板测试也能够使用多个采样点。
 * 对深度测试来说，每个顶点的深度值会在运行深度测试之前被插值到各个子样本中。
 * 对模板测试来说，我们对每个子样本，而不是每个像素，存储一个模板值。当然，
 * 这也意味着深度和模板缓冲的大小会乘以子采样点的个数。
 *
 * NOTE - OpenGL中的MSAA
 * 如果我们想要在OpenGL中使用MSAA，我们必须要使用一个能在每个像素中存储大于1个
 * 颜色值的颜色缓冲（因为多重采样需要我们为每个采样点都储存一个颜色）。所以，
 * 我们需要一个新的缓冲类型，来存储特定数量的多重采样样本，它叫做多重采样缓冲
 * (Multisample Buffer)。
 *
 * 大多数的窗口系统都应该提供了一个多重采样缓冲，用以代替默认的颜色缓冲。
 * GLFW同样给了我们这个功能，我们所要做的只是提示(Hint) GLFW，
 * 我们希望使用一个包含N个样本的多重采样缓冲。
 * 这可以在创建窗口之前调用glfwWindowHint来完成。
 *
 * 在大多数OpenGL的驱动上，多重采样都是默认启用的。
 * 只要默认的帧缓冲有了多重采样缓冲的附件，我们所要做的只是调用glEnable来启用多重采样。
 * 因为多重采样的算法都在OpenGL驱动的光栅器中实现了，我们不需要再多做什么。
 *
 * NOTE - 离屏MSAA
 * 由于GLFW负责了创建多重采样缓冲，启用MSAA非常简单。然而，如果我们想要使用
 * 我们自己的帧缓冲来进行离屏渲染，那么我们就必须要自己动手生成多重采样缓冲了。
 *
 * 有两种方式可以创建多重采样缓冲，将其作为帧缓冲的附件：
 * 纹理附件和渲染缓冲附件，这和在帧缓冲教程中所讨论的普通附件很相似。
 *
 * NOTE - 多重采样纹理附件
 * 为了创建一个支持储存多个采样点的纹理，我们使用glTexImage2DMultisample来替代
 * glTexImage2D，它的纹理目标是GL_TEXTURE_2D_MULTISAPLE。
 *
 * 因为多重采样缓冲有一点特别，我们不能直接将它们的缓冲图像用于其他运算，
 * 比如在着色器中对它们进行采样。
 *
 * 一个多重采样的图像包含比普通图像更多的信息，我们所要做的是缩小或者还原(Resolve)图像。
 * 多重采样帧缓冲的还原通常是通过glBlitFramebuffer来完成，它能够将一个帧缓冲中的某个区域
 * 复制到另一个帧缓冲中，并且将多重采样缓冲还原。
 *
 * glBlitFramebuffer会将一个用4个屏幕空间坐标所定义的源区域复制到一个同样用4个
 * 屏幕空间坐标所定义的目标区域中。你可能记得在帧缓冲教程中，当我们绑定到
 * GL_FRAMEBUFFER时，我们是同时绑定了读取和绘制的帧缓冲目标。我们也可以将
 * 帧缓冲分开绑定至GL_READ_FRAMEBUFFER与GL_DRAW_FRAMEBUFFER。
 * glBlitFramebuffer函数会根据这两个目标，决定哪个是源帧缓冲，哪个是目标帧缓冲。
 * 接下来，我们可以将图像位块传送(Blit)到默认的帧缓冲中，将多重采样的帧缓冲传送到屏幕上。
 *
 * NOTE - 自定义抗锯齿算法
 * 将一个多重采样的纹理图像不进行还原直接传入着色器也是可行的。GLSL提供了这样的选项，
 * 让我们能够对纹理图像的每个子样本进行采样，所以我们可以创建我们自己的抗锯齿算法。
 * 在大型的图形应用中通常都会这么做。
 */
