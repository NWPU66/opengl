#include "class_camera.hpp"

Camera::Camera(vec3 position = vec3(0.0f, 0.0f, 0.0f),
               vec3 up = vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
               float pitch = PITCH)
    : Front(vec3(0.0f, 0.0f, -1.0f)),
      MovementSpeed(SPEED),
      MouseSensitivity(SENSITIVITY),
      Zoom(ZOOM)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    this->updateCameraVectors();
}

Camera::~Camera() {}

mat4 Camera::GetViewMatrix() { return lookAt(Position, Position + Front, Up); }

void Camera::ProcessKeyboard(GLint direction[6], float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    Position += Front * velocity * (float)direction[0];
    Position -= Front * velocity * (float)direction[1];
    Position -= Right * velocity * (float)direction[2];
    Position += Right * velocity * (float)direction[3];
    Position += WorldUp * velocity * (float)direction[4];
    Position -= WorldUp * velocity * (float)direction[5];
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset,
                                  GLboolean constarinPitch = true)
{
    if (FirstFrameToView)
    {
        FirstFrameToView = false;
        return;
    }

    Yaw += xoffset * MouseSensitivity;
    Pitch -= yoffset * MouseSensitivity;

    if (constarinPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    this->updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom -= yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::SpeedUp(GLboolean speedUp)
{
    if (speedUp)
        this->MovementSpeed = SPEED * 3;
    else
        this->MovementSpeed = SPEED;
}

void Camera::updateCameraVectors()
{
    vec3 front;
    front.x = cos(radians(Yaw)) * cos(radians(Pitch));
    front.y = sin(radians(Pitch));
    front.z = sin(radians(Yaw)) * cos(radians(Pitch));
    Front = normalize(front);

    Right = normalize(cross(Front, WorldUp));
    Up = normalize(cross(Right, Front));
}