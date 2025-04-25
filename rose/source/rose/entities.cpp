#include <rose/entities.hpp>

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
        light_props[idx].radius();
    }
}
