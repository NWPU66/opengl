#include <glad/glad.h>
// GLAD first
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util/util.hpp"
#define MAX_NUM_LIGHTS_SPUUORT 16

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

/**NOTE - 高级渲染设置
 */
const Material material = Material();

/**NOTE - 几何体元数据
 */
// lightGroup中的第一盏灯是绑定在摄像机上的手电筒
vector<Light> lightGroup = {Light(2, vec3(0.0f), 0.0f, vec3(0.0f, 0.0f, 1.0f)),
                            Light(0, vec3(0.0f, 0.0f, 0.0f), 1.0f, vec3(-1.0f)),
                            Light(1, vec3(2.3f, -3.3f, -4.0f), 0.0f),
                            Light(1, vec3(0.0f, 0.0f, -3.0f), 0.0f)};

void framebuffer_size_callback(GLFWwindow* window, int w, int h);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
int  initGLFWWindow(GLFWwindow*& window);

int main(int argc, char** argv)
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (!initGLFWWindow(window)) return -1;

    // SECTION - 准备数据
    Model* model = new Model("./xinhui/xinhui.pmx");
    /**NOTE - 创建着色器
     */
    Shader* objShader =
        new Shader("./shader/mesh_obj.vs.glsl", "./shader/xinhui.fs.glsl");
    Shader* lightShader = new Shader("./shader/mesh_light.vs.glsl",
                                     "./shader/mesh_light.fs.glsl");

    /**NOTE - 向Shader写入静态光的数据
     */
    int idx = 0;
    objShader->use();
    while (idx < MAX_NUM_LIGHTS_SPUUORT)
    {
        string shaderVarName = "light[" + to_string(idx) + "]";
        if (idx < lightGroup.size())
            objShader->setParameter(shaderVarName, lightGroup[idx]);
        else  // 向shader填充无效的灯光
            objShader->setParameter(shaderVarName, Light(-1, vec3(0.0f), 0.0f));
        idx++;
    }

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);               // 启用深度缓冲
    glEnable(GL_STENCIL_TEST);             // 启用模板缓冲
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // 设置清空颜色
    // glStencilMask(0xFF); // 每一位写入模板缓冲时都保持原样
    // glStencilMask(0x00); // 每一位在写入模板缓冲时都会变成0（禁用写入）

    glStencilFunc(GL_EQUAL, 1, 0xFF);
    /**REVIEW - 模板函数
     * func：设置模板测试函数(Stencil Test Function)。
     * 这个测试函数将会应用到已储存的模板值上和glStencilFunc函数的ref值上。
     * ref：设置了模板测试的参考值(Reference
     * Value)。模板缓冲的内容将会与这个值进行比较。
     * mask：设置一个掩码，它将会与参考值和储存的模板值在测试比较它们之前进行与(AND)运算。初始情况下所有位都为1。
     */
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    /**REVIEW -
     * sfail：模板测试失败时采取的行为。
     * dpfail：模板测试通过，但深度测试失败时采取的行为。
     * dppass：模板测试和深度测试都通过时采取的行为。
     */
    //~SECTION

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 更新时钟、摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window);

        // SECTION - 渲染循环：绘制灯光和模型
        /**NOTE - 清空屏幕
         */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);  // 清除颜色、深度和模板缓冲

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

        /**NOTE - 绘制模型
         */
        objShader->use();
        objShader->setParameter("view", view);
        objShader->setParameter("projection", projection);
        objShader->setParameter("model", scale(mat4(1.f), vec3(0.5f)));
        objShader->setParameter("cameraPos", camera->Position);
        model->Draw(objShader);

        /**NOTE - 绘制轮廓
         */
        glStencilMask(0x00);  // 禁用写入模板值
        // 绘制不需要轮廓的物体
        // 绘制需要轮廓的物体
        glStencilMask(0xFF);                // 启用写入模板值
        glStencilFunc(GL_ALWAYS, 1, 0xFF);  // 总是写入模板值
        // 绘制需要轮廓的几何体
        // 绘制轮廓
        glStencilMask(0x00);                  // 禁用写入模板值
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);  // 模板值不等于1时通过
        glDisable(GL_DEPTH_TEST);             // 禁用深度测试
        // 用纯色Shader绘制一个稍微大一些的几何体
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glEnable(GL_DEPTH_TEST);  // 恢复模板测试和深度测试

        /**NOTE - 绘制灯光
         */
        // lightShader->use();
        // // 第0盏灯是摄像机上的聚光灯，无需渲染实体
        // for (int i = 1; i < lightGroup.size(); i++)
        // {
        //     mat4 model_light = translate(mat4(1.0f), lightGroup[i].position);
        //     model_light      = scale(model_light, vec3(0.1f));
        //     lightShader->setParameter("model", model_light);
        //     lightShader->setParameter("view", view);
        //     lightShader->setParameter("projection", projection);
        //     glBindVertexArray(lightVAO);
        //     glDrawArrays(GL_TRIANGLES, 0, 36);
        // }
        //~SECTION

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    /**NOTE - 删除OpenGL对象并释放内存
     */
    glfwTerminate();
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

/**REVIEW - 模板测试
 * 当片段着色器处理完一个片段之后，模板测试(Stencil
 * Test)会开始执行，和深度测试一样，
 * 它也可能会丢弃片段。接下来，被保留的片段会进入深度测试，它可能会丢弃更多的片段。
 * 模板测试是根据又一个缓冲来进行的，它叫做模板缓冲(Stencil Buffer)，
 * 我们可以在渲染的时候更新它来获得一些很有意思的效果。
 *
 * 一个模板缓冲中，（通常）每个模板值(Stencil Value)是8位的。
 * 所以每个像素/片段一共能有256种不同的模板值。我们可以将这些模板值设置为我们想要的值，
 * 然后当某一个片段有某一个模板值的时候，我们就可以选择丢弃或是保留这个片段了。
 * (每个窗口库都需要为你配置一个模板缓冲。GLFW自动做了这件事，
 * 所以我们不需要告诉GLFW来创建一个，但其它的窗口库可能不会默认给你创建一个模板库，
 * 所以记得要查看库的文档。)
 *
 * ![](https://learnopengl-cn.github.io/img/04/02/stencil_buffer.png)
 * 模板缓冲首先会被清除为0，之后在模板缓冲中使用1填充了一个空心矩形。
 * 场景中的片段将会只在片段的模板值为1的时候会被渲染（其它的都被丢弃了）。
 *
 * 模板缓冲操作允许我们在渲染片段时将模板缓冲设定为一个特定的值。
 * 通过在渲染时修改模板缓冲的内容，我们写入了模板缓冲。在同一个（或者接下来的）
 * 渲染迭代中，我们可以读取这些值，来决定丢弃还是保留某个片段。
 * 使用模板缓冲的时候你可以尽情发挥，但大体的步骤如下：
 * 1. 启用模板缓冲的写入。
 * 2. 渲染物体，更新模板缓冲的内容。
 * 3. 禁用模板缓冲的写入。
 * 4. 渲染（其它）物体，这次根据模板缓冲的内容丢弃特定的片段。
 *
 * 所以，通过使用模板缓冲，我们可以根据场景中已绘制的其它物体的片段，来决定是否丢弃特定的片段。
 *
 * 和深度测试的glDepthMask函数一样，模板缓冲也有一个类似的函数。
 * glStencilMask允许我们设置一个位掩码(Bitmask)，它会与将要写入缓冲的模板值进行
 * 与(AND)运算。默认情况下设置的位掩码所有位都为1，不影响输出，
 * 但如果我们将它设置为0x00，写入缓冲的所有模板值最后都会变成0.
 * 这与深度测试中的glDepthMask(GL_FALSE)是等价的。
 *
 * 大部分情况下你都只会使用0x00或者0xFF作为模板掩码(Stencil Mask)，
 * 但是知道有选项可以设置自定义的位掩码总是好的。
 *
 * NOTE - 模板函数
 * 和深度测试一样，我们对模板缓冲应该通过还是失败，以及它应该如何影响模板缓冲，
 * 也是有一定控制的。一共有两个函数能够用来配置模板测试：glStencilFunc和glStencilOp。
 *
 * 和深度测试一样，我们对模板缓冲应该通过还是失败，以及它应该如何影响模板缓冲，也是有一定控制的。
 * 一共有两个函数能够用来配置模板测试：glStencilFunc和glStencilOp。
 *
 * glStencilFunc(GL_EQUAL, 1, 0xFF)
 * 这会告诉OpenGL，只要一个片段的模板值等于(GL_EQUAL)参考值1，
 * 片段将会通过测试并被绘制，否则会被丢弃。
 * （例如模板输入值1100，参考值0110，mask=0101
 * 两者取AND运算之后都是0100，那么在GL_EQUAL的情况下可以通过模板测试）
 *
 * 但是glStencilFunc仅仅描述了OpenGL应该对模板缓冲内容做什么，而不是我们应该如何更新缓冲。
 * 这就需要glStencilOp这个函数了。
 *
 * 默认情况下glStencilOp是设置为(GL_KEEP, GL_KEEP,
 * GL_KEEP)的，所以不论任何测试的结果
 * 是如何，模板缓冲都会保留它的值。默认的行为不会更新模板缓冲，所以如果你想写入模板缓冲
 * 的话，你需要至少对其中一个选项设置不同的值。
 *
 * 所以，通过使用glStencilFunc和glStencilOp，
 * 我们可以精确地指定更新模板缓冲的时机与行为了，
 * 我们也可以指定什么时候该让模板缓冲通过，即什么时候片段需要被丢弃。
 *
 * NOTE - 物体轮廓
 * 物体轮廓所能做的事情正如它名字所描述的那样。我们将会为每个（或者一个）
 * 物体在它的周围创建一个很小的有色边框。当你想要在策略游戏中选中一个单位进行操作的，
 * 想要告诉玩家选中的是哪个单位的时候，这个效果就非常有用了。为物体创建轮廓的步骤如下：
 * 1. 在绘制（需要添加轮廓的）物体之前，将模板函数设置为GL_ALWAYS，
 * 每当物体的片段被渲染时，将模板缓冲更新为1。
 * 2. 渲染物体。
 * 3. 禁用模板写入以及深度测试。
 * 4. 将每个物体缩放一点点。
 * 5. 使用一个不同的片段着色器，输出一个单独的（边框）颜色。
 * 6. 再次绘制物体，但只在它们片段的模板值不等于1时才绘制。
 * 7. 再次启用模板写入和深度测试。
 *
 * 这个过程将每个物体的片段的模板缓冲设置为1，当我们想要绘制边框的时候，
 * 我们主要绘制放大版本的物体中模板测试通过的部分，也就是物体的边框的位置。
 * 我们主要使用模板缓冲丢弃了放大版本中属于原物体片段的部分。
 *
 * (glStencilMask()和glStencilFunc()里的mask好像不太一样
 * glStencilMask()是写入时的遮罩，
 * 而glStencilFunc()里的mask是测试时的遮罩）
 * 
 * 你看到的物体轮廓算法在需要显示选中物体的游戏（想想策略游戏）中非常常见。
 * 这样的算法能够在一个模型类中轻松实现。你可以在模型类中设置一个boolean标记，
 * 来设置需不需要绘制边框。如果你有创造力的话，你也可以使用后期处理滤镜(Filter)，
 * 像是高斯模糊(Gaussian Blur)，让边框看起来更自然。
 * 
 * 除了物体轮廓之外，模板测试还有很多用途，比如在一个后视镜中绘制纹理，
 * 让它能够绘制到镜子形状中，或者使用一个叫做阴影体积(Shadow Volume)
 * 的模板缓冲技术渲染实时阴影。
 * 模板缓冲为我们已经很丰富的OpenGL工具箱又提供了一个很好的工具。
 */
