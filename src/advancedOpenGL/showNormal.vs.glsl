#version 460 core
layout(location=0)in vec3 aPos;
layout(location=1)in vec3 aNormal;
layout(location=2)in vec2 aTexCoord;

out VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec3 viewNormal;
    vec2 texCoord;
}vs_out;

uniform mat4 model,view;

void main(){
    vs_out.globalPos=vec3(model*vec4(aPos,1));
    vs_out.globalNormal=normalize(vec3(transpose(inverse(model))*vec4(aNormal,0.f)));
    vs_out.viewNormal=normalize(vec3(transpose(inverse(view*model))*vec4(aNormal,0.f)));
    vs_out.texCoord=aTexCoord;
    
    gl_Position=view*vec4(vs_out.globalPos,1);
}