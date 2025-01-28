#ifndef ROSE_INCLUDE_LIGHTING
#define ROSE_INCLUDE_LIGHTING

#include <rose/alias.hpp>

#include <glm.hpp>

namespace rose {

struct DirLight {
    glm::vec3 direction = { 0.0f, -0.999848f, -0.0174525f };
    glm::vec3 color = { 0.55f, 0.55f, 0.55f };
};

struct PointLight {

    // calculate the radius of the point light
    inline void radius(float exposure) {
        float lum = glm::dot(glm::vec3(color.r, color.g, color.b), glm::vec3(0.2126, 0.7152, 0.0722));
        float threshold = 0.01f / exposure;
        rad = (-linear + std::sqrtf(linear * linear - 4.0f * quad * (1.0f - lum / threshold)) / (2.0f * quad));
    };

    glm::vec4 color = { 0.20f, 0.20f, 0.20f, 1.0f };
    float linear = 1.0f;
    float quad = 0.7f;
    float intensity = 1.8f;
    float rad = 1.0f;
};

struct ShadowCtx {
    u32 fbo = 0;
    u32 tex = 0;
    float bias = 0.005;
    u16 resolution = 2048;
};

}

#endif