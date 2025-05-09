#include <rose/lighting.hpp>

#include <algorithm>
#include <array>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

glm::mat4 get_cascade_mat(const glm::mat4& proj, const glm::mat4& view, glm::vec3 direction, f32 resolution) {
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
    f32 texel_sz = resolution / (frust_radius * 2.0f);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), { texel_sz, texel_sz, texel_sz });
    glm::mat4 look_at = scale * glm::lookAt({ 0.0f, 0.0f, 0.0f }, -direction, { 0.0f, 1.0f, 0.0f });

    frust_center = frust_center * look_at;
    frust_center.x = std::floor(frust_center.x);
    frust_center.y = std::floor(frust_center.y);
    frust_center = frust_center * glm::inverse(look_at);

    // final light view/proj matrices
    // note: adjustment to proj z-range to allow for shadow casters outside of the frustum
    glm::mat4 light_view = glm::lookAt(glm::vec3(frust_center) - (direction * frust_radius * 2.0f),
                                       glm::vec3(frust_center), { 0.0f, 1.0f, 0.0f });

    glm::mat4 light_proj =
        glm::ortho(-frust_radius, frust_radius, -frust_radius, frust_radius, -frust_radius * 5.0f, frust_radius * 5.0f);

    return light_proj * light_view;
}
