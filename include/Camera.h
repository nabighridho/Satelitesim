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

    // FOV zoom
    float fov;
    float targetFov;

    // Chase cam
    bool chaseMode;
    glm::vec3 chaseTarget;       // world pos of target
    glm::vec3 chaseTargetFront;  // direction target is facing

public:
    Window &window;
    glm::mat4 view;
    glm::mat4 projection;

    Camera(Window &window);
    ~Camera() = default;

    void processKeyboardInput(GLFWwindow *window);
    static void mouse_callback(GLFWwindow *window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

    // Chase cam
    void setChaseCamTarget(glm::vec3 targetPos, glm::vec3 targetFront);
    void setFreeMode();
    void resetToEarthView();
    bool isChaseMode() const { return chaseMode; }

    // FOV
    float getFOV() const { return fov; }

    inline glm::vec3 getCameraPos() const { return cameraPos; }
    inline glm::mat4 getViewMatrix() const { return view; }
    inline glm::mat4 getProjection() const { return projection; }
};
