#pragma once
#include "shader.h"
#include "Window.h"

class Camera
{
private:
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;

    float yaw;
    float pitch;
    float lastX;
    float lastY;
    bool firstMouse;

    float deltaTime;
    float lastFrame;
    float currentFrame;

public:
    Window &window;
    glm::mat4 view;
    glm::mat4 projection;

    Camera(Window &window);
    ~Camera() = default;

    void processKeyboardInput(GLFWwindow *window);
    static void mouse_callback(GLFWwindow *window, double xpos, double ypos);

    inline glm::vec3 getCameraPos() const { return cameraPos; }
    inline glm::mat4 getViewMatrix() const { return view; }
    inline glm::mat4 getProjection() const { return projection; }
};
