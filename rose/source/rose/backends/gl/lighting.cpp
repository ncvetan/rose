#include <rose/backends/gl/lighting.hpp>
#include <rose/core/err.hpp>

namespace gl {

rses DirShadowData::init() {

    // free existing data
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
    }
    if (tex) {
        glDeleteTextures(1, &tex);
    }
    if (light_mats_ubo) {
        glDeleteBuffers(1, &light_mats_ubo);
    }

    glCreateFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &tex);

    // note: resolution could vary between cascades to save memory
    glTextureStorage3D(tex, 1, GL_DEPTH_COMPONENT32F, resolution, resolution, n_cascades);

    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTextureParameteri(tex, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    GLfloat border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTextureParameterfv(tex, GL_TEXTURE_BORDER_COLOR, border);

    glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, tex, 0);
    glNamedFramebufferDrawBuffer(fbo, GL_NONE);
    glNamedFramebufferReadBuffer(fbo, GL_NONE);

    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return rses().gl("directional shadow framebuffer is incomplete");
    }

    glCreateBuffers(1, &light_mats_ubo);
    glNamedBufferStorage(light_mats_ubo, 192, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 6, light_mats_ubo);

    return {};
}

rses PtShadowData::init() {
    // free existing data
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
    }
    if (tex) {
        glDeleteTextures(1, &tex);
    }

    glCreateFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &tex);

    glTextureStorage2D(tex, 1, GL_DEPTH_COMPONENT32F, resolution, resolution);

    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, tex, 0);
    glNamedFramebufferDrawBuffer(fbo, GL_NONE);
    glNamedFramebufferReadBuffer(fbo, GL_NONE);

    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return rses().gl("point shadow framebuffer is incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return {};
}

} // namespace gl