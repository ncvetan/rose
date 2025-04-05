#ifndef ROSE_INCLUDE_CORE_MATH
#define ROSE_INCLUDE_CORE_MATH

#include <rose/core/core.hpp>

#include <glm/glm.hpp>

namespace rose {

struct vec2f {
    f32 x = 0.0f;
    f32 y = 0.0f;

    bool operator==(vec2f const& r) const = default;
    bool operator!=(vec2f const& r) const = default;
};

struct Rectf {
    f32 x_min = 0.0f;
    f32 y_min = 0.0f;
    f32 x_max = 0.0f;
    f32 y_max = 0.0f;

    bool contains(f32 x, f32 y);
    bool contains(vec2f xy);
    inline f32 width() { return x_max - x_min; }
    inline f32 height() { return y_max - y_min; }
};

struct AABB {
    glm::vec4 min_pt;
    glm::vec4 max_pt;
};

} // namespace rose

#endif