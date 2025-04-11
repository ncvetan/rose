#include <rose/lighting.hpp>

namespace rose {

std::optional<rses> init_dir_shadow(DirShadowData& shadow) {
    // free existing data
    if (shadow.fbo) {
        glDeleteFramebuffers(1, &shadow.fbo);
    }
    if (shadow.tex) {
        glDeleteTextures(1, &shadow.tex);
    }
    if (shadow.light_mats_ubo) {
        glDeleteBuffers(1, &shadow.light_mats_ubo);
    }

    glCreateFramebuffers(1, &shadow.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow.fbo);
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &shadow.tex);

    // TODO: Resolution could vary between cascades to save memory
    glTextureStorage3D(shadow.tex, 1, GL_DEPTH_COMPONENT32F, shadow.resolution, shadow.resolution, shadow.n_cascades);

    glTextureParameteri(shadow.tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(shadow.tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(shadow.tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(shadow.tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(shadow.tex, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    glNamedFramebufferTexture(shadow.fbo, GL_DEPTH_ATTACHMENT, shadow.tex, 0);
    glNamedFramebufferDrawBuffer(shadow.fbo, GL_NONE);
    glNamedFramebufferReadBuffer(shadow.fbo, GL_NONE);

    if (glCheckNamedFramebufferStatus(shadow.fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return rses().gl("directional shadow framebuffer is incomplete");
    }

    glCreateBuffers(1, &shadow.light_mats_ubo);
    glNamedBufferStorage(shadow.light_mats_ubo, 192, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 6, shadow.light_mats_ubo);
}

std::optional<rses> init_pt_shadow(PtShadowData& shadow) {
    // free existing data
    if (shadow.fbo) {
        glDeleteFramebuffers(1, &shadow.fbo);
    }
    if (shadow.tex) {
        glDeleteTextures(1, &shadow.tex);
    }

    glCreateFramebuffers(1, &shadow.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow.fbo);
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &shadow.tex);

    glTextureStorage2D(shadow.tex, 1, GL_DEPTH_COMPONENT32F, shadow.resolution, shadow.resolution);

    glTextureParameteri(shadow.tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(shadow.tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(shadow.tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(shadow.tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(shadow.tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glNamedFramebufferTexture(shadow.fbo, GL_DEPTH_ATTACHMENT, shadow.tex, 0);
    glNamedFramebufferDrawBuffer(shadow.fbo, GL_NONE);
    glNamedFramebufferReadBuffer(shadow.fbo, GL_NONE);

    if (glCheckNamedFramebufferStatus(shadow.fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return rses().gl("point shadow framebuffer is incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}