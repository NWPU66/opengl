#version 460 core

// input
layout(location=0)in vec3 position;
layout(location=1)in vec3 normal;
layout(location=2)in vec2 texcoord;

//output
out VS_OUT{
    vec3 global_position;
    vec3 global_normal;
    vec2 texcoord;
}vs_out;

//uniform
uniform mat4 model;

void main(){
    // calculate VS_OUT
    vs_out.global_position=vec3(model*vec4(position,1.F));
    vs_out.global_normal=normalize(mat3(transpose(inverse(model)))*normal);
    vs_out.texcoord=texcoord;
    
    gl_Position=vec4(texcoord*2.F-1.F,0.F,1.F);
}