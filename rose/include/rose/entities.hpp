// =============================================================================
//   entity handling system
// =============================================================================

#ifndef ROSE_INCLUDE_OBJECT
#define ROSE_INCLUDE_OBJECT

#include <rose/lighting.hpp>
#include <rose/model.hpp>
#include <rose/core/core.hpp>
#include <rose/gl/shader.hpp>

#include <glm.hpp>

#include <vector>

namespace rose {

enum class EntityFlags : u32 { 
    NONE        = 0,     // no effect
    EMIT_LIGHT  = bit1,  // make this object a light emitter
    HIDE        = bit2   // don't render this object
};

ENABLE_ROSE_ENUM_OPS(EntityFlags); 

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
    void update_light_radii();

    // SoA of program objects, should all be equal length
    std::vector<Model> models;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> scales;
    std::vector<PtLight> light_props;
    std::vector<EntityFlags> flags;
};

// TODO: this function is ugly any probably should not exist + inefficient for large collections
// temporary until I have a better solution
void update_light_ssbos(Entities& objs, ClusterData& clusters);

}

#endif