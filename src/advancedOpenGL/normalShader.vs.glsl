#version 460 core
//input
layout(location=0)in vec3 aPos;
layout(location=1)in vec3 aNormal;
layout(location=2)in vec2 aUV;
//output
out vec3 position;
out vec2 uv;
out vec3 normal;
//uniform
uniform mat4 projection,view,model;

//接口块
out VS_OUT{
    vec2 abc;
}vs_out;

// layout(std140,binding=2)uniform Matrices{
//     mat4 projection;
//     mat4 view;
// };

void main(){
    position=(model*vec4(aPos,1.f)).xyz;
    uv=aUV;
    normal=(transpose(inverse(model))*vec4(aNormal,0.f)).xyz;
    
    gl_Position=projection*view*model*vec4(aPos,1.f);
    // gl_PointSize=gl_Position.z;
    //gl_PointSize只在渲染点元的时候可用
}
