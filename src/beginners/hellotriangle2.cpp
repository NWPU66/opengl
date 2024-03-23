#include <glad/glad.h>
// GLAD first
#include <GLFW/glfw3.h>

#include <iostream>

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
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";
float vertices[] = {
    0.5f,  0.5f,  0.0f,  // 右上角
    0.5f,  -0.5f, 0.0f,  // 右下角
    -0.5f, -0.5f, 0.0f,  // 左下角
    -0.5f, 0.5f,  0.0f   // 左上角
};
unsigned int indices[] = {
    // 注意索引从0开始!
    // 此例的索引(0,1,2,3)就是顶点数组vertices的下标，
    // 这样可以由下标代表顶点组合成矩形

    0, 1, 3,  // 第一个三角形
    1, 2, 3   // 第二个三角形
};

GLubyte* outputData = new GLubyte[800 * 600 * 3];  // 从缓冲中读出数据可以用这个

/// @brief 视窗回调函数
void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
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

int main(int argc, char** argv) {
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    GLFWwindow* window =
        glfwCreateWindow(800, 600, "HelloTriangle", NULL, NULL);
    if (window == NULL) {
        std::cout << "failed to create a window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "failed to load GLAD!" << std::endl;
        glfwTerminate();
        return -1;
    }

    //-------------------------------------------------------------------------------------------
    // 创建顶点/片元着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    // 指定着色器的源码并编译
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);
    // 链接
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // 删除不再需要的源码
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 创建OpenGL对象，VAO和VBO
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // 绑定
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // 将顶点数组、绘制顺序送入VBO和EBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);
    // 设置顶点属性，描述VBO中数据的解读方式
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    /*
    错题本：
    3 * sizeof(float) ！= 3 * sizeof(vertices)
    float的大小是4B，而sizeof(vertices)是数组的大小
    sizeof(vertices) = 4B * 9 元素 = 36B
    */
    glEnableVertexAttribArray(0);
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    //-------------------------------------------------------------------------------------------

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        // 处理输入
        processInput(window);

        // 渲染指令
        //-------------------------------------------------------------------------------------------
        // 设置纯色背景
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 渲染
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // 渲染模式（线框or填充）
        // 填充模式 GL_FILL

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        //-------------------------------------------------------------------------------------------

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // 释放内存并退出
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
