#ifndef ELECTRON_CLOUD_HPP
#define ELECTRON_CLOUD_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "Physics.hpp"

enum class OrbitalType { 
    ORBITAL_1S, ORBITAL_2S,
    ORBITAL_2PX, ORBITAL_2PY, ORBITAL_2PZ,
    ORBITAL_3DZ2, ORBITAL_3DXY, ORBITAL_3DYZ, ORBITAL_3DXZ, ORBITAL_3DX2Y2 
};

class ElectronCloud {
public:
    ElectronCloud(int pointCount, const glm::vec3& posA, const glm::vec3& posB, 
                  OrbitalType type1 = OrbitalType::ORBITAL_1S, 
                  OrbitalType type2 = OrbitalType::ORBITAL_1S, 
                  float t = 0.0f, bool isBonding = true);
    ~ElectronCloud();

    void draw(const glm::mat4& viewProj, GLuint shaderProgram);
    void updatePoints(const glm::vec3& posA, const glm::vec3& posB, 
                      OrbitalType type1, OrbitalType type2, float t, bool isBonding);

private:
    void generatePoints(const glm::vec3& posA, const glm::vec3& posB, 
                        OrbitalType type1, OrbitalType type2, float t, bool isBonding);
    void setupBuffers();

    int targetPointCount;
    std::vector<glm::vec3> points;
    GLuint vao, vbo;
};

#endif
