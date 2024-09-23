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
u32 id_counter = 1;
} // namespace globals

static u32 gen_id() {
    u32 tmp = globals::id_counter;
    globals::id_counter++;
    return tmp;
}

Mesh::Mesh(std::vector<Vertex> verts, std::vector<u32> indices, std::vector<TextureRef> textures)
    : verts(verts), indices(indices), textures(textures) {}

Mesh::Mesh(Mesh&& other) noexcept {
    VAO = other.VAO;
    other.VAO = 0;
    VBO = other.VBO;
    other.VBO = 0;
    EBO = other.EBO;
    other.EBO = 0;
    verts = std::move(other.verts);
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

    // todo: convert to direct state access

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(u32), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, norm));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex));
    glBindVertexArray(0);
    id = gen_id();
}

void Mesh::draw(ShaderGL& shader, const GlobalState& state) const {

    shader.use();
    u32 diff_n = 0;
    u32 spec_n = 0;

    for (u32 i = 0; i < textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        switch (textures[i].ref->ty) {
        case TextureType::DIFFUSE:
            shader.set_int(std::format("materials[{}].diffuse_map", diff_n), i);
            diff_n++;
            break;
        case TextureType::SPECULAR:
            shader.set_int(std::format("materials[{}].specular_map", spec_n), i);
            spec_n++;
            break;
        default:
            assert(false);
            break;
        }
        glBindTexture(GL_TEXTURE_2D, textures[i].ref->id);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
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
            break;
        case aiTextureType_SPECULAR:
            texture = manager.load_texture(texture_path, TextureType::SPECULAR);
            break;
        }
        if (!texture.has_value()) {
            err::print(texture.error());
            continue;        
        }
        textures.push_back(texture.value());
    }
    return textures;
}

static void process_assimp_node(TextureManager& manager, aiNode* ai_node, const aiScene* ai_scene, std::vector<Mesh>& meshes,
                                const fs::path& root_path) {
    for (int i = 0; i < ai_node->mNumMeshes; ++i) {
        aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        std::vector<Mesh::Vertex> verts;
        std::vector<u32> indices;
        std::vector<TextureRef> textures;

        for (int i = 0; i < ai_mesh->mNumVertices; ++i) {
            Mesh::Vertex vertex;
            vertex.pos = { ai_mesh->mVertices[i].x, ai_mesh->mVertices[i].y, ai_mesh->mVertices[i].z };
            vertex.norm = { ai_mesh->mNormals[i].x, ai_mesh->mNormals[i].y, ai_mesh->mNormals[i].z };
            if (ai_mesh->mTextureCoords)
                vertex.tex = { ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y };
            else
                vertex.tex = { 0.0f, 0.0f };
            verts.push_back(std::move(vertex));
        }

        for (int i = 0; i < ai_mesh->mNumFaces; ++i) {
            aiFace face = ai_mesh->mFaces[i];
            for (int j = 0; j < face.mNumIndices; ++j) {
                indices.push_back(face.mIndices[j]);
            }
        }

        // Loading material textures
        if (ai_mesh->mMaterialIndex >= 0) {
            aiMaterial* material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
            std::vector<TextureRef> diff_maps = load_mat_textures(manager, material, aiTextureType_DIFFUSE, root_path);
            std::vector<TextureRef> spec_maps = load_mat_textures(manager, material, aiTextureType_SPECULAR, root_path);
            textures.reserve(diff_maps.size() + spec_maps.size());

            textures.insert(textures.end(), std::make_move_iterator(diff_maps.begin()),
                            std::make_move_iterator(diff_maps.end()));
            textures.insert(textures.end(), std::make_move_iterator(spec_maps.begin()),
                            std::make_move_iterator(spec_maps.end()));
        }

        Mesh mesh{ verts, indices, textures };
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
        import.ReadFile(path.generic_string(), aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_FlipUVs);

    fs::path root_path = path.parent_path();

    if (!scene) {
        return rses().io("Error importing scene : {}", import.GetErrorString());
    }

    process_assimp_node(manager, scene->mRootNode, scene, meshes, root_path);

    return std::nullopt;
}

Cube::Cube(Cube&& other) noexcept {
    VAO = other.VAO;
    other.VAO = 0;
    VBO = other.VBO;
    other.VBO = 0;
    verts = std::move(other.verts);
    model_mat = std::move(other.model_mat);
}

Cube& Cube::operator=(Cube&& other) noexcept {
    if (this == &other) return *this;
    this->~Cube();
    new (this) Cube(std::move(other));
    return *this;
}

void Cube::init() {
    glCreateVertexArrays(1, &VAO);
    glCreateBuffers(1, &VBO);
    glNamedBufferStorage(VBO, verts.size() * sizeof(Vertex), verts.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex));
    
    glEnableVertexArrayAttrib(VAO, 0);
    glEnableVertexArrayAttrib(VAO, 1);

    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
    glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, norm));
    
    id = gen_id();
}

void Cube::draw(ShaderGL& shader, const GlobalState& state) const {
    shader.use();
    shader.set_mat4("model", model_mat);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, verts.size());
    glBindVertexArray(0);
}

Cube::~Cube() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

TexturedCube::TexturedCube(TexturedCube&& other) noexcept {
    VAO = other.VAO;
    other.VAO = 0;
    VBO = other.VBO;
    other.VBO = 0;
    diff = std::move(other.diff);
    verts = std::move(other.verts);
    model_mat = std::move(other.model_mat);
    id = other.id;
    other.id = 0;
}

TexturedCube& TexturedCube::operator=(TexturedCube&& other) noexcept {
    if (this == &other) return *this;
    this->~TexturedCube();
    new (this) TexturedCube(std::move(other));
    return *this;
}

void TexturedCube::init() {
    glCreateVertexArrays(1, &VAO);
    glCreateBuffers(1, &VBO);
    glNamedBufferStorage(VBO, verts.size() * sizeof(Vertex), verts.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(VAO, 0);
    glEnableVertexArrayAttrib(VAO, 1);
    glEnableVertexArrayAttrib(VAO, 2);

    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
    glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, norm));
    glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, tex));

    glVertexArrayAttribBinding(VAO, 0, 0);
    glVertexArrayAttribBinding(VAO, 1, 0);
    glVertexArrayAttribBinding(VAO, 2, 0);

    id = gen_id();
}

std::optional<rses> TexturedCube::load(TextureManager& manager, const fs::path& diff_path,
                                       const fs::path& spec_path) {
    std::expected<TextureRef, rses> diff = manager.load_texture(diff_path, TextureType::DIFFUSE);
    if (!diff) {
        return diff.error();
    }
    std::expected<TextureRef, rses> spec = manager.load_texture(diff_path, TextureType::SPECULAR);
    if (!spec) {
        return spec.error();
    }
    
    this->diff = diff.value();
    this->spec = spec.value();
    return std::nullopt;
}

void TexturedCube::draw(ShaderGL& shader, const GlobalState& state) const {
    shader.use();
    shader.set_mat4("model", model_mat);
    glBindVertexArray(VAO);
    glBindTextureUnit(0, diff.ref->id);
    shader.set_int("materials[0].diffuse", 0);
    
    // todo: check if cube doesn't have a spec map, fix this
    if (spec.ref) {
        glBindTextureUnit(1, spec.ref->id);
        shader.set_int("materials[0].specular", 1);
    } 
    else {
        shader.set_int("materials[0].specular", 0);
    }
    shader.set_float("materials[0].shine", 32.0f);

    glDrawArrays(GL_TRIANGLES, 0, verts.size());
    glBindVertexArray(0);
}

TexturedCube::~TexturedCube() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

TexturedQuad::TexturedQuad(TexturedQuad&& other) noexcept {
    VAO = other.VAO;
    other.VAO = 0;
    VBO = other.VBO;
    other.VBO = 0;
    texture = std::move(other.texture);
    verts = std::move(other.verts);
    model_mat = std::move(other.model_mat);
    id = other.id;
    other.id = 0;
}

TexturedQuad& TexturedQuad::operator=(TexturedQuad&& other) noexcept {
    if (this == &other) return *this;
    this->~TexturedQuad();
    new (this) TexturedQuad(std::move(other));
    return *this;
}

void TexturedQuad::init() {
    glCreateVertexArrays(1, &VAO);
    glCreateBuffers(1, &VBO);
    glNamedBufferStorage(VBO, verts.size() * sizeof(Vertex), verts.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(VAO, 0);
    glEnableVertexArrayAttrib(VAO, 1);

    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
    glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, tex));

    glVertexArrayAttribBinding(VAO, 0, 0);
    glVertexArrayAttribBinding(VAO, 1, 0);

    id = gen_id();
}

std::optional<rses> TexturedQuad::load(TextureManager& manager, const fs::path& path) {
    std::expected<TextureRef, rses> tex = manager.load_texture(path, TextureType::DIFFUSE);
    if (!tex) {
        return tex.error();
    }
    texture = tex.value();
    return std::nullopt;
}

void TexturedQuad::draw(ShaderGL& shader, const GlobalState& state) const {
    shader.use();
    shader.set_mat4("model", model_mat);
    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.ref->id);
    shader.set_int("tex", 0);
    glDrawArrays(GL_TRIANGLES, 0, verts.size());
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

TexturedQuad::~TexturedQuad() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

SkyBox::SkyBox(SkyBox&& other) noexcept {
    VAO = other.VAO;
    other.VAO = 0;
    VBO = other.VBO;
    other.VBO = 0;
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
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void SkyBox::init() {
    glCreateVertexArrays(1, &VAO);
    glCreateBuffers(1, &VBO);
    glNamedBufferStorage(VBO, verts.size() * sizeof(Vertex), verts.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex));
    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
    glVertexArrayAttribBinding(VAO, 0, 0);
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
    glBindVertexArray(VAO);
    glBindTextureUnit(0, texture.ref->id);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

} // namespace rose