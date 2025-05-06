#include <rose/backends/gl/render.hpp>
#include <rose/model.hpp>

namespace gl {

void render(Shader& shader, const Model& model) {

    shader.use();
    shader.set_mat4("model", model.model_mat);
    glBindVertexArray(model.render_data.vao);

    for (const auto& mesh : model.meshes) {
        shader.set_bool("material.has_diffuse_map", false);
        shader.set_bool("material.has_normal_map", false);
        shader.set_bool("material.has_specular_map", false);
        for (u32 idx = mesh.matl_offset; idx < mesh.matl_offset + mesh.n_matls; ++idx) {
            switch (model.textures[idx].ref->ty) {
            case TextureType::DIFFUSE:
                shader.set_bool("material.has_diffuse_map", true);
                shader.set_tex("material.diffuse_map", 0, model.textures[idx].ref->id);
                break;
            case TextureType::SPECULAR:
                shader.set_bool("material.has_specular_map", true);
                shader.set_tex("material.specular_map", 1, model.textures[idx].ref->id);
                break;
            case TextureType::NORMAL:
                shader.set_bool("material.has_normal_map", true);
                shader.set_tex("material.normal_map", 2, model.textures[idx].ref->id);
                break;
            case TextureType::DISPLACE:
                shader.set_tex("material.displace_map", 3, model.textures[idx].ref->id);
                break;
            default:
                break;
            }
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, mesh.n_indices, GL_UNSIGNED_INT, (void*)(sizeof(u32) * mesh.base_idx),
                                 mesh.base_vert);
    }
}

void render(Shader& shader, const Model& model, MeshFlags test_flag, bool flag_on) {
    shader.use();
    shader.set_mat4("model", model.model_mat);
    glBindVertexArray(model.render_data.vao);

    for (auto& mesh : model.meshes) {

        if (!flag_on && !is_flag_set(mesh.flags, test_flag) || (flag_on && is_flag_set(mesh.flags, test_flag))) {
            // do not render meshes that do not meet the provided condition
            continue;
        }

        shader.set_bool("material.has_diffuse_map", false);
        shader.set_bool("material.has_normal_map", false);
        shader.set_bool("material.has_specular_map", false);

        for (u32 idx = mesh.matl_offset; idx < mesh.matl_offset + mesh.n_matls; ++idx) {
            switch (model.textures[idx].ref->ty) {
            case TextureType::DIFFUSE:
                shader.set_bool("material.has_diffuse_map", true);
                shader.set_tex("material.diffuse_map", 0, model.textures[idx].ref->id);
                break;
            case TextureType::SPECULAR:
                shader.set_bool("material.has_specular_map", true);
                shader.set_tex("material.specular_map", 1, model.textures[idx].ref->id);
                break;
            case TextureType::NORMAL:
                shader.set_bool("material.has_normal_map", true);
                shader.set_tex("material.normal_map", 2, model.textures[idx].ref->id);
                break;
            case TextureType::DISPLACE:
                shader.set_tex("material.displace_map", 3, model.textures[idx].ref->id);
                break;
            default:
                break;
            }
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, mesh.n_indices, GL_UNSIGNED_INT, (void*)(sizeof(u32) * mesh.base_idx),
                                 mesh.base_vert);
    }
}

void render(Shader& shader, SkyBox& skybox, u32 vao) {
    glDepthMask(GL_FALSE);
    shader.use();
    glBindVertexArray(vao);
    shader.set_tex("cube_map", 0, skybox.texture.ref->id);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}


}