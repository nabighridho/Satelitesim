#include "SceneObject.h"
#include <glm/gtc/matrix_transform.hpp>

SceneObject::SceneObject(const std::string &name, const std::string &desc,
                         glm::vec3 position, float interactionRadius)
    : name(name), description(desc), position(position),
      interactionRadius(interactionRadius)
{
    worldTransform = glm::translate(glm::mat4(1.0f), position);
}

void SceneObject::addPart(Mesh &&mesh, const Material &mat, const glm::mat4 &localTransform)
{
    MeshPart part;
    part.mesh = std::move(mesh);
    part.material = mat;
    part.localTransform = localTransform;
    parts.push_back(std::move(part));
}

void SceneObject::draw(Shader &shader) const
{
    for (const auto &part : parts)
    {
        glm::mat4 model = worldTransform * part.localTransform;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        shader.setUniformMatrix4fv("u_model", model);
        shader.setUniformMatrix3fv("u_normalMatrix", normalMatrix);

        shader.setUniform3f("material.ambient", part.material.ambient.r, part.material.ambient.g, part.material.ambient.b);
        shader.setUniform3f("material.diffuse", part.material.diffuse.r, part.material.diffuse.g, part.material.diffuse.b);
        shader.setUniform3f("material.specular", part.material.specular.r, part.material.specular.g, part.material.specular.b);
        shader.setUniform1f("material.shininess", part.material.shininess);

        // Bind texture if available
        if (part.material.textureID != 0)
        {
            shader.setUniform1i("u_useTexture", 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, part.material.textureID);
            shader.setUniform1i("u_texture", 0);
        }
        else
        {
            shader.setUniform1i("u_useTexture", 0);
        }

        part.mesh.draw();

        // Unbind texture
        if (part.material.textureID != 0)
        {
            glBindTexture(GL_TEXTURE_2D, 0);
            shader.setUniform1i("u_useTexture", 0);
        }
    }
}
