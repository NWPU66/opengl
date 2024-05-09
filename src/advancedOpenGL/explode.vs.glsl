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