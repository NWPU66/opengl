#include "lightGroup.hpp"

Light::Light() {}
Light::~Light() {}

Light::Light(GLint     lightType,
             glm::vec3 color,
             GLfloat   intensity,
             glm::vec3 position,
             glm::vec3 rotation,
             GLfloat   innerCutOff,
             GLfloat   outerCutOff)
    : lightType(lightType), color(color), intensity(intensity), position(position),
      rotation(rotation), innerCutOff(innerCutOff), outerCutOff(outerCutOff)
{
}

GLuint Light::calculateMemoryOccupation()
{
    /**NOTE - memory occupation
     * GLint     lightType;（4B）
     * glm::vec3 position;（16B）
     * GLfloat   intensity;（4B）
     * glm::vec3 position, rotation;（16B，16B）
     * GLfloat innerCutOff, outerCutOf,;（4B，4B）
     */
    return 64;
}

void Light::updateLightUniformBuffer(GLubyte* ptr)
{
    memcpy(ptr + 0, &(this->lightType), sizeof(GLint));
    memcpy(ptr + 4, &(this->color), sizeof(glm::vec3));
    memcpy(ptr + 20, &(this->intensity), sizeof(GLfloat));
    memcpy(ptr + 24, &(this->position), sizeof(glm::vec3));
    memcpy(ptr + 40, &(this->rotation), sizeof(glm::vec3));
    memcpy(ptr + 56, &(this->innerCutOff), sizeof(GLfloat));
    memcpy(ptr + 60, &(this->outerCutOff), sizeof(GLfloat));
    // 总大小 64 Byte
}

// -----------------------------------------------------------------------------------------------

LightGroup::LightGroup() {}
LightGroup::~LightGroup() {}

void LightGroup::createLightUniformBuffer()
{
    glGenBuffers(1, &lightsUniformBufferObject);
    glBindBuffer(GL_UNIFORM_BUFFER, lightsUniformBufferObject);

    // 计算灯光组占据的内存空间（以Byte为单位）
    int memoryOccupation = this->calculateMemoryOccupation();

    glBufferData(GL_UNIFORM_BUFFER, memoryOccupation + 4, nullptr, GL_STATIC_DRAW);
    // NOTE - 这里出GL_STATIC_DRAW，暂且认为灯光的数据初始化后不再修改。
    // NOTE - +4意味着缓冲的前4B是int型的灯光数量

    glBindBuffer(GL_UNIFORM_BUFFER, 0);  // 解绑

    updateLightUniformBuffer();  // 更新灯光组数据
}

void LightGroup::bindingUniformBuffer(GLuint bindingPoint)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, lightsUniformBufferObject);
}

void LightGroup::bindingUniformBuffer(GLuint bindingPoint, GLuint offset, GLuint size)
{
    glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, lightsUniformBufferObject, offset, size);
}

void LightGroup::updateLightUniformBuffer()
{
    glBindBuffer(GL_UNIFORM_BUFFER, lightsUniformBufferObject);
    int stride = Light::calculateMemoryOccupation();

    // 更新numLights
    int numLights = lights.size();
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 4, &numLights);

    if (lights.size() == 0)
    {
        std::cout
            << "Warning: No light in the light group, no need to update the light uniform buffer."
            << std::endl;
        return;  // 没有灯光，不需要更新
    }

    // 获取指向buffer的指针
    GLubyte* ptr = (GLubyte*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);

    // 逐元素更新buffer
    for (int i = 0; i < lights.size(); i++)
    {
        GLubyte* lightPtr = ptr + i * stride + 4;  // NOTE - +4意味着缓冲的前4B是int型的灯光数量
        lights[i].updateLightUniformBuffer(lightPtr);
    }
    glUnmapBuffer(GL_UNIFORM_BUFFER);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);  // 解绑
}

void LightGroup::addLight(Light light)
{
    if (lights.size() < MAX_LIGHTS_NUM) { lights.push_back(light); }
    else { std::cout << "Warning: Too many lights in the light group." << std::endl; }
}

void LightGroup::addLight(std::vector<Light> _lights)
{
    if (lights.size() + _lights.size() <= MAX_LIGHTS_NUM)
    {
        for (const auto& _light : _lights)
        {
            lights.push_back(_light);
        }
    }
    else
    {
        std::cout << "Warning: Too many lights in the light group, only the first "
                  << MAX_LIGHTS_NUM << " lights will be added." << std::endl;
    }
}

void LightGroup::removeLight(int idx)
{
    if (idx >= 0 && idx < lights.size()) { lights.erase(lights.begin() + idx); }
}

GLuint LightGroup::calculateMemoryOccupation()
{
    return lights.size() * Light::calculateMemoryOccupation();
}
