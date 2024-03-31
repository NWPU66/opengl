#version 460 core
//input
layout(location=0)in vec3 aPos;
layout(location=1)in vec2 aUV;
layout(location=2)in vec3 aNormal;
//output
out vec3 position;
out vec2 uv;
out vec3 normal;
//uniform
uniform mat4 projection,view,model;

void main(){
    position=(model*vec4(aPos,1.f)).xyz;
    uv=aUV;
    normal=(transpose(inverse(model))*vec4(aNormal,0.f)).xyz;
    
    gl_Position=projection*view*model*vec4(aPos,1.f);
}

/*FIXME - 错题本
[](https://zhuanlan.zhihu.com/p/383793695)
OpenGL会自动地处理透视矫正插值，我以为它只会单纯地根据视口空间的片元来做插值。

所以在顶点着色器中的输出，要注意以下几点：

1. out类型输出，必须是在透视除法前可以线性插值的属性。
（例如位移可以线性插值，但距离不行）
我们在片元着色器中获取到的in类型变量，就已经是透视矫正过的属性（OpenGL自动完成）

2. OpenGL预定义变量gl_Position设置为标准视图体积的坐标（透视投影变换后，透视除法前）
但不可以做透视除法，原因在于：
OpenGL需要齐次坐标的w分量（本质上是视空间的z深度）来做透视矫正插值
（只有attr/z类型的变量才可以在透视除法后的空间中线性插值）
如果我们提前做了透视除法，w分量变为1，OpenGL无法恢复透视除法前的属性
*/
