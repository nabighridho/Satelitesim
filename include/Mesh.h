#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct PhongVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh {
private:
    GLuint VAO, VBO, EBO;
    unsigned int indexCount;

public:
    void upload(const std::vector<PhongVertex> &vertices,
                const std::vector<unsigned int> &indices);
    Mesh() : VAO(0), VBO(0), EBO(0), indexCount(0) {}
    ~Mesh();

    static Mesh createBox(float width, float height, float depth);
    static Mesh createCylinder(float radius, float height, int segments = 16);
    static Mesh createCone(float radius, float height, int segments = 16);
    static Mesh createPlane(float width, float depth, float texRepeat = 1.0f);
    static Mesh createDisc(float radius, int segments = 16);
    static Mesh createSphere(float radius, int segments = 32, int rings = 16);

    void draw() const;

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;
    Mesh(Mesh &&other) noexcept;
    Mesh &operator=(Mesh &&other) noexcept;
};

struct GltfMesh {
    Mesh mesh;
    glm::vec3 color;
    GLuint textureID = 0;  // 0 = no texture
};

/// Load any 3D model file via Assimp (GLB, glTF, STL, OBJ, FBX, …).
/// Returns one GltfMesh per mesh primitive, each with its diffuse color.
std::vector<GltfMesh> loadModel(const std::string &path, float scale = 1.0f);

/// Backward-compatible alias — calls loadModel() internally.
std::vector<GltfMesh> loadGLBColored(const std::string &glbPath, float scale = 1.0f);

/// Load a texture from a file path and return its OpenGL texture ID.
GLuint loadTexture(const std::string &path);
