#ifndef ROSE_INCLUDE_MODEL
#define ROSE_INCLUDE_MODEL

#include <concepts>
#include <filesystem>
#include <vector>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <shader.hpp>
#include <texture.hpp>

namespace rose {

template <typename T>
concept Transformable = requires { T::model; };

template <Transformable T>
void translate(T& obj, const glm::vec3& vec) {
    obj.model = glm::translate(obj.model, vec);
}

template <Transformable T>
void scale(T& obj, float factor) {
    obj.model = glm::scale(obj.model, { factor, factor, factor });
}

template <Transformable T>
void scale(T& obj, const glm::vec3& factors) {
    obj.model = glm::scale(obj.model, factors);
}

template <Transformable T>
void rotate(T& obj, float deg, const glm::vec3& axis) {
    obj.model = glm::rotate(obj.model, glm::radians(deg), axis);
}

class Mesh {
  public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 tex;
    };

    Mesh() = delete;
    Mesh(std::vector<Vertex> verts, std::vector<uint32_t> indices, std::vector<Texture> textures);
    Mesh(const Mesh& other) = delete;
    Mesh(Mesh&& other) noexcept;
    ~Mesh();

    Mesh& operator=(const Mesh& other) = delete;
    Mesh& operator=(Mesh&& other) noexcept;

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
    void draw(ShaderGL& shader) const;
    std::optional<rses> load(const std::filesystem::path& path);

    glm::mat4 model = glm::mat4(1.0f);
    std::vector<Mesh> meshes;
};

class Cube {
  public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 norm;
    };

    Cube();
    Cube(const Cube& other) = delete;
    Cube(Cube&& other) noexcept;
    ~Cube();

    Cube& operator=(const Cube& other) = delete;
    Cube& operator=(Cube&& other) noexcept;

    void init();
    void draw(ShaderGL& shader) const;

    glm::mat4 model = glm::mat4(1.0f);
    uint32_t VAO = 0;
    uint32_t VBO = 0;

    std::vector<Vertex> verts = {
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0 } },  { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0 } },
        { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0 } },    { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0 } },
        { { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0 } },   { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0 } },
        { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },   { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
        { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },     { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
        { { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },    { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
        { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f } },   { { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f } },
        { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f } }, { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f } },
        { { -0.5f, -0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f } },  { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f } },
        { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } },     { { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
        { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },   { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
        { { 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } },    { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f } }, { { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f } },
        { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f } },   { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f } },
        { { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f } },  { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f } },
        { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },   { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },     { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },    { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } }
    };
};

} // namespace rose

#endif