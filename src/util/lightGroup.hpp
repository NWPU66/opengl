#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>
#include <vector>

// 定义灯光组的最大灯光数量
#define MAX_LIGHTS_NUM 16

/// @brief 灯光
/// @param lightType -1代表无效灯，0点光，1日光，2聚光，3面光。
class Light {
private:
    GLint     lightType = -1;
    glm::vec3 color;
    GLfloat   intensity;
    glm::vec3 position, rotation;
    GLfloat   innerCutOff, outerCutOff;  // for spot light

    // For shadow map
    GLuint shadowMap;

public:
    Light();
    Light(GLint     lightType   = -1,
          glm::vec3 color       = glm::vec3(1),
          GLfloat   intensity   = 1,
          glm::vec3 position    = glm::vec3(0),
          glm::vec3 rotation    = glm::vec3(0),
          GLfloat   innerCutOff = cos(glm::radians(12.5f)),
          GLfloat   outerCutOff = cos(glm::radians(17.5f)));
    ~Light();

    /// @brief 获取灯光的位置
    glm::vec3 getPostion() const;

    /// @brief 获取灯光的旋转（灯的方向）
    glm::vec3 getRotation() const;

    /// @brief 获取灯光的颜色
    glm::vec3 getColor() const;

    /// @brief
    /// @return
    GLint getLightType() const;

    /// @brief
    /// @return
    GLuint getShadowMap() const;

    /// @brief 计算在OpenGL 140布局下，单个灯光在缓冲中占用的空间
    /// @return 占用的缓冲空间，以Byte为单位
    static GLuint calculateMemoryOccupation();

    /// @brief 在以ptr为起始地址的空间上更新灯光的数据
    /// @param ptr 灯光Uniform缓冲的起始地址
    void updateLightUniformBuffer(GLubyte* ptr);

    /// @brief
    void createShadowMap();
};

/// @brief 灯光组
/// @note 使用方法：
/// 1-addLight()
/// 2-updateLightUniformBuffer()
/// 3-bindingUniformBuffer()
/// 4-正常使用
/// 5-（可选）更新灯光数据
/// 6-updateLightUniformBuffer()
class LightGroup {
private:
    std::vector<Light> lights;
    GLuint             lightsUniformBufferObject;

public:
    LightGroup();
    ~LightGroup();

    /// @brief 灯光组的get方法
    const Light& getLight(const int idx) const;

    /// @brief 灯光组的get方法
    const std::vector<Light>& getLights() const;

    /// @brief 向灯光组添加新的灯光
    /// @param light 灯光
    void addLight(Light light);

    /// @brief 向灯光组添加一组新的灯光
    /// @param _lights 一组灯光
    void addLight(std::vector<Light> _lights);

    /// @brief 从灯光组中移除一盏灯光
    /// @param idx 灯光的索引
    void removeLight(int idx);

    /// @brief 创建灯光组的Uniform缓冲
    void createLightUniformBuffer();

    /// @brief 更新灯光组的Uniform缓冲
    void updateLightUniformBuffer();

    /// @brief 将灯光组的Uniform缓冲绑定到系统的绑定点上
    /// @param bindingPoint 绑定点
    void bindingUniformBuffer(GLuint bindingPoint);

    /// @brief 将灯光组Uniform缓冲的一部分绑定到系统的绑定点上
    /// @param bindingPoint 绑定点
    /// @param offset 起始偏移
    /// @param size 预绑定的数据大小
    void bindingUniformBuffer(GLuint bindingPoint, GLuint offset, GLuint size);

    /// @brief 计算在OpenGL 140布局下，整个灯光组在缓冲中占用的空间
    /// @return 占用的缓冲空间，以Byte为单位
    GLuint calculateMemoryOccupation();

    /// @brief
    void printBufferData();
};
