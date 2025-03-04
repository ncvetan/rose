#include <rose/glstructs.hpp>

namespace rose {

std::optional<rses> FrameBuf::init(i32 w, i32 h, bool has_depth_buf, const std::vector<FrameBufTexCtx>& texs) {

    glCreateFramebuffers(1, &frame_buf);

    tex_bufs.resize(texs.size());
    attachments.resize(texs.size());

    width = static_cast<u32>(w);
    height = static_cast<u32>(h);

    for (int i = 0; i < tex_bufs.size(); ++i) {
        glCreateTextures(GL_TEXTURE_2D, 1, &tex_bufs[i]);
        glTextureStorage2D(tex_bufs[i], 1, texs[i].intern_format, w, h);
        glTextureParameteri(tex_bufs[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(tex_bufs[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(tex_bufs[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex_bufs[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(frame_buf, GL_COLOR_ATTACHMENT0 + i, tex_bufs[i], 0);
        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
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

    return std::nullopt;
}

void FrameBuf::draw(ShaderGL& shader) {
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

bool SSBO::init(u32 elem_sz, u32 n_elems, u32 base) { 
	glCreateBuffers(1, &ssbo);
    glNamedBufferStorage(ssbo, elem_sz * n_elems, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, base, ssbo);
    this->sz = elem_sz * n_elems;
    this->elem_sz = elem_sz;
    this->base = base;
    return true;
}

SSBO::~SSBO() { glDeleteBuffers(1, &ssbo); }

}