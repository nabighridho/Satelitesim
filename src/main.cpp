#include "SatelliteScene.h"
#include "Camera.h"
#include "Window.h"
#include "Renderer.h"
#include "InfoPanel.h"

int main()
{
    Window window;
    enableDebugging();

    Camera camera(window);
    glfwSetCursorPosCallback(window.p_GLFWwindow(), Camera::mouse_callback);
    glfwSetWindowUserPointer(window.p_GLFWwindow(), &camera);

    Shader phongShader("../res/shaders/phong.vert", "../res/shaders/phong.frag");
    SatelliteScene scene;
    InfoPanel infoPanel(window.getWidth(), window.getHeight());

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    float lastFrame = 0.0f;
    bool prevF = false, prevT = false;

    while (!glfwWindowShouldClose(window.p_GLFWwindow()))
    {
        float currentFrame = (float)glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.01f, 0.01f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.processKeyboardInput(window.p_GLFWwindow());

        // Toggle scan beam on F key rising edge
        bool currF = glfwGetKey(window.p_GLFWwindow(), GLFW_KEY_F) == GLFW_PRESS;
        bool currT = glfwGetKey(window.p_GLFWwindow(), GLFW_KEY_T) == GLFW_PRESS;
        if (currF && !prevF) scene.toggleScanBeam();
        if (currT && !prevT) scene.toggleTransmission();
        prevF = currF;
        prevT = currT;

        scene.update(dt);
        scene.render(phongShader, camera);

        std::string status =
            std::string("Acquisition: ") + (scene.isScanBeamOn() ? "ON " : "OFF") +
            "   Transmission: "          + (scene.isTransmissionOn() ? "ON " : "OFF") +
            "\n[F] Toggle Scan Beam   [T] Toggle Data Transmission   WASD / Q E to fly   Shift = fast";
        infoPanel.render("NASA Aqua Satellite", status);

        glfwSwapBuffers(window.p_GLFWwindow());
        glfwPollEvents();
    }

    return 0;
}
