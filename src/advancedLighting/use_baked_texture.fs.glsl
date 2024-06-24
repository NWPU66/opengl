#version 460 core
//定义灯光组的最大灯光数量
#define MAX_LIGHTS_NUM 16

//input
in VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 texCoord;
    vec3 globalTangent;
    vec3 globalBitangent;
    mat3 TBN;
}fs_in;

//output
out vec4 fragColor;

//uniform
uniform sampler2D baked_texture;

void main(){
    fragColor=vec4(texture(baked_texture,fs_in.texCoord).xyz,1);
    // fragColor=vec4(1);
}
