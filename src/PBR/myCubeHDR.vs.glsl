#version 460 core

layout(location=0)in vec3 aPos;
layout(location=1)in vec3 aNormal;

out vec3 globalPos;
out vec3 globalNormal;

uniform mat4 view[6];
uniform mat4 projection;

const vec3 faceNormals[6]={
    vec3(1,0,0),
    vec3(-1,0,0),
    vec3(0,1,0),
    vec3(0,-1,0),
    vec3(0,0,1),
    vec3(0,0,-1),
};

void main()
{
    float directions[6]={0,0,0,0,0,0};
    for(int i=0;i<6;i++)
    {
        directions[i]=max(dot(normalize(aNormal),faceNormals[i]),0);
    }
    mat4 newView=mat4(0);
    for(int i=0;i<6;i++){
        newView+=directions[i]*view[i];
    }
    
    globalPos=aPos;
    globalNormal=normalize(aNormal);
    gl_Position=projection*newView*vec4(aPos,1.);
}