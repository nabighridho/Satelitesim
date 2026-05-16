#pragma once
#include <string>
#include "shader.h"
#include "Window.h"

enum MenuAction { MENU_NONE, MENU_PLAY, MENU_QUIT };

struct MenuButton {
    float x, y, w, h;       // NDC coordinates
    std::string label;
    float r, g, b;           // base color
    float hoverGlow;         // 0..1 animated
    bool hovered;
};

class MainMenu
{
private:
    Shader overlayShader;
    GLuint bgVAO, bgVBO;
    GLuint textVAO, textVBO;
    GLuint fontTexture;
    float screenW, screenH;

    MenuButton playBtn;
    MenuButton quitBtn;

    float animTime;

    void initFont();
    void drawRect(float x, float y, float w, float h, float r, float g, float b, float a);
    void drawText(const std::string &text, float x, float y, float scale, float r, float g, float b, float a = 1.0f);
    void drawTextCentered(const std::string &text, float y, float scale, float r, float g, float b, float a = 1.0f);

public:
    MainMenu(float screenWidth, float screenHeight);
    ~MainMenu();

    void update(float dt, float mouseX, float mouseY);
    MenuAction handleClick(float mouseX, float mouseY);
    void render(float time);

    MainMenu(const MainMenu &) = delete;
    MainMenu &operator=(const MainMenu &) = delete;
};
