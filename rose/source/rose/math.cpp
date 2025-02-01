#include <rose/math.hpp>

namespace rose {

bool Rectf::contains(f32 x, f32 y) {
	if ((x_min <= x && x <= x_max) && (y_min <= y && y <= y_max)) {
		return true;
	}
	return false;
}

bool Rectf::contains(vec2f xy) {
	if ((x_min <= xy.x && xy.x <= x_max) && (y_min <= xy.y && xy.y <= y_max)) {
		return true;
	}
	return false;
}


} // namespace rose