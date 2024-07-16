#version 460 core
#define MAX_LIGHTS_NUM 16

//input
in vec2 TexCoords;

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

//functions
vec3 Lighting(int i);

void main(){
    vec3 outputColor=vec3(0);
    for(int i=0;i<numLights;i++){
        if(lights[i].lightType==-1){continue;}
        
        vec3 dispToLight=lights[i].position-globalPosition;
        vec3 dirToLight=normalize(dispToLight);
        if(lights[i].lightType==1){
            //日光
            dirToLight=-normalize(lights[i].rotation);
        }
        vec3 viewDir=normalize(viewPosition-globalPosition);
        
        //光源衰减
        float lightDistDropoff=1.f;
        float lightDist=max(distance(dispToLight,vec3(0)),1.f);
        if(lights[i].lightType!=1){
            //日光，不计算距离
            
            // float lightDistDropoff=1/dot(vec3(1,lightDist,pow(lightDist,2)),vec3(1,.09,.032));
            lightDistDropoff=1.f/pow(lightDist,2.f);
        }
        // 聚光灯裁切
        float spotLightCutOff=1.f;
        if(lights[i].lightType==2){
            spotLightCutOff=dot(-dirToLight,lights[i].rotation);
            float cutOffRange=lights[i].innerCutOff-lights[i].outerCutOff;
            spotLightCutOff=clamp((spotLightCutOff-lights[i].outerCutOff)/cutOffRange,0.f,1.f);
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
        outputColor+=(diffuse+specular)*lights[i].intensity*lightDistDropoff*spotLightCutOff;
        // outputColor+=vec3(lightDistDropoff);
    }
    
    // ambient加上一个随视角的改变，正视的时候强度小，斜视强度大
    vec3 ambient=texture(skybox,reflect(globalPosition-viewPosition,globalNormal)).xyz;
    vec3 fragToCamera=normalize(viewPosition-globalPosition);
    float fr=pow(1-max(dot(fragToCamera,globalNormal),0.f),8);
    
    FragColor=vec4(outputColor+ambient*fr,1.f);
    
    // FragColor=vec4(outputColor,1);
}
/**FIXME -
为什么直接算就没有问题？？？
FragColor=vec4(normalize(lights[0].position-globalPosition),1);
而在函数里算就不对？？？
outputColor+=Lighting(i);
FragColor=vec4(outputColor,1);

第一个问题：
opengl项目里灯光的管理是用numLights来指明灯的数量
而在CookieKiss Render中最大灯光数16盏，不足则会填充“空”灯，所以无需numLights指明数量

第二个问题：
//光源衰减
float lightDistDropoff=1.f;
if(lights[i].lightType!=1){
    float lightDistDropoff=1.f/pow(lightDist,2.f);
}
判断语句里面的float lightDistDropoff声明算局部变量，对局部变量的修改不会影响外面的变量
这里按道理会报错：重定义，但是glsl检查好像比较松
*/

vec3 Lighting(int i){
    if(lights[i].lightType==-1){
        return vec3(0);
    }
    
    vec3 dispToLight=lights[i].position-globalPosition;
    vec3 dirToLight=normalize(dispToLight);
    if(lights[i].lightType==1){
        //日光
        dirToLight=-normalize(lights[i].rotation);
    }
    vec3 viewDir=normalize(viewPosition-globalPosition);
    
    //光源衰减
    float lightDistDropoff=1;
    if(lights[i].lightType!=1){
        //日光，不计算距离
        float lightDist=distance(dispToLight,vec3(0));
        // float lightDistDropoff=1/dot(vec3(1,lightDist,pow(lightDist,2)),vec3(1,.09,.032));
        float lightDistDropoff=1/pow(lightDist,2);
    }
    // 聚光灯裁切
    float spotLightCutOff=1;
    if(lights[i].lightType==2){
        spotLightCutOff=dot(-dirToLight,lights[i].rotation);
        float cutOffRange=lights[i].innerCutOff-lights[i].outerCutOff;
        spotLightCutOff=clamp((spotLightCutOff-lights[i].outerCutOff)/cutOffRange,0.f,1.f);
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
    
    //ambient
    // vec3 ambient=texture(skybox,reflect(-viewDir,globalNormal)).xyz;
    
    //combine
    // return(diffuse+specular)*lights[i].intensity*lightDistDropoff*spotLightCutOff;
    return vec3(dirToLight);
}
