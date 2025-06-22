#include <rose/backends/gl/render.hpp>
#include <rose/model.hpp>

namespace gl {

static void render_mesh(Shader& shader, const Mesh& mesh, const std::vector<TextureRef>& textures) {

        shader.set_bool("material.has_albedo_map", false);
        shader.set_bool("material.has_normal_map", false);
        shader.set_bool("material.has_pbr_map", false);

        for (u32 idx = mesh.matl_offset; idx < mesh.matl_offset + mesh.n_matls; ++idx) {
            switch (textures[idx].ref->ty) {
            case TextureType::ALBEDO:
                shader.set_bool("material.has_albedo_map", true);
                shader.set_tex("material.albedo_map", 0, textures[idx].ref->id);
                break;
            case TextureType::GLTF_PBR:
                shader.set_bool("material.has_pbr_map", true);
                shader.set_tex("material.pbr_map", 1, textures[idx].ref->id);
                break;
            case TextureType::NORMAL:
                shader.set_bool("material.has_normal_map", true);
                shader.set_tex("material.normal_map", 2, textures[idx].ref->id);
                break;
            case TextureType::DISPLACE:
                shader.set_tex("material.displace_map", 3, textures[idx].ref->id);
                break;
            case TextureType::AMBIENT_OCCLUSION:
                shader.set_bool("material.has_ao_map", true);
                shader.set_tex("material.ao_map", 4, textures[idx].ref->id);
                break;
            default:
                break;
            }
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, mesh.n_indices, GL_UNSIGNED_INT, (void*)(sizeof(u32) * mesh.base_idx),
                                 mesh.base_vert);
}

void render(Shader& shader, const Model& model) {
    shader.use();
    shader.set_mat4("model", model.model_mat);
    glBindVertexArray(model.render_data.vao);
    for (const auto& mesh : model.meshes) {
        render_mesh(shader, mesh, model.textures);
    }
}

void render_opaque(Shader& shader, const Model& model) {
    shader.use();
    shader.set_mat4("model", model.model_mat);
    glBindVertexArray(model.render_data.vao);
    for (auto& mesh : model.meshes) {
        if (!is_flag_set(mesh.flags, MeshFlags::TRANSPARENT)) {
            render_mesh(shader, mesh, model.textures);
        }
    }
}

void render_transparent(Shader& shader, const Model& model) {
    shader.use();
    shader.set_mat4("model", model.model_mat);
    glBindVertexArray(model.render_data.vao);
    for (auto& mesh : model.meshes) {
        if (is_flag_set(mesh.flags, MeshFlags::TRANSPARENT)) {
            render_mesh(shader, mesh, model.textures);
        }
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