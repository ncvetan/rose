#ifndef ROSE_INCLUDE_OBJECT
#define ROSE_INCLUDE_OBJECT

#include <rose/lighting.hpp>
#include <rose/model.hpp>
#include <rose/shader.hpp>

#include <glm.hpp>

namespace rose {

// TODO: This probably shouldn't be here but I'm not sure where I want to put it yet
struct GlobalState {
    SkyBox sky_box;
    DirLight dir_light;
    u32 ubo;
    float gamma = 2.2f;
    float exposure = 1.0f;

    bool bloom = true;              // is bloom enabled
    int n_bloom_passes = 5;

    int tile_sz = 32;               // side length of a tile

    ShadowCtx shadow;
};

template <typename T>
concept drawable = requires(T d, ShaderGL& shader, const GlobalState& state) {
    { d.draw(shader, state) } -> std::same_as<void>;
};

enum class ObjectFlags : u8 { NONE = 0x00, EMIT_LIGHT = 0x01, HIDE = 0x02 };

template <drawable T>
struct Object {
    Object() = default;
    Object(const glm::vec3& pos) : pos(pos) {};
    Object(const glm::vec3& pos, const glm::vec3& scale) : pos(pos), scale(scale) {};
    Object(const glm::vec3& pos, const glm::vec3& scale, u8 flags) : pos(pos), scale(scale), flags(flags) {};

    inline void draw(ShaderGL& shader, const GlobalState& state) { model.draw(shader, state); }

    T model;
    glm::vec3 pos   = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    PointLight light_props;
    u8 flags = 0;
};

}

#endif