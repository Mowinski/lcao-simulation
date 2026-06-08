#include "ElectronCloud.hpp"
#include <random>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <ctime>

ElectronCloud::ElectronCloud(OpenCLManager* clManager, int pointCount, const glm::vec3& posA, const glm::vec3& posB, 
                            OrbitalType type1, OrbitalType type2, float t, bool isBonding, float range) 
    : clManager(clManager), targetPointCount(pointCount), vao(0), vbo(0) {
    generatePoints(posA, posB, type1, type2, t, isBonding, range);
    setupBuffers();
}

ElectronCloud::~ElectronCloud() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
}

// Utility to get orbital function on CPU for maxDensity estimation
static Physics::OrbitalFunc getCPUOrbital(OrbitalType type) {
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
                                 OrbitalType type1, OrbitalType type2, float t, bool isBonding, float range) {
    points.assign(targetPointCount, glm::vec4(0.0f));

    // Pre-estimate maxDensity on CPU (quick)
    Physics::OrbitalFunc orb1 = getCPUOrbital(type1);
    Physics::OrbitalFunc orb2 = getCPUOrbital(type2);
    float maxDensity = 0.0f;
    for(int i=0; i<1000; ++i) {
        glm::vec3 p = glm::vec3(((float)rand()/RAND_MAX * 2.0f - 1.0f) * range, ((float)rand()/RAND_MAX * 2.0f - 1.0f) * range, ((float)rand()/RAND_MAX * 2.0f - 1.0f) * range);
        maxDensity = std::max(maxDensity, Physics::calculate_density_hybrid(p, posA, posB, orb1, orb2, t, isBonding));
    }
    if (maxDensity < 1e-9f) maxDensity = 1.0f;

    // OpenCL Execution
    cl_int err;
    cl_mem buffer_out = clCreateBuffer(clManager->context, CL_MEM_WRITE_ONLY, sizeof(glm::vec4) * targetPointCount, nullptr, &err);

    int arg = 0;
    clSetKernelArg(clManager->kernel, arg++, sizeof(cl_mem), &buffer_out);
    clSetKernelArg(clManager->kernel, arg++, sizeof(float), &posA.x);
    clSetKernelArg(clManager->kernel, arg++, sizeof(float), &posA.y);
    clSetKernelArg(clManager->kernel, arg++, sizeof(float), &posA.z);
    clSetKernelArg(clManager->kernel, arg++, sizeof(float), &posB.x);
    clSetKernelArg(clManager->kernel, arg++, sizeof(float), &posB.y);
    clSetKernelArg(clManager->kernel, arg++, sizeof(float), &posB.z);
    
    int t1 = static_cast<int>(type1);
    int t2 = static_cast<int>(type2);
    int bondingInt = isBonding ? 1 : 0;
    clSetKernelArg(clManager->kernel, arg++, sizeof(int), &t1);
    clSetKernelArg(clManager->kernel, arg++, sizeof(int), &t2);
    clSetKernelArg(clManager->kernel, arg++, sizeof(float), &t);
    clSetKernelArg(clManager->kernel, arg++, sizeof(int), &bondingInt);
    clSetKernelArg(clManager->kernel, arg++, sizeof(float), &range);
    clSetKernelArg(clManager->kernel, arg++, sizeof(float), &maxDensity);
    unsigned int seed = (unsigned int)time(NULL);
    clSetKernelArg(clManager->kernel, arg++, sizeof(unsigned int), &seed);

    size_t global_size = targetPointCount;
    err = clEnqueueNDRangeKernel(clManager->queue, clManager->kernel, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);
    err = clEnqueueReadBuffer(clManager->queue, buffer_out, CL_TRUE, 0, sizeof(glm::vec4) * targetPointCount, points.data(), 0, nullptr, nullptr);

    clReleaseMemObject(buffer_out);

    calculateDensities(range);
}

void ElectronCloud::calculateDensities(float range) {
    const int radialBinCount = 100;
    const int angularBinCount = 100;
    const int azimuthalBinCount = 100;
    radialDensity.assign(radialBinCount, 0.0f);
    angularDensity.assign(angularBinCount, 0.0f);
    azimuthalDensity.assign(azimuthalBinCount, 0.0f);
    
    if (points.empty()) return;

    for (const auto& p : points) {
        if (p.w < 0.5f) continue; // Skip invalid points

        glm::vec3 pos = glm::vec3(p);
        float r = glm::length(pos);
        
        // Radial Density
        int rBin = static_cast<int>((r / range) * radialBinCount);
        if (rBin >= 0 && rBin < radialBinCount) {
            radialDensity[rBin] += 1.0f;
        }

        // Angular Density (Theta - angle from Z axis, 0 to PI)
        if (r > 1e-6f) {
            float cosTheta = pos.z / r;
            float theta = acos(glm::clamp(cosTheta, -1.0f, 1.0f));
            int aBin = static_cast<int>((theta / M_PI) * angularBinCount);
            if (aBin >= 0 && aBin < angularBinCount) {
                angularDensity[aBin] += 1.0f;
            }

            // Azimuthal Density (Phi - angle in XY plane, 0 to 2*PI)
            float phi = atan2(pos.y, pos.x);
            if (phi < 0) phi += 2.0f * M_PI;
            int azBin = static_cast<int>((phi / (2.0f * M_PI)) * azimuthalBinCount);
            if (azBin >= 0 && azBin < azimuthalBinCount) {
                azimuthalDensity[azBin] += 1.0f;
            }
        }
    }

    // Normalize Radial
    float maxRadial = 0.0f;
    for (float val : radialDensity) if (val > maxRadial) maxRadial = val;
    if (maxRadial > 0.0f) {
        for (float& val : radialDensity) val /= maxRadial;
    }

    // Normalize Angular
    float maxAngular = 0.0f;
    for (float val : angularDensity) if (val > maxAngular) maxAngular = val;
    if (maxAngular > 0.0f) {
        for (float& val : angularDensity) val /= maxAngular;
    }

    // Normalize Azimuthal
    float maxAzimuthal = 0.0f;
    for (float val : azimuthalDensity) if (val > maxAzimuthal) maxAzimuthal = val;
    if (maxAzimuthal > 0.0f) {
        for (float& val : azimuthalDensity) val /= maxAzimuthal;
    }
}

void ElectronCloud::setupBuffers() {
    if (vao == 0) glGenVertexArrays(1, &vao);
    if (vbo == 0) glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec4), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
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
                                OrbitalType type1, OrbitalType type2, float t, bool isBonding, float range) {
    generatePoints(posA, posB, type1, type2, t, isBonding, range);
    setupBuffers();
}

float ElectronCloud::getProbabilityWithinRadius(float r) const {
    if (points.empty()) return 0.0f;
    
    int countInside = 0;
    int totalValid = 0;
    for (const auto& p : points) {
        if (p.w < 0.5f) continue;
        totalValid++;
        if (glm::length(glm::vec3(p)) <= r) {
            countInside++;
        }
    }
    
    return totalValid > 0 ? static_cast<float>(countInside) / static_cast<float>(totalValid) : 0.0f;
}
