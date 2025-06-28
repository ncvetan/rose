// =============================================================================
//   contains wrappers around some OpenGL constructs
// =============================================================================

#ifndef ROSE_INCLUDE_BACKENDS_GL_STRUCTS
#define ROSE_INCLUDE_BACKENDS_GL_STRUCTS

#include <rose/texture.hpp>
#include <rose/backends/gl/shader.hpp>
#include <rose/core/core.hpp>
#include <rose/core/err.hpp>

#include <GL/glew.h>

#include <span>

namespace gl {

struct RenderData {

    RenderData() = default;

    RenderData(const RenderData& other) = delete;
    RenderData& operator=(const RenderData& other) = delete;

    RenderData(RenderData&& other) noexcept;
    RenderData& operator=(RenderData&& other) noexcept;

    ~RenderData();

    void init(const std::vector<glm::vec3>& pos, const std::vector<glm::vec3>& norm,
              const std::vector<glm::vec3>& tangent, const std::vector<glm::vec2>& uv, const std::vector<u32>& indices);

    u32 vao = 0;
    u32 pos_buf = 0;
    u32 norm_buf = 0;
    u32 tangent_buf = 0;
    u32 uv_buf = 0;
    u32 indices_buf = 0;
};

struct FrameBufTexCtx {
    GLenum intern_fmt = 0;
};

struct FBuf {

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 tex;
    };

    inline void bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, frame_buf);
        glViewport(0, 0, width, height);
    }
    
    ~FBuf();

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

struct UBO {
    // constructs ubo of given size in bytes and a binding point
    bool init(u32 size, u32 base);

    // clear and resize the buffer
    void realloc(u32 size);

    // copy data into the ubo at the given offset
    template <typename T>
    void update(u32 offset, std::span<T> data) {
        if (offset + data.size_bytes() > capacity) {
            assert(false);
            return;
        }
        glNamedBufferSubData(ubo, offset, data.size_bytes(), static_cast<void*>(data.data()));
    }

    ~UBO();

    u32 ubo = 0;        // ubo identifier
    u32 capacity = 0;   // size of the ubo in bytes
    u32 base = 0;       // bind idx
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

        count = data.size();
        glNamedBufferSubData(ssbo, 0, data.size_bytes(), static_cast<void*>(data.data()));
    }

    ~SSBO();

    u32 ssbo = 0;       // ssbo identifier
    u32 count = 0;      // number of elements
    u32 capacity = 0;   // size of the ssbo in bytes
    u32 base = 0;       // bind idx
};

// represents a single mip
struct Mip {
    TextureRef tex;
    glm::vec2 sz;
};

// generates a mip chain, where the width and height of each mips is halved
// for each mip within the chain
std::vector<Mip> create_mip_chain(TextureManager& texture_manager, u32 w, u32 h, u32 n_mips);

} // namespace gl

#endif