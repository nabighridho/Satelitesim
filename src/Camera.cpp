#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera(Window &window)
    : cameraPos(glm::vec3(0.0f, 15.0f, 45.0f)),
      cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)),
      window(window),
      lastX(window.getWidth() / 2.0f),
      lastY(window.getHeight() / 2.0f),
      firstMouse(true),
      deltaTime(0.0f), lastFrame(0.0f), currentFrame(0.0f),
      fov(60.0f), targetFov(60.0f),
      chaseMode(false),
      chaseTarget(0.0f), chaseTargetFront(0.0f, 0.0f, -1.0f),
      view(1.0f)
{
    // Point camera directly at Earth (origin)
    cameraFront = glm::normalize(glm::vec3(0.0f) - cameraPos);
    yaw = glm::degrees(atan2f(cameraFront.z, cameraFront.x));
    pitch = glm::degrees(asinf(cameraFront.y));
    view = glm::lookAt(cameraPos, glm::vec3(0.0f), cameraUp);
    projection = glm::perspective(
        glm::radians(fov),
        (float)window.getWidth() / (float)window.getHeight(),
        0.1f, 2000.0f);
}

void Camera::processKeyboardInput(GLFWwindow *window)
{
    currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // ESC handled in main.cpp

    // Smooth FOV interpolation
    fov += (targetFov - fov) * std::min(1.0f, 8.0f * deltaTime);
    projection = glm::perspective(
        glm::radians(fov),
        (float)this->window.getWidth() / (float)this->window.getHeight(),
        0.1f, 2000.0f);

    if (chaseMode)
    {
        // Chase camera: orbit behind the target
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::vec3 toTarget = glm::normalize(chaseTarget); // direction from origin

        // Camera offset: behind and above the satellite
        glm::vec3 offset = glm::normalize(toTarget) * 6.0f + glm::vec3(0.0f, 3.0f, 0.0f);
        glm::vec3 desiredPos = chaseTarget + offset;

        // Smooth follow
        float smoothSpeed = std::min(1.0f, 4.0f * deltaTime);
        cameraPos += (desiredPos - cameraPos) * smoothSpeed;

        view = glm::lookAt(cameraPos, chaseTarget, up);
        return;
    }

    float speed = 8.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 2.5f;

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += right * speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos -= cameraUp * speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos += cameraUp * speed;

    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void Camera::mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    Camera *camera = static_cast<Camera *>(glfwGetWindowUserPointer(window));

    if (camera->chaseMode)
        return; // Don't process mouse in chase mode

    if (camera->firstMouse)
    {
        camera->lastX = xpos;
        camera->lastY = ypos;
        camera->firstMouse = false;
    }

    float xoffset = xpos - camera->lastX;
    float yoffset = camera->lastY - ypos;
    camera->lastX = xpos;
    camera->lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera->yaw += xoffset;
    camera->pitch += yoffset;

    if (camera->pitch > 89.0f)
        camera->pitch = 89.0f;
    if (camera->pitch < -89.0f)
        camera->pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
    front.y = sin(glm::radians(camera->pitch));
    front.z = sin(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
    camera->cameraFront = glm::normalize(front);
}

void Camera::scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    Camera *camera = static_cast<Camera *>(glfwGetWindowUserPointer(window));
    camera->targetFov -= (float)yoffset * 3.0f;
    camera->targetFov = std::max(20.0f, std::min(120.0f, camera->targetFov));
}

void Camera::setChaseCamTarget(glm::vec3 targetPos, glm::vec3 targetFront)
{
    chaseMode = true;
    chaseTarget = targetPos;
    chaseTargetFront = targetFront;
}

void Camera::setFreeMode()
{
    if (chaseMode)
    {
        chaseMode = false;
        firstMouse = true; // Reset mouse to avoid camera jump

        // Set cameraFront to face the last target direction
        cameraFront = glm::normalize(chaseTarget - cameraPos);
        yaw = glm::degrees(atan2f(cameraFront.z, cameraFront.x));
        pitch = glm::degrees(asinf(cameraFront.y));
    }
}

void Camera::resetToEarthView()
{
    chaseMode = false;
    firstMouse = true;
    cameraPos = glm::vec3(0.0f, 15.0f, 45.0f);
    cameraFront = glm::normalize(glm::vec3(0.0f) - cameraPos);
    yaw = glm::degrees(atan2f(cameraFront.z, cameraFront.x));
    pitch = glm::degrees(asinf(cameraFront.y));
    view = glm::lookAt(cameraPos, glm::vec3(0.0f), cameraUp);
}
