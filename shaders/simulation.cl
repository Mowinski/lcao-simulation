
// Wavefunctions and Density Calculation Kernel for Hydrogen Simulation

float psi_1s(float r) {
    const float a0 = 1.0f;
    const float N = 0.56418958f; // 1/sqrt(pi * a0^3)
    return N * exp(-r / a0);
}

float psi_2s(float r) {
    const float a0 = 1.0f;
    const float N = 0.39894228f; // 1/(4 * sqrt(2 * pi * a0^3))
    float rho = r / a0;
    return N * (2.0f - rho) * exp(-rho / 2.0f);
}

float radial_3d(float r) {
    const float a0 = 1.0f;
    float rho = r / a0;
    const float N = 0.009048f; // 4 / (81 * sqrt(30) * a0^3.5) simplified
    return N * rho * rho * exp(-rho / 3.0f);
}

float get_orbital_val(int type, float3 rel_pos) {
    float r = length(rel_pos);
    if (r < 1e-6f) return 0.0f;
    const float a0 = 1.0f;
    const float N_2p = 0.19947114f; // 1 / (4 * sqrt(2 * pi * a0^5))

    switch(type) {
        case 0: return psi_1s(r); // 1s
        case 1: return psi_2s(r); // 2s
        case 2: return N_2p * rel_pos.x * exp(-r / (2.0f * a0)); // 2px
        case 3: return N_2p * rel_pos.y * exp(-r / (2.0f * a0)); // 2py
        case 4: return N_2p * rel_pos.z * exp(-r / (2.0f * a0)); // 2pz
        case 5: { // 3dz2
            float cosTheta = rel_pos.z / r;
            float angular = 0.31539157f * (3.0f * cosTheta * cosTheta - 1.0f); 
            return radial_3d(r) * angular;
        }
        case 6: { // 3dxy
            float angular = 1.09254843f * (rel_pos.x * rel_pos.y) / (r * r); 
            return radial_3d(r) * angular;
        }
        case 7: { // 3dyz
            float angular = 1.09254843f * (rel_pos.y * rel_pos.z) / (r * r);
            return radial_3d(r) * angular;
        }
        case 8: { // 3dxz
            float angular = 1.09254843f * (rel_pos.x * rel_pos.z) / (r * r);
            return radial_3d(r) * angular;
        }
        case 9: { // 3dx2y2
            float angular = 0.54627421f * (rel_pos.x * rel_pos.x - rel_pos.y * rel_pos.y) / (r * r); 
            return radial_3d(r) * angular;
        }
    }
    return 0.0f;
}

// PCG Random Number Generator for OpenCL
typedef struct { uint state;  uint inc; } pcg32_random_t;

uint pcg32_random_r(pcg32_random_t* rng) {
    uint oldstate = rng->state;
    rng->state = oldstate * 747796405u + rng->inc;
    uint word = ((oldstate >> ((oldstate >> 28u) + 4u)) ^ oldstate) * 277803737u;
    return (word >> 22u) ^ word;
}

float pcg_next_float(pcg32_random_t* rng) {
    return (float)pcg32_random_r(rng) / 4294967296.0f;
}

__kernel void generate_points(
    __global float4* out_points,
    const float posAx, const float posAy, const float posAz,
    const float posBx, const float posBy, const float posBz,
    const int type1,
    const int type2,
    const float mixFactor,
    const int isBonding,
    const float range,
    const float maxDensity,
    const uint seed_offset
) {
    int gid = get_global_id(0);
    
    float3 posA = (float3)(posAx, posAy, posAz);
    float3 posB = (float3)(posBx, posBy, posBz);

    // Seed decorrelation hash
    uint h = gid + seed_offset;
    h = (h ^ 61) ^ (h >> 16);
    h *= 9;
    h = h ^ (h >> 4);
    h *= 0x27d4eb2d;
    h = h ^ (h >> 15);
    
    pcg32_random_t rng;
    rng.state = h;
    rng.inc = (gid << 1u) | 1u;

    int attempts = 0;
    while (attempts < 3000) { 
        float3 p;
        p.x = (pcg_next_float(&rng) * 2.0f - 1.0f) * range;
        p.y = (pcg_next_float(&rng) * 2.0f - 1.0f) * range;
        p.z = (pcg_next_float(&rng) * 2.0f - 1.0f) * range;

        float psi1A = get_orbital_val(type1, p - posA);
        float psi1B = get_orbital_val(type1, p - posB);
        float psi2A = get_orbital_val(type2, p - posA);
        float psi2B = get_orbital_val(type2, p - posB);

        float psiA = (1.0f - mixFactor) * psi1A + mixFactor * psi2A;
        float psiB = (1.0f - mixFactor) * psi1B + mixFactor * psi2B;

        float psiTotal = isBonding ? (psiA + psiB) : (psiA - psiB);
        float rho = psiTotal * psiTotal;

        if (pcg_next_float(&rng) < rho / maxDensity) {
            out_points[gid] = (float4)(p.x, p.y, p.z, 1.0f);
            return;
        }
        attempts++;
    }
    out_points[gid] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
}
