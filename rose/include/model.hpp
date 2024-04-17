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

namespace fs = std::filesystem;

template <typename T>
concept Transformable = requires { T::model_mat; };

template <Transformable T>
void translate(T& obj, const glm::vec3& vec) {
    obj.model_mat = glm::translate(obj.model_mat, vec);
}

template <Transformable T>
void scale(T& obj, float factor) {
    obj.model_mat = glm::scale(obj.model_mat, { factor, factor, factor });
}

template <Transformable T>
void scale(T& obj, const glm::vec3& factors) {
    obj.model_mat = glm::scale(obj.model_mat, factors);
}

template <Transformable T>
void rotate(T& obj, float deg, const glm::vec3& axis) {
    obj.model_mat = glm::rotate(obj.model_mat, glm::radians(deg), axis);
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

    glm::mat4 model_mat = glm::mat4(1.0f);
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

    glm::mat4 model_mat = glm::mat4(1.0f);
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

class TextureCube {
  public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 tex;
    };

    TextureCube();
    TextureCube(const TextureCube& other) = delete;
    TextureCube(TextureCube&& other) noexcept;
    ~TextureCube();

    TextureCube& operator=(const TextureCube& other) = delete;
    TextureCube& operator=(TextureCube&& other) noexcept;

    void init();
    std::optional<rses> load(const fs::path& path);
    void draw(ShaderGL& shader) const;
    inline void reset() { model_mat = glm::mat4(1.0f); }

    uint32_t id = 0;
    Texture texture;
    glm::mat4 model_mat = glm::mat4(1.0f);
    uint32_t VAO = 0;
    uint32_t VBO = 0;

    std::vector<Vertex> verts = { { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
                                  { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
                                  { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
                                  { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
                                  { { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
                                  { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
                                  { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
                                  { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
                                  { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                                  { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
                                  { { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
                                  { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
                                  { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                  { { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
                                  { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
                                  { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
                                  { { -0.5f, -0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                                  { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                  { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                  { { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
                                  { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
                                  { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
                                  { { 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                                  { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                                  { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
                                  { { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
                                  { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
                                  { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
                                  { { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
                                  { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
                                  { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
                                  { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
                                  { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                                  { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
                                  { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
                                  { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } } };
};

class TextureQuad {
  public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 tex;
    };

    TextureQuad();
    TextureQuad(const TextureQuad& other) = delete;
    TextureQuad(TextureQuad&& other) noexcept;
    ~TextureQuad();

    TextureQuad& operator=(const TextureQuad& other) = delete;
    TextureQuad& operator=(TextureQuad&& other) noexcept;

    void init();
    std::optional<rses> load(const fs::path& path);
    void draw(ShaderGL& shader) const;
    inline void reset() { model_mat = glm::mat4(1.0f); }

    uint32_t id = 0;
    Texture texture;
    glm::mat4 model_mat = glm::mat4(1.0f);
    uint32_t VAO = 0;
    uint32_t VBO = 0;

    std::vector<Vertex> verts = {
        { {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
        { {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
        { {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } }
    };
};

} // namespace rose

#endif