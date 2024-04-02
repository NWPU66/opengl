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
        glEnable(GL_DEPTH_TEST);               // 启用深度缓冲
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // 设置清空颜色
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

        /**NOTE - 绘制模型
         */
        objShader->use();
        objShader->setParameter("view", view);
        objShader->setParameter("projection", projection);
        objShader->setParameter("model", scale(mat4(1.f), vec3(0.5f)));
        objShader->setParameter("cameraPos", camera->Position);
        model->Draw(objShader);

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

/**REVIEW - 网格
 * 通过使用Assimp，我们可以加载不同的模型到程序中，但是载入后它们都被储存为Assimp的数据结构。
 * 我们最终仍要将这些数据转换为OpenGL能够理解的格式，这样才能渲染这个物体。
 * 我们从上一节中学到，网格(Mesh)代表的是单个的可绘制实体，
 * 我们现在先来定义一个我们自己的网格类。
 *
 * 一个网格应该至少需要一系列的顶点，每个顶点包含一个位置向量、一个法向量和一个纹理坐标向量。
 * 一个网格还应该包含用于索引绘制的索引以及纹理形式的材质数据（漫反射/镜面光贴图）。
 *
 * 你可以看到这个类并不复杂。在构造器中，我们将所有必须的数据赋予了网格，
 * 我们在setupMesh函数中初始化缓冲，并最终使用Draw函数来绘制网格。
 * 注意我们将一个着色器传入了Draw函数中，将着色器传入网格类中可以让我们在
 * 绘制之前设置一些uniform（像是链接采样器到纹理单元）。
 *
 * C++结构体有一个很棒的特性，它们的内存布局是连续的(Sequential)。
 * 也就是说，如果我们将结构体作为一个数据数组使用，那么它将会以顺序排列结构体的变量，
 * 这将会直接转换为我们在数组缓冲中所需要的float（实际上是字节）数组。
 *
 * 由于有了这个有用的特性，我们能够直接传入一大列的Vertex结构体的指针作为缓冲的数据，
 * 它们将会完美地转换为glBufferData所能用的参数
 * 结构体的另外一个很好的用途是它的预处理指令offsetof(s,
 * m)，它的第一个参数是一个结构体，
 * 第二个参数是这个结构体中变量的名字。这个宏会返回那个变量距结构体头部的字节偏移量
 * (Byte Offset)。这正好可以用在定义glVertexAttribPointer函数中的偏移参数
 *
 * NOTE - 渲染
 * 们需要为Mesh类定义最后一个函数，它的Draw函数。在真正渲染这个网格之前，
 * 我们需要在调用glDrawElements函数之前先绑定相应的纹理。然而，这实际上有些困难，
 * 我们一开始并不知道这个网格（如果有的话）有多少纹理、纹理是什么类型的。
 * 所以我们该如何在着色器中设置纹理单元和采样器呢？
 *
 * 为了解决这个问题，我们需要设定一个命名标准：每个漫反射纹理被命名为texture_diffuseN，
 * 每个镜面光纹理应该被命名为texture_specularN，其中N的范围是1到纹理采样器最大允许的数字。
 *
 * 根据这个标准，我们可以在着色器中定义任意需要数量的纹理采样器，如果一个网格真的包含了
 * （这么多）纹理，我们也能知道它们的名字是什么。根据这个标准，我们也能在一个网格中处理
 * 任意数量的纹理，开发者也可以自由选择需要使用的数量，他只需要定义正确的采样器就可以了（
 * 虽然定义少的话会有点浪费绑定和uniform调用）。
 */

/**REVIEW - 模型
 * 现在是时候接触Assimp并创建实际的加载和转换代码了。这个教程的目标是创建另一个类来
 * 完整地表示一个模型，或者说是包含多个网格，甚至是多个物体的模型。
 * 一个包含木制阳台、塔楼、甚至游泳池的房子可能仍会被加载为一个模型。
 * 我们会使用Assimp来加载模型，并将它转换(Translate)至多个在上一节中创建的Mesh对象。
 *
 * NOTE - 导入3D模型到OpenGL
 * 首先需要调用的函数是loadModel，它会从构造器中直接调用。在loadModel中，
 * 我们使用Assimp来加载模型至Assimp的一个叫做scene的数据结构中。
 * 你可能还记得在模型加载章节的第一节教程中，这是Assimp数据接口的根对象。
 * 一旦我们有了这个场景对象，我们就能访问到加载后的模型中所有所需的数据了。
 *
 * Assimp允许我们设定一些选项来强制它对导入的数据做一些额外的计算或操作。
 *
 * aiProcess_Triangulate：如果模型不是全部由三角形组成，要将模型变换为三角形。
 * aiProcess_FlipUVs：将在处理的时候翻转y轴的纹理坐标（在GL中大部分的图像的y轴都是反的）
 * aiProcess_GenNormals：如果模型不包含法向量的话，就为每个顶点创建法线。
 * aiProcess_SplitLargeMeshes：将比较大的网格分割成更小的子网格
 * aiProcess_OptimizeMeshes：将多个小网格拼接为一个大的网格，减少绘制调用从而进行优化。
 * [](https://assimp.sourceforge.net/lib_html/postprocess_8h.html)
 *
 * 在我们加载了模型之后，我们会检查场景和其根节点不为null，并且检查了它的一个标记(Flag)，
 * 来查看返回的数据是不是不完整的。如果遇到了任何错误，
 * 我们都会通过导入器的GetErrorString函数来报告错误并返回。我们也获取了文件路径的目录路径。
 *
 * 如果什么错误都没有发生，我们希望处理场景中的所有节点，所以我们将第一个节点（根节点）
 * 传入了递归的processNode函数。因为每个节点（可能）包含有多个子节点，我们希望首先处理
 * 参数中的节点，再继续处理该节点所有的子节点，以此类推。
 *
 * Assimp的结构中，每个节点包含了一系列的网格索引，每个索引指向场景对象中的那个特定网格。
 * 我们接下来就想去获取这些网格索引，获取每个网格，处理每个网格，
 * 接着对每个节点的子节点重复这一过程。
 *
 * 纹理坐标的处理也大体相似，但Assimp允许一个模型在一个顶点上有最多8个不同的纹理坐标，
 * 我们不会用到那么多，我们只关心第一组纹理坐标。我们同样也想检查网格是否真的包含了纹理坐标
 *
 * NOTE - 索引
 * Assimp的接口定义了每个网格都有一个面(Face)数组，每个面代表了一个图元，
 * 在我们的例子中（由于使用了aiProcess_Triangulate选项）它总是三角形。
 * 一个面包含了多个索引，它们定义了在每个图元中，我们应该绘制哪个顶点，
 * 并以什么顺序绘制，所以如果我们遍历了所有的面，并储存了面的索引到indices
 * 这个vector中就可以了。
 *
 * NOTE - texture
 * 注意，我们假设了模型文件中纹理文件的路径是相对于模型文件的本地(Local)路径，
 * 比如说与模型文件处于同一目录下。我们可以将纹理位置字符串拼接到之前（在loadModel中）
 * 获取的目录字符串上，来获取完整的纹理路径（这也是为什么GetTexture函数也需要一个目录字符串）。
 *
 * 在网络上找到的某些模型会对纹理位置使用绝对(Absolute)路径，这就不能在每台机器上都工作了。
 * 在这种情况下，你可能会需要手动修改这个文件，来让它对纹理使用本地路径（如果可能的话）。
 *
 * NOTE - 重大优化
 * 大多数场景都会在多个网格中重用部分纹理。还是想想一个房子，它的墙壁有着花岗岩的纹理。
 * 这个纹理也可以被应用到地板、天花板、楼梯、桌子，甚至是附近的一口井上。
 * 加载纹理并不是一个开销不大的操作，在我们当前的实现中，即便同样的纹理已经被加载过很多遍了，
 * 对每个网格仍会加载并生成一个新的纹理。这很快就会变成模型加载实现的性能瓶颈。
 *
 * 所以我们会对模型的代码进行调整，将所有加载过的纹理全局储存，每当我们想加载一个纹理的时候，
 * 首先去检查它有没有被加载过。如果有的话，我们会直接使用那个纹理，并跳过整个加载流程，
 * 来为我们省下很多处理能力。
 */
