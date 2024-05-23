# 关于LightGroup中灯光数据在内存中的布局问题

---
## 介绍
在Shader中，灯光的Uniform缓冲的布局如下：
```glsl
struct Light{
    int lightType;
    vec3 color;
    float intensity;
    vec3 position,rotation;
    float innerCutOff,outerCutOff;// for spot light
};

layout(std140,binding=0)uniform lightGroup{
    int numLights;
    Light lights[MAX_LIGHTS_NUM];
};
```
缓冲中首先放置一个int型变量numLights，表示当前场景中灯光的数量。然后，根据numLights的值，依次放置numLights个Light结构体。在整个系统中灯光数量的上限是MAX_LIGHTS_NUM。

## 详细分析
接下来对它的内存布局做详细分析：
|变量类型|Base Align|Base Offset|Align Offset|占用的内存|备注|
|---|---|---|---|---|---|
|int lightType|4B|0|0|0->3||
|struct { |16B|4|16||结构体的基准对齐：内部元素的最大基准对齐，向上取整到16B的整数倍|
|int lightType|4B|16|16|16->19||
|vec3 color|16B|20|32|32->43|vec3占用12B，但是要对齐到vec4的大小|
|float intensity|4B|44|44|44->47||
|vec3 position|16B|48|48|48->60||
|vec3 rotation|16B|60|64|64->75||
|float innerCutOff|4B|76|76|76->79||
|float outerCutOff|4B|80|80|80->83||
|} Light ||84||||
|下一个结构体：|16B|84|96|96->......|结构体的对齐量是16B，84要向上取整到16的整数倍96|

## Uniform的整体结构
|numLights|填充|Light结构体|填充|Light结构体|填充|.....|
|---|---|---|---|---|---|---|
|4B|12B|68B|12B|68B|12B|......|
