#include "Starfield.h"
#include <cstdlib>
#include <cmath>
#include <glm/glm.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple pseudo-random float [0,1] using seed
static float randFloat(unsigned int &seed)
{
    seed = seed * 1103515245u + 12345u;
    return (float)(seed & 0x7FFFFFFFu) / (float)0x7FFFFFFFu;
}

Starfield::Starfield(int numStars)
    : shader("../res/shaders/starfield.vert", "../res/shaders/starfield.frag"),
      VAO(0), VBO(0), starCount(numStars)
{
    // Generate star data: position (3) + brightness (1) + size (1) = 5 floats per star
    std::vector<float> data;
    data.reserve(numStars * 5);

    unsigned int seed = 42;
    float skyRadius = 800.0f;

    for (int i = 0; i < numStars; i++)
    {
        // Random point on sphere (uniform distribution)
        float theta = 2.0f * M_PI * randFloat(seed);
        float phi = acosf(2.0f * randFloat(seed) - 1.0f);

        float x = skyRadius * sinf(phi) * cosf(theta);
        float y = skyRadius * sinf(phi) * sinf(theta);
        float z = skyRadius * cosf(phi);

        float brightness = 0.3f + 0.7f * randFloat(seed);
        float size = 1.0f + 2.5f * randFloat(seed);

        // Some stars are extra bright (giant stars)
        if (randFloat(seed) < 0.05f)
        {
            brightness = 0.9f + 0.1f * randFloat(seed);
            size = 3.0f + 2.0f * randFloat(seed);
        }

        data.push_back(x);
        data.push_back(y);
        data.push_back(z);
        data.push_back(brightness);
        data.push_back(size);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    // position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // brightness (location 1)
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // size (location 2)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

Starfield::~Starfield()
{
    if (VAO)
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
}

void Starfield::render(const Camera &camera, float time)
{
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDepthMask(GL_FALSE); // Don't write to depth — stars are always behind everything

    shader.bind();
    shader.setUniformMatrix4fv("u_view", camera.getViewMatrix());
    shader.setUniformMatrix4fv("u_projection", camera.getProjection());
    shader.setUniform1f("u_time", time);

    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, starCount);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_PROGRAM_POINT_SIZE);
}
