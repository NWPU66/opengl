#version 460 core
in vec3 fcolor;
out vec4 FragColor;
void main(){
    FragColor=vec4(fcolor,1.f);
}