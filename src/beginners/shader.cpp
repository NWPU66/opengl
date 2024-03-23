#include <glad/glad.h>
// GLAD include first
#include <GLFW/glfw3.h>

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

#include "util/class_shader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

float vertices[] = {
    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
    0.8f,  0.8f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 右上
    0.8f,  -0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // 右下
    -0.8f, -0.8f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // 左下
    -0.8f, 0.8f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // 左上
};

GLuint idx[] = {0, 1, 3, 1, 2, 3};  // 绘制顺序

/// @brief 视窗回调函数
void framebuffer_size_callback(GLFWwindow *window, int w, int h) {
    glViewport(0, 0, w, h);
}

/// @brief 在每个周期处理来外部输入的事件和操作
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
}

/// @brief 创建一个GLFW窗口并初始化GLAD，成功返回1，不成功则返回0
int initGLFWWindow(GLFWwindow *&window) {
    /*
    错题本：GLFWwindow* &window

    这个GLFWwindow* window为什么错？

    进函数前有一个GLFWwindow型的指针ptr1，
    进函数后GLFWwindow* window会把ptr1复制一份赋值给ptr2
    在函数中我们修改指针ptr2指向被的地方
    当我们退出函数时，ptr1仍然没有变化

    所以GLFWwindow* &window这里要加引用，相对于ptr2是ptr1的别名
    当我们修改ptr2时，ptr1也被修改了。
    */

    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // 创建窗口
    window = glfwCreateWindow(800, 600, "Graph", NULL, NULL);
    if (window == NULL) {
        std::cout << "failed to create a window!" << std::endl;
        glfwTerminate();
        return 0;
    }
    glfwMakeContextCurrent(window);
    // 注册视窗回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "failed to load GLAD!" << std::endl;
        glfwTerminate();
        return 0;
    }

    return 1;
}

int createImageObjrct(const char *imagePath) {
    /*
    错题本："wall.jpg"是字符串常量，
    而char* imagePath声明imagePath所指向的字符串是变量
    所以要加上const，const char* imagePath申明imagePath的内容是常量
    但是imagePath指针本身是变量，可以改变
    */
    // 读取
    GLint width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);  // 加载图片时翻转y轴
    GLubyte *data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
    // 创建纹理对象
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    /*
    错题本：这里要用Texture系列的函数：glGenTextures() & glBindTexture()
    */
    //
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 生成纹理
    if (data) {
        if (nrChannels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data);
            /*
            glTexImage2D参数：
            纹理目标、MipMap级别、存储格式、纹理的宽度和高度、
            总为0、源图的格式和数据类型以及图像数据
            */
        } else {
            // nrChannels == 4
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
        return -1;
    }
    stbi_image_free(data);  // 释放图像的内存，无论有没有data都释放
    glBindTexture(GL_TEXTURE_2D, 0);  // 解绑

    return texture;
}

int main(int argc, char **argv) {
    GLFWwindow *window;  // create a GLFW window
    if (!initGLFWWindow(window)) return -1;

    // int nrAttributes;  // 查看顶点着色器上能声明的最大顶点属性数量
    // glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    // std::cout << "Max vertex attributes supported: " << nrAttributes
    //           << std::endl;

    // SECTION - 准备数据
    // 创建着色器程序---------------------------------------------------------
    Shader *shader = new Shader("shader.vs.glsl", "shader.fs.glsl");
    shader->use();  // 在设置任何uniform参数之前，记得启动着色器。

    // 读入并生成纹理---------------------------------------------------------
    // SECTION - 读入并生成纹理
    GLuint texture1 = createImageObjrct("wall.jpg");
    GLuint texture2 = createImageObjrct("awesomeface.png");

    // 设置纹理对应的采样器插槽号
    shader->setInt("myTexture1", 0);
    shader->setInt("myTexture2", 1);
    // 绑定时激活OpenGL的纹理槽
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);
    /*
    错题本：glActiveTexture()里面填写的是GL_TEXTURE0，而不是数字0！！！
    要使用Texture系列的函数
    */
    //~SECTION

    // 创建顶点数据-----------------------------------------------------------
    // SECTION - 创建顶点数据
    GLuint VBO, VAO, EBO;
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);
    // 绑定
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // 将顶点数组、绘制顺序送入VBO和EBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    // 设置顶点属性，描述VBO中数据的解读方式
    // VAO格式：[0] = 顶点位置；[1] = 顶点颜色；[2] = 顶点纹理坐标
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    //~SECTION
    //~SECTION

    // 渲染循环-----------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {
        // 处理输入
        processInput(window);

        // 渲染指令-----------------------------------------------------------
        // SECTION - 渲染指令
        // 清空屏幕
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader->use();  // 激活着色器程序

        // 在shader外部设置我们的outsideSetColor，让它随时间变化颜色
        float greenValue = (sin(glfwGetTime()) / 2.0f) + 0.5f;
        int vertexColorLocation =
            glGetUniformLocation(shader->ID, "outsideSetColor");
        // 注意，查询uniform地址不要求你之前使用过着色器程序，
        // 但是更新之前你必须先使用glUseProgram激活着色器程序中设置uniform。
        glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);

        // SECTION - 矩阵运算
        glm::mat4 trans(1.0f);  // 默认初始化为0矩阵
        trans = glm::scale(trans, glm::vec3(.5f, .5f, .5f));
        trans = glm::rotate(trans, (float)glfwGetTime(),
                            glm::vec3(0.0f, 0.0f, 1.0f));
        trans = glm::translate(trans, glm::vec3(0.0f, 1.0f, 0.0f));
        trans = glm::rotate(trans, 2.5f * (float)glfwGetTime(),
                            glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "transform"), 1,
                           GL_FALSE, glm::value_ptr(trans));
        //~SECTION

        // 绘制图形
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        //~SECTION
        //---------------------------------------------------------------------

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 删除OpenGL对象并释放内存
    glfwTerminate();
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shader->ID);
    delete shader;

    return 0;
}

/*
GLSL:
着色器是使用一种叫GLSL的类C语言写成的。
GLSL是为图形计算量身定制的，它包含一些针对向量和矩阵操作的有用特性。

着色器的开头总是要声明版本，接着是输入和输出变量、uniform和main函数。
每个着色器的入口点都是main函数，在这个函数中我们处理所有的输入变量，
并将结果输出到输出变量中。

当我们特别谈论到顶点着色器的时候，每个输入变量也叫顶点属性(Vertex Attribute)。
我们能声明的顶点属性是有上限的，它一般由硬件来决定。
OpenGL确保至少有16个包含4分量的顶点属性可用，
但是有些硬件或许允许更多的顶点属性
*/

/*
数据类型：
和其他编程语言一样，GLSL有数据类型可以来指定变量的种类。
GLSL中包含C等其它语言大部分的默认基础数据类型：int、float、double、uint和bool。
GLSL也有两种容器类型，分别是向量(Vector)和矩阵(Matrix)

向量：
GLSL中的向量是一个可以包含有2、3或者4个分量的容器，
分量的类型可以是前面默认基础类型的任意一个。
大多数时候我们使用vecn，因为float足够满足大多数要求了。

一个向量的分量可以通过vec.x这种方式获取，这里x是指这个向量的第一个分量。
你可以分别使用.x、.y、.z和.w来获取它们的第1、2、3、4个分量。
GLSL也允许你对颜色使用rgba，或是对纹理坐标使用stpq访问相同的分量。
*/

/*
输入与输出：
虽然着色器是各自独立的小程序，但是它们都是一个整体的一部分，
出于这样的原因，我们希望每个着色器都有输入和输出，这样才能进行数据交流和传递。
GLSL定义了in和out关键字专门来实现这个目的。
每个着色器使用这两个关键字设定输入和输出，
只要一个输出变量与下一个着色器阶段的输入匹配，它就会传递下去。
但在顶点和片段着色器中会有点不同。

顶点着色器应该接收的是一种特殊形式的输入，否则就会效率低下。
顶点着色器的输入特殊在，它从顶点数据中直接接收输入。
为了定义顶点数据该如何管理，我们使用location这一元数据指定输入变量，
这样我们才可以在CPU上配置顶点属性。
我们已经在前面的教程看过这个了，layout (location = 0)。
顶点着色器需要为它的输入提供一个额外的layout标识，这样我们才能把它链接到顶点数据。
*/

/*
Uniform：
Uniform是一种从CPU中的应用向GPU中的着色器发送数据的方式，但它和顶点属性有些不同。
首先，uniform是全局的(Global)。
全局意味着uniform变量必须在每个着色器程序对象中都是独一无二的，
而且它可以被着色器程序的任意着色器在任意阶段访问。
第二，无论你把uniform值设置成什么，
uniform会一直保存它们的数据，直到它们被重置或更新。

因为OpenGL在其核心是一个C库，所以它不支持类型重载，在函数参数不同的时候就要为其定义新的函数；glUniform是一个典型例子。
这个函数有一个特定的后缀，标识设定的uniform的类型。可能的后缀有：
*/

/*
更多属性：

*/

/*
纹理：
除了图像以外，纹理也可以被用来储存大量的数据，
这些数据可以发送到着色器上，但是这不是我们现在的主题。

为了能够把纹理映射(Map)到三角形上，我们需要指定三角形的每个顶点各自对应纹理的哪个部分。
这样每个顶点就会关联着一个纹理坐标(Texture Coordinate)，
用来标明该从纹理图像的哪个部分采样。
之后在图形的其它片段上进行片段插值(Fragment Interpolation)。

纹理坐标在x和y轴上，范围为0到1之间（注意我们使用的是2D纹理图像）。
使用纹理坐标获取纹理颜色叫做采样(Sampling)。
纹理坐标起始于(0, 0)，也就是纹理图片的左下角，终始于(1,
1)，即纹理图片的右上角。

我们为三角形指定了3个纹理坐标点。
如上图所示，我们希望三角形的左下角对应纹理的左下角，
因此我们把三角形左下角顶点的纹理坐标设置为(0, 0)；
三角形的上顶点对应于图片的上中位置所以我们把它的纹理坐标设置为(0.5, 1.0)；
同理右下方的顶点设置为(1, 0)。
我们只要给顶点着色器传递这三个纹理坐标就行了，接下来它们会被传片段着色器中，
它会为每个片段进行纹理坐标的插值。
*/

/*
纹理环绕方式：
纹理坐标的范围通常是从(0, 0)到(1,
1)，那如果我们把纹理坐标设置在范围之外会发生什么？
OpenGL默认的行为是重复这个纹理图像（我们基本上忽略浮点纹理坐标的整数部分）


环绕方式	描述
GL_REPEAT	对纹理的默认行为。重复纹理图像。
GL_MIRRORED_REPEAT	和GL_REPEAT一样，但每次重复图片是镜像放置的。
GL_CLAMP_TO_EDGE
纹理坐标会被约束在0到1之间，超出的部分会重复纹理坐标的边缘，产生一种边缘被拉伸的效果。
GL_CLAMP_TO_BORDER	超出的坐标为用户指定的边缘颜色。

前面提到的每个选项都可以使用glTexParameter*函数对单独的一个坐标轴设置（
s、t（如果是使用3D纹理那么还有一个r）它们和x、y、z是等价的）：
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
*/

/*
纹理过滤：
纹理坐标不依赖于分辨率(Resolution)，它可以是任意浮点值，
所以OpenGL需要知道怎样将纹理像素(Texture
Pixel，也叫Texel，译注1)映射到纹理坐标。
当你有一个很大的物体但是纹理的分辨率很低的时候这就变得很重要了。
你可能已经猜到了，OpenGL也有对于纹理过滤(Texture Filtering)的选项。
纹理过滤有很多个选项，但是现在我们只讨论最重要的两种：GL_NEAREST和GL_LINEAR。

GL_NEAREST（也叫邻近过滤，Nearest Neighbor
Filtering）是OpenGL默认的纹理过滤方式。
当设置为GL_NEAREST的时候，OpenGL会选择中心点最接近纹理坐标的那个像素。
下图中你可以看到四个像素，加号代表纹理坐标。
左上角那个纹理像素的中心距离纹理坐标最近，所以它会被选择为样本颜色：

GL_LINEAR（也叫线性过滤，(Bi)linear Filtering）它会基于纹理坐标附近的纹理像素，
计算出一个插值，近似出这些纹理像素之间的颜色。一个纹理像素的中心距离纹理坐标越近，
那么这个纹理像素的颜色对最终的样本颜色的贡献越大。
下图中你可以看到返回的颜色是邻近像素的混合色：

GL_NEAREST产生了颗粒状的图案，我们能够清晰看到组成纹理的像素，
而GL_LINEAR能够产生更平滑的图案，很难看出单个的纹理像素。
GL_LINEAR可以产生更真实的输出，但有些开发者更喜欢8-bit风格，
所以他们会用GL_NEAREST选项。

当进行放大(Magnify)和缩小(Minify)操作的时候可以设置纹理过滤的选项，
比如你可以在纹理被缩小的时候使用邻近过滤，被放大时使用线性过滤。
我们需要使用glTexParameter*函数为放大和缩小指定过滤方式。
这段代码看起来会和纹理环绕方式的设置很相似
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
*/

/*
多级渐远纹理：
想象一下，假设我们有一个包含着上千物体的大房间，每个物体上都有纹理。
有些物体会很远，但其纹理会拥有与近处物体同样高的分辨率。
由于远处的物体可能只产生很少的片段，
OpenGL从高分辨率纹理中为这些片段获取正确的颜色值就很困难，
因为它需要对一个跨过纹理很大部分的片段只拾取一个纹理颜色。
在小物体上这会产生不真实的感觉，更不用说对它们使用高分辨率纹理浪费内存的问题了。

OpenGL使用一种叫做多级渐远纹理(Mipmap)的概念来解决这个问题，
它简单来说就是一系列的纹理图像，后一个纹理图像是前一个的二分之一。
多级渐远纹理背后的理念很简单：距观察者的距离超过一定的阈值，
OpenGL会使用不同的多级渐远纹理，即最适合物体的距离的那个。
由于距离远，解析度不高也不会被用户注意到。
同时，多级渐远纹理另一加分之处是它的性能非常好。

幸好OpenGL有一个glGenerateMipmaps函数，
在创建完一个纹理后调用它OpenGL就会承担接下来的所有工作了。

在渲染中切换多级渐远纹理级别(Level)时，
OpenGL在两个不同级别的多级渐远纹理层之间会产生不真实的生硬边界。
就像普通的纹理过滤一样，
切换多级渐远纹理级别时你也可以在两个不同多级渐远纹理级别之间使用NEAREST和LINEAR过滤。
为了指定不同多级渐远纹理级别之间的过滤方式，
你可以使用下面四个选项中的一个代替原有的过滤方式：
GL_NEAREST_MIPMAP_NEAREST
使用最邻近的多级渐远纹理来匹配像素大小，并使用邻近插值进行纹理采样
GL_LINEAR_MIPMAP_NEAREST
使用最邻近的多级渐远纹理级别，并使用线性插值进行采样 GL_NEAREST_MIPMAP_LINEAR
在两个最匹配像素大小的多级渐远纹理之间进行线性插值，使用邻近插值进行采样
GL_LINEAR_MIPMAP_LINEAR
在两个邻近的多级渐远纹理之间使用线性插值，并使用线性插值进行采样
（GL_{Opt1}_MIPMAP_{Opt2}：opt1指在单张纹理中的插值方式，opt2指选择mipmap的方法）

一个常见的错误是，将放大过滤的选项设置为多级渐远纹理过滤选项之一。这样没有任何效果，
因为多级渐远纹理主要是使用在纹理被缩小的情况下的：纹理放大不会使用多级渐远纹理，
为放大过滤设置多级渐远纹理的选项会产生一个GL_INVALID_ENUM错误代码。
*/

/*
纹理单元：
使用glUniform1i，我们可以给纹理采样器分配一个位置值，
这样的话我们能够在一个片段着色器中设置多个纹理。
一个纹理的位置值通常称为一个纹理单元(Texture Unit)。
一个纹理的默认纹理单元是0，它是默认的激活纹理单元。

纹理单元的主要目的是让我们在着色器中可以使用多于一个的纹理。
通过把纹理单元赋值给采样器，我们可以一次绑定多个纹理，只要我们首先激活对应的纹理单元。
就像glBindTexture一样，我们可以使用glActiveTexture激活纹理单元，
传入我们需要使用的纹理单元：

纹理单元GL_TEXTURE0默认总是被激活

OpenGL至少保证有16个纹理单元供你使用，
也就是说你可以激活从GL_TEXTURE0到GL_TEXTRUE15。
它们都是按顺序定义的，所以我们也可以通过GL_TEXTURE0 + 8的方式获得GL_TEXTURE8，
这在当我们需要循环一些纹理单元的时候会很有用。
*/

/**NOTE - GLM
 * GLM库从0.9.9版本起，默认会将矩阵类型初始化为一个零矩阵（所有元素均为0），
 * 而不是单位矩阵（对角元素为1，其它元素为0）。如果你使用的是0.9.9或0.9.9以上的版本，
 * 你需要将所有的矩阵初始化改为 glm::mat4 mat = glm::mat4(1.0f)。
 *
 */
