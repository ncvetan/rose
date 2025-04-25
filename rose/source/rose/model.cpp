#include <rose/model.hpp>
#include <rose/gl/platform.hpp>
#include <rose/core/err.hpp>

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <format>
#include <unordered_map>

Model::Model(Model&& other) noexcept {
    render_data = other.render_data;
    other.render_data = gl::RenderData();
    pos = std::move(other.pos);
    norms = std::move(other.norms);
    tangents = std::move(other.tangents);
    uvs = std::move(other.uvs);
    indices = std::move(other.indices);
    textures = std::move(other.textures);
    meshes = std::move(other.meshes);
}

Model& Model::operator=(Model&& other) noexcept {
    if (this == &other) return *this;
    this->~Model();
    new (this) Model(std::move(other));
    return *this;
}

static void load_matl_textures(TextureManager& manager, Model& model, aiMaterial* mat, aiTextureType ty,
                               const fs::path& root_path, u32 mesh_idx) {
    for (int idx = 0; idx < mat->GetTextureCount(ty); ++idx) {
        aiString ai_str;
        mat->GetTexture(ty, idx, &ai_str);
        fs::path texture_path = root_path / std::string(ai_str.C_Str());
        std::expected<TextureRef, rses> texture;

        switch (ty) {
        case aiTextureType_DIFFUSE:
            texture = manager.load_texture(texture_path, TextureType::DIFFUSE);
            model.textures.push_back(texture.value());
            break;
        case aiTextureType_SPECULAR:
            texture = manager.load_texture(texture_path, TextureType::SPECULAR);
            model.textures.push_back(texture.value());
            break;
        case aiTextureType_HEIGHT:
            texture = manager.load_texture(texture_path, TextureType::NORMAL);
            model.textures.push_back(texture.value());
            break;
        case aiTextureType_NORMALS:
            texture = manager.load_texture(texture_path, TextureType::NORMAL);
            model.textures.push_back(texture.value());
            break;
        case aiTextureType_DISPLACEMENT:
            texture = manager.load_texture(texture_path, TextureType::DISPLACE);
            model.textures.push_back(texture.value());
            break;
        default:
            break;
        }

        if (!texture.has_value()) {
            err::print(texture.error());
            continue;
        }

        // TODO: a bit hacky, would like to refactor model loading to better handle these sorts of cases
        if (is_flag_set(texture->ref->flags, TextureFlags::TRANSPARENT)) {
            model.meshes[mesh_idx].flags |= MeshFlags::TRANSPARENT;
        }
    }
}

// determine the number of meshes in the model
static void get_n_meshes(aiNode* ai_node, const aiScene* ai_scene, u32& n_meshes) {
    n_meshes += ai_node->mNumMeshes;
    for (int i = 0; i < ai_node->mNumChildren; ++i) {
        get_n_meshes(ai_node->mChildren[i], ai_scene, n_meshes);
    }
}

static void init_meshes(aiNode* ai_node, const aiScene* ai_scene, Model& model, u32& n_verts, u32& n_indices,
                        u32& n_textures) {
    for (int mesh_idx = 0; mesh_idx < ai_node->mNumMeshes; ++mesh_idx) {
        aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[mesh_idx]];

        Mesh mesh = { .n_indices = ai_mesh->mNumFaces * 3,
                      .base_vert = n_verts,
                      .base_idx = n_indices,
                      .matl_offset = n_textures,
                      .n_matls = 0,
                      .flags = MeshFlags::NONE };

        model.meshes.push_back(mesh);
        n_verts += ai_mesh->mNumVertices;
        n_indices += model.meshes.back().n_indices;

        if (ai_mesh->mMaterialIndex >= 0) {
            aiMaterial* matl = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
            model.meshes.back().n_matls =
                matl->GetTextureCount(aiTextureType_DIFFUSE) + matl->GetTextureCount(aiTextureType_SPECULAR) +
                matl->GetTextureCount(aiTextureType_HEIGHT) + matl->GetTextureCount(aiTextureType_NORMALS) +
                matl->GetTextureCount(aiTextureType_DISPLACEMENT);
            n_textures += model.meshes.back().n_matls;
        }
    }

    for (int idx = 0; idx < ai_node->mNumChildren; ++idx) {
        init_meshes(ai_node->mChildren[idx], ai_scene, model, n_verts, n_indices, n_textures);
    }
}

static void process_assimp_node(TextureManager& manager, aiNode* ai_node, const aiScene* ai_scene, Model& model,
                                const fs::path& root_path, u32& mesh_offset) {
    for (int mesh_idx = 0; mesh_idx < ai_node->mNumMeshes; ++mesh_idx) {
        aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[mesh_idx]];

        for (int j = 0; j < ai_mesh->mNumVertices; ++j) {
            model.pos.push_back({ ai_mesh->mVertices[j].x, ai_mesh->mVertices[j].y, ai_mesh->mVertices[j].z });
            model.norms.push_back({ ai_mesh->mNormals[j].x, ai_mesh->mNormals[j].y, ai_mesh->mNormals[j].z });
            // note: right now I am just using nil values for these if not present
            // can be changed in the future to reduce memory consumption
            glm::vec3 tan = { 0.0f, 0.0f, 0.0f };
            if (ai_mesh->mTangents) {
                tan = { ai_mesh->mTangents[j].x, ai_mesh->mTangents[j].y, ai_mesh->mTangents[j].z };
            }
            model.tangents.push_back(tan);
            glm::vec2 uv = { 0.0f, 0.0f };
            if (ai_mesh->mTextureCoords[0]) {
                uv = { ai_mesh->mTextureCoords[0][j].x, ai_mesh->mTextureCoords[0][j].y };
            }
            model.uvs.push_back(uv);
        }

        for (int face_idx = 0; face_idx < ai_mesh->mNumFaces; ++face_idx) {
            aiFace face = ai_mesh->mFaces[face_idx];
            for (int ind_idx = 0; ind_idx < face.mNumIndices; ++ind_idx) {
                model.indices.push_back(face.mIndices[ind_idx]);
            }
        }

        // Loading material textures
        if (ai_mesh->mMaterialIndex >= 0) {
            aiMaterial* matl = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
            load_matl_textures(manager, model, matl, aiTextureType_DIFFUSE, root_path, mesh_offset + mesh_idx);
            load_matl_textures(manager, model, matl, aiTextureType_SPECULAR, root_path, mesh_offset + mesh_idx);
            load_matl_textures(manager, model, matl, aiTextureType_HEIGHT, root_path, mesh_offset + mesh_idx);
            load_matl_textures(manager, model, matl, aiTextureType_NORMALS, root_path, mesh_offset + mesh_idx);
            load_matl_textures(manager, model, matl, aiTextureType_DISPLACEMENT, root_path, mesh_offset + mesh_idx);
        }
    }

    mesh_offset += ai_node->mNumMeshes;

    for (int i = 0; i < ai_node->mNumChildren; ++i) {
        process_assimp_node(manager, ai_node->mChildren[i], ai_scene, model, root_path, mesh_offset);
    }
}

std::optional<rses> Model::load(TextureManager& manager, const fs::path& path) {
    Assimp::Importer import;

    auto flags = aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_CalcTangentSpace |
                 aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs;

    const aiScene* scene =
        import.ReadFile(path.generic_string(), flags);

    if (!scene) {
        return rses().io("Error importing scene : {}", import.GetErrorString());
    }

    fs::path root_path = path.parent_path();

    // determine number of meshes to reserve their space
    u32 n_meshes = 0;
    get_n_meshes(scene->mRootNode, scene, n_meshes);
    meshes.reserve(n_meshes);

    // reserve space for all remaining buffers
    u32 n_verts = 0;
    u32 n_indices = 0;
    u32 n_textures = 0;
    init_meshes(scene->mRootNode, scene, *this, n_verts, n_indices, n_textures);

    indices.reserve(n_indices);
    pos.reserve(n_verts);
    norms.reserve(n_verts);
    tangents.reserve(n_verts);
    uvs.reserve(n_verts);
    textures.reserve(n_textures);

    // fill out buffers and initialize opengl data
    u32 mesh_offset = 0;
    process_assimp_node(manager, scene->mRootNode, scene, *this, root_path, mesh_offset);

    render_data.init(pos, norms, tangents, uvs, indices);

    return std::nullopt;
}

Model Model::copy() { 
    
    Model model;
    model.model_mat = model_mat;
    model.meshes = meshes;
    model.textures = textures;
    model.indices = indices;
    model.pos = pos;
    model.norms = norms;
    model.tangents = tangents;
    model.uvs = uvs;

    render_data.init(pos, norms, tangents, uvs, indices);

    return model;
}

void Model::GL_render(gl::Shader& shader, const gl::PlatformState& state) const {

    shader.use();
    shader.set_mat4("model", model_mat);
    glBindVertexArray(render_data.vao);

    for (auto& mesh : meshes) {
        shader.set_bool("material.has_diffuse_map", false);
        shader.set_bool("material.has_normal_map", false);
        shader.set_bool("material.has_specular_map", false);
        for (u32 idx = mesh.matl_offset; idx < mesh.matl_offset + mesh.n_matls; ++idx) {
            switch (textures[idx].ref->ty) {
            case TextureType::DIFFUSE:
                shader.set_bool("material.has_diffuse_map", true);
                shader.set_tex("material.diffuse_map", 0, textures[idx].ref->id);
                break;
            case TextureType::SPECULAR:
                shader.set_bool("material.has_specular_map", true);
                shader.set_tex("material.specular_map", 1, textures[idx].ref->id);
                break;
            case TextureType::NORMAL:
                shader.set_bool("material.has_normal_map", true);
                shader.set_tex("material.normal_map", 2, textures[idx].ref->id);
                break;
            case TextureType::DISPLACE:
                shader.set_tex("material.displace_map", 3, textures[idx].ref->id);
                break;
            default:
                break;
            }
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, mesh.n_indices, GL_UNSIGNED_INT,
                                    (void*)(sizeof(u32) * mesh.base_idx), mesh.base_vert);
    }
}

void Model::GL_render(gl::Shader& shader, const gl::PlatformState& state, MeshFlags mesh_cond, bool invert_cond) const {
    shader.use();
    shader.set_mat4("model", model_mat);
    glBindVertexArray(render_data.vao);

    for (auto& mesh : meshes) {
        
        if (!invert_cond && ((mesh.flags & mesh_cond) == MeshFlags::NONE) || 
            (invert_cond && ((mesh.flags & mesh_cond) != MeshFlags::NONE))) {
            // do not render meshes that do not meet the provided condition
            continue;
        }

        shader.set_bool("material.has_diffuse_map", false);
        shader.set_bool("material.has_normal_map", false);
        shader.set_bool("material.has_specular_map", false);

        for (u32 idx = mesh.matl_offset; idx < mesh.matl_offset + mesh.n_matls; ++idx) {
            switch (textures[idx].ref->ty) {
            case TextureType::DIFFUSE:
                shader.set_bool("material.has_diffuse_map", true);
                shader.set_tex("material.diffuse_map", 0, textures[idx].ref->id);
                break;
            case TextureType::SPECULAR:
                shader.set_bool("material.has_specular_map", true);
                shader.set_tex("material.specular_map", 1, textures[idx].ref->id);
                break;
            case TextureType::NORMAL:
                shader.set_bool("material.has_normal_map", true);
                shader.set_tex("material.normal_map", 2, textures[idx].ref->id);
                break;
            case TextureType::DISPLACE:
                shader.set_tex("material.displace_map", 3, textures[idx].ref->id);
                break;
            default:
                break;
            }
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, mesh.n_indices, GL_UNSIGNED_INT, (void*)(sizeof(u32) * mesh.base_idx),
                                 mesh.base_vert);
    }
}

SkyBox::SkyBox(SkyBox&& other) noexcept {
    vao = other.vao;
    other.vao = 0;
    verts_buf = other.verts_buf;
    other.verts_buf = 0;
    texture = std::move(other.texture);
    verts = std::move(other.verts);
    model_mat = std::move(other.model_mat);
}

SkyBox& SkyBox::operator=(SkyBox&& other) noexcept {
    if (this == &other) return *this;
    this->~SkyBox();
    new (this) SkyBox(std::move(other));
    return *this;
}

SkyBox::~SkyBox() {
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &verts_buf);
    }
}

void SkyBox::init() {
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &verts_buf);
    glNamedBufferStorage(verts_buf, verts.size() * sizeof(glm::vec3), verts.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(vao, 0, verts_buf, 0, sizeof(glm::vec3));
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
}

std::optional<rses> SkyBox::load(TextureManager& manager, const std::array<fs::path, 6>& paths) {
    std::expected<TextureRef, rses> tex = manager.load_cubemap(paths);
    if (!tex) {
        return tex.error().io("unable to load skybox");
    }
    texture = tex.value();
    return std::nullopt;
}

void SkyBox::draw(gl::Shader& shader, const gl::PlatformState& state) const {
    glDepthMask(GL_FALSE);
    shader.use();
    glBindVertexArray(vao);
    shader.set_tex("cube_map", 0, texture.ref->id);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}
