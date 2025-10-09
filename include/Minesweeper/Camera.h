#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
public:
  glm::vec3 Position;
  glm::vec3 Front;
  glm::vec3 Up;
  glm::vec3 Right;
  glm::vec3 WorldUp;
  float Yaw;
  float Pitch;
  float MovementSpeed;
  float MouseSensitivity;

  Camera(glm::vec3 position = {0.0f, 0.0f, 3.0f})
      : Front({0.0f, 0.0f, -1.0f}), Yaw(-90.0f), Pitch(0.0f),
        MovementSpeed(2.5f), MouseSensitivity(0.1f) {
    Position = position;
    WorldUp = {0.0f, 1.0f, 0.0f};
    updateCameraVectors();
  }

  glm::mat4 GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
  }

  void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
      Position += Front * velocity;
    if (direction == BACKWARD)
      Position -= Front * velocity;
    if (direction == LEFT)
      Position -= Right * velocity;
    if (direction == RIGHT)
      Position += Right * velocity;
  }

  void ProcessMouseMovement(float xoffset, float yoffset,
                            bool constrainPitch = true) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    Yaw += xoffset;
    Pitch += yoffset; // <-- normal: moving mouse up looks up
    if (constrainPitch) {
      if (Pitch > 89.0f)
        Pitch = 89.0f;
      if (Pitch < -89.0f)
        Pitch = -89.0f;
    }
    updateCameraVectors();
  }

private:
  void updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) *
              cos(glm::radians(Pitch)); // positive z for normal orientation
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
  }
};
