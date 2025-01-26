#ifndef ROSE_INCLUDE_GLSTRUCTS
#define ROSE_INCLUDE_GLSTRUCTS

#include <rose/alias.hpp>
#include <rose/err.hpp>
#include <rose/object.hpp>
#include <rose/shader.hpp>

#include <GL/glew.h>

#include <span>

namespace rose {

struct FrameBufTexCtx {
    GLenum intern_format = 0;
    GLenum format = 0;
    GLenum type = 0;
};

struct FrameBuf {

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 tex;
    };

    ~FrameBuf();

    std::optional<rses> init(int w, int h, bool has_depth_buf, const std::vector<FrameBufTexCtx>& texs);
    void draw(ShaderGL& shader, const GlobalState& state);

    u32 frame_buf = 0;
    u32 render_buf = 0;
    std::vector<u32> tex_bufs;
    std::vector<GLenum> attachments;

    u32 vertex_arr = 0;
    u32 vertex_buf = 0;

    std::vector<Vertex> verts = {
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },   { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } }
    };
};

struct SSBO {

    bool init(u32 elem_sz, u32 n_elems, u32 base);

    template <typename T>
    void update(u32 offset, std::span<T> data) {

        // check if we have exceeded the capacity of the SSBO and need to resize it
        // 
        // note: if using the .length() function in GLSL, it will return the capacity and
        // not the actual number of elements intialized in the buffer, therefore might
        // want to also keep track of the number of elements
        if (elem_sz * offset + data.size_bytes() > sz) {
            u32 realloced_ssbo = 0;
            glCreateBuffers(1, &realloced_ssbo);
            glNamedBufferStorage(realloced_ssbo, sz * 2, nullptr, GL_DYNAMIC_STORAGE_BIT);
            glCopyNamedBufferSubData(ssbo, realloced_ssbo, 0, 0, sz);
            glDeleteBuffers(1, &ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, base, realloced_ssbo);
            sz *= 2;
            ssbo = realloced_ssbo;
        }

        glNamedBufferSubData(ssbo, elem_sz * offset, data.size_bytes(), static_cast<void*>(data.data()));
    }

    // zero out all the memory after the given offset
    inline void zero(u32 offset) {
        glNamedBufferSubData(ssbo, elem_sz * offset, (sz - elem_sz * offset), nullptr);
    };

    ~SSBO();

    u32 ssbo = 0;       // ssbo identifier
    u32 elem_sz;        // size of each ssbo element in bytes
    u32 sz = 0;         // size of the ssbo in bytes
    u32 base = 0;       // bind idx
};

// TODO: Put this somewhere else
struct AABB {
    glm::vec4 min_pt;
    glm::vec4 max_pt;
};

// data related to cluster shading
struct ClusterCtx {
    glm::uvec3 grid_sz = { 16, 9, 24 }; // size of cluster grid (xyz)
    s32 max_lights_in_cluster = 100;    // number of lights that will be considered for a single cluster
    SSBO clusters_aabb_ssbo;            // AABBs for each cluster
    SSBO lights_ssbo;                   // light parameters for each light in the scene
    SSBO lights_pos_ssbo;               // light positions for each light in the scene
    SSBO clusters_ssbo;                 // light for each cluster
};

}
#endif