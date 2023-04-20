#include <camera.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : m_front(glm::vec3(0.0f, 0.0f, -1.0f)), m_speed(SPEED), m_sensitivity(SENSITIVITY), m_zoom(ZOOM)
{
    m_position = position;
    m_world_up = up;
    m_yaw = yaw;
    m_pitch = pitch;
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : m_front(glm::vec3(0.0f, 0.0f, -1.0f)), m_speed(SPEED), m_sensitivity(SENSITIVITY), m_zoom(ZOOM)
{
    m_position = glm::vec3(posX, posY, posZ);
    m_world_up = glm::vec3(upX, upY, upZ);
    m_yaw = yaw;
    m_pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = m_speed * deltaTime;
    if (direction == CameraMovement::FORWARD)
        m_position += m_front * velocity;
    if (direction == CameraMovement::BACKWARD)
        m_position -= m_front * velocity;
    if (direction == CameraMovement::LEFT)
        m_position -= m_right * velocity;
    if (direction == CameraMovement::RIGHT)
        m_position += m_right * velocity;
    if (direction == CameraMovement::UP)
        m_position += m_up * velocity;
    if (direction == CameraMovement::DOWN)
        m_position -= m_up * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= m_sensitivity;
    yoffset *= m_sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (constrainPitch)
    {
        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;
    }
   
    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    m_zoom -= yoffset;

    if (m_zoom < 1.0f) m_zoom = 1.0f;
    if (m_zoom > 45.0f) m_zoom = 45.0f;
}

void Camera::updateCameraVectors()
{
 
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);
   
    m_right = glm::normalize(glm::cross(front, m_world_up));  
    m_up = glm::normalize(glm::cross(m_right, front));
}