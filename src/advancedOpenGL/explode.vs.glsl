#version 460 core
layout(location=0)in vec3 aPos;
layout(location=1)in vec3 aNormal;
layout(location=2)in vec2 aTexCoords;

out VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 TexCoords;
}vs_out;

uniform mat4 model,view,projection;

void main(){
    vs_out.globalPos=vec3(model*vec4(aPos,1.));
    vs_out.globalNormal=mat3(transpose(inverse(model)))*aNormal;
    vs_out.TexCoords=aTexCoords;

    gl_Position=projection*view*vec4(vs_out.globalPos,1.f);
}
/**FIXME - 关于几何着色器在什么空间？
图形管线：
(Vertex Shader) => (Geometry Shader) => Clip Space => (透视除法) => 
NDC => (视口变换) => Window Space => (Fragment Shader)
几何着色器拿到的是顶点着色器的输出，即透视投影变换后，透视除法之前的值。*/