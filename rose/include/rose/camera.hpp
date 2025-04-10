// =============================================================================
//   camera handling code
// =============================================================================

#ifndef ROSE_INCLUDE_CAMERA
#define ROSE_INCLUDE_CAMERA

#include <rose/core/core.hpp>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

namespace rose {

enum class CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

struct Camera {
    glm::mat4 view() const;
    glm::mat4 projection(f32 aspect_ratio) const;
    void handle_keyboard(CameraMovement direction, f32 dt);
    void handle_mouse(f32 xoffset, f32 yoffset);
    void handle_scroll(f32 yoffset);

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 front = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);

    f32 yaw = -90.0f;
    f32 pitch = 0.0f;
    f32 speed = 2.5f;
    f32 sensitivity = 0.1f;
    f32 zoom = 45.0f;
    f32 near_plane = 0.1f;
    f32 far_plane = 100.0f;
};

} // namespace rose

#endif