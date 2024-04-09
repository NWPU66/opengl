#version 460 core

in vec3 position;
in vec2 uv;
in vec3 normal;

out vec4 fragColor;

uniform vec3 toneColor;
uniform vec3 cameraPos;

void main(){
    vec3 sunlightDir=normalize(vec3(1.f,1.f,1.f));
    float diffusionFac=max(dot(sunlightDir,normalize(normal)),0.f);
    vec3 amibient=vec3(.1f,.2f,.5f)*(1.f-diffusionFac);
    fragColor=vec4((vec3(diffusionFac)+amibient)*toneColor,1.f);
    // fragColor=vec4(vec3(1.f-sunlightDir),1.f);
}