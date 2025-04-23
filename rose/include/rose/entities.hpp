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
    HIDE        = bit1,  // don't render this object
    EMIT_LIGHT  = bit2,  // make this object a light emitter
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

    // add an entity to the scene
    std::optional<rses> add_object(TextureManager& manager, const EntityCtx& ent_def);

    // duplicates an existing object with the given index
    void dup_object(i64 idx);

    // delete the object at the given index
    void del_object(i64 idx);

    // updates the light radius field of all entities that are set as light emitters
    void update_light_radii();

    // returns the number of entities, both active and deleted
    inline size_t size() { return positions.size(); }

    inline size_t empty() { return positions.empty(); }

    // returns true if the entity at the given index is active (i.e., not deleted)
    inline bool is_alive(i64 idx) { return !tombs[idx]; }

    // returns true if the entity at the given index is a light emitter
    inline bool is_light(i64 idx) { 
        return is_flag_set(flags[idx], EntityFlags::EMIT_LIGHT);
    }

    // return a new id and increment
    inline u64 new_id() { return id_counter++; }

    // SoA of program objects, should all be equal length
    std::vector<u64> ids;
    std::vector<bool> tombs;
    std::vector<Model> models;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> scales;
    std::vector<PtLight> light_props;
    std::vector<EntityFlags> flags;

    // right now, only a single point light can cast shadows
    // this stores the index of the current caster until support
    // is extended for multiple casters
    u32 pt_caster_idx = 0;

    // used to assign an id to an entity, always increasing
    u64 id_counter = 0;
    std::vector<i64> free_idxs;  // free indices
};

// TODO: this function is ugly any probably should not exist + inefficient for large collections
// temporary until I have a better solution
void update_light_ssbos(Entities& objs, ClusterData& clusters);

}

#endif