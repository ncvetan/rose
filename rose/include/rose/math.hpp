#ifndef ROSE_INCLUDE_MATH
#define ROSE_INCLUDE_MATH

#include <rose/alias.hpp>

namespace rose {

struct vec2f {
    float x = 0.0f;
    float y = 0.0f;
};

struct Rectf {
    float x_min = 0.0f;
    float y_min = 0.0f;
    float x_max = 0.0f;
    float y_max = 0.0f;

    bool contains(float x, float y);
};

} // namespace rose

#endif