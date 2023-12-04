#include <camera.hpp>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <iostream>

namespace rose {

CameraGL::CameraGL() {}

glm::mat4 CameraGL::view_matrix() {
    auto test = glm::lookAt(position, position + front, up);
    return test;
}

glm::mat4 CameraGL::projection_matrix(float aspect_ratio) {
    return glm::perspective(glm::radians(zoom), aspect_ratio, near_plane, far_plane);
}

void CameraGL::process_keyboard(CameraMovement direction, float delta_time) {
    float velocity = speed * delta_time;
    if (direction == CameraMovement::FORWARD) position += front * velocity;
    if (direction == CameraMovement::BACKWARD) position -= front * velocity;
    if (direction == CameraMovement::LEFT) position -= right * velocity;
    if (direction == CameraMovement::RIGHT) position += right * velocity;
    if (direction == CameraMovement::UP) position += up * velocity;
    if (direction == CameraMovement::DOWN) position -= up * velocity;
}

void CameraGL::process_mouse_movement(float xoffset, float yoffset) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    update_camera_vectors();
}

void CameraGL::process_mouse_scroll(float yoffset) {
    zoom -= yoffset;

    if (zoom < 1.0f) zoom = 1.0f;
    if (zoom > 45.0f) zoom = 45.0f;
}

void CameraGL::update_camera_vectors() {
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);
    right = glm::normalize(glm::cross(front, world_up));
    up = glm::normalize(glm::cross(right, front));
}

} // namespace rose
