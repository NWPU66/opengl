#ifndef STRUCTURE_CPP
#define STRUCTURE_CPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

/**
 * 上面的代码定义了一个名为
 * Material的结构体，其中包含环境色、漫反射色、镜面反射色以及光泽度的属性。
 * @property {vec3} ambient -
 * “Material”结构中的“ambient”属性表示材质的环境颜色。它是当没有特定光源直接照射材料时由材料反射的颜色。
 * @property {vec3} diffuse -
 * “Material”结构中的“diffuse”属性表示物体被光源照射时的颜色。它决定了物体如何在各个方向上均匀地反射光线，从而赋予其基色。
 * @property {vec3} specular -
 * “Material”结构中的“specular”属性表示材质的镜面高光的颜色。镜面高光是当光线集中反射时表面上出现的亮点。此属性定义材质的这些高光的颜色。
 * @property {float} shininess -
 * 光泽度是指材料反射光时呈现出的光泽或光泽程度。光泽度值越高，镜面高光就越集中、越小，从而使材料呈现出更光滑、反光效果更好的外观。
 */
struct Material
{
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
    Material(vec3  ambient   = vec3(1.0f),
             vec3  diffuse   = vec3(1.0f),
             vec3  specular  = vec3(1.0f),
             float shininess = 128.0f)
    {
        this->ambient   = ambient;
        this->diffuse   = diffuse;
        this->specular  = specular;
        this->shininess = shininess;
    }
};

/**
 * Light 结构定义了 3D 环境中光源的属性。
 * @property {vec3} position - “Light”结构的“position”属性表示光源在 3D
 * 空间中的位置。它是“vec3”类型，通常由三个浮点值组成，分别表示光源位置的 x、y
 * 和 z 坐标。
 * @property {vec3} ambient -
 * 光的环境属性表示场景中并非直接来自特定光源的整体光强度。它有助于模拟场景中间接照明的效果。
 * @property {vec3} diffuse -
 * “Light”结构中的“diffuse”属性表示从光源向各个方向均匀散射的光的颜色和强度。它确定有多少光以漫射方式从表面反射，这意味着它不依赖于视角。
 * @property {vec3} specular -
 * “Light”结构中的“specular”属性表示光源镜面反射的颜色。镜面反射是光从表面的镜面反射，“镜面”属性定义了光源的这种反射的颜色。
 * @property {float} intensity -
 * “Light”结构中的“intensity”属性表示光源的亮度或强度。它是一个标量值，决定光源发出的光量。较高的强度值将导致更亮的光，而较低的强度值将导致更暗的光。
 */
struct Light
{
    vec3  position;
    float intensity;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    vec3  dropOffFac;
    Light(vec3  position   = vec3(0.0f),
          float intensity  = 1.0f,
          vec3  ambient    = vec3(1.0f),
          vec3  diffuse    = vec3(1.0f),
          vec3  specular   = vec3(1.0f),
          vec3  dropOffFac = vec3(1.0f, 0.09f, 0.032f))
    {
        this->position   = position;
        this->ambient    = ambient;
        this->diffuse    = diffuse;
        this->specular   = specular;
        this->intensity  = intensity;
        this->dropOffFac = dropOffFac;
    }
};

#endif
