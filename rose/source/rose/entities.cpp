#include <rose/entities.hpp>

void Entities::add_object(TextureManager& manager, const EntityCtx& ent_def) {
    Model model;
    model.load(manager, ent_def.model_pth);

    if (free_idxs.empty()) {
        ids.push_back(new_id());
        slot_empty.push_back(false);
        models.push_back(std::move(model));
        positions.push_back(ent_def.pos);
        scales.push_back(ent_def.scale);
        rotations.push_back(ent_def.rotation);
        light_data.push_back(ent_def.light_data);
        flags.push_back(ent_def.flags);
    } 
    else {
        auto idx = free_idxs.back();
        ids[idx] = new_id();
        slot_empty[idx] = false;
        models[idx] = std::move(model);
        positions[idx] = ent_def.pos;
        scales[idx] = ent_def.scale;
        rotations[idx] = ent_def.rotation;
        light_data[idx] = ent_def.light_data;
        flags[idx] = ent_def.flags;
        free_idxs.pop_back();
    }
}

void Entities::dup_object(i64 idx) { 
    if (free_idxs.empty()) {
        ids.push_back(new_id());
        slot_empty.push_back(false);
        // TODO: this copies all buffers of the model, could instead
        // store an index to reduce memory duplication
        models.push_back(models[idx].copy());
        positions.push_back(positions[idx] + glm::vec3(0.25f, 0.25f, 0.25f));
        scales.push_back(scales[idx]);
        rotations.push_back(rotations[idx]);
        light_data.push_back(light_data[idx]);
        flags.push_back(flags[idx]);
    } 
    else {
        auto new_idx = free_idxs.back();
        ids[new_idx] = new_id();
        slot_empty[new_idx] = false;
        models[new_idx] = std::move(models[idx].copy());
        positions[new_idx] = positions[idx] + glm::vec3(0.25f, 0.25f, 0.25f);
        scales[new_idx] = scales[idx];
        rotations[new_idx] = rotations[idx];
        light_data[new_idx] = light_data[idx];
        flags[new_idx] = flags[idx];
        free_idxs.pop_back();
    }
}

void Entities::del_object(i64 idx) { 
    slot_empty[idx] = true; 
    free_idxs.push_back(idx);
}
