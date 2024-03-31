#version 460 core
//input
layout(location=0)in vec3 aPos;
//output
out vec3 attr_lightDisplacement;
out float deep_inverse;
//uniform
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;

void main(){
    //把所有的数据转换进视空间
    vec4 vertexPos_view=view*model*vec4(aPos,1.f);
    vec4 lightPos_view=view*model*vec4(lightPos,1.f);
    
    //计算你自己想要的属性Attribute
    //属性除以z，在透视空间中是可以线性插值的，片元着色器会自动帮我完成插值的工作
    attr_lightDisplacement=(vertexPos_view-lightPos_view).xyz/vertexPos_view.z;
    deep_inverse=1/vertexPos_view.z;//为了在片元着色器中把属性解出来，z分之一也可以插值
    
    //计算透视空间的顶点，赋值给glsl预定义变量gl_Position
    gl_Position=projection*vertexPos_view;
}