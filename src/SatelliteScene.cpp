#include "SatelliteScene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <string>
#include <algorithm>

const char* SatelliteScene::satNames[] = { "Aqua", "ICESat-2", "Landsat 8" };

// ─── helpers ────────────────────────────────────────────────────────────────

static Material makeMat(glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, float shine)
{
    Material m; m.ambient = amb; m.diffuse = diff; m.specular = spec; m.shininess = shine;
    return m;
}

// ─── constructor ────────────────────────────────────────────────────────────

SatelliteScene::SatelliteScene()
    : earthRotAngle(0.0f),
      moonOrbitAngle(0.0f),
      groundStationPos(EARTH_RADIUS + 0.3f, 0.0f, 0.0f),
      showScanBeam(false),
      showParticles(false),
      showOrbits(true),
      selectedSatellite(-1),
      timeScale(1.0f),
      paused(false),
      orbitShader(nullptr)
{
    // ── Sun directional light (smooth parallel rays, no attenuation) ────
    glm::vec3 sunPos(200.0f, 100.0f, 200.0f);
    sunLight = {
        glm::normalize(-sunPos),                // direction toward scene
        glm::vec3(0.08f, 0.08f, 0.10f),         // ambient
        glm::vec3(1.0f,  0.95f, 0.85f),         // diffuse
        glm::vec3(0.5f,  0.5f,  0.45f)          // specular
    };
    fillLight = {
        glm::vec3(-100.0f, -40.0f, -100.0f),
        glm::vec3(0.0f),
        glm::vec3(0.06f, 0.08f, 0.12f),
        glm::vec3(0.0f),
        1.0f, 0.0f, 0.0f
    };

    // ── Orbit definitions ─────────────────────────────────────────────────
    //  { startAngle, speed,  inclination (rad),  orbitRadius, pos }
    orbits.push_back({ 0.0f,          0.50f, glm::radians( 25.0f), 15.0f, {} });  // Aqua
    orbits.push_back({ glm::pi<float>(), 0.38f, glm::radians(-18.0f), 17.5f, {} });  // Terra
    orbits.push_back({ glm::half_pi<float>(), 0.72f, glm::radians( 10.0f), 12.5f, {} });  // ISS

    for (auto &o : orbits)
        o.pos = computeOrbitPos(o);

    // ── Per-satellite particle phases ─────────────────────────────────────
    for (int s = 0; s < (int)orbits.size(); s++)
    {
        std::vector<float> ph(NUM_PARTICLES);
        for (int i = 0; i < NUM_PARTICLES; i++)
            ph[i] = (float)i / NUM_PARTICLES;
        particleT.push_back(ph);
    }

    buildEarth();
    buildSatellites();
    buildGroundStation();
    buildSun();
    buildMoon();

    scanBeamMesh = Mesh::createCone(1.1f, 2.5f, 24);
    scanBeamMat  = makeMat({0.15f,0.7f,1.0f}, {0.0f,0.3f,0.8f}, {0.0f,0.0f,0.0f}, 1.0f);

    particleMesh = Mesh::createSphere(0.2f, 8, 6);
    particleMat  = makeMat({1.0f,0.85f,0.0f}, {0.4f,0.35f,0.0f}, {0.0f,0.0f,0.0f}, 1.0f);

    buildOrbitTrails();
    buildAtmosphere();
}

SatelliteScene::~SatelliteScene()
{
    for (auto &trail : orbitTrails) {
        glDeleteVertexArrays(1, &trail.VAO);
        glDeleteBuffers(1, &trail.VBO);
    }
    delete orbitShader;
}

// ─── orbit math ─────────────────────────────────────────────────────────────

glm::vec3 SatelliteScene::computeOrbitPos(const SatOrbit &o) const
{
    float x = o.radius * sinf(o.angle) * cosf(o.inclination);
    float y = o.radius * sinf(o.angle) * sinf(o.inclination);
    float z = o.radius * cosf(o.angle);
    return glm::vec3(x, y, z);
}

// ─── scene builders ─────────────────────────────────────────────────────────

void SatelliteScene::buildEarth()
{
    earth = std::make_unique<SceneObject>("Earth", "", glm::vec3(0.0f), 0.0f);

    // Load NASA Earth GLB (auto-normalized to 1 unit, scale to Earth diameter)
    auto parts = loadModel("../res/models/Earth_1_12756.glb", EARTH_RADIUS * 2.0f);
    if (!parts.empty())
    {
        for (auto &gm : parts)
        {
            Material mat;
            if (gm.textureID != 0)
            {
                // Textured: use white diffuse so texture shows at full brightness
                mat = makeMat({0.15f,0.15f,0.15f}, {1.0f,1.0f,1.0f}, {0.15f,0.20f,0.30f}, 24.0f);
            }
            else
            {
                glm::vec3 c = gm.color;
                mat = makeMat(c * 0.15f, c * 0.85f, {0.15f,0.20f,0.30f}, 24.0f);
            }
            mat.textureID = gm.textureID;
            earth->addPart(std::move(gm.mesh), mat);
        }
    }
    else
    {
        // Fallback: procedural sphere if GLB fails to load
        earth->addPart(
            Mesh::createSphere(EARTH_RADIUS, 64, 32),
            makeMat({0.04f,0.10f,0.28f}, {0.08f,0.32f,0.65f}, {0.15f,0.20f,0.30f}, 24.0f));
    }
}

void SatelliteScene::buildSatellites()
{
    struct SatDef { const char *name; const char *glbPath; float scale; };
    SatDef defs[] = {
        { "Aqua",      "../res/models/aqua.glb",     2.0f },
        { "ICESat-2",  "../res/models/icesat2.glb",  2.0f },
        { "Landsat 8", "../res/models/landsat8.glb", 2.5f },
    };

    for (int i = 0; i < (int)orbits.size(); i++)
    {
        auto obj = std::make_unique<SceneObject>(defs[i].name, "", orbits[i].pos, 0.0f);

        auto parts = loadGLBColored(defs[i].glbPath, defs[i].scale);
        if (!parts.empty())
        {
            for (auto &gm : parts)
            {
                glm::vec3 c = gm.color;
                Material mat = makeMat(c * 0.15f, c * 0.85f, {0.5f,0.5f,0.55f}, 32.0f);
                mat.textureID = gm.textureID;
                obj->addPart(std::move(gm.mesh), mat);
            }
        }
        else
        {
            // Fallback primitive geometry if GLB fails to load
            Material busBody = makeMat({0.18f,0.18f,0.20f}, {0.55f,0.55f,0.60f}, {0.8f,0.8f,0.9f}, 64.0f);
            Material panel   = makeMat({0.02f,0.04f,0.12f}, {0.05f,0.10f,0.45f}, {0.3f,0.3f,0.5f}, 48.0f);
            Material antMat  = makeMat({0.20f,0.20f,0.20f}, {0.60f,0.60f,0.60f}, {0.9f,0.9f,0.9f}, 80.0f);
            obj->addPart(Mesh::createBox(1.0f, 0.5f, 1.4f), busBody);
            glm::mat4 lp = glm::translate(glm::mat4(1.0f), glm::vec3(-1.9f, 0.0f, 0.0f));
            glm::mat4 rp = glm::translate(glm::mat4(1.0f), glm::vec3( 1.9f, 0.0f, 0.0f));
            obj->addPart(Mesh::createBox(2.6f, 0.04f, 0.8f), panel, lp);
            obj->addPart(Mesh::createBox(2.6f, 0.04f, 0.8f), panel, rp);
            glm::mat4 dt = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.4f, -0.4f));
            obj->addPart(Mesh::createDisc(0.4f, 16), antMat, dt);
        }

        satellites.push_back(std::move(obj));
    }
}

void SatelliteScene::buildGroundStation()
{
    groundStation = std::make_unique<SceneObject>("Ground Station", "", groundStationPos, 0.0f);

    float scale = 0.5f; // scale the station appropriately
    auto parts = loadModel("../res/models/ground_station/source/Ground Satellite 2.obj", scale);
    if (!parts.empty())
    {
        for (auto &gm : parts)
        {
            Material mat;
            if (gm.textureID != 0)
            {
                mat = makeMat({0.15f,0.15f,0.15f}, {0.8f,0.8f,0.8f}, {0.3f,0.3f,0.3f}, 32.0f);
            }
            else
            {
                glm::vec3 c = gm.color;
                mat = makeMat(c * 0.15f, c, {0.3f,0.3f,0.3f}, 32.0f);
            }
            mat.textureID = gm.textureID;
            groundStation->addPart(std::move(gm.mesh), mat);
        }
    }
    else
    {
        // Fallback if model fails to load
        Material base = makeMat({0.15f,0.15f,0.15f}, {0.50f,0.50f,0.50f}, {0.2f,0.2f,0.2f},  8.0f);
        Material dish = makeMat({0.25f,0.25f,0.25f}, {0.75f,0.75f,0.75f}, {0.9f,0.9f,0.9f}, 64.0f);

        groundStation->addPart(Mesh::createBox(0.36f, 0.24f, 0.36f), base);

        glm::mat4 mast = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.24f, 0.0f));
        groundStation->addPart(Mesh::createCylinder(0.03f, 0.36f, 8), base, mast);

        glm::mat4 dishT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.44f, 0.0f)) *
                          glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(1,0,0));
        groundStation->addPart(Mesh::createDisc(0.2f, 14), dish, dishT);
    }
}

void SatelliteScene::buildSun()
{
    glm::vec3 sunPos(200.0f, 100.0f, 200.0f);
    sun = std::make_unique<SceneObject>("Sun", "", sunPos, 0.0f);

    // Load Sun texture (extracted from USDZ)
    GLuint sunTex = loadTexture("../res/models/sun_texture.jpg");
    Material mat = makeMat({0.0f,0.0f,0.0f}, {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, 1.0f);
    mat.textureID = sunTex;
    sun->addPart(Mesh::createSphere(18.0f, 32, 16), mat);
}

void SatelliteScene::buildMoon()
{
    moon = std::make_unique<SceneObject>("Moon", "", glm::vec3(0.0f), 0.0f);

    // Moon diameter ~3474km, Earth ~12756km => ratio ~0.27
    // Earth radius in scene = 10, so Moon diameter = ~5.4 units
    float moonScale = EARTH_RADIUS * 2.0f * 0.27f;
    auto parts = loadModel("../res/models/Moon_NASA_LRO_8k_Topo_Small.glb", moonScale);
    if (!parts.empty())
    {
        for (auto &gm : parts)
        {
            Material mat;
            if (gm.textureID != 0)
            {
                mat = makeMat({0.10f,0.10f,0.10f}, {0.9f,0.9f,0.9f}, {0.1f,0.1f,0.1f}, 16.0f);
            }
            else
            {
                glm::vec3 c = gm.color;
                mat = makeMat(c * 0.12f, c * 0.80f, {0.1f,0.1f,0.1f}, 16.0f);
            }
            mat.textureID = gm.textureID;
            moon->addPart(std::move(gm.mesh), mat);
        }
    }
    else
    {
        // Fallback: grey procedural sphere
        moon->addPart(
            Mesh::createSphere(moonScale * 0.5f, 32, 16),
            makeMat({0.10f,0.10f,0.10f}, {0.6f,0.6f,0.6f}, {0.1f,0.1f,0.1f}, 16.0f));
    }
}

// ─── draw helper ────────────────────────────────────────────────────────────

void SatelliteScene::drawMesh(Shader &shader, const Mesh &mesh, const Material &mat, const glm::mat4 &model) const
{
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    shader.setUniformMatrix4fv("u_model",       model);
    shader.setUniformMatrix3fv("u_normalMatrix", normalMatrix);
    shader.setUniform3f("material.ambient",  mat.ambient.r,  mat.ambient.g,  mat.ambient.b);
    shader.setUniform3f("material.diffuse",  mat.diffuse.r,  mat.diffuse.g,  mat.diffuse.b);
    shader.setUniform3f("material.specular", mat.specular.r, mat.specular.g, mat.specular.b);
    shader.setUniform1f("material.shininess", mat.shininess);
    mesh.draw();
}

void SatelliteScene::setLights(Shader &shader) const
{
    // ── Directional light (Sun) ──────────────────────────────────────────
    shader.setUniform3f("dirLight.direction", sunLight.direction.x, sunLight.direction.y, sunLight.direction.z);
    shader.setUniform3f("dirLight.ambient",   sunLight.ambient.r,  sunLight.ambient.g,  sunLight.ambient.b);
    shader.setUniform3f("dirLight.diffuse",   sunLight.diffuse.r,  sunLight.diffuse.g,  sunLight.diffuse.b);
    shader.setUniform3f("dirLight.specular",  sunLight.specular.r, sunLight.specular.g, sunLight.specular.b);

    // ── Point lights ─────────────────────────────────────────────────────
    auto uploadPL = [&](int idx, const PointLight &l) {
        std::string b = "pointLights[" + std::to_string(idx) + "].";
        shader.setUniform3f(b+"position",  l.position.x,  l.position.y,  l.position.z);
        shader.setUniform3f(b+"ambient",   l.ambient.r,   l.ambient.g,   l.ambient.b);
        shader.setUniform3f(b+"diffuse",   l.diffuse.r,   l.diffuse.g,   l.diffuse.b);
        shader.setUniform3f(b+"specular",  l.specular.r,  l.specular.g,  l.specular.b);
        shader.setUniform1f(b+"constant",  l.constant);
        shader.setUniform1f(b+"linear",    l.linear);
        shader.setUniform1f(b+"quadratic", l.quadratic);
    };

    // [0] Fill light
    uploadPL(0, fillLight);

    // [1] Moonlight — reflected sunlight, intensity depends on lunar phase
    glm::vec3 moonPos = moon->getWorldTransform()[3]; // extract position
    glm::vec3 moonToEarth = glm::normalize(-moonPos);
    glm::vec3 sunDir = glm::normalize(-sunLight.direction); // direction FROM sun

    // Phase factor: how much of the lit hemisphere faces Earth
    // dot > 0 = Moon's lit face visible from Earth (gibbous/full), dot < 0 = crescent/new
    float phase = glm::clamp(glm::dot(sunDir, moonToEarth), 0.0f, 1.0f);
    float moonIntensity = phase * 0.12f; // subtle reflected glow

    PointLight moonLight = {
        moonPos,
        glm::vec3(0.0f),
        glm::vec3(0.08f, 0.10f, 0.14f) * moonIntensity,  // cool blue-white moonlight
        glm::vec3(0.0f),
        1.0f, 0.007f, 0.0002f
    };
    uploadPL(1, moonLight);

    shader.setUniform1i("numPointLights", 2);
}

// ─── update ─────────────────────────────────────────────────────────────────

void SatelliteScene::update(float dt)
{
    // Apply time control
    if (paused) dt = 0.0f;
    else dt *= timeScale;

    // Earth rotation
    earthRotAngle += 0.05f * dt;
    if (earthRotAngle > glm::two_pi<float>()) earthRotAngle -= glm::two_pi<float>();
    glm::mat4 earthRot = glm::rotate(glm::mat4(1.0f), earthRotAngle, glm::vec3(0,1,0));
    earth->setWorldTransform(earthRot);

    // Lock ground station to Indonesia (Lat: -2.5, Lon: 115) and rotate with Earth
    float lat = glm::radians(-2.5f);
    float lon = glm::radians(115.0f);
    float r   = EARTH_RADIUS + 0.05f; // sit on surface
    glm::vec3 basePos(
        r * cosf(lat) * sinf(lon),
        r * sinf(lat),
        r * cosf(lat) * cosf(lon)
    );
    
    groundStationPos = glm::vec3(earthRot * glm::vec4(basePos, 1.0f));
    
    // Orient the ground station model so "up" points away from Earth center
    glm::vec3 up = glm::normalize(groundStationPos);
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0,1,0), up));
    if (glm::length(right) < 0.001f) right = glm::vec3(1,0,0);
    glm::vec3 fwd = glm::cross(up, right);
    
    glm::mat4 gsOrient(1.0f);
    gsOrient[0] = glm::vec4(right, 0.0f);
    gsOrient[1] = glm::vec4(up,    0.0f);
    gsOrient[2] = glm::vec4(fwd,   0.0f);
    
    groundStation->setWorldTransform(glm::translate(glm::mat4(1.0f), groundStationPos) * gsOrient);

    // Each satellite orbits independently
    for (int i = 0; i < (int)orbits.size(); i++)
    {
        orbits[i].angle += orbits[i].speed * dt;
        if (orbits[i].angle > glm::two_pi<float>())
            orbits[i].angle -= glm::two_pi<float>();

        orbits[i].pos = computeOrbitPos(orbits[i]);

        // Orient satellite to always face Earth (nadir-pointing)
        glm::vec3 pos = orbits[i].pos;
        glm::vec3 toEarth = glm::normalize(-pos);
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        // Avoid degenerate case when satellite is directly above/below
        if (std::abs(glm::dot(toEarth, up)) > 0.99f)
            up = glm::vec3(0.0f, 0.0f, 1.0f);

        glm::vec3 right = glm::normalize(glm::cross(up, toEarth));
        glm::vec3 corrUp = glm::cross(toEarth, right);

        glm::mat4 orient(1.0f);
        orient[0] = glm::vec4(right,    0.0f);
        orient[1] = glm::vec4(corrUp,   0.0f);
        orient[2] = glm::vec4(-toEarth, 0.0f);  // -Z faces Earth

        satellites[i]->setWorldTransform(glm::translate(glm::mat4(1.0f), pos) * orient);
    }

    // Moon orbit (slower, farther out, slight inclination)
    moonOrbitAngle += 0.08f * dt;
    if (moonOrbitAngle > glm::two_pi<float>()) moonOrbitAngle -= glm::two_pi<float>();

    float moonRadius = 40.0f;
    float moonInc = glm::radians(5.1f);  // real Moon inclination ~5.1°
    glm::vec3 moonPos(
        moonRadius * cosf(moonOrbitAngle),
        moonRadius * sinf(moonOrbitAngle) * sinf(moonInc),
        moonRadius * sinf(moonOrbitAngle) * cosf(moonInc)
    );
    moon->setWorldTransform(glm::translate(glm::mat4(1.0f), moonPos));

    // Particle animation
    if (showParticles)
    {
        for (int s = 0; s < (int)particleT.size(); s++)
        {
            // Only animate packets if satellite has line-of-sight to ground station
            // Ray-sphere intersection: does the line from sat to ground hit Earth?
            glm::vec3 satPos = orbits[s].pos;
            glm::vec3 rayDir = glm::normalize(groundStationPos - satPos);
            float rayLen = glm::length(groundStationPos - satPos);

            // Closest point on ray to Earth center (origin)
            float tClosest = glm::dot(-satPos, rayDir);
            tClosest = glm::clamp(tClosest, 0.0f, rayLen);
            glm::vec3 closest = satPos + tClosest * rayDir;
            float distToCenter = glm::length(closest);

            bool hasLoS = (distToCenter > EARTH_RADIUS - 0.1f);

            for (int p = 0; p < NUM_PARTICLES; p++)
            {
                if (hasLoS)
                {
                    particleT[s][p] += 0.45f * dt;
                    if (particleT[s][p] > 1.0f) particleT[s][p] -= 1.0f;
                }
                // When no LoS, particles freeze — transmission paused
            }
        }
    }
}

// ─── render ─────────────────────────────────────────────────────────────────

void SatelliteScene::render(Shader &shader, const Camera &camera)
{
    shader.bind();
    shader.setUniform1i("u_useTexture", 0);
    shader.setUniform1f("u_alpha",      1.0f);
    shader.setUniform3f("u_emissive",   0.0f, 0.0f, 0.0f);
    shader.setUniformMatrix4fv("u_view",       camera.getViewMatrix());
    shader.setUniformMatrix4fv("u_projection", camera.getProjection());
    glm::vec3 cp = camera.getCameraPos();
    shader.setUniform3f("u_viewPos", cp.x, cp.y, cp.z);

    setLights(shader);

    // ── Spot lights (scan beams from satellites) ─────────────────────────
    if (showScanBeam)
    {
        shader.setUniform1i("numSpotLights", (int)orbits.size());
        for (int i = 0; i < (int)orbits.size(); i++)
        {
            std::string b = "spotLights[" + std::to_string(i) + "].";
            glm::vec3 dir = glm::normalize(-orbits[i].pos); // toward Earth
            shader.setUniform3f(b+"position",    orbits[i].pos.x, orbits[i].pos.y, orbits[i].pos.z);
            shader.setUniform3f(b+"direction",   dir.x, dir.y, dir.z);
            shader.setUniform3f(b+"diffuse",     0.4f, 0.8f, 1.0f);   // cyan-blue beam
            shader.setUniform3f(b+"specular",    0.3f, 0.5f, 0.7f);
            shader.setUniform1f(b+"cutOff",      glm::cos(glm::radians(8.0f)));   // inner cone
            shader.setUniform1f(b+"outerCutOff", glm::cos(glm::radians(14.0f)));  // soft outer edge
            shader.setUniform1f(b+"constant",    1.0f);
            shader.setUniform1f(b+"linear",      0.007f);
            shader.setUniform1f(b+"quadratic",   0.0002f);
        }
    }
    else
    {
        shader.setUniform1i("numSpotLights", 0);
    }

    // ── Opaque geometry ──────────────────────────────────────────────────
    earth->draw(shader);
    moon->draw(shader);
    groundStation->draw(shader);

    for (auto &sat : satellites)
        sat->draw(shader);

    // ── Sun (pure emissive) ──────────────────────────────────────────────
    shader.setUniform3f("u_emissive", 1.4f, 1.1f, 0.45f);
    sun->draw(shader);
    shader.setUniform3f("u_emissive", 0.0f, 0.0f, 0.0f);

    // ── Scan beams (semi-transparent, one per satellite) ─────────────────
    if (showScanBeam)
    {
        glDisable(GL_CULL_FACE);
        shader.setUniform1f("u_alpha", 0.28f);

        for (auto &o : orbits)
        {
            float satDist = glm::length(o.pos);
            float beamLen = satDist - EARTH_RADIUS;
            if (beamLen < 0.1f) beamLen = 0.1f;

            glm::vec3 earthToSat = glm::normalize(o.pos);
            glm::vec3 up(0.0f, 1.0f, 0.0f);
            float cosA    = glm::clamp(glm::dot(up, earthToSat), -1.0f, 1.0f);
            glm::vec3 ax  = glm::cross(up, earthToSat);
            glm::mat4 R   = (glm::length(ax) > 0.001f)
                            ? glm::rotate(glm::mat4(1.0f), acosf(cosA), glm::normalize(ax))
                            : (cosA > 0.0f ? glm::mat4(1.0f)
                                           : glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1,0,0)));
            glm::mat4 S   = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, beamLen / 2.5f, 1.0f));
            glm::vec3 T   = o.pos - beamLen * earthToSat;
            glm::mat4 M   = glm::translate(glm::mat4(1.0f), T) * R * S;

            drawMesh(shader, scanBeamMesh, scanBeamMat, M);
        }

        shader.setUniform1f("u_alpha", 1.0f);
        glEnable(GL_CULL_FACE);
    }

    // ── Data downlink (packets from satellite to ground station) ─────────
    if (showParticles)
    {
        shader.setUniform3f("u_emissive", 0.3f, 0.25f, 0.0f);

        for (int s = 0; s < (int)orbits.size(); s++)
        {
            glm::vec3 satPos = orbits[s].pos;
            glm::vec3 rayDir = glm::normalize(groundStationPos - satPos);
            float rayLen = glm::length(groundStationPos - satPos);

            // Line-of-sight check
            float tClosest = glm::dot(-satPos, rayDir);
            tClosest = glm::clamp(tClosest, 0.0f, rayLen);
            glm::vec3 closest = satPos + tClosest * rayDir;
            bool hasLoS = (glm::length(closest) > EARTH_RADIUS - 0.1f);

            if (!hasLoS) continue;

            // Draw a faint link line (thin beam)
            glDisable(GL_CULL_FACE);
            shader.setUniform1f("u_alpha", 0.12f);
            {
                // Thin line from sat to ground
                glm::vec3 mid = (satPos + groundStationPos) * 0.5f;
                glm::vec3 dir = glm::normalize(groundStationPos - satPos);
                glm::vec3 up(0,1,0);
                if (std::abs(glm::dot(dir, up)) > 0.99f) up = glm::vec3(1,0,0);
                glm::vec3 right = glm::normalize(glm::cross(up, dir));
                glm::vec3 corrUp = glm::cross(dir, right);

                glm::mat4 orient(1.0f);
                orient[0] = glm::vec4(right * 0.03f, 0.0f);
                orient[1] = glm::vec4(dir * (rayLen / 0.4f), 0.0f); // particleMesh diameter is 0.4
                orient[2] = glm::vec4(corrUp * 0.03f, 0.0f);
                glm::mat4 M = glm::translate(glm::mat4(1.0f), mid) * orient;
                drawMesh(shader, particleMesh, particleMat, M);
            }
            shader.setUniform1f("u_alpha", 1.0f);
            glEnable(GL_CULL_FACE);

            // Draw packets along the path
            for (int p = 0; p < NUM_PARTICLES; p++)
            {
                float t = particleT[s][p];

                // Arc path: push outward if packet dips below Earth surface
                glm::vec3 linear = glm::mix(satPos, groundStationPos, t);
                glm::vec3 midDir = glm::normalize(linear);
                float minAlt = EARTH_RADIUS + 0.5f;
                float currentAlt = glm::length(linear);
                glm::vec3 pPos = (currentAlt < minAlt)
                    ? midDir * minAlt
                    : linear;

                drawMesh(shader, particleMesh, particleMat,
                         glm::translate(glm::mat4(1.0f), pPos));
            }
        }

        shader.setUniform3f("u_emissive", 0.0f, 0.0f, 0.0f);
    }

    // ── Orbit trails ─────────────────────────────────────────────────────
    if (showOrbits)
    {
        // Animate orbit trail alpha (highlight selected)
        for (int i = 0; i < (int)orbitTrails.size(); i++)
        {
            if (selectedSatellite == i)
                orbitTrails[i].targetAlpha = 0.85f;
            else if (selectedSatellite >= 0)
                orbitTrails[i].targetAlpha = 0.12f;
            else
                orbitTrails[i].targetAlpha = 0.35f;

            float &a = orbitTrails[i].alpha;
            a += (orbitTrails[i].targetAlpha - a) * 0.08f;
        }

        glDisable(GL_CULL_FACE);
        glLineWidth(2.0f);

        Shader &os = *orbitShader;
        os.bind();
        os.setUniform1i("u_useTexture", 0);

        for (int i = 0; i < (int)orbitTrails.size(); i++)
        {
            auto &trail = orbitTrails[i];
            float pulse = (selectedSatellite == i) ? (0.85f + 0.15f * sinf((float)glfwGetTime() * 3.0f)) : 1.0f;
            os.setUniform4f("u_color",
                trail.color.r * pulse, trail.color.g * pulse, trail.color.b * pulse,
                trail.alpha);

            // Set matrices — orbit trails are in world space
            os.setUniformMatrix4fv("u_view", camera.getViewMatrix());
            os.setUniformMatrix4fv("u_projection", camera.getProjection());

            glBindVertexArray(trail.VAO);
            glDrawArrays(GL_LINE_LOOP, 0, trail.vertexCount);
            glBindVertexArray(0);
        }

        glLineWidth(1.0f);
        glEnable(GL_CULL_FACE);
    }

    // ── Atmosphere glow ───────────────────────────────────────────────────
    {
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive blending

        atmosphereShader->bind();
        float atmoScale = EARTH_RADIUS + 0.35f;
        glm::mat4 atmoModel = glm::scale(glm::mat4(1.0f), glm::vec3(atmoScale / EARTH_RADIUS));
        // Apply earth rotation to atmosphere too
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(atmoModel)));

        atmosphereShader->setUniformMatrix4fv("u_model", atmoModel);
        atmosphereShader->setUniformMatrix4fv("u_view", camera.getViewMatrix());
        atmosphereShader->setUniformMatrix4fv("u_projection", camera.getProjection());
        atmosphereShader->setUniformMatrix3fv("u_normalMatrix", normalMat);

        glm::vec3 cp2 = camera.getCameraPos();
        atmosphereShader->setUniform3f("u_viewPos", cp2.x, cp2.y, cp2.z);

        glm::vec3 sunDir = glm::normalize(-sunLight.direction);
        atmosphereShader->setUniform3f("u_sunDir", sunDir.x, sunDir.y, sunDir.z);
        atmosphereShader->setUniform1f("u_time", (float)glfwGetTime());

        atmosphereMesh.draw();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }
}

// ─── new builders ───────────────────────────────────────────────────────────

void SatelliteScene::buildOrbitTrails()
{
    orbitShader = new Shader("../res/shaders/orbit_line.vert", "../res/shaders/orbit_line.frag");

    glm::vec3 colors[] = {
        {0.0f, 0.85f, 1.0f},   // cyan  - Aqua
        {1.0f, 0.3f,  0.8f},   // magenta - ICESat-2
        {0.3f, 1.0f,  0.4f},   // green - Landsat 8
    };

    int segments = 120;

    for (int i = 0; i < (int)orbits.size(); i++)
    {
        std::vector<float> verts;
        verts.reserve(segments * 3);

        for (int s = 0; s < segments; s++)
        {
            float angle = glm::two_pi<float>() * s / segments;
            SatOrbit tempOrbit = orbits[i];
            tempOrbit.angle = angle;
            glm::vec3 pos = computeOrbitPos(tempOrbit);
            verts.push_back(pos.x);
            verts.push_back(pos.y);
            verts.push_back(pos.z);
        }

        OrbitTrail trail;
        trail.vertexCount = segments;
        trail.color = colors[i];
        trail.alpha = 0.35f;
        trail.targetAlpha = 0.35f;

        glGenVertexArrays(1, &trail.VAO);
        glGenBuffers(1, &trail.VBO);
        glBindVertexArray(trail.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, trail.VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        orbitTrails.push_back(trail);
    }
}

void SatelliteScene::buildAtmosphere()
{
    atmosphereShader = std::make_unique<Shader>(
        "../res/shaders/atmosphere.vert", "../res/shaders/atmosphere.frag");
    atmosphereMesh = Mesh::createSphere(EARTH_RADIUS, 64, 32);
}

// ─── satellite selection ────────────────────────────────────────────────────

void SatelliteScene::selectSatellite(int idx)
{
    if (idx >= 0 && idx < (int)orbits.size())
        selectedSatellite = idx;
    else
        selectedSatellite = -1;
}

SatInfo SatelliteScene::getSelectedInfo() const
{
    SatInfo info;
    if (selectedSatellite < 0 || selectedSatellite >= (int)orbits.size())
    {
        info.name = "Free Camera";
        info.altitude = 0; info.speed = 0; info.hasLoS = false;
        return info;
    }

    const auto &o = orbits[selectedSatellite];
    info.name = satNames[selectedSatellite];
    info.altitude = glm::length(o.pos) - EARTH_RADIUS;
    info.speed = o.speed;

    // Line-of-sight check to ground station
    glm::vec3 rayDir = glm::normalize(groundStationPos - o.pos);
    float rayLen = glm::length(groundStationPos - o.pos);
    float tClosest = glm::dot(-o.pos, rayDir);
    tClosest = glm::clamp(tClosest, 0.0f, rayLen);
    glm::vec3 closest = o.pos + tClosest * rayDir;
    info.hasLoS = (glm::length(closest) > EARTH_RADIUS - 0.1f);

    return info;
}

glm::vec3 SatelliteScene::getSatellitePos(int idx) const
{
    if (idx >= 0 && idx < (int)orbits.size())
        return orbits[idx].pos;
    return glm::vec3(0.0f);
}

glm::vec3 SatelliteScene::getSatelliteFront(int idx) const
{
    if (idx >= 0 && idx < (int)orbits.size())
        return glm::normalize(-orbits[idx].pos); // facing Earth
    return glm::vec3(0.0f, 0.0f, -1.0f);
}

// ─── time control ───────────────────────────────────────────────────────────

void SatelliteScene::increaseTimeScale()
{
    const float steps[] = {0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
    for (float s : steps) { if (s > timeScale + 0.01f) { timeScale = s; return; } }
}

void SatelliteScene::decreaseTimeScale()
{
    const float steps[] = {10.0f, 5.0f, 2.0f, 1.0f, 0.5f, 0.25f};
    for (float s : steps) { if (s < timeScale - 0.01f) { timeScale = s; return; } }
}

void SatelliteScene::resetAll()
{
    timeScale = 1.0f;
    paused = false;
    selectedSatellite = -1;
    showScanBeam = false;
    showParticles = false;
    showOrbits = true;
}

void SatelliteScene::renderMinimap(const Camera &camera, int screenW, int screenH)
{
    int mapSize = 160;
    int mapX = screenW - mapSize - 20;
    int mapY = screenH - (mapSize * 2);  // 2 kali ukuran minimap dari atas

    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glViewport(mapX, mapY, mapSize, mapSize);
    glScissor(mapX, mapY, mapSize, mapSize);
    glEnable(GL_SCISSOR_TEST);

    // Dark background
    glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float viewRange = 25.0f;
    glm::mat4 orthoProj = glm::ortho(-viewRange, viewRange, -viewRange, viewRange, -100.0f, 100.0f);
    glm::mat4 topView = glm::lookAt(glm::vec3(0, 60, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));

    if (!orbitShader) { glDisable(GL_SCISSOR_TEST); glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]); return; }

    orbitShader->bind();
    orbitShader->setUniformMatrix4fv("u_view", topView);
    orbitShader->setUniformMatrix4fv("u_projection", orthoProj);

    // ── Radar grid rings ─────────────────────────────────────────────────
    {
        const int ringSegs = 64;
        float radii[] = { EARTH_RADIUS, EARTH_RADIUS + 2.5f, EARTH_RADIUS + 5.0f, EARTH_RADIUS + 7.5f };
        float alphas[] = { 0.35f, 0.12f, 0.12f, 0.12f };

        for (int r = 0; r < 4; r++)
        {
            std::vector<float> ring;
            ring.reserve(ringSegs * 3);
            for (int s = 0; s < ringSegs; s++)
            {
                float a = glm::two_pi<float>() * s / ringSegs;
                ring.push_back(radii[r] * cosf(a));
                ring.push_back(0.0f);
                ring.push_back(radii[r] * sinf(a));
            }

            GLuint vao, vbo;
            glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, ring.size() * sizeof(float), ring.data(), GL_STREAM_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(0);

            float c = (r == 0) ? 0.25f : 0.08f;
            orbitShader->setUniform4f("u_color", 0.15f, c + 0.2f, 0.5f, alphas[r]);
            glDrawArrays(GL_LINE_LOOP, 0, ringSegs);
            glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo);
        }

        // Cross-hair lines
        float crossLen = viewRange * 0.9f;
        float crossVerts[] = {
            -crossLen, 0, 0,   crossLen, 0, 0,
            0, 0, -crossLen,   0, 0, crossLen
        };
        GLuint cvao, cvbo;
        glGenVertexArrays(1, &cvao); glGenBuffers(1, &cvbo);
        glBindVertexArray(cvao);
        glBindBuffer(GL_ARRAY_BUFFER, cvbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(crossVerts), crossVerts, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        orbitShader->setUniform4f("u_color", 0.1f, 0.25f, 0.4f, 0.15f);
        glDrawArrays(GL_LINES, 0, 4);
        glDeleteVertexArrays(1, &cvao); glDeleteBuffers(1, &cvbo);

        // Radar sweep line (rotating)
        float sweepAngle = (float)glfwGetTime() * 1.5f;
        float sx = viewRange * 0.85f * cosf(sweepAngle);
        float sz = viewRange * 0.85f * sinf(sweepAngle);
        float sweepV[] = { 0,0,0,  sx, 0, sz };
        GLuint svao, svbo;
        glGenVertexArrays(1, &svao); glGenBuffers(1, &svbo);
        glBindVertexArray(svao);
        glBindBuffer(GL_ARRAY_BUFFER, svbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(sweepV), sweepV, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        orbitShader->setUniform4f("u_color", 0.1f, 0.8f, 0.3f, 0.25f);
        glDrawArrays(GL_LINES, 0, 2);
        glDeleteVertexArrays(1, &svao); glDeleteBuffers(1, &svbo);
    }

    // ── Orbit trails ─────────────────────────────────────────────────────
    glLineWidth(1.5f);
    for (int i = 0; i < (int)orbitTrails.size(); i++)
    {
        auto &trail = orbitTrails[i];
        float a = (selectedSatellite == i) ? 0.85f : 0.3f;
        orbitShader->setUniform4f("u_color", trail.color.r, trail.color.g, trail.color.b, a);
        glBindVertexArray(trail.VAO);
        glDrawArrays(GL_LINE_LOOP, 0, trail.vertexCount);
        glBindVertexArray(0);
    }

    // ── Points (satellites + ground station) ─────────────────────────────
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Helper lambda to draw a single point
    auto drawPoint = [&](float x, float y, float z, float r, float g, float b, float a, float) {
        float pos[] = { x, y, z };
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        orbitShader->setUniform4f("u_color", r, g, b, a);
        glDrawArrays(GL_POINTS, 0, 1);
        glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo);
    };

    // Earth center (blue-green dot)
    glPointSize(10.0f);
    drawPoint(0, 0, 0, 0.15f, 0.4f, 0.7f, 0.8f, 10);

    // Satellites
    for (int i = 0; i < (int)orbits.size(); i++)
    {
        bool sel = (selectedSatellite == i);
        float sz = sel ? 8.0f : 5.0f;
        float bright = sel ? 1.0f : 0.75f;
        glPointSize(sz);
        drawPoint(orbits[i].pos.x, orbits[i].pos.y, orbits[i].pos.z,
                  orbitTrails[i].color.r * bright,
                  orbitTrails[i].color.g * bright,
                  orbitTrails[i].color.b * bright, 1.0f, sz);
    }

    // Ground station (yellow diamond)
    glPointSize(6.0f);
    drawPoint(groundStationPos.x, groundStationPos.y, groundStationPos.z,
              1.0f, 0.85f, 0.1f, 0.9f, 6);

    glDisable(GL_PROGRAM_POINT_SIZE);
    glLineWidth(1.0f);

    // ── Border frame ─────────────────────────────────────────────────────
    {
        float b = viewRange * 0.98f;
        float borderV[] = {
            -b, 0, -b,   b, 0, -b,
             b, 0, -b,   b, 0,  b,
             b, 0,  b,  -b, 0,  b,
            -b, 0,  b,  -b, 0, -b
        };
        GLuint bvao, bvbo;
        glGenVertexArrays(1, &bvao); glGenBuffers(1, &bvbo);
        glBindVertexArray(bvao);
        glBindBuffer(GL_ARRAY_BUFFER, bvbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(borderV), borderV, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        orbitShader->setUniform4f("u_color", 0.15f, 0.5f, 0.8f, 0.6f);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, 8);
        glDeleteVertexArrays(1, &bvao); glDeleteBuffers(1, &bvbo);
        glLineWidth(1.0f);
    }

    glDisable(GL_SCISSOR_TEST);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

