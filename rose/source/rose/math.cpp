#include <rose/math.hpp>

namespace rose {

bool Rectf::contains(float x, float y) {
	if ((x_min <= x && x <= x_max) && (y_min <= y && y <= y_max)) {
		return true;
	}
	return false;
}

} // namespace rose