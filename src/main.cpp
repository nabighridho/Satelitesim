#include "SatelliteScene.h"
#include "Camera.h"
#include "Window.h"
#include "Renderer.h"
#include "InfoPanel.h"
#include "Starfield.h"
#include "MainMenu.h"
#include <cstdio>

enum AppState { STATE_MENU, STATE_GAME };

// Global for mouse click callback
static AppState g_appState = STATE_MENU;
static bool g_mouseClicked = false;
static double g_clickX = 0, g_clickY = 0;

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && g_appState == STATE_MENU)
    {
        glfwGetCursorPos(window, &g_clickX, &g_clickY);
        g_mouseClicked = true;
    }
}

int main()
{
    Window window;
    enableDebugging();

    Camera camera(window);
    glfwSetCursorPosCallback(window.p_GLFWwindow(), Camera::mouse_callback);
    glfwSetScrollCallback(window.p_GLFWwindow(), Camera::scroll_callback);
    glfwSetMouseButtonCallback(window.p_GLFWwindow(), mouse_button_callback);
    glfwSetWindowUserPointer(window.p_GLFWwindow(), &camera);

    Shader phongShader("../res/shaders/phong.vert", "../res/shaders/phong.frag");
    SatelliteScene scene;
    InfoPanel infoPanel(window.getWidth(), window.getHeight());
    Starfield starfield(2500);
    MainMenu mainMenu(window.getWidth(), window.getHeight());

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Start in menu mode with cursor visible
    window.showCursor();

    float lastFrame = 0.0f;
    bool prevF = false, prevT = false, prevO = false, prevP = false, prevR = false;
    bool prevInc = false, prevDec = false, prevIncKP = false, prevDecKP = false;
    bool prev1 = false, prev2 = false, prev3 = false, prev0 = false;
    bool prevEsc = false;

    while (!glfwWindowShouldClose(window.p_GLFWwindow()))
    {
        float currentFrame = (float)glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.01f, 0.01f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLFWwindow *w = window.p_GLFWwindow();
        bool currEsc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;

        if (g_appState == STATE_MENU)
        {
            // ── MENU STATE ───────────────────────────────────────────────
            // Render animated background
            starfield.render(camera, currentFrame);

            // Slowly rotate scene in background (eye candy)
            scene.update(dt * 0.3f);
            scene.render(phongShader, camera);

            // Mouse position for hover
            double mx, my;
            glfwGetCursorPos(w, &mx, &my);
            mainMenu.update(dt, (float)mx, (float)my);

            // Handle click
            if (g_mouseClicked)
            {
                g_mouseClicked = false;
                MenuAction action = mainMenu.handleClick((float)g_clickX, (float)g_clickY);
                if (action == MENU_PLAY)
                {
                    g_appState = STATE_GAME;
                    window.hideCursor();
                    camera.resetToEarthView();   // reset camera back to Earth
                    scene.resetAll();
                }
                else if (action == MENU_QUIT)
                {
                    glfwSetWindowShouldClose(w, true);
                }
            }

            mainMenu.render(currentFrame);
        }
        else // STATE_GAME
        {
            // ── GAME STATE ───────────────────────────────────────────────
            camera.processKeyboardInput(w);

            // ESC → back to menu
            if (currEsc && !prevEsc)
            {
                g_appState = STATE_MENU;
                window.showCursor();
                scene.selectSatellite(-1);
                camera.setFreeMode();
            }

            // Toggle inputs (rising edge)
            bool currF = glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS;
            bool currT = glfwGetKey(w, GLFW_KEY_T) == GLFW_PRESS;
            bool currO = glfwGetKey(w, GLFW_KEY_O) == GLFW_PRESS;
            bool currP = glfwGetKey(w, GLFW_KEY_P) == GLFW_PRESS;
            bool currR = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS;
            bool currInc   = glfwGetKey(w, GLFW_KEY_EQUAL) == GLFW_PRESS;
            bool currDec   = glfwGetKey(w, GLFW_KEY_MINUS) == GLFW_PRESS;
            bool currIncKP = glfwGetKey(w, GLFW_KEY_KP_ADD) == GLFW_PRESS;
            bool currDecKP = glfwGetKey(w, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS;
            bool curr1 = glfwGetKey(w, GLFW_KEY_1) == GLFW_PRESS;
            bool curr2 = glfwGetKey(w, GLFW_KEY_2) == GLFW_PRESS;
            bool curr3 = glfwGetKey(w, GLFW_KEY_3) == GLFW_PRESS;
            bool curr0 = glfwGetKey(w, GLFW_KEY_0) == GLFW_PRESS;

            if (currF && !prevF) scene.toggleScanBeam();
            if (currT && !prevT) scene.toggleTransmission();
            if (currO && !prevO) scene.toggleOrbits();
            if (currP && !prevP) scene.togglePause();
            if ((currInc && !prevInc) || (currIncKP && !prevIncKP)) scene.increaseTimeScale();
            if ((currDec && !prevDec) || (currDecKP && !prevDecKP)) scene.decreaseTimeScale();
            if (currR && !prevR) { scene.resetAll(); camera.setFreeMode(); }

            if (curr1 && !prev1) { scene.selectSatellite(0); camera.setChaseCamTarget(scene.getSatellitePos(0), scene.getSatelliteFront(0)); }
            if (curr2 && !prev2) { scene.selectSatellite(1); camera.setChaseCamTarget(scene.getSatellitePos(1), scene.getSatelliteFront(1)); }
            if (curr3 && !prev3) { scene.selectSatellite(2); camera.setChaseCamTarget(scene.getSatellitePos(2), scene.getSatelliteFront(2)); }
            if (curr0 && !prev0) { scene.selectSatellite(-1); camera.setFreeMode(); }

            prevF = currF; prevT = currT; prevO = currO; prevP = currP; prevR = currR;
            prevInc = currInc; prevDec = currDec;
            prevIncKP = currIncKP; prevDecKP = currDecKP;
            prev1 = curr1; prev2 = curr2; prev3 = curr3; prev0 = curr0;

            int sel = scene.getSelectedSatellite();
            if (sel >= 0)
                camera.setChaseCamTarget(scene.getSatellitePos(sel), scene.getSatelliteFront(sel));

            // Render
            starfield.render(camera, currentFrame);
            scene.update(dt);
            scene.render(phongShader, camera);

            // Minimap (bottom-right corner)
            scene.renderMinimap(camera, window.getWidth(), window.getHeight());

            // HUD
            SatInfo info = scene.getSelectedInfo();
            char speedBuf[32];
            if (scene.isPaused()) snprintf(speedBuf, sizeof(speedBuf), "PAUSED");
            else snprintf(speedBuf, sizeof(speedBuf), "%.2fx", scene.getTimeScale());

            std::string title, status;
            if (sel >= 0) {
                char altBuf[64];
                snprintf(altBuf, sizeof(altBuf), "Alt: %.1f   Spd: %.2f   LoS: %s",
                         info.altitude, info.speed, info.hasLoS ? "YES" : "NO");
                title = info.name + "  [" + std::string(speedBuf) + "]";
                status = std::string(altBuf) +
                    "\n[F] Scan [T] Data [O] Orbits [1-3] Sat [0] Free [+/-] Speed [P] Pause [R] Reset";
            } else {
                title = std::string("Satellite Simulation  [") + speedBuf + "]";
                status = std::string("Scan: ") + (scene.isScanBeamOn() ? "ON " : "OFF") +
                    "   Data: " + (scene.isTransmissionOn() ? "ON " : "OFF") +
                    "   Orbits: " + (scene.isOrbitsOn() ? "ON" : "OFF") +
                    "\n[F] Scan [T] Data [O] Orbits [1-3] Sat [+/-] Speed [P] Pause [R] Reset  ESC=Menu";
            }
            infoPanel.render(title, status);
        }

        prevEsc = currEsc;
        glfwSwapBuffers(window.p_GLFWwindow());
        glfwPollEvents();
    }

    return 0;
}
