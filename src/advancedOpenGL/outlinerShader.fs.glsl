#version 460 core
out vec4 fragColor;

uniform float outlineShine;

void main(){
    fragColor=vec4(vec3(outlineShine),1.f);
}