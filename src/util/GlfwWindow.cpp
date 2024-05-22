#include "GlfwWindow.hpp"

GlfwWindow::GlfwWindow(const int width, const int height, const char* title)
{
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfwWindowHint(GLFW_SAMPLES, 4);  // 多重采样缓冲

    // 创建窗口
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        throw std::runtime_error("failed to create a window!");
    }
    glfwMakeContextCurrent(window);
    // 注册回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwTerminate();
        throw std::runtime_error("failed to initialize GLAD!");
    }
}

GlfwWindow::~GlfwWindow() {}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    throw std::runtime_error("framebuffer_size_callback not implemented!");
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    throw std::runtime_error("mouse_callback not implemented!");
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    throw std::runtime_error("scroll_callback not implemented!");
}
