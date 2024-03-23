#version 460 core
//input
in vec3 position;
in vec2 uv;
in vec3 normal;
//output
out vec4 fragColor;
//uniform
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform vec3 ambientLighting;

void main(){
    //diffusion
    vec3 lightDir=lightPos-position;
    float lightDistance=distance(lightDir,vec3(0.f));
    float diffusionFac=max(dot(normalize(lightDir),normalize(normal)),0.f);
    float lightDropOff=1.f/pow(lightDistance,2.f);
    vec3 diffusion=lightColor*diffusionFac*lightDropOff;
    
    //ambient
    vec3 ambient=ambientLighting;
    
    //specular
    vec3 viewDir=normalize(cameraPos-position);
    vec3 halfVec=normalize(normalize(lightDir)+viewDir);
    float specularFac=pow(max(dot(halfVec,normalize(normal)),0.f),128.f);
    vec3 specular=.5f*specularFac*lightColor;
    
    //final color
    fragColor=vec4((ambient+specular+diffusion)*objectColor,1.f);
}