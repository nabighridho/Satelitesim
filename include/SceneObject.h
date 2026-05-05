#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.h"
#include "Material.h"
#include "shader.h"

struct MeshPart {
    Mesh mesh;
    Material material;
    glm::mat4 localTransform;
};

class SceneObject {
private:
    std::vector<MeshPart> parts;
    glm::mat4 worldTransform;
    glm::vec3 position;

    std::string name;
    std::string description;
    float interactionRadius;

public:
    SceneObject(const std::string &name, const std::string &desc,
                glm::vec3 position, float interactionRadius = 3.5f);

    void addPart(Mesh &&mesh, const Material &mat,
                 const glm::mat4 &localTransform = glm::mat4(1.0f));

    void draw(Shader &shader) const;

    void setWorldTransform(const glm::mat4 &t) { worldTransform = t; }
    const glm::mat4 &getWorldTransform() const { return worldTransform; }
    glm::vec3 getPosition() const { return position; }
    const std::string &getName() const { return name; }
    const std::string &getDescription() const { return description; }
    float getInteractionRadius() const { return interactionRadius; }
};
