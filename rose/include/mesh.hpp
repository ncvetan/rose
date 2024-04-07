#ifndef ROSE_INCLUDE_MESH
#define ROSE_INCLUDE_MESH

#include <filesystem>
#include <vector>

#include <glm.hpp>

#include <shader.hpp>
#include <texture.hpp>

namespace rose {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 tex;
};

class Mesh {
  public:
    Mesh() = default;
    Mesh(std::vector<Vertex> verts, std::vector<uint32_t> indices, std::vector<Texture> textures);

    void init();
    void draw(ShaderGL& shader) const;

    std::vector<Vertex> verts;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;

    uint32_t VAO = 0;
    uint32_t VBO = 0;
    uint32_t EBO = 0;
};

class Model {
public:
    Model() = default;

    void draw(ShaderGL& shader) const;
    std::optional<rses> load(const std::filesystem::path& path);

    std::vector<Mesh> meshes;
};

} // namespace rose

#endif