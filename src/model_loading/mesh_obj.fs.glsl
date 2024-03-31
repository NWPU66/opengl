#version 460 core
#define MAX_NUM_LIGHTS_SPUUORT 16

struct Material
{
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_normal1;
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

struct GeomtryInfo{
    vec3 normal;
    vec3 fragPos;
    vec3 viewDir;
    vec3 diffuseTexCol;
    vec3 specularTexCol;
};

//input
in vec3 position;
in vec2 uv;
in vec3 normal;
//output
out vec4 fragColor;
//uniforms
uniform vec3 cameraPos;
uniform Material material;
uniform Light light[MAX_NUM_LIGHTS_SPUUORT];
//functions
GeomtryInfo perapreGeomtryInfo();
vec3 Lighting(Light light,GeomtryInfo geom);

void main(){
    //准备数据
    GeomtryInfo geom=perapreGeomtryInfo();
    
    vec3 outputColor=vec3(0.f);
    for(int i=0;i<MAX_NUM_LIGHTS_SPUUORT;i++){
        if(light[i].type!=-1){
            outputColor+=Lighting(light[i],geom);
        }
    }
    
    fragColor=vec4(outputColor,1.f);
    // fragColor=vec4(texture(material.texture_diffuse1,uv).xyz,1.f);
}

GeomtryInfo perapreGeomtryInfo(){
    GeomtryInfo geom;
    
    geom.diffuseTexCol=texture(material.texture_diffuse1,uv).xyz;
    geom.specularTexCol=texture(material.texture_specular1,uv).xyz;
    geom.normal=normalize(normal);
    geom.fragPos=position;
    geom.viewDir=normalize(cameraPos-position);
    
    return geom;
}

vec3 Lighting(Light light,GeomtryInfo geom){
    //light direction
    vec3 lightDir;
    if(light.type!=0){
        lightDir=normalize(light.position-geom.fragPos);//点状光源
    }else{
        lightDir=normalize(-light.lightDir);//方向光源
    }
    
    //attenuation with light distance
    float lightDropOff;
    if(light.type!=0){
        //点状光源
        float lightDistance=distance(light.position,geom.fragPos);
        vec3 lightDistVec=vec3(1.f,lightDistance,pow(lightDistance,2));
        lightDropOff=1.f/dot(lightDistVec,light.dropOffFac);
    }else{
        lightDropOff=1.f;//方向光源
    }
    
    //diffuse
    float diffusionFac=max(dot(lightDir,geom.normal),0.f);
    vec3 diffColor=light.diffuse*geom.diffuseTexCol;
    vec3 diffuse=light.intensity*diffColor*diffusionFac*lightDropOff;
    
    //specular
    vec3 halfVec=normalize(lightDir+geom.viewDir);
    float specularFac=pow(max(dot(halfVec,geom.normal),0.f),128.f);
    vec3 specularColor=geom.specularTexCol*light.specular;
    vec3 specular=light.intensity*specularFac*specularColor*lightDropOff;
    
    //ambient
    vec3 ambient=light.ambient*geom.diffuseTexCol*.1f;
    
    //combine
    if(light.type!=2){
        return(diffuse+specular+ambient);
    }else{
        // 聚光灯裁切
        float spotLightCutOff=dot(-light.lightDir,lightDir);
        float cutOffRange=light.innerCutOff-light.outerCutOff;
        spotLightCutOff=clamp((spotLightCutOff-light.outerCutOff)/cutOffRange,0.f,1.f);
        return((diffuse+specular)*spotLightCutOff+ambient);
    }
}
