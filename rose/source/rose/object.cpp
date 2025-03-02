#include <rose/object.hpp>

namespace rose {

std::optional<rses> Objects::add_object(TextureManager& manager, const ObjectCtx& obj_def) {

    Model model;
    std::optional<rses> err = model.load(manager, obj_def.model_pth);

    if (err) {
        return err;
    }

    models.push_back(std::move(model));
    positions.push_back(obj_def.pos);
    scales.push_back(obj_def.scale);
    light_props.push_back(obj_def.light_props);
    flags.push_back(obj_def.flags);

    if (obj_def.flags & ObjectFlags::EMIT_LIGHT) {
        // do stuff here to initialize lights...
    }

    return std::nullopt;
}

void Objects::update_light_radii(f32 exposure) {
    for (size_t idx = 0; idx < size(); ++idx) {
        if (flags[idx] & ObjectFlags::EMIT_LIGHT) {
            light_props[idx].radius(exposure);
        }
    }
}

void update_light_state(Objects& objs, ClusterData& clusters) {
    std::vector<PtLight> pnt_lights_props;
    std::vector<glm::vec4> pnt_lights_pos;
    u32 n_lights = 0;

    // linear search, probably not a big deal for now
    for (size_t idx = 0; idx < objs.size(); ++idx) {
        if (objs.flags[idx] & ObjectFlags::EMIT_LIGHT) {
            pnt_lights_pos.push_back(glm::vec4(objs.positions[idx], 1.0f));
            pnt_lights_props.push_back(objs.light_props[idx]);
            n_lights++;
        }
    }

    clusters.lights_ssbo.update(0, std::span(pnt_lights_props.begin(), pnt_lights_props.end()));
    clusters.lights_pos_ssbo.update(0, std::span(pnt_lights_pos.begin(), pnt_lights_pos.end()));

    // zero out the rest of the buffer
    clusters.lights_ssbo.zero(n_lights);
    clusters.lights_pos_ssbo.zero(n_lights);
}

}