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

struct GlobalState;

template <typename T>
concept Transformable = requires { T::model_mat; };

template <Transformable T>
void translate(T& obj, const glm::vec3& vec) {
    obj.model_mat = glm::translate(obj.model_mat, vec);
}

template <Transformable T>
void scale(T& obj, f32 factor) {
    obj.model_mat = glm::scale(obj.model_mat, { factor, factor, factor });
}

template <Transformable T>
void scale(T& obj, const glm::vec3& factors) {
    obj.model_mat = glm::scale(obj.model_mat, factors);
}

template <Transformable T>
void rotate(T& obj, f32 deg, const glm::vec3& axis) {
    obj.model_mat = glm::rotate(obj.model_mat, glm::radians(deg), axis);
}

struct Mesh {
    u64 n_indices = 0;
    u64 base_vert = 0;
    u64 base_idx = 0;
    u32 matl_offset = 0;
    u32 n_matls = 0;
};

struct Model {
    
    Model() = default;

    Model(Model&& other) noexcept;

    Model& operator=(Model&& other) noexcept;

    ~Model();
    
    void draw(ShaderGL& shader, const GlobalState& state) const;
    
    std::optional<rses> load(TextureManager& manager, const std::filesystem::path& path);
    
    // this should only be called once and after vectors have been populated
    void init_gl();
    
    inline void reset() { model_mat = glm::mat4(1.0f); }

    glm::mat4 model_mat = glm::mat4(1.0f);
    std::vector<Mesh> meshes;
    std::vector<TextureRef> textures;

    std::vector<u32> indices;
    std::vector<glm::vec3> pos;
    std::vector<glm::vec3> norm;
    std::vector<glm::vec3> tangent;
    std::vector<glm::vec2> uv;

    u32 vao = 0;
    u32 pos_buf = 0;
    u32 norm_buf = 0;
    u32 tangent_buf = 0;
    u32 uv_buf = 0;
    u32 indices_buf = 0;
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
    std::optional<rses> load(TextureManager& manager, const std::array<fs::path, 6>& paths);
    void draw(ShaderGL& shader, const GlobalState& state) const;
    inline void reset() { model_mat = glm::mat4(1.0f); }

    TextureRef texture;
    glm::mat4 model_mat = glm::mat4(1.0f);
    u32 vao = 0;
    u32 verts_buf = 0;

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