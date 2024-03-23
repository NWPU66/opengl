#include <glad/glad.h>
// GLAD include first
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

const char* vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const char* fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor=vec4(1.f,.5f,.2f,1.f);\n"
    "}\0";

/// @brief 视窗回调函数
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
}

GLFWwindow* initWindow() {
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    GLFWwindow* window =
        glfwCreateWindow(800, 600, "HelloTriangle", NULL, NULL);
    if (window == NULL) {
        std::cout << "failed to create a window!" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "failed to load GLAD!" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    // 声明GL视窗大小，并注册视窗回调函数
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return window;
}

/// @brief 检查着色器的编译是否正常
int checkShaderCompiling(GLuint vertexShader) {
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    return success;
}

/// @brief 检查着色器程序的编译是否正常
int checkShaderProgramCompiling(GLuint shaderProgram) {
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    return success;
}

GLuint perpareShaderProgram() {
    // 在运行时动态编译顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);  // 创建顶点着色器
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);  // 传递源码
    glCompileShader(vertexShader);  // 编译着色器
    // 片元着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // 把两个着色器对象链接到一个用来渲染的着色器程序上
    GLuint shaderProgram = glCreateProgram();
    // link
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // 使用这个shader程序
    glUseProgram(shaderProgram);
    // 删除已经不需要的着色器源码
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main(int argc, char** argv) {
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "failed to create a GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "failed to load GLAD!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 声明GL视窗大小，并注册视窗回调函数
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 创建一个VAO
    GLuint VAO;
    glGenBuffers(1, &VAO);
    glBindVertexArray(VAO);  // 绑定
    // 创建一个EBO
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // 处理顶点着色器的输入，这个输入由VBO管理
    float vertices[] = {
        0.5f,  0.5f,  0.0f,  // 右上角
        0.5f,  -0.5f, 0.0f,  // 右下角
        -0.5f, -0.5f, 0.0f,  // 左下角
        -0.5f, 0.5f,  0.0f   // 左上角
    };                       // 定义顶点
    unsigned int indices[] = {
        // 注意索引从0开始!
        // 此例的索引(0,1,2,3)就是顶点数组vertices的下标，
        // 这样可以由下标代表顶点组合成矩形
        0, 1, 3,  // 第一个三角形
        1, 2, 3   // 第二个三角形
    };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);
    GLuint VBO;
    glGenBuffers(1, &VBO);  // 创建VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // 此时，我们使用的任何缓冲调用都会用来配置当前绑定的缓冲(VBO)。
    // 把定义的顶点数据复制到缓冲的内存中
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    /*
    glBufferData的第四个参数指定了GPU如何管理这些数据：
    GL_STATIC_DRAW ：数据不会或几乎不会改变。
    GL_DYNAMIC_DRAW：数据会被改变很多。
    GL_STREAM_DRAW ：数据每次绘制时都会改变。
    */

    GLuint shaderProgram = perpareShaderProgram();  // 准备一个简单的着色器

    /*
    就快要完成了，但还没结束，OpenGL还不知道它该如何解释内存中的顶点数据，
    以及它该如何将顶点数据链接到顶点着色器的属性上。我们需要告诉OpenGL怎么做。
    */

    // 链接顶点属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)0);
    // 这个时候设置的index0指向VAO的第一个元素，VAO[0]指向上述的vertex arrt设定
    glEnableVertexAttribArray(0);  // 注意这个函数，有好几个长得很像的
    /*
    glVertexAttribPointer()：
    1. 索引：指定了一个要修改的顶点数据组
    2. 个数：每个顶点数据组中包含元素的个数
    3. 元素的类型
    4. （手册上）询问定点数要不要被规格化
    5. 步长，0代表紧密排列
    6. 数据的起始偏移
    */

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        // 处理输入

        glClearColor(0.2f, 0.5f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // 渲染指令
        //--------------------------------------------------------------------------------
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // glDrawArrays(GL_TRIANGLES, 0, 3);
        /*
        glDrawArrays()：
        1. 打算绘制的图元类型
        2和3：对于每一个顶点数组，从第几个元素开始绘制，总共绘制几个元素。
        */
       glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // glDrawElements第三个参数是索引的类型
        //--------------------------------------------------------------------------------

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

/*
顶点数组对象：VAO
顶点缓冲对象：VBO
元素缓冲对象（索引缓冲对象）：EBO或IBO

3D坐标转为2D坐标的处理过程是由OpenGL的图形渲染管线管理的。
图形渲染管线：
1. 把你的3D坐标转换为2D坐标，
2. 把2D坐标转变为实际的有颜色的像素。

![](https://learnopengl-cn.github.io/img/01/04/pipeline.png)
OpenGL着色器是用OpenGL着色器语言(OpenGL Shading Language, GLSL)写成的。

为了让OpenGL知道我们的坐标和颜色值构成的到底是什么，
OpenGL需要你去指定这些数据所表示的渲染类型。
我们是希望把这些数据渲染成一系列的点？一系列的三角形？还是仅仅是一个长长的线？
做出的这些提示叫做图元(Primitive)，任何一个绘制指令的调用都将把图元传递给OpenGL。
这是其中的几个：GL_POINTS、GL_TRIANGLES、GL_LINE_STRIP。

1- 图形渲染管线的第一个部分是顶点着色器(Vertex
Shader)，它把一个单独的顶点作为输入。
顶点着色器主要的目的是把3D坐标转为另一种3D坐标（后面会解释），
同时顶点着色器允许我们对顶点属性进行一些基本处理。

2- 图元装配(Primitive Assembly)阶段将顶点着色器输出的所有顶点作为输入
将所有的点装配成指定图元的形状；

3- 图元装配阶段的输出会传递给几何着色器(Geometry Shader)。
几何着色器把图元形式的一系列顶点的集合作为输入，
它可以通过产生新顶点构造出新的（或是其它的）图元来生成其他形状。

4- 几何着色器的输出会被传入光栅化阶段(Rasterization Stage)，
这里它会把图元映射为最终屏幕上相应的像素，
生成供片段着色器(Fragment Shader)使用的片段(Fragment)。

5-
片段着色器的主要目的是计算一个像素的最终颜色，这也是所有OpenGL高级效果产生的地方。
通常，片段着色器包含3D场景的数据（比如光照、阴影、光的颜色等等），
这些数据可以被用来计算最终像素的颜色。

6- 最后一个阶段：Alpha测试和混合(Blending)阶段。这个阶段检测片段的深度和模板值
用它们来判断这个像素是其它物体的前面还是后面，决定是否应该丢弃。
这个阶段也会检查alpha值并对物体进行混合。

在现代OpenGL中，我们必须定义至少一个顶点着色器和片段着色器（因为没有默认的顶点/片段着色器）。
*/

/*
顶点输入：
OpenGL不是简单地把所有的3D坐标变换为屏幕上的2D像素；
OpenGL仅当3D坐标在3个轴（x、y和z）上-1.0到1.0的范围内时才处理它。
所有在这个范围内的坐标叫做标准化设备坐标(Normalized Device Coordinates)，
此范围内的坐标最终显示在屏幕上（在这个范围以外的坐标则不会显示）。

一旦你的顶点坐标已经在顶点着色器中处理过，它们就应该是标准化设备坐标了，
标准化设备坐标是一个x、y和z值在-1.0到1.0的一小段空间。
任何落在范围外的坐标都会被丢弃/裁剪，不会显示在你的屏幕上。

通过使用由glViewport函数提供的数据，进行视口变换(Viewport Transform)，
标准化设备坐标会变换为屏幕空间坐标（光栅渲染的第三个变换）。
所得的屏幕空间坐标又会被变换为片段输入到片段着色器中。

定义好数据以后，我们会把它作为输入发送给图形渲染管线的第一个处理阶段：顶点着色器。
它会在GPU上创建内存用于储存我们的顶点数据，还要配置OpenGL如何解释这些内存，
并且指定其如何发送给显卡。顶点着色器接着会处理我们在内存中指定数量的顶点。

我们通过顶点缓冲对象(VBO)管理这个内存（存储顶点着色器的输入），
它会在GPU内存（通常被称为显存）中储存大量顶点。
从CPU把数据发送到显卡相对较慢，所以只要可能我们都要尝试尽量一次性发送尽可能多的数据。
当数据发送至显卡的内存中后，顶点着色器几乎能立即访问顶点。
*/

/*
着色器程序：
着色器程序对象(Shader Program Object)是多个着色器合并之后并最终链接完成的版本。
如果要使用刚才编译的着色器我们必须把它们链接(Link)为一个着色器程序对象，
然后在渲染对象的时候激活这个着色器程序。
已激活着色器程序的着色器将在我们发送渲染调用的时候被使用。

当链接着色器至一个程序的时候，它会把每个着色器的输出链接到下个着色器的输入。
当输出和输入不匹配的时候，会发生连接错误。
*/

/*
链接顶点属性：
顶点着色器允许我们指定任何以顶点属性为形式的输入。这使其具有很强的灵活性的同时，
它还的确意味着我们必须手动指定输入数据的哪一个部分对应顶点着色器的哪一个顶点属性。
所以，我们必须在渲染前指定OpenGL该如何解释顶点数据。
![](https://learnopengl-cn.github.io/img/01/04/vertex_attribute_pointer.png)

每个顶点属性从一个VBO管理的内存中获得它的数据，
而具体是从哪个VBO获取，则是通过在调用glVertexAttribPointer时绑定到GL_ARRAY_BUFFER的VBO决定的。
由于在调用glVertexAttribPointer之前绑定的是先前定义的VBO对象，
顶点属性0现在会链接到它的顶点数据。
*/

/*
顶点数组对象：
顶点数组对象(Vertex Array Object, VAO)可以像顶点缓冲对象那样被绑定，
任何随后的顶点属性调用都会储存在这个VAO中。
样的好处就是，当配置顶点属性指针时，你只需要将那些调用执行一次，
之后再绘制物体的时候只需要绑定相应的VAO就行了。
这使在不同顶点数据和属性配置之间切换变得非常简单，只需要绑定不同的VAO就行了。

VAO中存储的是attr pointer
vertex attr实际上是对VBO数据的不同解读方式

一个顶点数组对象会储存以下这些内容：
glEnableVertexAttribArray和glDisableVertexAttribArray的调用。
通过glVertexAttribPointer设置的顶点属性配置。
通过glVertexAttribPointer调用与顶点属性关联的顶点缓冲对象。

要想使用VAO，要做的只是使用glBindVertexArray绑定VAO。
从绑定之后起，我们应该绑定和配置对应的VBO和属性指针，之后解绑VAO供之后使用。
当我们打算绘制一个物体时，只要在绘制前简单地把VAO绑定到希望使用的设定上就行了。
*/

/*
元素缓存对象EBO（索引缓存对象IBO）：
假设我们不再绘制一个三角形而是绘制一个矩形。
我们可以绘制两个三角形来组成一个矩形（OpenGL主要处理三角形）。
float vertices[] = {
    // 第一个三角形
    0.5f, 0.5f, 0.0f,   // 右上角
    0.5f, -0.5f, 0.0f,  // 右下角
    -0.5f, 0.5f, 0.0f,  // 左上角
    // 第二个三角形
    0.5f, -0.5f, 0.0f,  // 右下角
    -0.5f, -0.5f, 0.0f, // 左下角
    -0.5f, 0.5f, 0.0f   // 左上角
};
可以看到，有几个顶点叠加了。
我们指定了右下角和左上角两次！一个矩形只有4个而不是6个顶点，这样就产生50%的额外开销。
当我们有包括上千个三角形的模型之后这个问题会更糟糕。
更好的解决方案是只储存不同的顶点，并设定绘制这些顶点的顺序。
这样子我们只要储存4个顶点就能绘制矩形了，之后只要指定绘制的顺序就行了。

 EBO是一个缓冲区，就像一个顶点缓冲区对象一样，
 它存储 OpenGL 用来决定要绘制哪些顶点的索引。
 这种所谓的索引绘制(Indexed Drawing)正是我们问题的解决方案。

 glDrawElements函数从当前绑定到GL_ELEMENT_ARRAY_BUFFER目标的EBO中获取其索引。
 这意味着我们每次想要使用索引渲染对象时都必须绑定相应的EBO，这又有点麻烦。
 碰巧顶点数组对象也跟踪元素缓冲区对象绑定。
 在绑定VAO时，绑定的最后一个元素缓冲区对象存储为VAO的元素缓冲区对象。
 然后，绑定到VAO也会自动绑定该EBO。

 当目标是GL_ELEMENT_ARRAY_BUFFER的时候，VAO会储存glBindBuffer的函数调用。
 这也意味着它也会储存解绑调用，所以确保你没有在解绑VAO之前解绑索引数组缓冲，
 否则它就没有这个EBO配置了。
 （EBO先绑定，VAO会记录他为最后一个EBO对象，
 这时候VAO再绑定，VAO会顺手帮EBO也绑定上去（尽管EBO已经绑定好了），
 最后再解除EBO的绑定，此时OpenGL上下文中不再有绑定EBO对象）
*/
