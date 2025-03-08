#include <rose/model.hpp>
#include <rose/gl/gl_platform.hpp>
#include <rose/core/err.hpp>

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <format>
#include <unordered_map>

namespace rose {

Model::Model(Model&& other) noexcept {
    vao = other.vao;
    other.vao = 0;
    pos_buf = other.pos_buf;
    other.pos_buf = 0;
    norm_buf = other.norm_buf;
    other.norm_buf = 0;
    tangent_buf = other.tangent_buf;
    other.tangent_buf = 0;
    uv_buf = other.uv_buf;
    other.uv_buf = 0;
    indices_buf = other.indices_buf;
    other.indices_buf = 0;
    pos = std::move(other.pos);
    norm = std::move(other.norm);
    tangent = std::move(other.tangent);
    uv = std::move(other.uv);
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

void Model::init_gl() {
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &pos_buf);
    glCreateBuffers(1, &norm_buf);
    glCreateBuffers(1, &tangent_buf);
    glCreateBuffers(1, &uv_buf);
    glCreateBuffers(1, &indices_buf);

    glNamedBufferStorage(pos_buf, pos.size() * sizeof(glm::vec3), pos.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(norm_buf, norm.size() * sizeof(glm::vec3), norm.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(tangent_buf, tangent.size() * sizeof(glm::vec3), tangent.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(uv_buf, uv.size() * sizeof(glm::vec2), uv.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(indices_buf, indices.size() * sizeof(u32), indices.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayElementBuffer(vao, indices_buf);

    glVertexArrayVertexBuffer(vao, 0, pos_buf, 0, sizeof(glm::vec3));
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glEnableVertexArrayAttrib(vao, 0);

    glVertexArrayVertexBuffer(vao, 1, norm_buf, 0, sizeof(glm::vec3));
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 1, 1);
    glEnableVertexArrayAttrib(vao, 1);

    glVertexArrayVertexBuffer(vao, 2, tangent_buf, 0, sizeof(glm::vec3));
    glVertexArrayAttribFormat(vao, 2, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 2, 2);
    glEnableVertexArrayAttrib(vao, 2);

    glVertexArrayVertexBuffer(vao, 3, uv_buf, 0, sizeof(glm::vec2));
    glVertexArrayAttribFormat(vao, 3, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 3, 3);
    glEnableVertexArrayAttrib(vao, 3);
}

Model::~Model() {
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &pos_buf);
        glDeleteBuffers(1, &norm_buf);
        glDeleteBuffers(1, &tangent_buf);
        glDeleteBuffers(1, &uv_buf);
        glDeleteBuffers(1, &indices_buf);
    }
}

void Model::draw(GL_Shader& shader, const GL_PlatformState& state) const {
    shader.use();
    shader.set_mat4("model", model_mat);
    glBindVertexArray(vao);

    for (auto& mesh : meshes) {
        shader.set_bool("material.has_normal_map", false);
        shader.set_bool("material.has_specular_map", false);
        for (u32 idx = mesh.matl_offset; idx < mesh.matl_offset + mesh.n_matls; ++idx) {
            switch (textures[idx].ref->ty) {
            case TextureType::DIFFUSE:
                glBindTextureUnit(0, textures[idx].ref->id);
                shader.set_int("material.diffuse_map", 0);
                break;
            case TextureType::SPECULAR:
                glBindTextureUnit(1, textures[idx].ref->id);
                shader.set_int("material.specular_map", 1);
                shader.set_bool("material.has_specular_map", true);
                break;
            case TextureType::NORMAL:
                glBindTextureUnit(2, textures[idx].ref->id);
                shader.set_int("material.normal_map", 2);
                shader.set_bool("material.has_normal_map", true);
                break;
            case TextureType::DISPLACE:
                glBindTextureUnit(3, textures[idx].ref->id);
                shader.set_int("material.displace_map", 3);
                break;
            default:
                break;
            }
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, mesh.n_indices, GL_UNSIGNED_INT, (void*)(sizeof(u32) * mesh.base_idx),
                                 mesh.base_vert);
    }
}

static void load_matl_textures(TextureManager& manager, Model& model, aiMaterial* mat, aiTextureType ty,
                               const fs::path& root_path) {
    for (int i = 0; i < mat->GetTextureCount(ty); ++i) {
        aiString ai_str;
        mat->GetTexture(ty, i, &ai_str);
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
    }
}

// determine the number of meshes in the model
static void get_n_meshes(aiNode* ai_node, const aiScene* ai_scene, u32& n_meshes) {
    n_meshes += ai_node->mNumMeshes;
    for (int i = 0; i < ai_node->mNumChildren; ++i) {
        get_n_meshes(ai_node->mChildren[i], ai_scene, n_meshes);
    }
}

static void init_meshes(aiNode* ai_node, const aiScene* ai_scene, Model& model, u32& n_verts, u32& n_indices, u32& n_textures) {
    for (int i = 0; i < ai_node->mNumMeshes; ++i) {
        aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];

        Mesh mesh = {
                        .n_indices = ai_mesh->mNumFaces * 3,
                        .base_vert = n_verts,
                        .base_idx = n_indices,
                        .matl_offset = n_textures,
                        .n_matls = 0
                    };

        model.meshes.push_back(mesh);
        n_verts += ai_mesh->mNumVertices;
        n_indices += model.meshes.back().n_indices;

        if (ai_mesh->mMaterialIndex >= 0) {
            aiMaterial* matl = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
            model.meshes.back().n_matls = matl->GetTextureCount(aiTextureType_DIFFUSE) + matl->GetTextureCount(aiTextureType_SPECULAR) +
                matl->GetTextureCount(aiTextureType_HEIGHT) + matl->GetTextureCount(aiTextureType_DISPLACEMENT);
            n_textures += model.meshes.back().n_matls;
        }
    }

    for (int i = 0; i < ai_node->mNumChildren; ++i) {
        init_meshes(ai_node->mChildren[i], ai_scene, model, n_verts, n_indices, n_textures);
    }
}

static void process_assimp_node(TextureManager& manager, aiNode* ai_node, const aiScene* ai_scene, Model& model,
                                const fs::path& root_path) {
    for (int i = 0; i < ai_node->mNumMeshes; ++i) {
        aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];

        for (int j = 0; j < ai_mesh->mNumVertices; ++j) {
            model.pos.push_back({ ai_mesh->mVertices[j].x, ai_mesh->mVertices[j].y, ai_mesh->mVertices[j].z });
            model.norm.push_back({ ai_mesh->mNormals[j].x, ai_mesh->mNormals[j].y, ai_mesh->mNormals[j].z });
            model.tangent.push_back({ ai_mesh->mTangents[j].x, ai_mesh->mTangents[j].y, ai_mesh->mTangents[j].z });
            glm::vec2 uv = { 0.0f, 0.0f };
            if (ai_mesh->mTextureCoords) {
                uv = { ai_mesh->mTextureCoords[0][j].x, ai_mesh->mTextureCoords[0][j].y };
            }
            model.uv.push_back(uv);
        }

        for (int i = 0; i < ai_mesh->mNumFaces; ++i) {
            aiFace face = ai_mesh->mFaces[i];
            for (int j = 0; j < face.mNumIndices; ++j) {
                model.indices.push_back(face.mIndices[j]);
            }
        }

        // Loading material textures
        if (ai_mesh->mMaterialIndex >= 0) {
            aiMaterial* matl = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
            load_matl_textures(manager, model, matl, aiTextureType_DIFFUSE, root_path);
            load_matl_textures(manager, model, matl, aiTextureType_SPECULAR, root_path);
            load_matl_textures(manager, model, matl, aiTextureType_HEIGHT, root_path);
            load_matl_textures(manager, model, matl, aiTextureType_DISPLACEMENT, root_path);
        }
    }

    for (int i = 0; i < ai_node->mNumChildren; ++i) {
        process_assimp_node(manager, ai_node->mChildren[i], ai_scene, model, root_path);
    }
}

std::optional<rses> Model::load(TextureManager& manager, const fs::path& path) {
    Assimp::Importer import;

    // note: for dx12 this will need the aiProcess_MakeLeftHanded flag set
    const aiScene* scene =
        import.ReadFile(path.generic_string(), aiProcess_GenSmoothNormals | aiProcess_Triangulate |
                                                   aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

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
    norm.reserve(n_verts);
    tangent.reserve(n_verts);
    uv.reserve(n_verts);
    textures.reserve(n_textures);

    // fill out buffers and initialize opengl data
    process_assimp_node(manager, scene->mRootNode, scene, *this, root_path);
    init_gl();

    return std::nullopt;
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
    glNamedBufferStorage(verts_buf, verts.size() * sizeof(Vertex), verts.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(vao, 0, verts_buf, 0, sizeof(Vertex));
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
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

void SkyBox::draw(GL_Shader& shader, const GL_PlatformState& state) const {
    glDepthMask(GL_FALSE);
    shader.use();
    glBindVertexArray(vao);
    glBindTextureUnit(0, texture.ref->id);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

} // namespace rose