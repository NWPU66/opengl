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
