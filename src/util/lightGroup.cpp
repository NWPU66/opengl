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

glm::vec3 Light::getPostion() const
{
    return position;
}

glm::vec3 Light::getRotation() const
{
    return rotation;
}

glm::vec3 Light::getColor() const
{
    return color;
}

GLint Light::getLightType() const
{
    return lightType;
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
    /**FIXME - 错题本
     * std140的布局理解错了！
     * 详细的计算分析请看 opengl/src/advancedLighting/memoryLayout.md
     */
    return 80;
}

void Light::updateLightUniformBuffer(GLubyte* ptr)
{
    memcpy(ptr + 0, &(this->lightType), sizeof(GLint));
    memcpy(ptr + 16, &(this->color), sizeof(glm::vec3));
    memcpy(ptr + 28, &(this->intensity), sizeof(GLfloat));
    memcpy(ptr + 32, &(this->position), sizeof(glm::vec3));
    memcpy(ptr + 48, &(this->rotation), sizeof(glm::vec3));
    memcpy(ptr + 60, &(this->innerCutOff), sizeof(GLfloat));
    memcpy(ptr + 64, &(this->outerCutOff), sizeof(GLfloat));
    /**FIXME - 错题本
     * std140的布局理解错了！
     * 详细的计算分析请看 opengl/src/advancedLighting/memoryLayout.md
     */
}

// -----------------------------------------------------------------------------------------------

LightGroup::LightGroup() {}
LightGroup::~LightGroup() {}

const Light& LightGroup::getLight(const int idx) const
{
    return lights[idx];
}

const std::vector<Light>& LightGroup::getLights() const
{
    return lights;
    /**FIXME - 错题本
     * 关于const函数：为了确保传出类的内部要用值。绝对不会被修改
     * 返回的引用值必须为常值引用（const refer）
     *
     * 另外，常值对象只能调用常函数
     */
}

void LightGroup::createLightUniformBuffer()
{
    glGenBuffers(1, &lightsUniformBufferObject);
    glBindBuffer(GL_UNIFORM_BUFFER, lightsUniformBufferObject);

    // 计算灯光组占据的内存空间（以Byte为单位）
    int memoryOccupation = this->calculateMemoryOccupation();

    glBufferData(GL_UNIFORM_BUFFER, memoryOccupation + 16, nullptr, GL_STATIC_DRAW);
    // NOTE - 这里出GL_STATIC_DRAW，暂且认为灯光的数据初始化后不再修改。
    // FIXME - +16意味着缓冲的前4B是int型的灯光数量，但是要对齐到16B

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
    // FIXME - 前16B = 4B整数 + 12B空填充

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
        GLubyte* lightPtr = ptr + i * stride + 16;  // FIXME - +16意味着缓冲的前16B是int型的灯光数量
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

void LightGroup::printBufferData()
{
    glBindBuffer(GL_UNIFORM_BUFFER, lightsUniformBufferObject);
    // 获取指向buffer的指针
    GLubyte* ptr        = (GLubyte*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    int      bufferSize = calculateMemoryOccupation() + 16;  // FIXME - 前16B放一个int变量

    // 将ptr起始bufferSize大小的数据以16进制的形式打印出来
    for (int i = 0; i < bufferSize; i++)
    {
        printf("%02X ", *(ptr + i));
        if ((i + 1) % 4 == 0) { printf(" "); }
        if ((i + 1) % 32 == 0) { printf("\n"); }
    }

    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);  // 解绑}
}
