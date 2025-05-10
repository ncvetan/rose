#include <rose/entities.hpp>

i64 Entities::add_object(TextureManager& manager, const EntityCtx& ent_def) {
    Model model;
    model.load(manager, ent_def.model_path);
    i64 ret = 0;

    if (free_idxs.empty()) {
        ids.push_back(new_id());
        slot_empty.push_back(false);
        models.push_back(std::move(model));
        positions.push_back(ent_def.pos);
        scales.push_back(ent_def.scale);
        rotations.push_back(ent_def.rotation);
        light_data.push_back(ent_def.light_data);
        flags.push_back(ent_def.flags);
        ret = (i64)size() - 1;
    } 
    else {
        auto new_idx = free_idxs.back();
        ids[new_idx] = new_id();
        slot_empty[new_idx] = false;
        models[new_idx] = std::move(model);
        positions[new_idx] = ent_def.pos;
        scales[new_idx] = ent_def.scale;
        rotations[new_idx] = ent_def.rotation;
        light_data[new_idx] = ent_def.light_data;
        flags[new_idx] = ent_def.flags;
        free_idxs.pop_back();
        ret = new_idx;
    }

    return ret;
}

i64 Entities::dup_object(i64 dup_idx) { 
    i64 ret = 0;
    
    if (free_idxs.empty()) {
        ids.push_back(new_id());
        slot_empty.push_back(false);
        // TODO: this copies all buffers of the model, could instead
        // store an index to reduce memory duplication
        models.push_back(models[dup_idx].copy());
        positions.push_back(positions[dup_idx] + glm::vec3(0.25f, 0.25f, 0.25f));
        scales.push_back(scales[dup_idx]);
        rotations.push_back(rotations[dup_idx]);
        light_data.push_back(light_data[dup_idx]);
        flags.push_back(flags[dup_idx]);
        ret = (i64)size() - 1;
    } 
    else {
        auto new_idx = free_idxs.back();
        ids[new_idx] = new_id();
        slot_empty[new_idx] = false;
        models[new_idx] = std::move(models[dup_idx].copy());
        positions[new_idx] = positions[dup_idx] + glm::vec3(0.25f, 0.25f, 0.25f);
        scales[new_idx] = scales[dup_idx];
        rotations[new_idx] = rotations[dup_idx];
        light_data[new_idx] = light_data[dup_idx];
        flags[new_idx] = flags[dup_idx];
        free_idxs.pop_back();
        ret = new_idx;
    }

    return ret;
}

void Entities::del_object(i64 idx) { 
    slot_empty[idx] = true;    
    free_idxs.push_back(idx);
}
