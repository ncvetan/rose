// =============================================================================
//   structures for loading and rendering 3D models
// =============================================================================

#ifndef ROSE_INCLUDE_MODEL
#define ROSE_INCLUDE_MODEL

#ifdef USE_OPENGL
#include <rose/backends/gl/structs.hpp>
#else
static_assert("no backend selected");
#endif 

#include <rose/texture.hpp>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <concepts>
#include <filesystem>
#include <vector>

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
void rotate(T& obj, const glm::vec3& rotation) {
    obj.model_mat = glm::rotate(obj.model_mat, glm::radians(rotation.x), { 1.0f, 0.0f, 0.0f });
    obj.model_mat = glm::rotate(obj.model_mat, glm::radians(rotation.y), { 0.0f, 1.0f, 0.0f });
    obj.model_mat = glm::rotate(obj.model_mat, glm::radians(rotation.z), { 0.0f, 0.0f, 1.0f });
}

enum class MeshFlags : u32 {
    NONE = 0,           // no effect
    TRANSPARENT = bit1, // this mesh contains transparent textures
};

ENABLE_ROSE_ENUM_OPS(MeshFlags);

struct Mesh {
    u64 n_indices = 0;
    u64 base_vert = 0;
    u64 base_idx = 0;
    u32 matl_offset = 0;
    u32 n_matls = 0;
    MeshFlags flags = MeshFlags::NONE;
};

struct Model {

    Model() = default;
    Model(const Model& other) = delete;
    Model(Model&& other) noexcept;

    Model& operator=(const Model& other) = delete;
    Model& operator=(Model&& other) noexcept;

    // returns a copy of this model
    Model copy();

    void load(TextureManager& manager, const std::filesystem::path& path);

    inline void reset() { model_mat = glm::mat4(1.0f); }

#ifdef  USE_OPENGL
    gl::RenderData render_data;
#else
    static_assert("no backend selected");
#endif 

    glm::mat4 model_mat = glm::mat4(1.0f);
    std::vector<Mesh> meshes;
    std::vector<TextureRef> textures;

    std::vector<u32> indices;
    std::vector<glm::vec3> pos;
    std::vector<glm::vec3> norms;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec2> uvs;
};

struct SkyBox {

    SkyBox() = default;

    SkyBox(const SkyBox& other) = delete;
    SkyBox& operator=(const SkyBox& other) = delete;
    
    SkyBox(SkyBox&& other) noexcept;
    SkyBox& operator=(SkyBox&& other) noexcept;
    
    ~SkyBox();

    void init();
    void load(TextureManager& manager, const std::array<fs::path, 6>& paths);
    inline void reset() { model_mat = glm::mat4(1.0f); }

    glm::mat4 model_mat = glm::mat4(1.0f);

    TextureRef texture;
    u32 vao = 0;
    u32 verts_buf = 0;

    std::vector<glm::vec3> verts = {
        { -1.0f, 1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f },  { 1.0f, -1.0f, -1.0f },
        { 1.0f, 1.0f, -1.0f },  { -1.0f, 1.0f, -1.0f },  { -1.0f, -1.0f, 1.0f },  { -1.0f, -1.0f, -1.0f },
        { -1.0f, 1.0f, -1.0f }, { -1.0f, 1.0f, -1.0f },  { -1.0f, 1.0f, 1.0f },   { -1.0f, -1.0f, 1.0f },
        { 1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, 1.0f },   { 1.0f, 1.0f, 1.0f },    { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, -1.0f },  { 1.0f, -1.0f, -1.0f },  { -1.0f, -1.0f, 1.0f },  { -1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },   { 1.0f, 1.0f, 1.0f },    { 1.0f, -1.0f, 1.0f },   { -1.0f, -1.0f, 1.0f },
        { -1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, -1.0f },   { 1.0f, 1.0f, 1.0f },    { 1.0f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f },  { -1.0f, 1.0f, -1.0f },  { -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, 1.0f },
        { 1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f },  { -1.0f, -1.0f, 1.0f },  { 1.0f, -1.0f, 1.0f }
    };
};

#endif