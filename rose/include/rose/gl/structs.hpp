// =============================================================================
//   contains wrappers around some OpenGL constructs
// =============================================================================

#ifndef ROSE_INCLUDE_GL_STRUCTS
#define ROSE_INCLUDE_GL_STRUCTS

#include <rose/core/core.hpp>
#include <rose/core/err.hpp>
#include <rose/gl/shader.hpp>

#include <GL/glew.h>

#include <span>

namespace gl {

struct FrameBufTexCtx {
    GLenum intern_format = 0;
};

struct FrameBuf {

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 tex;
    };

    ~FrameBuf();

    inline void bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, frame_buf);
        glViewport(0, 0, width, height);
    }

    rses init(i32 w, i32 h, bool has_depth_buf, const std::vector<FrameBufTexCtx>& texs);
    void draw(Shader& shader);

    u32 frame_buf = 0;
    u32 render_buf = 0;

    u32 height = 0;
    u32 width = 0;

    u32 vertex_arr = 0;
    u32 vertex_buf = 0;

    std::vector<u32> tex_bufs;
    std::vector<GLenum> attachments;

    std::vector<Vertex> verts = {
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },   { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } }, { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } }
    };
};

struct SSBO {

    // constructs ssbo given size in bytes and a binding point
    bool init(u32 size, u32 base);

    template <typename T>
    void update(std::span<T> data) {

        // check if we have exceeded the capacity of the SSBO and need to resize it
        if (data.size_bytes() > capacity) {
            u32 realloced_ssbo = 0;
            glCreateBuffers(1, &realloced_ssbo);
            glNamedBufferStorage(realloced_ssbo, capacity * 2, nullptr, GL_DYNAMIC_STORAGE_BIT);
            glDeleteBuffers(1, &ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, base, realloced_ssbo);
            capacity *= 2;
            ssbo = realloced_ssbo;
        }

        n_elems = data.size();
        glNamedBufferSubData(ssbo, 0, data.size_bytes(), static_cast<void*>(data.data()));
    }

    ~SSBO();

    u32 ssbo = 0;     // ssbo identifier
    u32 n_elems = 0;  // number of elements
    u32 capacity = 0; // size of the ssbo in bytes
    u32 base = 0;     // bind idx
};

} // namespace gl

#endif