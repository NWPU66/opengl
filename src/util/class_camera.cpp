#include "class_camera.hpp"

/**
 * Camera 构造函数初始化相机的位置、方向和其他参数。
 *
 * @param position “Camera”构造函数中的“position”参数表示相机在 3D
 * 空间中的初始位置。它是“vec3”类型，通常包含三个浮点值，表示相机位置的 x、y 和 z
 * 坐标。在提供的代码片段中，
 * @param up
 * “Camera”构造函数中的“up”参数表示相机局部坐标系中的向上方向。它通常用于定义相机的方向。在这种情况下，“up”的默认值是指向正
 * y 方向的向量（0.0f
 * @param yaw
 * “Camera”构造函数中的“yaw”参数表示相机的水平旋转角度（以度为单位）。它用于确定相机沿水平轴朝向的方向。
 * @param pitch
 * “Camera”构造函数中的“pitch”参数表示相机的垂直旋转角度（以度为单位）。它通常用于控制相机视图的上下移动。正的俯仰值使相机的视角向上倾斜，而负的俯仰值则使相机的视角向下倾斜。
 */
Camera::Camera(vec3 position, vec3 up, float yaw, float pitch)
    : Front(vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY),
      Zoom(ZOOM)
{
    Position = position;
    WorldUp  = up;
    Yaw      = yaw;
    Pitch    = pitch;
    this->updateCameraVectors();
}

Camera::~Camera() {}

mat4 Camera::GetViewMatrix()
{
    return lookAt(Position, Position + Front, Up);
}

/**
 * 函数“Camera::ProcessKeyboard”根据输入方向和经过的时间更新相机的位置。
 *
 * @param direction “direction”参数是一个包含 6 个元素的数组，表示 3D
 * 空间中的移动方向。每个元素对应一个特定的方向：
 * @param deltaTime `deltaTime`
 * 参数表示当前帧与前一帧之间的时间差。它通常用于游戏开发，以确保无论帧速率如何，移动都能流畅一致。
 */
void Camera::ProcessKeyboard(GLint direction[6], float deltaTime)
{
    float velocity  = MovementSpeed * deltaTime;
    vec3  targetPos = Position;
    targetPos += Front * velocity * (float)direction[0];
    targetPos -= Front * velocity * (float)direction[1];
    targetPos -= Right * velocity * (float)direction[2];
    targetPos += Right * velocity * (float)direction[3];
    targetPos += WorldUp * velocity * (float)direction[4];
    targetPos -= WorldUp * velocity * (float)direction[5];

    Position += (targetPos - Position) * 1.0f;
}

/**
 * “Camera”类中的函数“ProcessMouseMovement”根据鼠标移动更新相机的偏航和俯仰，并可以选择将俯仰限制在一定范围内。
 *
 * @param xoffset `xoffset` 参数表示鼠标光标的水平移动。
 * @param yoffset `yoffset` 参数表示鼠标光标的垂直移动。正值表示向上移动，负值表示向下移动。
 * @param constarinPitch
 * “Camera::ProcessMouseMovement”函数中的“constarinPitch”参数是一个布尔标志，用于确定俯仰角是否应限制在一定范围内。如果`constarinPitch`设置为`true`，俯仰角度将被限制为最大89度
 *
 * @return 在给定的代码片段中，函数“ProcessMouseMovement”返回 void。
 */
void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constarinPitch = true)
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
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
    }

    this->updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {}

void Camera::SpeedUp(GLboolean speedUp)
{
    if (speedUp)
        this->MovementSpeed = SPEED * 3;
    else
        this->MovementSpeed = SPEED;
}

/**
 * 函数“updateCameraVectors”根据相机的偏航角和俯仰角计算其前、右和上矢量。
 */
void Camera::updateCameraVectors()
{
    vec3 front;
    front.x = cos(radians(Yaw)) * cos(radians(Pitch));
    front.y = sin(radians(Pitch));
    front.z = sin(radians(Yaw)) * cos(radians(Pitch));
    Front   = normalize(front);

    Right = normalize(cross(Front, WorldUp));
    Up    = normalize(cross(Right, Front));
}