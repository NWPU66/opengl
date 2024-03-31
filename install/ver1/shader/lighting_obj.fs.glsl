#version 460 core

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    sampler2D diffuseMap;
    sampler2D specularMap;
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
    //combine
    vec3 diffColor=light.diffuse*texture(material.diffuseMap,uv).xyz;
    vec3 diffusion=light.intensity*diffColor*diffusionFac*lightDropOff;
    
    //ambient
    vec3 ambient=light.ambient*material.ambient*texture(material.diffuseMap,uv).xyz;
    
    //specular
    vec3 viewDir=normalize(cameraPos-position);
    vec3 halfVec=normalize(normalize(lightDir)+viewDir);
    float specularFac=pow(max(dot(halfVec,normalize(normal)),0.f),material.shininess);
    //combine
    vec3 specularColor=texture(material.specularMap,uv).xyz*light.specular*material.specular;
    vec3 specular=light.intensity*specularFac*specularColor;
    
    //final color
    fragColor=vec4((ambient+specular+diffusion),1.f);
    // fragColor=texture(material.specularMap,uv);
}