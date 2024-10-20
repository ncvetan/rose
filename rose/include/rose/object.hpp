#ifndef ROSE_INCLUDE_OBJECT
#define ROSE_INCLUDE_OBJECT

#include <rose/model.hpp>
#include <rose/shader.hpp>

#include <glm.hpp>

namespace rose {

struct DirLight {
    glm::vec3 direction = { 0.0f, -0.999848f, -0.0174525f };
    glm::vec3 ambient = { 0.025f, 0.025f, 0.025f };
    glm::vec3 diffuse = { 0.10f, 0.10f, 0.10f };
    glm::vec3 specular = { 1.0f, 1.0f, 1.0f };
};

struct PointLight {
    glm::vec3 ambient = { 0.02f, 0.02f, 0.02f };
    glm::vec3 diffuse = { 0.05f, 0.05f, 0.05f };
    glm::vec3 specular = { 0.2f, 0.2f, 0.2f };
    float attenuation = 1.0f;
};

// TODO: This probably shouldn't be here but I'm not sure where I want to put it yet
struct GlobalState {
    SkyBox sky_box;
    DirLight dir_light;
    u32 ubo;
    float gamma = 2.2f;
    float exposure = 1.0f;

    struct ShadowCtx {
        u32 fbo = 0;
        u32 tex = 0;
        float bias = 0.005;
        u16 resolution = 2048;
    };
    
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
    glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    PointLight light_props;
    u8 flags = 0;
};

}

#endif