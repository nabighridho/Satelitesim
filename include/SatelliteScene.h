#pragma once
#include <memory>
#include <vector>
#include <string>
#include "SceneObject.h"
#include "Light.h"
#include "Camera.h"
#include "shader.h"

struct SatOrbit {
    float angle;
    float speed;
    float inclination;
    float radius;
    glm::vec3 pos;
};

struct SatInfo {
    std::string name;
    float altitude;    // distance from Earth surface
    float speed;       // orbital speed
    bool hasLoS;       // line of sight to ground station
};

class SatelliteScene
{
public:
    static constexpr float EARTH_RADIUS = 10.0f;
    static constexpr int   NUM_PARTICLES = 5;

private:
    std::unique_ptr<SceneObject>              earth;
    std::unique_ptr<SceneObject>              moon;
    std::vector<std::unique_ptr<SceneObject>> satellites;
    std::unique_ptr<SceneObject>              groundStation;
    std::unique_ptr<SceneObject>              sun;

    std::vector<SatOrbit> orbits;
    std::vector<std::vector<float>> particleT;   // [satIdx][particleIdx]

    Mesh     scanBeamMesh;
    Material scanBeamMat;
    Mesh     particleMesh;
    Material particleMat;

    DirLight   sunLight;
    PointLight fillLight;

    float      earthRotAngle;
    float      moonOrbitAngle;
    glm::vec3  groundStationPos;

    bool showScanBeam;
    bool showParticles;

    // ── New features ──
    bool showOrbits;
    int  selectedSatellite;   // -1 = none selected
    float timeScale;
    bool  paused;

    // Orbit trail rendering
    struct OrbitTrail {
        GLuint VAO, VBO;
        int vertexCount;
        glm::vec3 color;
        float alpha;        // current alpha (animated)
        float targetAlpha;  // target alpha
    };
    std::vector<OrbitTrail> orbitTrails;
    Shader *orbitShader;   // reuses overlay shader

    // Atmosphere
    Mesh atmosphereMesh;
    std::unique_ptr<Shader> atmosphereShader;

    glm::vec3 computeOrbitPos(const SatOrbit &o) const;
    void buildEarth();
    void buildSatellites();
    void buildGroundStation();
    void buildSun();
    void buildMoon();
    void buildOrbitTrails();
    void buildAtmosphere();
    void drawMesh(Shader &s, const Mesh &m, const Material &mat, const glm::mat4 &model) const;
    void setLights(Shader &shader) const;

    // Satellite names
    static const char* satNames[];

public:
    SatelliteScene();
    ~SatelliteScene();

    void update(float dt);
    void render(Shader &shader, const Camera &camera);
    void renderMinimap(const Camera &camera, int screenW, int screenH);

    void toggleScanBeam()     { showScanBeam  = !showScanBeam; }
    void toggleTransmission() { showParticles = !showParticles; }
    bool isScanBeamOn()       const { return showScanBeam; }
    bool isTransmissionOn()   const { return showParticles; }

    // Orbit trails
    void toggleOrbits()       { showOrbits = !showOrbits; }
    bool isOrbitsOn()         const { return showOrbits; }

    // Satellite selection
    void selectSatellite(int idx);
    int  getSelectedSatellite() const { return selectedSatellite; }
    SatInfo getSelectedInfo() const;
    glm::vec3 getSatellitePos(int idx) const;
    glm::vec3 getSatelliteFront(int idx) const;

    // Time control
    void increaseTimeScale();
    void decreaseTimeScale();
    void togglePause()        { paused = !paused; }
    float getTimeScale()      const { return timeScale; }
    bool  isPaused()          const { return paused; }

    // Reset
    void resetAll();

    int  numSatellites()      const { return (int)satellites.size(); }
};
