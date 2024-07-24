#ifndef ROSE_INCLUDE_MODEL
#define ROSE_INCLUDE_MODEL

#include <rose/shader.hpp>
#include <rose/texture.hpp>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <concepts>
#include <filesystem>
#include <vector>

namespace rose {

namespace fs = std::filesystem;

struct GlobalState;

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

struct Mesh {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 tex;
    };

    Mesh() = delete;
    Mesh(std::vector<Vertex> verts, std::vector<u32> indices, std::vector<TextureRef> textures);
    Mesh(const Mesh& other) = delete;
    Mesh(Mesh&& other) noexcept;
    ~Mesh();

    Mesh& operator=(const Mesh& other) = delete;
    Mesh& operator=(Mesh&& other) noexcept;

    void init();
    void draw(ShaderGL& shader, const GlobalState& state) const;

    u32 id = 0;
    std::vector<Vertex> verts;
    std::vector<u32> indices;
    std::vector<TextureRef> textures;

    u32 VAO = 0;
    u32 VBO = 0;
    u32 EBO = 0;
};

struct Model {
    void draw(ShaderGL& shader, const GlobalState& state) const;
    std::optional<rses> load(const std::filesystem::path& path);
    inline void reset() { model_mat = glm::mat4(1.0f); }

    glm::mat4 model_mat = glm::mat4(1.0f);
    std::vector<Mesh> meshes;
};

struct Cube {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 norm;
    };

    Cube() = default;
    Cube(const Cube& other) = delete;
    Cube(Cube&& other) noexcept;
    ~Cube();

    Cube& operator=(const Cube& other) = delete;
    Cube& operator=(Cube&& other) noexcept;

    void init();
    void draw(ShaderGL& shader, const GlobalState& state) const;
    inline void reset() { model_mat = glm::mat4(1.0f); }

    u32 id = 0;
    glm::mat4 model_mat = glm::mat4(1.0f);
    u32 VAO = 0;
    u32 VBO = 0;

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

struct TexturedCube {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 tex;
    };

    TexturedCube() = default;
    TexturedCube(const TexturedCube& other) = delete;
    TexturedCube(TexturedCube&& other) noexcept;
    ~TexturedCube();

    TexturedCube& operator=(const TexturedCube& other) = delete;
    TexturedCube& operator=(TexturedCube&& other) noexcept;

    void init();
    std::optional<rses> load(const fs::path& diff_path, const fs::path& spec_path);
    void draw(ShaderGL& shader, const GlobalState& state) const;
    inline void reset() { model_mat = glm::mat4(1.0f); }

    u32 id = 0;
    TextureRef diff;
    TextureRef spec;
    glm::mat4 model_mat = glm::mat4(1.0f);
    u32 VAO = 0;
    u32 VBO = 0;

    std::vector<Vertex> verts = {
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, 
        { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },  
        { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, 
        { { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },

        { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },  
        { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
        { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },    
        { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
        { { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },   
        { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },

        { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },   
        { { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } }, 
        { { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },  
        { { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },

        { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },    
        { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
        { { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },   
        { { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
        { { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },    
        { { 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },

        { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }, 
        { { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
        { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },   
        { { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
        { { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },  
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },

        { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },  
        { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
        { { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },   
        { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
        { { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },  
        { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } }
    };
};

struct TexturedQuad {
    struct Vertex {
        glm::vec3 pos;
        glm::vec2 tex;
    };

    TexturedQuad() = default;
    TexturedQuad(const TexturedQuad& other) = delete;
    TexturedQuad(TexturedQuad&& other) noexcept;
    ~TexturedQuad();

    TexturedQuad& operator=(const TexturedQuad& other) = delete;
    TexturedQuad& operator=(TexturedQuad&& other) noexcept;

    void init();
    std::optional<rses> load(const fs::path& path);
    void draw(ShaderGL& shader, const GlobalState& state) const;
    inline void reset() { model_mat = glm::mat4(1.0f); }

    u32 id = 0;
    TextureRef texture;
    glm::mat4 model_mat = glm::mat4(1.0f);
    u32 VAO = 0;
    u32 VBO = 0;

    std::vector<Vertex> verts = {
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },   { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } }
    };
};

struct SkyBox {

    struct Vertex {
        glm::vec3 pos;
    };

    SkyBox() = default;
    SkyBox(const SkyBox& other) = delete;
    SkyBox(SkyBox&& other) noexcept;
    ~SkyBox();

    SkyBox& operator=(const SkyBox& other) = delete;
    SkyBox& operator=(SkyBox&& other) noexcept;

    void init();
    std::optional<rses> load(const std::vector<fs::path>& paths);
    void draw(ShaderGL& shader, const GlobalState& state) const;
    inline void reset() { model_mat = glm::mat4(1.0f); }

    u32 id = 0;
    TextureRef texture;
    glm::mat4 model_mat = glm::mat4(1.0f);
    u32 VAO = 0;
    u32 VBO = 0;

    std::vector<Vertex> verts = { { { -1.0f, 1.0f, -1.0f } },  { { -1.0f, -1.0f, -1.0f } }, { { 1.0f, -1.0f, -1.0f } },
                                  { { 1.0f, -1.0f, -1.0f } },  { { 1.0f, 1.0f, -1.0f } },   { { -1.0f, 1.0f, -1.0f } },
                                  { { -1.0f, -1.0f, 1.0f } },  { { -1.0f, -1.0f, -1.0f } }, { { -1.0f, 1.0f, -1.0f } },
                                  { { -1.0f, 1.0f, -1.0f } },  { { -1.0f, 1.0f, 1.0f } },   { { -1.0f, -1.0f, 1.0f } },
                                  { { 1.0f, -1.0f, -1.0f } },  { { 1.0f, -1.0f, 1.0f } },   { { 1.0f, 1.0f, 1.0f } },
                                  { { 1.0f, 1.0f, 1.0f } },    { { 1.0f, 1.0f, -1.0f } },   { { 1.0f, -1.0f, -1.0f } },
                                  { { -1.0f, -1.0f, 1.0f } },  { { -1.0f, 1.0f, 1.0f } },   { { 1.0f, 1.0f, 1.0f } },
                                  { { 1.0f, 1.0f, 1.0f } },    { { 1.0f, -1.0f, 1.0f } },   { { -1.0f, -1.0f, 1.0f } },
                                  { { -1.0f, 1.0f, -1.0f } },  { { 1.0f, 1.0f, -1.0f } },   { { 1.0f, 1.0f, 1.0f } },
                                  { { 1.0f, 1.0f, 1.0f } },    { { -1.0f, 1.0f, 1.0f } },   { { -1.0f, 1.0f, -1.0f } },
                                  { { -1.0f, -1.0f, -1.0f } }, { { -1.0f, -1.0f, 1.0f } },  { { 1.0f, -1.0f, -1.0f } },
                                  { { 1.0f, -1.0f, -1.0f } },  { { -1.0f, -1.0f, 1.0f } },  { { 1.0f, -1.0f, 1.0f } } };
};

} // namespace rose

#endif