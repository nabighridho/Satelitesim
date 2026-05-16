#pragma once
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

class Window
{
private:
    static void framebuffer_size_callback(GLFWwindow *window, int width, int height);
    GLFWwindow *window;
    const unsigned int WIN_WIDTH = 1920;
    const unsigned int WIN_HEIGHT = 1080;

public:
    inline GLFWwindow *p_GLFWwindow() { return window; }
    inline unsigned int getWidth() { return WIN_WIDTH; }
    inline unsigned int getHeight() { return WIN_HEIGHT; }

    void showCursor() { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
    void hideCursor() { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); }

    Window();
    ~Window();
};
