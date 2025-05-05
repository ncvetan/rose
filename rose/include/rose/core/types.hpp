// =============================================================================
//   mathematical constructs
// =============================================================================

#ifndef ROSE_INCLUDE_CORE_TYPES
#define ROSE_INCLUDE_CORE_TYPES

#include <rose/core/core.hpp>

#include <glm/glm.hpp>

struct vec2f {
    bool operator==(vec2f const& r) const = default;
    bool operator!=(vec2f const& r) const = default;

    f32 x = 0.0f;
    f32 y = 0.0f;
};

struct Rectf {
    bool contains(f32 x, f32 y);
    bool contains(vec2f xy);
    inline f32 width() { return x_max - x_min; }
    inline f32 height() { return y_max - y_min; }

    f32 x_min = 0.0f;
    f32 y_min = 0.0f;
    f32 x_max = 0.0f;
    f32 y_max = 0.0f;
};

struct AABB {
    glm::vec4 min_pt;
    glm::vec4 max_pt;
};

#endif