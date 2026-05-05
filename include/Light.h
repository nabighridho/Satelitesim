#pragma once
#include <glm/glm.hpp>

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;   // direction light is shining (toward scene)
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;   // direction the cone points
    glm::vec3 diffuse;
    glm::vec3 specular;
    float cutOff;          // cos(inner angle)
    float outerCutOff;     // cos(outer angle) — soft edge
    float constant;
    float linear;
    float quadratic;
};
