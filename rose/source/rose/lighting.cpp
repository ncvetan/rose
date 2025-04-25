#include <rose/lighting.hpp>

#include <algorithm>
#include <array>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

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
    glTextureParameteri(shadow.tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(shadow.tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTextureParameteri(shadow.tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTextureParameteri(shadow.tex, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    GLfloat border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTextureParameterfv(shadow.tex, GL_TEXTURE_BORDER_COLOR, border);

    glNamedFramebufferTexture(shadow.fbo, GL_DEPTH_ATTACHMENT, shadow.tex, 0);
    glNamedFramebufferDrawBuffer(shadow.fbo, GL_NONE);
    glNamedFramebufferReadBuffer(shadow.fbo, GL_NONE);

    if (glCheckNamedFramebufferStatus(shadow.fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return rses().gl("directional shadow framebuffer is incomplete");
    }

    glCreateBuffers(1, &shadow.light_mats_ubo);
    glNamedBufferStorage(shadow.light_mats_ubo, 192, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 6, shadow.light_mats_ubo);

    return std::nullopt;
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

    return std::nullopt;
}

glm::mat4 get_dir_light_mat(const glm::mat4& proj, const glm::mat4& view, const DirLight& light) {
    // transform frustum from clip space to world space and find its center
    std::array<glm::vec4, 8> frustum_corners = {
        glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), glm::vec4(1.0f, -1.0f, -1.0f, 1.0f), glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),
        glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),   glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f), glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
        glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),   glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
    };

    glm::mat4 pv_inv = glm::inverse(proj * view);
    glm::vec4 frust_center = glm::vec4(0.0f);

    for (auto& corner : frustum_corners) {
        corner = pv_inv * corner; // [ clip -> world ]
        corner /= corner.w;
        frust_center += corner;
    }

    frust_center /= 8.0f;

    // determine the radius of the frustum, used for consistent projection sizing
    f32 frust_radius = 0.0f;
    for (const auto& corner : frustum_corners) {
        frust_radius = std::max(frust_radius, std::ceil(glm::length(corner - frust_center)));
    }

    // snap projection to texel
    f32 texel_sz = light.shadow_data.resolution / (frust_radius * 2.0f);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), { texel_sz, texel_sz, texel_sz });
    glm::mat4 look_at = scale * glm::lookAt({ 0.0f, 0.0f, 0.0f }, -light.direction, { 0.0f, 1.0f, 0.0f });

    frust_center = frust_center * look_at;
    frust_center.x = std::floor(frust_center.x);
    frust_center.y = std::floor(frust_center.y);
    frust_center = frust_center * glm::inverse(look_at);

    // final light view/proj matrices
    // note: adjustment to proj z-range to allow for shadow casters outside of the frustum
    glm::mat4 light_view = glm::lookAt(glm::vec3(frust_center) - (light.direction * frust_radius * 2.0f),
                                       glm::vec3(frust_center), { 0.0f, 1.0f, 0.0f });

    glm::mat4 light_proj =
        glm::ortho(-frust_radius, frust_radius, -frust_radius, frust_radius, -frust_radius * 5.0f, frust_radius * 5.0f);

    return light_proj * light_view;
}
