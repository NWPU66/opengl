#version 460 core

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    sampler2D diffuseMap;
    sampler2D specularMap;
    sampler2D normalMap;
};

struct Light
{
    //灯光的类型
    int type;
    
    vec3 position;
    float intensity;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 dropOffFac;
    
    //聚光灯设置
    vec3 lightDir;
    float innerCutOff;
    float outerCutOff;
};

//input
in vec3 position;
in vec2 uv;
in vec3 normal;
//output
out vec4 fragColor;
//uniform
uniform vec3 cameraPos;
uniform Material material;
#define MAX_NUM_LIGHTS_SPUUORT 16
uniform Light light[MAX_NUM_LIGHTS_SPUUORT];
//function
vec3 dirLighting(Light light,vec3 normal,vec3 viewDir);
vec3 pointLighting(Light light,vec3 normal,vec3 fragPos,vec3 viewDir);
vec3 spotLighting(Light light,vec3 normal,vec3 fragPos,vec3 viewDir);

void main(){
    // vec3 normTex=texture(material.normalMap,uv).xyz;
    // vec3 normTexInverse=vec3(normTex.x,-normTex.y,normTex.z);
    // vec3 norm=normalize(normal+normTexInverse);
    vec3 norm=normalize(normal);
    vec3 viewDir=normalize(cameraPos-position);
    vec3 outputColor=vec3(0.f);
    for(int i=0;i<MAX_NUM_LIGHTS_SPUUORT;i++){
        switch(light[i].type){
            case-1:break;
            case 0:outputColor+=dirLighting(light[i],norm,viewDir);break;
            case 1:outputColor+=pointLighting(light[i],norm,position,viewDir);break;
            case 2:outputColor+=spotLighting(light[i],norm,position,viewDir);break;
            
        }
    }
    fragColor=vec4(outputColor,1.f);
    // fragColor=vec4(normal,1.f);
}

vec3 dirLighting(Light light,vec3 normal,vec3 viewDir){
    vec3 lightDir=normalize(-light.lightDir);
    
    //diffusion
    float diffusionFac=max(dot(lightDir,normal),0.f);
    vec3 diffColor=light.diffuse*texture(material.diffuseMap,uv).xyz;
    vec3 diffuse=light.intensity*diffColor*diffusionFac;
    
    //specular
    vec3 halfVec=normalize(lightDir+viewDir);
    float specularFac=pow(max(dot(halfVec,normal),0.f),material.shininess);
    vec3 specularColor=texture(material.specularMap,uv).xyz*light.specular;
    vec3 specular=light.intensity*specularFac*specularColor;
    
    //ambient
    vec3 ambient=light.ambient*material.ambient*texture(material.diffuseMap,uv).xyz;
    
    //combine
    return(diffuse+specular+ambient);
}

vec3 pointLighting(Light light,vec3 normal,vec3 fragPos,vec3 viewDir){
    vec3 lightDir=normalize(light.position-fragPos);
    
    //diffuse
    float lightDistance=distance(lightDir,vec3(0.f));
    float diffusionFac=max(dot(lightDir,normal),0.f);
    vec3 lightDistVec=vec3(1.f,lightDistance,pow(lightDistance,2));
    float lightDropOff=1.f/dot(lightDistVec,light.dropOffFac);
    vec3 diffColor=light.diffuse*texture(material.diffuseMap,uv).xyz;
    vec3 diffuse=light.intensity*diffColor*diffusionFac*lightDropOff;
    
    //specular
    vec3 halfVec=normalize(lightDir+viewDir);
    float specularFac=pow(max(dot(halfVec,normal),0.f),material.shininess);
    vec3 specularColor=texture(material.specularMap,uv).xyz*light.specular;
    vec3 specular=light.intensity*specularFac*specularColor*lightDropOff;
    
    //ambient
    vec3 ambient=light.ambient*material.ambient*texture(material.diffuseMap,uv).xyz;
    
    //combine
    return(diffuse+specular+ambient);
}

vec3 spotLighting(Light light,vec3 normal,vec3 fragPos,vec3 viewDir){
    vec3 lightDir=normalize(light.position-fragPos);
    // 聚光灯裁切
    float spotLightCutOff=dot(-light.lightDir,lightDir);
    float cutOffRange=light.innerCutOff-light.outerCutOff;
    spotLightCutOff=clamp((spotLightCutOff-light.outerCutOff)/cutOffRange,0.f,1.f);
    
    //diffuse
    float lightDistance=distance(lightDir,vec3(0.f));
    float diffusionFac=max(dot(lightDir,normal),0.f);
    vec3 lightDistVec=vec3(1.f,lightDistance,pow(lightDistance,2));
    float lightDropOff=1.f/dot(lightDistVec,light.dropOffFac);
    vec3 diffColor=light.diffuse*texture(material.diffuseMap,uv).xyz;
    vec3 diffuse=light.intensity*diffColor*diffusionFac*lightDropOff*spotLightCutOff;
    
    //specular
    vec3 halfVec=normalize(lightDir+viewDir);
    float specularFac=pow(max(dot(halfVec,normal),0.f),material.shininess);
    vec3 specularColor=texture(material.specularMap,uv).xyz*light.specular;
    vec3 specular=light.intensity*specularFac*specularColor*lightDropOff*spotLightCutOff;
    
    //ambient
    vec3 ambient=light.ambient*material.ambient*texture(material.diffuseMap,uv).xyz;
    
    //combine
    return(diffuse+specular+ambient);
}
