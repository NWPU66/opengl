#version 460 core

layout(location=0)in vec3 aPos;
//location=0，应该是指定了VAO的索引，每次渲染只能指定一个VAO
layout(location=1)in vec3 aCol;
layout(location=2)in vec2 aTexCoord;

out vec4 vertexColor;
out vec2 texCoord;

uniform mat4 transform;

void main(){
    gl_Position=transform*vec4(aPos,1.);
    vertexColor=vec4(aCol,1.);
    texCoord=aTexCoord;
}

void vecReconstruction(){
    //向量的重组
    vec2 temp1;
    vec4 temp2=temp1.xyxx;
    vec3 temp3=temp2.zyw;
    vec4 temp4=temp1.xxxx+temp3.yxzy;
    //我们也可以把一个向量作为一个参数传给不同的向量构造函数
    vec4 temp5=vec4(temp4.xyz,1.);
}