#version 460 core
layout(location=0)in vec3 aPos;
layout(location=1)in vec3 aNormal;
layout(location=2)in vec2 aTexCoord;
layout(location=3)in vec2 aOffset;

out VS_OUT
{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 TexCoord;
}vs_out;

uniform mat4 view,projection;
// uniform vec2 offsets[100];

mat4 modelMatrix(vec2 offset,vec3 scale);

void main()
{
    // mat4 model=modelMatrix(offsets[gl_InstanceID],vec3(.5));
    mat4 model=modelMatrix(aOffset,vec3(gl_InstanceID/100.));
    vec4 globalPos=model*vec4(aPos,1);
    /**FIXME - 错题本
    vec3()接收浮点数作为输入，gl_InstanceID/50是整数除法
    gl_InstanceID/50.才会隐式转换为float*/
    
    vs_out.globalPos=vec3(globalPos);
    vs_out.globalNormal=vec3((transpose(inverse(model))*vec4(aNormal,0)));
    vs_out.TexCoord=aTexCoord;
    
    gl_Position=projection*view*globalPos;
}

mat4 modelMatrix(vec2 offset,vec3 scale){
    vec4 translate=vec4(offset.x,0,offset.y,1);
    return mat4(vec4(scale.x,0,0,0),vec4(0,scale.y,0,0),vec4(0,0,scale.z,0),translate);
}
