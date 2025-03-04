#ifndef ROSE_INCLUDE_OBJECT
#define ROSE_INCLUDE_OBJECT

#include <rose/lighting.hpp>
#include <rose/model.hpp>
#include <rose/core/alias.hpp>
#include <rose/gl/shader.hpp>

#include <glm.hpp>

#include <vector>

namespace rose {

// TODO: This probably shouldn't be here but I'm not sure where I want to put it yet

// bundle of state used throughout the program
struct GlobalState {
    SkyBox sky_box;
    
    u32 global_ubo = 0;     // ubo storing values available across shaders
    
    f32 gamma = 2.2f;
    f32 exposure = 1.0f;

    bool bloom_on = true;
    i32 n_bloom_passes = 5;

    DirLight dir_light;
    ShadowData pt_shadow;
};

enum class EntityFlags : u32 { 
    NONE       = bit1,  // no effect
    EMIT_LIGHT = bit2,  // make this object a light emitter
    HIDE       = bit3   // don't render this object
};

inline bool operator&(EntityFlags lhs, EntityFlags rhs) {
    return (static_cast<u32>(lhs) & static_cast<u32>(rhs)) != 0;
}

// context used to construct an entity
struct EntityCtx {
    fs::path model_pth;
    glm::vec3 pos;
    glm::vec3 scale;
    PtLight light_props;
    EntityFlags flags;
};

struct Entities {

    inline size_t size() { return positions.size(); }

    std::optional<rses> add_object(TextureManager& manager, const EntityCtx& ent_def);

    // updates the light radius field of all entities that are set as light emitters
    void update_light_radii(f32 exposure);

    // SoA of program objects, should all be equal length
    std::vector<Model> models;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> scales;
    std::vector<PtLight> light_props;
    std::vector<EntityFlags> flags;
};

// TODO: this function is ugly any probably should not exist + inefficient for large collections
// temporary until I have a better solution
void update_light_state(Entities& objs, ClusterData& clusters);

}

#endif