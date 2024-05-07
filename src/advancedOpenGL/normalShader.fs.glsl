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
    vec3 R_reflect=reflect(I,normalize(normal));
    vec3 skyColor_reflect=pow(texture(skybox,R_reflect).rgb,vec3(.5f));
    
    //天空盒折射
    vec3 R_refract=refract(I,normalize(normal),1.f/1.52f);
    vec3 skyColor_refrac=pow(texture(skybox,R_refract).rgb,vec3(1.1f));
    
    //菲涅尔
    float fresnel=pow(max(1.f-dot(normalize(cameraPos-position),normalize(normal)),0.f),5.f);
    
    vec3 finalCol=(vec3(diffusionFac)+amibient)*toneColor;
    fragColor=vec4(mix(skyColor_refrac,skyColor_reflect,fresnel),1.f);
    // fragColor=vec4(vec3(diffusionFac),1.f);
}