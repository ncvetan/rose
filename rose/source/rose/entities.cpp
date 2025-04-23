#include <rose/entities.hpp>

namespace rose {

std::optional<rses> Entities::add_object(TextureManager& manager, const EntityCtx& ent_def) {
    Model model;
    std::optional<rses> err = model.load(manager, ent_def.model_pth);

    if (err) {
        return err;
    }

    if (free_idxs.empty()) {
        ids.push_back(new_id());
        tombs.push_back(false);
        models.push_back(std::move(model));
        positions.push_back(ent_def.pos);
        scales.push_back(ent_def.scale);
        light_props.push_back(ent_def.light_props);
        light_props.back().radius();
        flags.push_back(ent_def.flags);
    } 
    else {
        auto idx = free_idxs.back();
        ids[idx] = new_id();
        tombs[idx] = false;
        models[idx] = std::move(model);
        positions[idx] = ent_def.pos;
        scales[idx] = ent_def.scale;
        light_props[idx] = ent_def.light_props;
        light_props[idx].radius();
        flags[idx] = ent_def.flags;
        free_idxs.pop_back();
    }

    return std::nullopt;
}

void Entities::dup_object(i64 idx) { 
    if (free_idxs.empty()) {
        ids.push_back(new_id());
        tombs.push_back(false);
        // TODO: this copies all buffers of the model, could instead
        // store an index to reduce memory duplication
        models.push_back(models[idx].copy());
        positions.push_back(positions[idx] + glm::vec3(0.25f, 0.25f, 0.25f));
        scales.push_back(scales[idx]);
        light_props.push_back(light_props[idx]);
        light_props.back().radius();
        flags.push_back(flags[idx]);
    } 
    else {
        auto new_idx = free_idxs.back();
        ids[new_idx] = new_id();
        tombs[new_idx] = false;
        models[new_idx] = std::move(models[idx].copy());
        positions[new_idx] = positions[idx] + glm::vec3(0.25f, 0.25f, 0.25f);
        scales[new_idx] = scales[idx];
        light_props[new_idx] = light_props[idx];
        light_props[new_idx].radius();
        flags[new_idx] = flags[idx];
        free_idxs.pop_back();
    }
}

void Entities::del_object(i64 idx) { 
    tombs[idx] = true; 
    free_idxs.push_back(idx);
}

void Entities::update_light_radii() {
    for (size_t idx = 0; idx < size(); ++idx) {
        if (is_light(idx)) {
            light_props[idx].radius();
        }
    }
}

// TODO: come up with a better solution
void update_light_ssbos(Entities& entities, ClusterData& clusters) {
    std::vector<PtLight> pnt_lights_props;
    std::vector<glm::vec4> pnt_lights_pos;
    std::vector<u32> pnt_lights_ids;
    u32 n_lights = 0;

    // linear search, probably not a big deal for now
    for (size_t idx = 0; idx < entities.size(); ++idx) {
        if (entities.is_alive(idx) && entities.is_light(idx)) {
            pnt_lights_pos.push_back(glm::vec4(entities.positions[idx], 1.0f));
            pnt_lights_props.push_back(entities.light_props[idx]);
            // note: using u32 ids for now, can use the u64 extension
            pnt_lights_ids.push_back(static_cast<u32>(entities.ids[idx]));
            n_lights++;
        }
    }

    clusters.lights_ssbo.update(std::span(pnt_lights_props.begin(), pnt_lights_props.end()));
    clusters.lights_pos_ssbo.update(std::span(pnt_lights_pos.begin(), pnt_lights_pos.end()));
    clusters.lights_ids_ssbo.update(std::span(pnt_lights_ids.begin(), pnt_lights_ids.end()));
}

}