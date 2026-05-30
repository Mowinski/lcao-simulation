#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include <glm/glm.hpp>
#include <cmath>
#include <functional>

namespace Physics {
    // Wavefunction for 1s orbital: psi = (1/sqrt(pi * a0^3)) * e^(-r/a0)
    // For simplicity, we use units where a0 = 1.0 (Bohr radius)
    inline float psi_1s(float r) {
        const float a0 = 1.0f;
        const float N = 1.0f / std::sqrt(M_PI * std::pow(a0, 3));
        return N * std::exp(-r / a0);
    }

    inline float psi_2s(float r) {
        const float a0 = 1.0f;
        const float N = 1.0f / (4.0f * std::sqrt(2.0f * M_PI * std::pow(a0, 3)));
        float rho = r / a0;
        return N * (2.0f - rho) * std::exp(-rho / 2.0f);
    }

    // Wavefunction for 2p_z orbital: psi = (1 / (4 * sqrt(2 * pi * a0^5))) * r * e^(-r / (2 * a0)) * cos(theta)
    // Here we use the z-projection for simplicity, or we can use the radial part.
    // For LCAO, the orientation of p-orbitals matters.
    inline float psi_2pz(const glm::vec3& rel_pos) {
        float r = glm::length(rel_pos);
        if (r < 1e-6f) return 0.0f;
        const float a0 = 1.0f;
        const float N = 1.0f / (4.0f * std::sqrt(2.0f * M_PI * std::pow(a0, 5)));
        float cosTheta = rel_pos.z / r;
        return N * r * std::exp(-r / (2.0f * a0)) * cosTheta;
    }

    inline float psi_2px(const glm::vec3& rel_pos) {
        float r = glm::length(rel_pos);
        if (r < 1e-6f) return 0.0f;
        const float a0 = 1.0f;
        const float N = 1.0f / (4.0f * std::sqrt(2.0f * M_PI * std::pow(a0, 5)));
        float sinThetaCosPhi = rel_pos.x / r;
        return N * r * std::exp(-r / (2.0f * a0)) * sinThetaCosPhi;
    }

    inline float psi_2py(const glm::vec3& rel_pos) {
        float r = glm::length(rel_pos);
        if (r < 1e-6f) return 0.0f;
        const float a0 = 1.0f;
        const float N = 1.0f / (4.0f * std::sqrt(2.0f * M_PI * std::pow(a0, 5)));
        float sinThetaSinPhi = rel_pos.y / r;
        return N * r * std::exp(-r / (2.0f * a0)) * sinThetaSinPhi;
    }

    // Wavefunctions for 3d orbitals (n=3, l=2)
    // General form: psi = R_nl(r) * Y_lm(theta, phi)
    // R_32(r) = (4 / (81 * sqrt(30) * a0^3.5)) * (r/a0)^2 * e^(-r / (3 * a0))
    
    inline float radial_3d(float r) {
        const float a0 = 1.0f;
        float rho = r / a0;
        float N = 4.0f / (81.0f * std::sqrt(30.0f * std::pow(a0, 3)));
        return N * rho * rho * std::exp(-rho / 3.0f);
    }

    inline float psi_3dz2(const glm::vec3& rel_pos) {
        float r = glm::length(rel_pos);
        if (r < 1e-6f) return 0.0f;
        float cosTheta = rel_pos.z / r;
        float angular = std::sqrt(5.0f / (16.0f * M_PI)) * (3.0f * cosTheta * cosTheta - 1.0f);
        return radial_3d(r) * angular;
    }

    inline float psi_3dxy(const glm::vec3& rel_pos) {
        float r = glm::length(rel_pos);
        if (r < 1e-6f) return 0.0f;
        float angular = std::sqrt(15.0f / (4.0f * M_PI)) * (rel_pos.x * rel_pos.y) / (r * r);
        return radial_3d(r) * angular;
    }

    inline float psi_3dyz(const glm::vec3& rel_pos) {
        float r = glm::length(rel_pos);
        if (r < 1e-6f) return 0.0f;
        float angular = std::sqrt(15.0f / (4.0f * M_PI)) * (rel_pos.y * rel_pos.z) / (r * r);
        return radial_3d(r) * angular;
    }

    inline float psi_3dxz(const glm::vec3& rel_pos) {
        float r = glm::length(rel_pos);
        if (r < 1e-6f) return 0.0f;
        float angular = std::sqrt(15.0f / (4.0f * M_PI)) * (rel_pos.x * rel_pos.z) / (r * r);
        return radial_3d(r) * angular;
    }

    inline float psi_3dx2y2(const glm::vec3& rel_pos) {
        float r = glm::length(rel_pos);
        if (r < 1e-6f) return 0.0f;
        float angular = std::sqrt(15.0f / (16.0f * M_PI)) * (rel_pos.x * rel_pos.x - rel_pos.y * rel_pos.y) / (r * r);
        return radial_3d(r) * angular;
    }

    // Updated LCAO Probability Density to support 3D position-aware orbitals
    typedef std::function<float(const glm::vec3&)> OrbitalFunc;

    inline float calculate_density_hybrid(const glm::vec3& r, const glm::vec3& posA, const glm::vec3& posB, 
                                         OrbitalFunc orbital1, OrbitalFunc orbital2, float t,
                                         bool isBonding = true) {
        glm::vec3 relA = r - posA;
        glm::vec3 relB = r - posB;
        
        // Mixed orbital at each nucleus
        auto mixedOrbital = [&](const glm::vec3& rel) {
            return (1.0f - t) * orbital1(rel) + t * orbital2(rel);
        };

        float psiA = mixedOrbital(relA);
        float psiB = mixedOrbital(relB);
        
        float psiTotal = isBonding ? (psiA + psiB) : (psiA - psiB); 
        return psiTotal * psiTotal;
    }
}

#endif
