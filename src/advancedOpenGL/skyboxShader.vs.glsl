#version 460 core
layout(location=0)in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords=aPos;
    gl_Position=(projection*view*vec4(aPos,1.)).xyww;
    //让透视除法后的z-depth（w/w=1），永远等于1，1是最大深度
}