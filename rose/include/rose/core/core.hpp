// =============================================================================
//   typedefs and utilities used across Rose
// =============================================================================

#ifndef ROSE_INCLUDE_CORE_CORE
#define ROSE_INCLUDE_CORE_CORE

#include <GL/glew.h>

#include <glm.hpp>

#include <filesystem>

// typedefs =======================================================================================

namespace fs = std::filesystem;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

constexpr u32 bit1 = (1 << 0);
constexpr u32 bit2 = (1 << 1);
constexpr u32 bit3 = (1 << 2);
constexpr u32 bit4 = (1 << 3);
constexpr u32 bit5 = (1 << 4);
constexpr u32 bit6 = (1 << 5);
constexpr u32 bit7 = (1 << 6);
constexpr u32 bit8 = (1 << 7);
constexpr u32 bit9 = (1 << 8);
constexpr u32 bit10 = (1 << 9);
constexpr u32 bit11 = (1 << 10);
constexpr u32 bit12 = (1 << 11);
constexpr u32 bit13 = (1 << 12);
constexpr u32 bit14 = (1 << 13);
constexpr u32 bit15 = (1 << 14);
constexpr u32 bit16 = (1 << 15);
constexpr u32 bit17 = (1 << 16);
constexpr u32 bit18 = (1 << 17);
constexpr u32 bit19 = (1 << 18);
constexpr u32 bit20 = (1 << 19);
constexpr u32 bit21 = (1 << 20);
constexpr u32 bit22 = (1 << 21);
constexpr u32 bit23 = (1 << 22);
constexpr u32 bit24 = (1 << 23);
constexpr u32 bit25 = (1 << 24);
constexpr u32 bit26 = (1 << 25);
constexpr u32 bit27 = (1 << 26);
constexpr u32 bit28 = (1 << 27);
constexpr u32 bit29 = (1 << 28);
constexpr u32 bit30 = (1 << 29);
constexpr u32 bit31 = (1 << 30);
constexpr u32 bit32 = (1 << 31);

namespace constants {
	constexpr f32 f32_max = std::numeric_limits<f32>::max();
    constexpr f32 f32_min = std::numeric_limits<f32>::lowest();
    constexpr glm::vec3 vec3_min = { f32_min, f32_min, f32_min };
    constexpr glm::vec3 vec3_max = { f32_max, f32_max, f32_max };
}

// utilities ======================================================================================

template <typename T>
struct enable_rose_enum_ops : std::false_type {};

#define ENABLE_ROSE_ENUM_OPS(E)                                                                                          \
    template <>                                                                                                          \
    struct enable_rose_enum_ops<E> : std::true_type {};

template <typename T>
concept RoseEnumT = std::is_enum_v<T> && enable_rose_enum_ops<T>::value;

template<RoseEnumT T>
T operator&(T lhs, T rhs) {
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) & static_cast<std::underlying_type_t<T>>(rhs));
}

template <RoseEnumT T>
T& operator&=(T& lhs, T rhs) { return lhs = lhs & rhs; }

template <RoseEnumT T>
T operator|(T lhs, T rhs) {
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) | static_cast<std::underlying_type_t<T>>(rhs));
}

template <RoseEnumT T>
T& operator|=(T& lhs, T rhs) { return lhs = lhs | rhs; }

template <RoseEnumT T>
T operator~(T f) {
    return static_cast<T>(~static_cast<std::underlying_type_t<T>>(f));
}

template <RoseEnumT T>
bool is_flag_set(T flag, T compare) {
    return ((flag & compare) != T::NONE);
}

template <RoseEnumT T>
void set_flag(T& flag, T compare) {
    flag &= ~compare;
}

template <RoseEnumT T>
void unset_flag(T& flag, T compare) {
    flag |= compare;
}
#endif