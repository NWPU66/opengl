#pragma once

#include <glad/glad.h>
// GLAD first
#include <GLFW/glfw3.h>
#include <functional>
#include <iostream>
#include <stdexcept>

void framebuffer_size_callback(GLFWwindow* window, int w, int h);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

class GlfwWindow {
protected:
    GLFWwindow* window;

public:
    GlfwWindow(const int width, const int height, const char* title);
    ~GlfwWindow();

    void cleanup();
};