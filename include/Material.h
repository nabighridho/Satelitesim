#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    GLuint textureID = 0;  // 0 = no texture

    static Material metal() {
        return {{0.25f, 0.25f, 0.25f}, {0.7f, 0.7f, 0.75f}, {1.0f, 1.0f, 1.0f}, 64.0f};
    }
    static Material plastic() {
        return {{0.1f, 0.1f, 0.1f}, {0.55f, 0.55f, 0.55f}, {0.7f, 0.7f, 0.7f}, 32.0f};
    }
    static Material darkPlastic() {
        return {{0.05f, 0.05f, 0.05f}, {0.2f, 0.2f, 0.22f}, {0.4f, 0.4f, 0.4f}, 16.0f};
    }
    static Material concrete() {
        return {{0.15f, 0.15f, 0.15f}, {0.6f, 0.6f, 0.58f}, {0.2f, 0.2f, 0.2f}, 8.0f};
    }
    static Material pcbGreen() {
        return {{0.02f, 0.08f, 0.02f}, {0.1f, 0.45f, 0.15f}, {0.3f, 0.5f, 0.3f}, 16.0f};
    }
    static Material redPlastic() {
        return {{0.1f, 0.02f, 0.02f}, {0.8f, 0.15f, 0.1f}, {0.7f, 0.6f, 0.6f}, 32.0f};
    }
    static Material bluePlastic() {
        return {{0.02f, 0.02f, 0.1f}, {0.1f, 0.2f, 0.8f}, {0.6f, 0.6f, 0.7f}, 32.0f};
    }
    static Material yellowPlastic() {
        return {{0.1f, 0.1f, 0.02f}, {0.8f, 0.7f, 0.1f}, {0.7f, 0.7f, 0.6f}, 32.0f};
    }
    static Material whitePlastic() {
        return {{0.15f, 0.15f, 0.15f}, {0.9f, 0.9f, 0.9f}, {0.8f, 0.8f, 0.8f}, 32.0f};
    }
    static Material floor() {
        return {{0.12f, 0.12f, 0.14f}, {0.45f, 0.45f, 0.5f}, {0.3f, 0.3f, 0.3f}, 16.0f};
    }
    static Material wall() {
        return {{0.15f, 0.15f, 0.17f}, {0.65f, 0.65f, 0.7f}, {0.1f, 0.1f, 0.1f}, 4.0f};
    }
    static Material ceiling() {
        return {{0.2f, 0.2f, 0.22f}, {0.85f, 0.85f, 0.88f}, {0.1f, 0.1f, 0.1f}, 4.0f};
    }
};
