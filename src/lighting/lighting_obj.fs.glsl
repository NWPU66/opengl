#version 460 core

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light
{
    vec3 position;
    float intensity;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
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
uniform Light light;

void main(){
    //diffusion
    vec3 lightDir=light.position-position;
    float lightDistance=distance(lightDir,vec3(0.f));
    float diffusionFac=max(dot(normalize(lightDir),normalize(normal)),0.f);
    float lightDropOff=1.f/pow(lightDistance,2.f);
    vec3 diffusion=light.intensity*light.diffuse*diffusionFac*lightDropOff;
    
    //ambient
    vec3 ambient=light.ambient*material.ambient;
    
    //specular
    vec3 viewDir=normalize(cameraPos-position);
    vec3 halfVec=normalize(normalize(lightDir)+viewDir);
    float specularFac=pow(max(dot(halfVec,normalize(normal)),0.f),material.shininess);
    vec3 specular=.5f*light.specular*light.intensity*material.specular*specularFac;
    
    //final color
    fragColor=vec4((ambient+specular+diffusion)*material.diffuse,1.f);
    // fragColor=vec4(ambient,1.f);
}