#include <rose/backends/gl/structs.hpp>

namespace gl {

void RenderData::init(const std::vector<glm::vec3>& pos, const std::vector<glm::vec3>& norm,
                      const std::vector<glm::vec3>& tangent, const std::vector<glm::vec2>& uv,
                      const std::vector<u32>& indices) {

    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &pos_buf);
    glCreateBuffers(1, &norm_buf);
    glCreateBuffers(1, &tangent_buf);
    glCreateBuffers(1, &uv_buf);
    glCreateBuffers(1, &indices_buf);

    glNamedBufferStorage(pos_buf, pos.size() * sizeof(glm::vec3), pos.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(norm_buf, norm.size() * sizeof(glm::vec3), norm.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(tangent_buf, tangent.size() * sizeof(glm::vec3), tangent.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(uv_buf, uv.size() * sizeof(glm::vec2), uv.data(), GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(indices_buf, indices.size() * sizeof(u32), indices.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayElementBuffer(vao, indices_buf);

    glVertexArrayVertexBuffer(vao, 0, pos_buf, 0, sizeof(glm::vec3));
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glEnableVertexArrayAttrib(vao, 0);

    glVertexArrayVertexBuffer(vao, 1, norm_buf, 0, sizeof(glm::vec3));
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 1, 1);
    glEnableVertexArrayAttrib(vao, 1);

    glVertexArrayVertexBuffer(vao, 2, tangent_buf, 0, sizeof(glm::vec3));
    glVertexArrayAttribFormat(vao, 2, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 2, 2);
    glEnableVertexArrayAttrib(vao, 2);

    glVertexArrayVertexBuffer(vao, 3, uv_buf, 0, sizeof(glm::vec2));
    glVertexArrayAttribFormat(vao, 3, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 3, 3);
    glEnableVertexArrayAttrib(vao, 3);
}

RenderData::RenderData(RenderData&& other) noexcept {
    vao = other.vao;
    pos_buf = other.pos_buf;
    norm_buf = other.norm_buf;
    tangent_buf = other.tangent_buf;
    uv_buf = other.uv_buf;
    indices_buf = other.indices_buf;

    other.vao = 0;
    other.pos_buf = 0;
    other.norm_buf = 0;
    other.tangent_buf = 0;
    other.uv_buf = 0;
    other.indices_buf = 0;
}

RenderData& RenderData::operator=(RenderData&& other) noexcept {
    if (this == &other) return *this;
    this->~RenderData();
    new (this) RenderData(std::move(other));
    return *this;
}

RenderData::~RenderData() {
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &pos_buf);
        glDeleteBuffers(1, &norm_buf);
        glDeleteBuffers(1, &tangent_buf);
        glDeleteBuffers(1, &uv_buf);
        glDeleteBuffers(1, &indices_buf);
    }
}

rses FrameBuf::init(i32 w, i32 h, bool has_depth_buf, const std::vector<FrameBufTexCtx>& texs) {

    glCreateFramebuffers(1, &frame_buf);

    tex_bufs.resize(texs.size());
    attachments.resize(texs.size());

    width = static_cast<u32>(w);
    height = static_cast<u32>(h);

    for (size_t idx = 0; idx < tex_bufs.size(); ++idx) {
        glCreateTextures(GL_TEXTURE_2D, 1, &tex_bufs[idx]);
        glTextureStorage2D(tex_bufs[idx], 1, texs[idx].intern_format, w, h);
        glTextureParameteri(tex_bufs[idx], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(tex_bufs[idx], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(tex_bufs[idx], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex_bufs[idx], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(frame_buf, GL_COLOR_ATTACHMENT0 + idx, tex_bufs[idx], 0);
        attachments[idx] = GL_COLOR_ATTACHMENT0 + idx;
    }
    
    if (has_depth_buf) {
        glCreateRenderbuffers(1, &render_buf);
        glNamedRenderbufferStorage(render_buf, GL_DEPTH24_STENCIL8, w, h);
        glNamedFramebufferRenderbuffer(frame_buf, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render_buf);
    }

    if (glCheckNamedFramebufferStatus(frame_buf, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return rses().gl("framebuffer is incomplete");
    }

    glCreateVertexArrays(1, &vertex_arr);
    glCreateBuffers(1, &vertex_buf);
    glNamedBufferStorage(vertex_buf, verts.size() * sizeof(Vertex), verts.data(), GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(vertex_arr, 0, vertex_buf, 0, sizeof(Vertex));

    glEnableVertexArrayAttrib(vertex_arr, 0);
    glEnableVertexArrayAttrib(vertex_arr, 1);

    glVertexArrayAttribFormat(vertex_arr, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
    glVertexArrayAttribFormat(vertex_arr, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, tex));

    glVertexArrayAttribBinding(vertex_arr, 0, 0);
    glVertexArrayAttribBinding(vertex_arr, 1, 0);

    return {};
}

void FrameBuf::draw(Shader& shader) {
    shader.use();
    glBindVertexArray(vertex_arr);
    glDrawArrays(GL_TRIANGLES, 0, verts.size());
}

FrameBuf::~FrameBuf() {
    glDeleteFramebuffers(1, &frame_buf);
    if (render_buf) {
        glDeleteRenderbuffers(1, &render_buf);
    }
    for (auto& buf : tex_bufs) {
        glDeleteTextures(1, &buf);
    }
}

bool SSBO::init(u32 size, u32 base) { 
	glCreateBuffers(1, &ssbo);
    glNamedBufferStorage(ssbo, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, base, ssbo);
    this->capacity = size;
    this->n_elems = 0;
    this->base = base;
    return true;
}

SSBO::~SSBO() { glDeleteBuffers(1, &ssbo); }

} // namespace gl
