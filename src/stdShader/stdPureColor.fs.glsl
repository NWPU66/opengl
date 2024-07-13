#version 460 core
//定义灯光组的最大灯光数量
#define MAX_LIGHTS_NUM 16
//input
in VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 texCoord;
}fs_in;

struct Light{
    int lightType;
    vec3 color;
    float intensity;
    vec3 position,rotation;
    float innerCutOff,outerCutOff;// for spot light
};

layout(std140,binding=0)uniform lightGroup{
    int numLights;
    Light lights[MAX_LIGHTS_NUM];
};

uniform vec3 lightColor;

//output
layout(location=0)out vec4 fragColor;
layout(location=1)out vec4 brightColor;

void main(){
    fragColor=vec4(lightColor,1);
    // if(dot(lightColor,vec3(.2126f,.7152f,.0722f))/3.f>.95f){brightColor=vec4(fragColor.rgb,1.);}
    brightColor=vec4(fragColor.rgb,1.);
}