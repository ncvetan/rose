#ifndef ROSE_INCLUDE_OBJECT
#define ROSE_INCLUDE_OBJECT

#include <rose/alias.hpp>
#include <rose/lighting.hpp>
#include <rose/model.hpp>
#include <rose/shader.hpp>

#include <glm.hpp>

#include <vector>

namespace rose {

// TODO: This probably shouldn't be here but I'm not sure where I want to put it yet
struct GlobalState {
    SkyBox sky_box;
    DirLight dir_light;
    u32 global_ubo;         // ubo storing values available across shaders
    f32 gamma = 2.2f;
    f32 exposure = 1.0f;

    bool bloom = true;
    int n_bloom_passes = 5;

    ShadowData shadow;
};

enum class ObjectFlags : u32 { 
    NONE = bit1,        // no effect
    EMIT_LIGHT = bit2,  // make this object a light emitter
    HIDE = bit3         // don't render this object
};

inline bool operator&(ObjectFlags lhs, ObjectFlags rhs) {
    return (static_cast<u32>(lhs) & static_cast<u32>(rhs)) != 0;
}

// context used to construct an object
struct ObjectCtx {
    fs::path model_pth;
    glm::vec3 pos;
    glm::vec3 scale;
    PointLight light_props;
    ObjectFlags flags;
};

struct Objects {

    inline void draw(ShaderGL& shader, const GlobalState& state) { 
        for (auto& model : models) {
            model.draw(shader, state); 
        }
    }

    inline size_t size() { return posns.size(); }

    std::optional<rses> add_object(TextureManager& manager, const ObjectCtx& obj_def);

    void update_light_radii(f32 exposure);

    // SoA of program objects, should all be equal length
    std::vector<Model> models;
    std::vector<glm::vec3> posns;
    std::vector<glm::vec3> scales;
    std::vector<PointLight> light_props;
    std::vector<ObjectFlags> flags;

    // indices that are set as lights
    std::vector<u32> light_idxs;
};

// TODO: this function is ugly any probably should not exist + inefficient for large collections
// temporary until I have a better solution
void update_light_state(Objects& objs, ClusterData& clusters);

}

#endif