#include <rose/core/types.hpp>

bool Rectf::contains(f32 x, f32 y) {
    return ((x_min <= x && x <= x_max) && (y_min <= y && y <= y_max));
}

bool Rectf::contains(vec2f xy) {
    return ((x_min <= xy.x && xy.x <= x_max) && (y_min <= xy.y && xy.y <= y_max));
}
