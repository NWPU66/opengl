#version 460 core
//input
in VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 texCoord;
}fs_in;

//output
out vec4 fragColor;

void main(){
    fragColor=vec4(1);
}