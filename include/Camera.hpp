#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 Center;
    float Distance;
    float Yaw;
    float Pitch;
    float Zoom; // Field of View

    Camera(glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f), float distance = 5.0f) 
        : Center(center), Distance(distance), Yaw(0.0f), Pitch(20.0f), Zoom(45.0f) {}

    glm::mat4 GetViewMatrix() {
        float x = Distance * cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
        float y = Distance * sin(glm::radians(Pitch));
        float z = Distance * cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
        
        glm::vec3 position = Center + glm::vec3(x, y, z);
        return glm::lookAt(position, Center, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        float sensitivity = 0.2f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        Yaw   -= xoffset; // Reverse for orbital feel
        Pitch += yoffset;

        if (constrainPitch) {
            if (Pitch > 89.0f) Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }
    }

    void ProcessMouseScroll(float yoffset) {
        Distance -= (float)yoffset * 0.5f;
        if (Distance < 0.5f) Distance = 0.5f;
        if (Distance > 50.0f) Distance = 50.0f;
    }
};

#endif
