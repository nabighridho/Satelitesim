#pragma once
#include "shader.h"
#include <string>
#include <vector>

class InfoPanel {
private:
    Shader overlayShader;

    // Background quad
    GLuint bgVAO, bgVBO;

    // Text rendering
    GLuint textVAO, textVBO;
    GLuint fontTexture;
    float screenW, screenH;

    void initFont();
    void drawBackground(float x, float y, float w, float h);
    void drawText(const std::string &text, float x, float y, float scale, float r, float g, float b);

public:
    InfoPanel(float screenWidth, float screenHeight);
    ~InfoPanel();

    void render(const std::string &name, const std::string &description);
};
