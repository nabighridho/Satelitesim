#include "Mesh.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Mesh::~Mesh()
{
    if (VAO)
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
}

Mesh::Mesh(Mesh &&other) noexcept
    : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), indexCount(other.indexCount)
{
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.indexCount = 0;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept
{
    if (this != &other)
    {
        if (VAO)
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        indexCount = other.indexCount;
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.indexCount = 0;
    }
    return *this;
}

void Mesh::upload(const std::vector<PhongVertex> &vertices,
                  const std::vector<unsigned int> &indices)
{
    indexCount = indices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(PhongVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PhongVertex), (void *)offsetof(PhongVertex, position));
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PhongVertex), (void *)offsetof(PhongVertex, normal));
    glEnableVertexAttribArray(1);
    // texCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(PhongVertex), (void *)offsetof(PhongVertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::draw() const
{
    if (VAO == 0)
        return;
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

Mesh Mesh::createBox(float width, float height, float depth)
{
    float hw = width / 2.0f;
    float hh = height / 2.0f;
    float hd = depth / 2.0f;

    std::vector<PhongVertex> verts;
    std::vector<unsigned int> inds;

    auto addFace = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 normal) {
        unsigned int base = verts.size();
        verts.push_back({p0, normal, {0.0f, 0.0f}});
        verts.push_back({p1, normal, {1.0f, 0.0f}});
        verts.push_back({p2, normal, {1.0f, 1.0f}});
        verts.push_back({p3, normal, {0.0f, 1.0f}});
        inds.push_back(base + 0);
        inds.push_back(base + 1);
        inds.push_back(base + 2);
        inds.push_back(base + 2);
        inds.push_back(base + 3);
        inds.push_back(base + 0);
    };

    // Front (+Z)
    addFace({-hw, -hh, hd}, {hw, -hh, hd}, {hw, hh, hd}, {-hw, hh, hd}, {0, 0, 1});
    // Back (-Z)
    addFace({hw, -hh, -hd}, {-hw, -hh, -hd}, {-hw, hh, -hd}, {hw, hh, -hd}, {0, 0, -1});
    // Right (+X)
    addFace({hw, -hh, hd}, {hw, -hh, -hd}, {hw, hh, -hd}, {hw, hh, hd}, {1, 0, 0});
    // Left (-X)
    addFace({-hw, -hh, -hd}, {-hw, -hh, hd}, {-hw, hh, hd}, {-hw, hh, -hd}, {-1, 0, 0});
    // Top (+Y)
    addFace({-hw, hh, hd}, {hw, hh, hd}, {hw, hh, -hd}, {-hw, hh, -hd}, {0, 1, 0});
    // Bottom (-Y)
    addFace({-hw, -hh, -hd}, {hw, -hh, -hd}, {hw, -hh, hd}, {-hw, -hh, hd}, {0, -1, 0});

    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}

Mesh Mesh::createPlane(float width, float depth, float texRepeat)
{
    float hw = width / 2.0f;
    float hd = depth / 2.0f;

    std::vector<PhongVertex> verts = {
        {{-hw, 0, hd}, {0, 1, 0}, {0, 0}},
        {{hw, 0, hd}, {0, 1, 0}, {texRepeat, 0}},
        {{hw, 0, -hd}, {0, 1, 0}, {texRepeat, texRepeat}},
        {{-hw, 0, -hd}, {0, 1, 0}, {0, texRepeat}},
    };
    std::vector<unsigned int> inds = {0, 1, 2, 2, 3, 0};

    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}

Mesh Mesh::createCylinder(float radius, float height, int segments)
{
    std::vector<PhongVertex> verts;
    std::vector<unsigned int> inds;
    float halfH = height / 2.0f;

    // Side vertices
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle);
        float z = sin(angle);
        float u = (float)i / segments;

        glm::vec3 normal = glm::vec3(x, 0.0f, z);
        verts.push_back({{radius * x, -halfH, radius * z}, normal, {u, 0.0f}});
        verts.push_back({{radius * x, halfH, radius * z}, normal, {u, 1.0f}});
    }

    // Side indices
    for (int i = 0; i < segments; i++)
    {
        unsigned int b = i * 2;
        inds.push_back(b);
        inds.push_back(b + 1);
        inds.push_back(b + 3);
        inds.push_back(b);
        inds.push_back(b + 3);
        inds.push_back(b + 2);
    }

    // Top cap
    unsigned int topCenter = verts.size();
    verts.push_back({{0, halfH, 0}, {0, 1, 0}, {0.5f, 0.5f}});
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle);
        float z = sin(angle);
        verts.push_back({{radius * x, halfH, radius * z}, {0, 1, 0}, {0.5f + 0.5f * x, 0.5f + 0.5f * z}});
    }
    for (int i = 0; i < segments; i++)
    {
        inds.push_back(topCenter);
        inds.push_back(topCenter + 1 + i);
        inds.push_back(topCenter + 2 + i);
    }

    // Bottom cap
    unsigned int botCenter = verts.size();
    verts.push_back({{0, -halfH, 0}, {0, -1, 0}, {0.5f, 0.5f}});
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle);
        float z = sin(angle);
        verts.push_back({{radius * x, -halfH, radius * z}, {0, -1, 0}, {0.5f + 0.5f * x, 0.5f + 0.5f * z}});
    }
    for (int i = 0; i < segments; i++)
    {
        inds.push_back(botCenter);
        inds.push_back(botCenter + 2 + i);
        inds.push_back(botCenter + 1 + i);
    }

    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}

Mesh Mesh::createCone(float radius, float height, int segments)
{
    std::vector<PhongVertex> verts;
    std::vector<unsigned int> inds;

    float slope = radius / height;

    // Side
    unsigned int tipIdx = 0;
    verts.push_back({{0, height, 0}, {0, 1, 0}, {0.5f, 1.0f}});

    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle);
        float z = sin(angle);
        glm::vec3 normal = glm::normalize(glm::vec3(x, slope, z));
        verts.push_back({{radius * x, 0, radius * z}, normal, {(float)i / segments, 0.0f}});
    }
    for (int i = 0; i < segments; i++)
    {
        inds.push_back(tipIdx);
        inds.push_back(1 + i + 1);
        inds.push_back(1 + i);
    }

    // Bottom cap
    unsigned int botCenter = verts.size();
    verts.push_back({{0, 0, 0}, {0, -1, 0}, {0.5f, 0.5f}});
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle);
        float z = sin(angle);
        verts.push_back({{radius * x, 0, radius * z}, {0, -1, 0}, {0.5f + 0.5f * x, 0.5f + 0.5f * z}});
    }
    for (int i = 0; i < segments; i++)
    {
        inds.push_back(botCenter);
        inds.push_back(botCenter + 2 + i);
        inds.push_back(botCenter + 1 + i);
    }

    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}

Mesh Mesh::createDisc(float radius, int segments)
{
    std::vector<PhongVertex> verts;
    std::vector<unsigned int> inds;

    verts.push_back({{0, 0, 0}, {0, 1, 0}, {0.5f, 0.5f}});
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cos(angle);
        float z = sin(angle);
        verts.push_back({{radius * x, 0, radius * z}, {0, 1, 0}, {0.5f + 0.5f * x, 0.5f + 0.5f * z}});
    }
    for (int i = 0; i < segments; i++)
    {
        inds.push_back(0);
        inds.push_back(1 + i);
        inds.push_back(2 + i);
    }

    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}

Mesh Mesh::createSphere(float radius, int segments, int rings)
{
    std::vector<PhongVertex> verts;
    std::vector<unsigned int> inds;

    for (int ring = 0; ring <= rings; ring++)
    {
        float phi = M_PI * ring / rings;
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        for (int seg = 0; seg <= segments; seg++)
        {
            float theta = 2.0f * M_PI * seg / segments;
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);

            glm::vec3 pos(radius * sinPhi * cosTheta,
                          radius * cosPhi,
                          radius * sinPhi * sinTheta);
            glm::vec3 normal = glm::normalize(pos);
            glm::vec2 uv((float)seg / segments, (float)ring / rings);
            verts.push_back({pos, normal, uv});
        }
    }

    for (int ring = 0; ring < rings; ring++)
    {
        for (int seg = 0; seg < segments; seg++)
        {
            unsigned int a = ring * (segments + 1) + seg;
            unsigned int b = a + 1;
            unsigned int c = a + (segments + 1);
            unsigned int d = c + 1;

            inds.push_back(a); inds.push_back(c); inds.push_back(b);
            inds.push_back(b); inds.push_back(c); inds.push_back(d);
        }
    }

    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}

// ── Assimp-based model loader ──────────────────────────────────────────

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stb_image.h>

/// Load a diffuse texture from an Assimp material.
/// Handles embedded textures (GLB) and file-path textures.
/// Returns OpenGL texture ID, or 0 if no texture found.
static GLuint extractTexture(const aiMaterial *mat, const aiScene *scene, const std::string &modelDir)
{
    aiString texPath;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) != AI_SUCCESS)
        return 0;

    std::string path(texPath.C_Str());

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *data = nullptr;

    // Check for embedded texture (path starts with '*')
    if (path.size() > 0 && path[0] == '*')
    {
        int texIndex = std::atoi(path.c_str() + 1);
        if (texIndex >= 0 && (unsigned)texIndex < scene->mNumTextures)
        {
            const aiTexture *aiTex = scene->mTextures[texIndex];
            if (aiTex->mHeight == 0)
            {
                // Compressed format (JPEG/PNG) — decode from memory
                data = stbi_load_from_memory(
                    reinterpret_cast<const unsigned char *>(aiTex->pcData),
                    aiTex->mWidth, &width, &height, &channels, 0);
            }
            else
            {
                // Raw RGBA pixels
                width = aiTex->mWidth;
                height = aiTex->mHeight;
                channels = 4;
                data = reinterpret_cast<unsigned char *>(aiTex->pcData);
            }
        }
    }
    else
    {
        // External file texture
        std::string fullPath = modelDir + path;
        data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);
    }

    if (!data)
    {
        fprintf(stderr, "  Failed to load texture: %s\n", path.c_str());
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 4) format = GL_RGBA;
    else if (channels == 1) format = GL_RED;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Only free if stbi allocated (not raw embedded pointer)
    if (path[0] == '*')
    {
        const aiTexture *aiTex = scene->mTextures[std::atoi(path.c_str() + 1)];
        if (aiTex->mHeight == 0)
            stbi_image_free(data);
    }
    else
    {
        stbi_image_free(data);
    }

    fprintf(stderr, "  Loaded texture: %s (%dx%d, %d ch)\n", path.c_str(), width, height, channels);
    return texID;
}

/// Extract the diffuse base color from an Assimp material.
/// Falls back to name-based heuristics when the color is missing or white
/// (texture-only materials from Sketchfab exports, etc.).
static glm::vec3 extractColor(const aiMaterial *mat)
{
    aiColor4D color(0.6f, 0.6f, 0.6f, 1.0f);
    bool hasColor = (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS);

    // Treat near-white as "no real color" (common in texture-only materials)
    bool isWhite = (!hasColor ||
                    (color.r > 0.85f && color.g > 0.85f && color.b > 0.85f));

    if (!isWhite)
        return glm::vec3(color.r, color.g, color.b);

    // Name-based color fallback for texture-only materials
    aiString aiName;
    mat->Get(AI_MATKEY_NAME, aiName);
    std::string name(aiName.C_Str());
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if      (name.find("gold")    != std::string::npos) return {0.75f, 0.55f, 0.08f};
    else if (name.find("silver")  != std::string::npos) return {0.65f, 0.65f, 0.70f};
    else if (name.find("solar")   != std::string::npos ||
             name.find("panel")   != std::string::npos) return {0.04f, 0.08f, 0.30f};
    else if (name.find("foil")    != std::string::npos ||
             name.find("thermal") != std::string::npos) return {0.60f, 0.55f, 0.45f};
    else if (name.find("white")   != std::string::npos) return {0.85f, 0.85f, 0.85f};
    else if (name.find("black")   != std::string::npos) return {0.08f, 0.08f, 0.10f};
    else if (name.find("alum")    != std::string::npos ||
             name.find("metal")   != std::string::npos) return {0.70f, 0.70f, 0.72f};

    return {0.55f, 0.55f, 0.60f}; // default grey
}

std::vector<GltfMesh> loadModel(const std::string &path, float scale)
{
    std::vector<GltfMesh> result;

    // Compute directory for resolving relative texture paths
    std::string modelDir = path;
    size_t lastSlash = modelDir.find_last_of("/\\");
    modelDir = (lastSlash != std::string::npos) ? modelDir.substr(0, lastSlash + 1) : "";

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path,
        aiProcess_Triangulate           |
        aiProcess_GenSmoothNormals      |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices  |
        aiProcess_SortByPType);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        fprintf(stderr, "Assimp error loading '%s': %s\n",
                path.c_str(), importer.GetErrorString());
        return result;
    }

    // ── First pass: collect raw mesh data & compute global bounding box ──
    struct RawMesh {
        std::vector<PhongVertex> verts;
        std::vector<unsigned int> indices;
        glm::vec3 color;
        GLuint textureID = 0;
    };

    std::vector<RawMesh> rawMeshes;
    glm::vec3 bboxMin( 1e18f);
    glm::vec3 bboxMax(-1e18f);

    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        const aiMesh *aiM = scene->mMeshes[m];
        if (!(aiM->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
            continue;

        int vertCount = (int)aiM->mNumVertices;

        RawMesh raw;
        raw.verts.resize(vertCount);

        for (int i = 0; i < vertCount; i++)
        {
            const aiVector3D &p = aiM->mVertices[i];
            glm::vec3 pos(p.x, p.y, p.z);
            raw.verts[i].position = pos;

            // Track bounding box
            bboxMin = glm::min(bboxMin, pos);
            bboxMax = glm::max(bboxMax, pos);

            if (aiM->HasNormals())
            {
                const aiVector3D &n = aiM->mNormals[i];
                raw.verts[i].normal = glm::normalize(glm::vec3(n.x, n.y, n.z));
            }
            else
            {
                raw.verts[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            if (aiM->HasTextureCoords(0))
            {
                const aiVector3D &t = aiM->mTextureCoords[0][i];
                raw.verts[i].texCoord = glm::vec2(t.x, t.y);
            }
            else
            {
                raw.verts[i].texCoord = glm::vec2(0.0f);
            }
        }

        // Build index buffer
        raw.indices.reserve(aiM->mNumFaces * 3);
        for (unsigned int f = 0; f < aiM->mNumFaces; f++)
        {
            const aiFace &face = aiM->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                raw.indices.push_back(face.mIndices[j]);
        }

        // Extract material color and texture
        raw.color = glm::vec3(0.55f, 0.55f, 0.60f);
        if (aiM->mMaterialIndex < scene->mNumMaterials)
        {
            const aiMaterial *aiMat = scene->mMaterials[aiM->mMaterialIndex];
            raw.color = extractColor(aiMat);
            raw.textureID = extractTexture(aiMat, scene, modelDir);
        }

        rawMeshes.push_back(std::move(raw));
    }

    if (rawMeshes.empty())
        return result;

    // ── Compute normalization: center model and fit to unit size ─────────
    glm::vec3 center = (bboxMin + bboxMax) * 0.5f;
    glm::vec3 extent = bboxMax - bboxMin;
    float maxExtent  = std::max({extent.x, extent.y, extent.z, 0.0001f});
    float normalize  = 1.0f / maxExtent;   // fits model into a 1-unit cube

    fprintf(stderr, "  bbox: (%.2f,%.2f,%.2f)-(%.2f,%.2f,%.2f)  maxExtent=%.4f\n",
            bboxMin.x, bboxMin.y, bboxMin.z,
            bboxMax.x, bboxMax.y, bboxMax.z, maxExtent);

    // ── Second pass: normalize, scale, and upload ────────────────────────
    for (auto &raw : rawMeshes)
    {
        for (auto &v : raw.verts)
            v.position = (v.position - center) * normalize * scale;

        Mesh glMesh;
        glMesh.upload(raw.verts, raw.indices);

        GltfMesh gm;
        gm.mesh      = std::move(glMesh);
        gm.color     = raw.color;
        gm.textureID = raw.textureID;
        result.push_back(std::move(gm));
    }

    fprintf(stderr, "Assimp loaded: %s (%zu mesh primitives, scale=%.6f)\n",
            path.c_str(), result.size(), scale);
    return result;
}

// Backward-compatible alias
std::vector<GltfMesh> loadGLBColored(const std::string &glbPath, float scale)
{
    return loadModel(glbPath, scale);
}

GLuint loadTexture(const std::string &path)
{
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data)
    {
        fprintf(stderr, "Failed to load texture file: %s\n", path.c_str());
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 4) format = GL_RGBA;
    else if (channels == 1) format = GL_RED;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    fprintf(stderr, "Loaded texture file: %s (%dx%d, %d ch)\n", path.c_str(), width, height, channels);
    return texID;
}
