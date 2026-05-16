#pragma once
#include <vector>
#include <glad/glad.h>
#include "shader.h"
#include "Camera.h"

class Starfield
{
private:
    Shader shader;
    GLuint VAO, VBO;
    int starCount;

public:
    Starfield(int numStars = 2000);
    ~Starfield();

    void render(const Camera &camera, float time);

    Starfield(const Starfield &) = delete;
    Starfield &operator=(const Starfield &) = delete;
};
