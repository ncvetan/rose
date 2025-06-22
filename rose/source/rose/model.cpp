#include <rose/model.hpp>
#include <rose/core/err.hpp>

#ifdef USE_OPENGL
#include <rose/backends/gl/backend.hpp>
#else
static_assert("no backend selected");
#endif 

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/GltfMaterial.h>
#include <assimp/material.h>

#include <format>
#include <unordered_map>

Model::Model(Model&& other) noexcept {
#ifdef USE_OPENGL
    render_data = std::move(other.render_data);
#else
    static_assert("no backend selected");
#endif 
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
    
    for (u32 idx = 0; idx < mat->GetTextureCount(ty); ++idx) {
        aiString ai_str;
        mat->GetTexture(ty, idx, &ai_str);
        fs::path texture_path = root_path / std::string(ai_str.C_Str());
        TextureRef texture;

        switch (ty) {
        case aiTextureType_BASE_COLOR:
            texture = manager.load_texture(texture_path, TextureType::ALBEDO);
            model.textures.push_back(texture);
            break;
        case aiTextureType_GLTF_METALLIC_ROUGHNESS:
            // NOTE: in the GLTF file format, ao (R), roughness (G) and metallic values (B) are combined
            // into a single texture
            texture = manager.load_texture(texture_path, TextureType::GLTF_PBR);
            model.textures.push_back(texture);
            break;
        case aiTextureType_AMBIENT_OCCLUSION:
            texture = manager.load_texture(texture_path, TextureType::AMBIENT_OCCLUSION);
            model.textures.push_back(texture);
            break;
        case aiTextureType_HEIGHT:
            texture = manager.load_texture(texture_path, TextureType::NORMAL);
            model.textures.push_back(texture);
            break;
        case aiTextureType_NORMALS:
            texture = manager.load_texture(texture_path, TextureType::NORMAL);
            model.textures.push_back(texture);
            break;
        case aiTextureType_DISPLACEMENT:
            texture = manager.load_texture(texture_path, TextureType::DISPLACE);
            model.textures.push_back(texture);
            break;
        default:
            break;
        }

        // TODO: a bit hacky, would like to refactor model loading to better handle these sorts of cases
        if (texture.ref && is_flag_set(texture.ref->flags, TextureFlags::TRANSPARENT)) {
            model.meshes[mesh_idx].flags |= MeshFlags::TRANSPARENT;
        }
    }
}

// determine the number of meshes in the model
static void get_n_meshes(aiNode* ai_node, const aiScene* ai_scene, u32& n_meshes) {
    n_meshes += ai_node->mNumMeshes;
    for (u32 idx = 0; idx < ai_node->mNumChildren; ++idx) {
        get_n_meshes(ai_node->mChildren[idx], ai_scene, n_meshes);
    }
}

static void init_meshes(aiNode* ai_node, const aiScene* ai_scene, Model& model, u32& n_verts, u32& n_indices,
                        u32& n_textures) {
    for (u32 mesh_idx = 0; mesh_idx < ai_node->mNumMeshes; ++mesh_idx) {
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
                matl->GetTextureCount(aiTextureType_BASE_COLOR) + matl->GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS) + 
                matl->GetTextureCount(aiTextureType_HEIGHT) + matl->GetTextureCount(aiTextureType_NORMALS) + 
                matl->GetTextureCount(aiTextureType_DISPLACEMENT) + matl->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION);
            n_textures += model.meshes.back().n_matls;
        }
    }

    for (u32 idx = 0; idx < ai_node->mNumChildren; ++idx) {
        init_meshes(ai_node->mChildren[idx], ai_scene, model, n_verts, n_indices, n_textures);
    }
}

static void process_assimp_node(TextureManager& manager, aiNode* ai_node, const aiScene* ai_scene, Model& model,
                                const fs::path& root_path, u32& mesh_offset) {
    for (u32 mesh_idx = 0; mesh_idx < ai_node->mNumMeshes; ++mesh_idx) {
        aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[mesh_idx]];

        for (int vert_idx = 0; vert_idx < ai_mesh->mNumVertices; ++vert_idx) {
            model.pos.push_back({ ai_mesh->mVertices[vert_idx].x, ai_mesh->mVertices[vert_idx].y, ai_mesh->mVertices[vert_idx].z });
            model.norms.push_back({ ai_mesh->mNormals[vert_idx].x, ai_mesh->mNormals[vert_idx].y, ai_mesh->mNormals[vert_idx].z });
            // note: right now I am just using nil values for these if not present
            // can be changed in the future to reduce memory consumption
            glm::vec3 tan = { 0.0f, 0.0f, 0.0f };
            if (ai_mesh->mTangents) {
                tan = { ai_mesh->mTangents[vert_idx].x, ai_mesh->mTangents[vert_idx].y, ai_mesh->mTangents[vert_idx].z };
            }
            model.tangents.push_back(tan);
            glm::vec2 uv = { 0.0f, 0.0f };
            if (ai_mesh->mTextureCoords[0]) {
                uv = { ai_mesh->mTextureCoords[0][vert_idx].x, ai_mesh->mTextureCoords[0][vert_idx].y };
            }
            model.uvs.push_back(uv);
        }

        for (u32 face_idx = 0; face_idx < ai_mesh->mNumFaces; ++face_idx) {
            aiFace face = ai_mesh->mFaces[face_idx];
            for (int ind_idx = 0; ind_idx < face.mNumIndices; ++ind_idx) {
                model.indices.push_back(face.mIndices[ind_idx]);
            }
        }

        // Loading material textures
        if (ai_mesh->mMaterialIndex >= 0) {
            aiMaterial* matl = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
            load_matl_textures(manager, model, matl, aiTextureType_BASE_COLOR, root_path, mesh_offset + mesh_idx);
            load_matl_textures(manager, model, matl, aiTextureType_GLTF_METALLIC_ROUGHNESS, root_path, mesh_offset + mesh_idx);
            load_matl_textures(manager, model, matl, aiTextureType_AMBIENT_OCCLUSION, root_path, mesh_offset + mesh_idx);
            load_matl_textures(manager, model, matl, aiTextureType_HEIGHT, root_path, mesh_offset + mesh_idx);
            load_matl_textures(manager, model, matl, aiTextureType_NORMALS, root_path, mesh_offset + mesh_idx);
            load_matl_textures(manager, model, matl, aiTextureType_DISPLACEMENT, root_path, mesh_offset + mesh_idx);
        }
    }

    mesh_offset += ai_node->mNumMeshes;

    for (u32 idx = 0; idx < ai_node->mNumChildren; ++idx) {
        process_assimp_node(manager, ai_node->mChildren[idx], ai_scene, model, root_path, mesh_offset);
    }
}

void Model::load(TextureManager& manager, const fs::path& path) {
    Assimp::Importer import;

    auto flags = aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_CalcTangentSpace |
                 aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs;

    const aiScene* scene =
        import.ReadFile(path.generic_string(), flags);

    if (!scene) {
        return;
    }

    fs::path root_path = path.parent_path();

    // 1. determine number of meshes to reserve their space
    u32 n_meshes = 0;
    get_n_meshes(scene->mRootNode, scene, n_meshes);
    meshes.reserve(n_meshes);

    // 2. reserve space for all remaining buffers
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

    // 3. fill out buffers and initialize renderer data
    u32 mesh_offset = 0;
    process_assimp_node(manager, scene->mRootNode, scene, *this, root_path, mesh_offset);

#ifdef USE_OPENGL
    render_data.init(pos, norms, tangents, uvs, indices);
#else
    static_assert("no backend selected");
#endif 

    return;
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

#ifdef USE_OPENGL
    model.render_data.init(pos, norms, tangents, uvs, indices);
#else
    static_assert("no backend selected");
#endif 

    return model;  
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

void SkyBox::load(TextureManager& manager, const std::array<fs::path, 6>& paths) {
    texture = manager.load_cubemap(paths);
}
