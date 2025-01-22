#include <rose/object.hpp>

namespace rose {

std::optional<rses> Objects::add_object(TextureManager& manager, const ObjectCtx& obj_def) {

    Model model;
    std::optional<rses> err = model.load(manager, obj_def.model_pth);

    if (err) {
        return err;
    }

    models.push_back(std::move(model));
    posns.push_back(obj_def.pos);
    scales.push_back(obj_def.scale);
    light_props.push_back(obj_def.light_props);
    flags.push_back(obj_def.flags);

    if (obj_def.flags & ObjectFlags::EMIT_LIGHT) {
        // do stuff here to initialize lights...
    }

    return std::nullopt;
}

}