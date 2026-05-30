#include "ElectronCloud.hpp"
#include <random>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

ElectronCloud::ElectronCloud(int pointCount, const glm::vec3& posA, const glm::vec3& posB, 
                            OrbitalType type1, OrbitalType type2, float t, bool isBonding) 
    : targetPointCount(pointCount), vao(0), vbo(0) {
    generatePoints(posA, posB, type1, type2, t, isBonding);
    setupBuffers();
}

ElectronCloud::~ElectronCloud() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
}

Physics::OrbitalFunc getOrbitalFunc(OrbitalType type) {
    switch(type) {
        case OrbitalType::ORBITAL_1S:   return [](const glm::vec3& p) { return Physics::psi_1s(glm::length(p)); };
        case OrbitalType::ORBITAL_2S:   return [](const glm::vec3& p) { return Physics::psi_2s(glm::length(p)); };
        case OrbitalType::ORBITAL_2PX:  return Physics::psi_2px;
        case OrbitalType::ORBITAL_2PY:  return Physics::psi_2py;
        case OrbitalType::ORBITAL_2PZ:  return Physics::psi_2pz;
        case OrbitalType::ORBITAL_3DZ2: return Physics::psi_3dz2;
        case OrbitalType::ORBITAL_3DXY: return Physics::psi_3dxy;
        case OrbitalType::ORBITAL_3DYZ: return Physics::psi_3dyz;
        case OrbitalType::ORBITAL_3DXZ: return Physics::psi_3dxz;
        case OrbitalType::ORBITAL_3DX2Y2: return Physics::psi_3dx2y2;
        default: return [](const glm::vec3& p) { return 0.0f; };
    }
}

void ElectronCloud::generatePoints(const glm::vec3& posA, const glm::vec3& posB, 
                                 OrbitalType type1, OrbitalType type2, float t, bool isBonding) {
    points.clear();
    points.reserve(targetPointCount);

    std::random_device rd;
    std::mt19937 gen(rd());
    
    float range = (type1 == OrbitalType::ORBITAL_1S && type2 == OrbitalType::ORBITAL_1S) ? 6.0f : 15.0f;
    if (type1 == OrbitalType::ORBITAL_2S || type2 == OrbitalType::ORBITAL_2S) range = 12.0f;
    std::uniform_real_distribution<float> disPos(-range, range);
    std::uniform_real_distribution<float> disProb(0.0f, 1.0f);

    Physics::OrbitalFunc orb1 = getOrbitalFunc(type1);
    Physics::OrbitalFunc orb2 = getOrbitalFunc(type2);

    float maxDensity = 0.0f;
    for(int i=0; i<3000; ++i) {
        glm::vec3 p(disPos(gen), disPos(gen), disPos(gen));
        maxDensity = std::max(maxDensity, Physics::calculate_density_hybrid(p, posA, posB, orb1, orb2, t, isBonding));
    }
    if (maxDensity < 1e-9f) maxDensity = 1.0f;

    int attempts = 0;
    while (points.size() < targetPointCount && attempts < targetPointCount * 300) {
        glm::vec3 p(disPos(gen), disPos(gen), disPos(gen));
        float rho = Physics::calculate_density_hybrid(p, posA, posB, orb1, orb2, t, isBonding);
        
        if (disProb(gen) < rho / maxDensity) {
            points.push_back(p);
        }
        attempts++;
    }
}

void ElectronCloud::setupBuffers() {
    if (vao == 0) glGenVertexArrays(1, &vao);
    if (vbo == 0) glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void ElectronCloud::draw(const glm::mat4& viewProj, GLuint shaderProgram) {
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uViewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));
    
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
    glBindVertexArray(0);
}

void ElectronCloud::updatePoints(const glm::vec3& posA, const glm::vec3& posB, 
                                OrbitalType type1, OrbitalType type2, float t, bool isBonding) {
    generatePoints(posA, posB, type1, type2, t, isBonding);
    setupBuffers();
}
