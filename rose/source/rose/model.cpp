#include <rose/err.hpp>
#include <rose/logger.hpp>
#include <rose/model.hpp>
#include <rose/window.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <GL/glew.h>

#include <format>
#include <unordered_map>

namespace rose {

namespace globals {
// start at one and use zero as an unitialized value
u32 id_counter = 1;
} // namespace globals

static u32 gen_id() {
    u32 tmp = globals::id_counter;
    globals::id_counter++;
    return tmp;
}

Mesh::Mesh(Mesh&& other) noexcept {
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
    id = other.id;
    other.id = 0;
    pos = std::move(other.pos);
    norm = std::move(other.norm);
    tangent = std::move(other.tangent);
    uv = std::move(other.uv);
    indices = std::move(other.indices);
    textures = std::move(other.textures);
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this == &other) return *this;
    this->~Mesh();
    new (this) Mesh(std::move(other));
    return *this;
}

void Mesh::init() {
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

    id = gen_id();
}

void Mesh::draw(ShaderGL& shader, const GlobalState& state) const {

    shader.use();
    u32 diff_n = 0;
    u32 spec_n = 0;
    u32 norm_n = 0;
    u32 disp_n = 0;

    for (u32 tex_unit = 0; tex_unit < textures.size(); ++tex_unit) {
        glActiveTexture(GL_TEXTURE0 + tex_unit);
        switch (textures[tex_unit].ref->ty) {
        case TextureType::DIFFUSE:
            shader.set_int(std::format("materials[{}].diffuse_map", diff_n), tex_unit);
            diff_n++;
            break;
        case TextureType::SPECULAR:
            shader.set_int(std::format("materials[{}].specular_map", spec_n), tex_unit);
            spec_n++;
            break;
        case TextureType::NORMAL:
            shader.set_int(std::format("materials[{}].normal_map", norm_n), tex_unit);
            norm_n++;
            break;
        case TextureType::DISPLACE:
            shader.set_int(std::format("materials[{}].displace_map", disp_n), tex_unit);
            disp_n++;
            break;
        default:
            break;
        }
        glBindTexture(GL_TEXTURE_2D, textures[tex_unit].ref->id);
    }

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glActiveTexture(GL_TEXTURE0);
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &pos_buf);
    glDeleteBuffers(1, &norm_buf);
    glDeleteBuffers(1, &tangent_buf);
    glDeleteBuffers(1, &uv_buf);
    glDeleteBuffers(1, &indices_buf);
}

void Model::draw(ShaderGL& shader, const GlobalState& state) const {
    shader.use();
    shader.set_mat4("model", model_mat);

    for (auto& mesh : meshes) {
        mesh.draw(shader, state);
    }
}

static std::vector<TextureRef> load_mat_textures(TextureManager& manager, aiMaterial* mat, aiTextureType ty, const fs::path& root_path) {

    std::vector<TextureRef> textures;
    textures.reserve(mat->GetTextureCount(ty));

    for (int i = 0; i < mat->GetTextureCount(ty); ++i) {
        
        aiString ai_str;
        mat->GetTexture(ty, i, &ai_str);
        fs::path texture_path = root_path / std::string(ai_str.C_Str());
        std::expected<TextureRef, rses> texture;
        
        switch (ty) {
        case aiTextureType_DIFFUSE:
            texture = manager.load_texture(texture_path, TextureType::DIFFUSE);
            textures.push_back(texture.value());
            break;
        case aiTextureType_SPECULAR:
            texture = manager.load_texture(texture_path, TextureType::SPECULAR);
            textures.push_back(texture.value());
            break;
        case aiTextureType_HEIGHT:
            texture = manager.load_texture(texture_path, TextureType::NORMAL);
            textures.push_back(texture.value());
            break;
        case aiTextureType_DISPLACEMENT:
            texture = manager.load_texture(texture_path, TextureType::DISPLACE);
            textures.push_back(texture.value());
            break;
        }
        if (!texture.has_value()) {
            err::print(texture.error());
            continue;        
        }
        
    }
    return textures;
}

static void process_assimp_node(TextureManager& manager, aiNode* ai_node, const aiScene* ai_scene, std::vector<Mesh>& meshes,
                                const fs::path& root_path) {
    
    for (int i = 0; i < ai_node->mNumMeshes; ++i) {
        Mesh mesh;
        aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];

        mesh.pos.reserve(ai_mesh->mNumVertices);
        mesh.norm.reserve(ai_mesh->mNumVertices);
        mesh.tangent.reserve(ai_mesh->mNumVertices);
        mesh.uv.reserve(ai_mesh->mNumVertices);

        for (int i = 0; i < ai_mesh->mNumVertices; ++i) {
            mesh.pos.push_back({ ai_mesh->mVertices[i].x, ai_mesh->mVertices[i].y, ai_mesh->mVertices[i].z });
            mesh.norm.push_back({ ai_mesh->mNormals[i].x, ai_mesh->mNormals[i].y, ai_mesh->mNormals[i].z });
            mesh.tangent.push_back({ ai_mesh->mTangents[i].x, ai_mesh->mTangents[i].y, ai_mesh->mTangents[i].z });
            glm::vec2 uv = { 0.0f, 0.0f };
            if (ai_mesh->mTextureCoords) {
                uv = { ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y };
            } 
            mesh.uv.push_back(uv);
        }

        for (int i = 0; i < ai_mesh->mNumFaces; ++i) {
            aiFace face = ai_mesh->mFaces[i];
            for (int j = 0; j < face.mNumIndices; ++j) {
                mesh.indices.push_back(face.mIndices[j]);
            }
        }

        // Loading material textures
        if (ai_mesh->mMaterialIndex >= 0) {
            aiMaterial* material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

            std::vector<TextureRef> diff_maps = load_mat_textures(manager, material, aiTextureType_DIFFUSE,      root_path);
            std::vector<TextureRef> spec_maps = load_mat_textures(manager, material, aiTextureType_SPECULAR,     root_path);
            std::vector<TextureRef> norm_maps = load_mat_textures(manager, material, aiTextureType_HEIGHT,       root_path);
            std::vector<TextureRef> disp_maps = load_mat_textures(manager, material, aiTextureType_DISPLACEMENT, root_path);

            mesh.textures.reserve(diff_maps.size() + spec_maps.size() + norm_maps.size() + disp_maps.size());

            mesh.textures.insert(mesh.textures.end(), std::make_move_iterator(diff_maps.begin()),
                                 std::make_move_iterator(diff_maps.end()));
            mesh.textures.insert(mesh.textures.end(), std::make_move_iterator(spec_maps.begin()),
                                 std::make_move_iterator(spec_maps.end()));
            mesh.textures.insert(mesh.textures.end(), std::make_move_iterator(norm_maps.begin()),
                                 std::make_move_iterator(norm_maps.end()));
            mesh.textures.insert(mesh.textures.end(), std::make_move_iterator(disp_maps.begin()),
                                 std::make_move_iterator(disp_maps.end()));
        }

        meshes.push_back(std::move(mesh));
    }

    for (int i = 0; i < ai_node->mNumChildren; ++i) {
        process_assimp_node(manager, ai_node->mChildren[i], ai_scene, meshes, root_path);
    }
}

std::optional<rses> Model::load(TextureManager& manager, const fs::path& path) {
    Assimp::Importer import;

    // note: for dx12 this will need the aiProcess_MakeLeftHanded flag set
    const aiScene* scene =
        import.ReadFile(path.generic_string(), aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_CalcTangentSpace);

    if (!scene) {
        return rses().io("Error importing scene : {}", import.GetErrorString());
    }

    fs::path root_path = path.parent_path();

    process_assimp_node(manager, scene->mRootNode, scene, meshes, root_path);

    for (auto& mesh : meshes) {
        mesh.init();
    }

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
    id = other.id;
    other.id = 0;
}

SkyBox& SkyBox::operator=(SkyBox&& other) noexcept {
    if (this == &other) return *this;
    this->~SkyBox();
    new (this) SkyBox(std::move(other));
    return *this;
}

SkyBox::~SkyBox() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &verts_buf);
}

void SkyBox::init() {
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &verts_buf);
    glNamedBufferStorage(verts_buf, verts.size() * sizeof(Vertex), verts.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(vao, 0, verts_buf, 0, sizeof(Vertex));
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
    glVertexArrayAttribBinding(vao, 0, 0);
    id = gen_id();
}

std::optional<rses> SkyBox::load(TextureManager& manager, const std::vector<fs::path>& paths) {
    std::optional<TextureRef> tex = manager.load_cubemap(paths);
    if (!tex) {
        return rses().io("unable to load skybox");
    }
    texture = tex.value();
    return std::nullopt;
}

void SkyBox::draw(ShaderGL& shader, const GlobalState& state) const {
    glDepthMask(GL_FALSE);
    shader.use();
    glBindVertexArray(vao);
    glBindTextureUnit(0, texture.ref->id);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

} // namespace rose