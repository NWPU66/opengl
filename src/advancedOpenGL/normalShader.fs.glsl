#version 460 core

in vec3 position;
in vec2 uv;
in vec3 normal;

out vec4 fragColor;

uniform vec3 toneColor;
uniform vec3 cameraPos;
uniform samplerCube skybox;

void main(){
    vec3 sunlightDir=normalize(vec3(1.f,1.f,1.f));
    float diffusionFac=max(dot(sunlightDir,normalize(normal)),0.f);
    vec3 amibient=vec3(.1f,.2f,.5f)*(1.f-diffusionFac);
    
    //天空盒反射
    vec3 I=normalize(position-cameraPos);
    vec3 R=reflect(I,normalize(normal));
    vec3 skyColor=texture(skybox,R).rgb;
    
    //菲涅尔
    float fresnel=pow(max(1.f-dot(normalize(cameraPos-position),normalize(normal)),0.f),5.f);
    
    vec3 finalCol=(vec3(diffusionFac)+amibient)*toneColor;
    fragColor=vec4(mix(finalCol,skyColor,fresnel),1.f);
    // fragColor=vec4(vec3(fresnel),1.f);
}