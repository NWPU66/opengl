#ifndef CAMERA_H
#define CAMERA_H
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

// NOTE - 定义摄像机的默认参数
const float YAW         = -90.0f;  // 偏航
const float PITCH       = 0.0f;    // 俯仰
const float SPEED       = 5.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM        = 45.0f;

class Camera {
public:
    /**NOTE - 摄像机参数
     */
    vec3  Position, Front, Up, Right, WorldUp;
    float Yaw, Pitch;                             // euler Angles
    float MovementSpeed, MouseSensitivity, Zoom;  // 相机可选项
    bool  FirstFrameToView = true;

    /**NOTE - 摄像机构造函数
     * 在C++中，可以使用成员初始化列表来初始化类的成员变量
     */
    Camera(vec3 position, vec3 up, float yaw, float pitch);
    ~Camera();

    /**NOTE - 实用函数
     */
    mat4 GetViewMatrix();
    void ProcessKeyboard(GLint direction[6], float deltaTime);
    void ProcessMouseMovement(float     xoffset,
                              float     yoffset,
                              GLboolean constarinPitch);
    void ProcessMouseScroll(float yoffset);
    void SpeedUp(GLboolean speedUp);

private:
    void updateCameraVectors();
};

#endif