#include <err.hpp>
#include <logger.hpp>
#include <model.hpp>

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <format>
#include <unordered_map>

namespace fs = std::filesystem;

namespace rose {

// Used for avoiding loading the same texture from disk multiple times
namespace globals {
static std::unordered_map<std::string, Texture> loaded_textures;
} // namespace globals

Mesh::Mesh(std::vector<Vertex> verts, std::vector<uint32_t> indices, std::vector<Texture> textures)
    : verts(verts), indices(indices), textures(textures) {
    init();
}

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
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, norm));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex));

    glBindVertexArray(0);
}

void Mesh::draw(ShaderGL& shader) const {

    shader.use();
    uint32_t diff_n = 0;
    uint32_t spec_n = 0;

    for (uint32_t i = 0; i < textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        switch (textures[i].type) {
        case Texture::Type::DIFFUSE:
            shader.set_int(std::format("materials[{}].diffuse_map", diff_n), i);
            diff_n++;
            break;
        case Texture::Type::SPECULAR:
            shader.set_int(std::format("materials[{}].specular_map", spec_n), i);
            spec_n++;
            break;
        default:
            assert(false);
            break;
        }
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
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

void Model::draw(ShaderGL& shader) const {
    shader.use();
    shader.set_mat4("model", model);
    for (auto& mesh : meshes) {
        mesh.draw(shader);
    }
}

static void process_assimp_node(aiNode* ai_node, const aiScene* ai_scene, std::vector<Mesh>& meshes,
                                const fs::path& root_path);

std::optional<rses> Model::load(const fs::path& path) {
    Assimp::Importer import;

    // note: for dx12 this will need the aiProcess_MakeLeftHanded flag set
    const aiScene* scene =
        import.ReadFile(path.generic_string(), aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_FlipUVs);

    fs::path root_path = path.parent_path();

    if (!scene) {
        return rses().io("Error importing scene : {}", import.GetErrorString());
    }

    process_assimp_node(scene->mRootNode, scene, meshes, root_path);

    return std::nullopt;
}

static std::vector<Texture> load_mat_textures(aiMaterial* mat, aiTextureType ty, const fs::path& root_path) {

    std::vector<Texture> textures;
    textures.reserve(mat->GetTextureCount(ty));

    for (int i = 0; i < mat->GetTextureCount(ty); ++i) {
        aiString ai_str;
        mat->GetTexture(ty, i, &ai_str);
        fs::path texture_path = root_path / std::string(ai_str.C_Str());

        // check to see if the texture has been loaded previously before loading from disk
        if (globals::loaded_textures.contains(texture_path.generic_string())) {
            textures.push_back(globals::loaded_textures[texture_path.generic_string()]);
        } else {
            std::optional<uint32_t> text_id = load_texture(texture_path);
            if (!text_id.has_value()) {
                LOG_ERROR("Unable to load texture at path: {}", texture_path.generic_string());
                continue;
            }

            Texture texture = { .id = text_id.value(), .type = Texture::Type::NONE, .path = texture_path };
            switch (ty) {
            case aiTextureType_DIFFUSE:
                texture.type = Texture::Type::DIFFUSE;
                break;
            case aiTextureType_SPECULAR:
                texture.type = Texture::Type::SPECULAR;
                break;
            }
            textures.push_back(texture);
            globals::loaded_textures[texture_path.generic_string()] = texture;
        }
    }
    return textures;
}

static void process_assimp_node(aiNode* ai_node, const aiScene* ai_scene, std::vector<Mesh>& meshes,
                                const fs::path& root_path) {
    for (int i = 0; i < ai_node->mNumMeshes; ++i) {
        aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        std::vector<Mesh::Vertex> verts;
        std::vector<uint32_t> indices;
        std::vector<Texture> textures;

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
            std::vector<Texture> diff_maps = load_mat_textures(material, aiTextureType_DIFFUSE, root_path);
            std::vector<Texture> spec_maps = load_mat_textures(material, aiTextureType_SPECULAR, root_path);
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
        process_assimp_node(ai_node->mChildren[i], ai_scene, meshes, root_path);
    }
}

Cube::Cube() { init(); }

Cube::Cube(Cube&& other) noexcept {
    VAO = other.VAO;
    other.VAO = 0;
    VBO = other.VBO;
    other.VBO = 0;
    verts = std::move(other.verts);
}

Cube& Cube::operator=(Cube&& other) noexcept {
    if (this == &other) return *this;
    this->~Cube();
    new (this) Cube(std::move(other));
    return *this;
}

void Cube::init() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, norm));

    glBindVertexArray(0);
}

void Cube::draw(ShaderGL& shader) const {
    shader.use();
    shader.set_mat4("model", model);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, verts.size());
    glBindVertexArray(0);
}

Cube::~Cube() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

} // namespace rose