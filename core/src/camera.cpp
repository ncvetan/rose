#include <camera.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace rose
{
    CameraGL::CameraGL(float fov, float aspect_ratio, float near_plane, float far_plane) 
        : fov(fov), aspect_ratio(aspect_ratio), up(glm::vec3(0.0f, 1.0f, 0.0f)), near_plane(near_plane), far_plane(far_plane), world_up(glm::vec3(0.0f, 1.0f, 0.0f)) {};

    //CameraGL::CameraGL(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : front(glm::vec3(0.0f, 0.0f, -1.0f)), speed(SPEED), sensitivity(SENSITIVITY), fov(ZOOM)
    //{
    //    camera_position = position;
    //    world_up = up;
    //    yaw = yaw;
    //    pitch = pitch;
    //    updateCameraVectors();
    //}
    //
    //CameraGL::CameraGL(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : front(glm::vec3(0.0f, 0.0f, -1.0f)), speed(SPEED), sensitivity(SENSITIVITY), fov(ZOOM)
    //{
    //    camera_position = glm::vec3(posX, posY, posZ);
    //    world_up = glm::vec3(upX, upY, upZ);
    //    yaw = yaw;
    //    pitch = pitch;
    //    updateCameraVectors();
    //}

    glm::mat4 CameraGL::GetViewMatrix()
    {
        return glm::lookAt(camera_position, camera_position + front, up);
    }

    void CameraGL::ProcessKeyboard(CameraMovement direction, float deltaTime)
    {
        float velocity = speed * deltaTime;

        if (direction == CameraMovement::FORWARD)
            camera_position += front * velocity;
        if (direction == CameraMovement::BACKWARD)
            camera_position -= front * velocity;
        if (direction == CameraMovement::LEFT)
            camera_position -= right * velocity;
        if (direction == CameraMovement::RIGHT)
            camera_position += right * velocity;
        if (direction == CameraMovement::UP)
            camera_position += up * velocity;
        if (direction == CameraMovement::DOWN)
            camera_position -= up * velocity;
    }

    void CameraGL::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
    {
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (constrainPitch)
        {
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }
   
        updateCameraVectors();
    }

    void CameraGL::ProcessMouseScroll(float yoffset)
    {
        fov -= yoffset;

        if (fov < 1.0f) fov = 1.0f;
        if (fov > 45.0f) fov = 45.0f;
    }

    void CameraGL::UpdateProjectionMatrix()
    {
        projection = glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
    };

    void CameraGL::updateCameraVectors()
    {

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        right = glm::normalize(glm::cross(front, world_up));
        up = glm::normalize(glm::cross(right, front));
    }
}
