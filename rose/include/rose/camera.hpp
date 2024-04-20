#ifndef ROSE_INCLUDE_CAMERA
#define ROSE_INCLUDE_CAMERA

#include <rose/logger.hpp>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <vector>

namespace rose {

enum class CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

class CameraGL {
  public:
    CameraGL();

    glm::mat4 view_matrix();
    glm::mat4 projection_matrix(float aspect_ratio);
    void process_keyboard(CameraMovement direction, float delta_time);
    void process_mouse_movement(float xoffset, float yoffset);
    void process_mouse_scroll(float yoffset);

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 front = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 2.5f;
    float sensitivity = 0.1f;
    float zoom = 45.0f;
    float near_plane = 0.1f;
    float far_plane = 100.0f;

    void update_camera_vectors();

  private:
};

} // namespace rose

#endif