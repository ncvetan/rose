#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <logger.hpp>

namespace rose
{
    enum class CameraMovement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    const float YAW = -90.0f;
    const float PITCH = 0.0f;
    const float SPEED = 2.5f;
    const float SENSITIVITY = 0.1f;
    const float ZOOM = 45.0f;

    class CameraGL
    {
    public:

        CameraGL(float fov, float aspect_ratio, float near_clip = 0.1f, float far_clip = 100.0f);

        CameraGL(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);

        CameraGL(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

        glm::mat4 GetViewMatrix();

        void UpdateProjectionMatrix();

        void ProcessKeyboard(CameraMovement direction, float deltaTime);

        void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

        void ProcessMouseScroll(float yoffset);

        glm::vec3 camera_position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 world_up;

        glm::mat4x4 projection;

        float yaw;
        float pitch;
        float aspect_ratio;
        float near_plane;
        float far_plane;
        float speed;
        float sensitivity;
        float fov;

    private:

        void updateCameraVectors();

    };
}