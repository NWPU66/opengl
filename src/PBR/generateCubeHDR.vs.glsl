#version 460 core
layout(location=0)in vec3 aPos;
layout(location=1)in vec3 aNormal;

out vec3 globalPosition;
out vec3 globalNormal;

void main()
{
    globalPosition=aPos;
    globalNormal=aNormal;
    gl_Position=vec4(aPos,1);
}