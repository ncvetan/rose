#include <rose/camera.hpp>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

namespace rose {

glm::mat4 CameraGL::view() { return glm::lookAt(position, position + front, up); }

glm::mat4 CameraGL::projection(float aspect_ratio) {
    return glm::perspective(glm::radians(zoom), aspect_ratio, near_plane, far_plane);
}

void CameraGL::process_keyboard(CameraMovement direction, float delta_time) {
    float velocity = speed * delta_time;
    switch (direction) {
    case CameraMovement::FORWARD:
        position += front * velocity;
        break;
    case CameraMovement::BACKWARD:
        position -= front * velocity;
        break;
    case CameraMovement::LEFT:
        position -= right * velocity;
        break;
    case CameraMovement::RIGHT:
        position += right * velocity;
        break;
    case CameraMovement::UP:
        position += up * velocity;
        break;
    case CameraMovement::DOWN:
        position -= up * velocity;
        break;
    }
}

void CameraGL::process_mouse_movement(float xoffset, float yoffset) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);
    right = glm::normalize(glm::cross(front, world_up));
    up = glm::normalize(glm::cross(right, front));
}

void CameraGL::process_mouse_scroll(float yoffset) {
    zoom -= yoffset;
    if (zoom < 1.0f) zoom = 1.0f;
    else if (zoom > 45.0f) zoom = 45.0f;
}

} // namespace rose
