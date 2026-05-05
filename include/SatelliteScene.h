#pragma once
#include <memory>
#include <vector>
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

    glm::vec3 computeOrbitPos(const SatOrbit &o) const;
    void buildEarth();
    void buildSatellites();
    void buildGroundStation();
    void buildSun();
    void buildMoon();
    void drawMesh(Shader &s, const Mesh &m, const Material &mat, const glm::mat4 &model) const;
    void setLights(Shader &shader) const;

public:
    SatelliteScene();

    void update(float dt);
    void render(Shader &shader, const Camera &camera);

    void toggleScanBeam()     { showScanBeam  = !showScanBeam; }
    void toggleTransmission() { showParticles = !showParticles; }
    bool isScanBeamOn()       const { return showScanBeam; }
    bool isTransmissionOn()   const { return showParticles; }

    int  numSatellites()      const { return (int)satellites.size(); }
};
