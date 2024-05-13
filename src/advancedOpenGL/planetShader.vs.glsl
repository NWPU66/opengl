#version 460 core
layout(location=0)in vec3 aPos;
layout(location=1)in vec3 aNormal;
layout(location=2)in vec2 aTexCoords;

out VS_OUT
{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 TexCoord;
}vs_out;

uniform mat4 model,view,projection;

void main()
{
    vec4 globalPos4=model*vec4(aPos,1.);
    vs_out.globalPos=vec3(globalPos4);
    vs_out.globalNormal=vec3(transpose(inverse(model))*vec4(aNormal,0.));
    vs_out.TexCoord=aTexCoords;
    gl_Position=projection*view*globalPos4;
}