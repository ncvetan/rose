#include <err.hpp>
#include <mesh.hpp>

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <format>
#include <logger.hpp>

namespace rose {

Mesh::Mesh(std::vector<Vertex> verts, std::vector<uint32_t> indices, std::vector<Texture> textures)
    : verts(verts), indices(indices), textures(textures) {}

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
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, norm));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex));

    glBindVertexArray(0);
}

void Mesh::draw(ShaderGL& shader) {

    uint32_t diff_n = 1;
    uint32_t spec_n = 1;

    for (uint32_t i = 0; i < textures.size(); ++i) {

        glActiveTexture(GL_TEXTURE0 + i);

        switch (textures[i].type) {
        case Texture::Type::DIFFUSE:
            shader.set_float(std::format("material.texture_diffuse{}", diff_n), i);
            diff_n++;
            break;
        case Texture::Type::SPECULAR:
            shader.set_float(std::format("material.texture_specular{}", spec_n), i);
            spec_n++;
            break;
        default:
            assert(false);
            break;
        }

        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Model::draw(ShaderGL& shader) {
    for (auto& mesh : meshes) {
        mesh.draw(shader);
    }
}

std::optional<rses> Model::load(const std::filesystem::path& path) {
    Assimp::Importer import;
    // note: for dx12 this will need the aiProcess_MakeLeftHanded flag set
    const aiScene* scene =
        import.ReadFile(path.generic_string(), aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene) {
        return rses().io("Error importing scene : {}", import.GetErrorString());
    }

    meshes = {};

    // todo: Do loading here

    return std::nullopt;
}

static Mesh process_assimp_mesh(aiMesh* ai_mesh, const aiScene* ai_scene) {

    std::vector<Vertex> verts;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;

    for (int i = 0; i < ai_mesh->mNumVertices; ++i) {
        Vertex vertex;
        vertex.pos = { ai_mesh->mVertices[i].x, ai_mesh->mVertices[i].y, ai_mesh->mVertices[i].z };
        vertex.norm = { ai_mesh->mNormals[i].x, ai_mesh->mNormals[i].y, ai_mesh->mNormals[i].z };
        if (ai_mesh->mTextureCoords) vertex.tex = { ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y };
        else vertex.tex = { 0.0f, 0.0f };
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
        std::vector<Texture> diff_maps;
        std::vector<Texture> spec_maps;

        for (int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); ++i) {
            aiString path;
            material->GetTexture(aiTextureType_DIFFUSE, i, &path);
            Texture tex;
            tex.init(path.C_Str());
            tex.type = Texture::Type::DIFFUSE;
            diff_maps.push_back(tex);
        }

        for (int i = 0; i < material->GetTextureCount(aiTextureType_SPECULAR); ++i) {
            aiString path;
            material->GetTexture(aiTextureType_SPECULAR, i, &path);
            Texture tex;
            tex.init(path.C_Str());
            tex.type = Texture::Type::SPECULAR;
            spec_maps.push_back(tex);
        }

    }

    return { verts, indices, textures };
}

static void process_assimp_node(aiNode* ai_node, const aiScene* ai_scene, std::vector<Mesh>& meshes) {
    for (int i = 0; i < ai_node->mNumMeshes; ++i) {
        aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        meshes.push_back(process_assimp_mesh(ai_mesh, ai_scene));
    }

    // meshes vector is flattened
    for (int i = 0; i < ai_node->mNumChildren; ++i) {
        process_assimp_node(ai_node->mChildren[i], ai_scene, meshes);
    }
}

} // namespace rose