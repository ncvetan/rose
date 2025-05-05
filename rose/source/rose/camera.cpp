#include <rose/camera.hpp>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

glm::mat4 Camera::view() const { return glm::lookAt(position, position + front, up); }

glm::mat4 Camera::projection(f32 aspect_ratio) const {
    return glm::perspective(glm::radians(zoom), aspect_ratio, near_plane, far_plane);
}

void Camera::handle_keyboard(CameraMovement direction, f32 dt) {
    f32 velocity = speed * dt;
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

void Camera::handle_mouse(f32 xoffset, f32 yoffset) {
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

void Camera::handle_scroll(f32 yoffset) {
    zoom -= yoffset;
    if (zoom < 1.0f) zoom = 1.0f;
    else if (zoom > 45.0f) zoom = 45.0f;
}
