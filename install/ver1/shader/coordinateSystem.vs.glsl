#version 460 core

//input
layout(location=0)in vec3 aPos;
// layout(location=1)in vec3 aCol;
// layout(location=2)in vec2 aTexCoord;
layout(location=1)in vec2 aTexCoord;

//output
out vec4 vertexColor;
out vec2 texCoord;

//uniform
uniform mat4 transform;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    // gl_Position=transform*vec4(aPos,1.);
    gl_Position=projection*view*transform*model*vec4(aPos,1.f);
    vertexColor=vec4(aPos+.5f,1.f);
    texCoord=aTexCoord;
}