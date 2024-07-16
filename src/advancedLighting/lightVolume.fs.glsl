#version 460 core
#define MAX_LIGHTS_NUM 16

//input
// in vec2 TexCoords;
uniform float screenWidth;
uniform float screenHeight;
vec2 TexCoords=vec2(gl_FragCoord.x/screenWidth,gl_FragCoord.y/screenHeight);

//output
out vec4 FragColor;

//uniform
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
vec3 globalPosition=texture(gPosition,TexCoords).xyz;
vec3 globalNormal=texture(gNormal,TexCoords).xyz;
vec3 albedo=texture(gAlbedoSpec,TexCoords).xyz;
float specular=texture(gAlbedoSpec,TexCoords).a;

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

uniform vec3 viewPosition;
uniform samplerCube skybox;
uniform int lightIndex;

void main(){
    vec3 outputColor=vec3(0);
    
    if(lights[lightIndex].lightType==-1){return;}
    
    vec3 dispToLight=lights[lightIndex].position-globalPosition;
    vec3 dirToLight=normalize(dispToLight);
    if(lights[lightIndex].lightType==1){
        //日光
        dirToLight=-normalize(lights[lightIndex].rotation);
    }
    vec3 viewDir=normalize(viewPosition-globalPosition);
    
    //光源衰减
    float lightDistDropoff=1.f;
    float lightDist=max(distance(dispToLight,vec3(0)),1.f);
    if(lights[lightIndex].lightType!=1){
        //日光，不计算距离
        
        // float lightDistDropoff=1/dot(vec3(1,lightDist,pow(lightDist,2)),vec3(1,.09,.032));
        lightDistDropoff=1.f/pow(lightDist,2.f);
    }
    // 聚光灯裁切
    float spotLightCutOff=1.f;
    if(lights[lightIndex].lightType==2){
        spotLightCutOff=dot(-dirToLight,lights[lightIndex].rotation);
        float cutOffRange=lights[lightIndex].innerCutOff-lights[lightIndex].outerCutOff;
        spotLightCutOff=clamp((spotLightCutOff-lights[lightIndex].outerCutOff)/cutOffRange,0.f,1.f);
    }
    
    //diffusion
    float diffuseFac=max(dot(dirToLight,globalNormal),0.f);
    vec3 diffuseColor=albedo;
    vec3 diffuse=diffuseColor*diffuseFac;
    
    //specular
    vec3 halfVec=normalize(dirToLight+viewDir);
    float specularFac=pow(max(dot(halfVec,globalNormal),0.f),64);
    vec3 specularColor=albedo;
    vec3 specular=specularColor*specularFac*specular;
    
    //combine
    outputColor+=(diffuse+specular)*lights[lightIndex].intensity*lightDistDropoff*spotLightCutOff;
    // outputColor+=vec3(lightDistDropoff);
    
    // ambient加上一个随视角的改变，正视的时候强度小，斜视强度大
    vec3 ambient=texture(skybox,reflect(globalPosition-viewPosition,globalNormal)).xyz;
    vec3 fragToCamera=normalize(viewPosition-globalPosition);
    float fr=pow(1-max(dot(fragToCamera,globalNormal),0.f),8);
    
    FragColor=vec4(outputColor+ambient*fr,1.f);
    
    // FragColor=vec4(outputColor,1);
}
