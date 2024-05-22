#version 460 core
//input
in VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 texCoord;
}fs_in;
uniform sampler2D texture0;

//output
out vec4 fragColor;

void main(){
    fragColor=vec4(texture(texture0,fs_in.texCoord).xyz,1);
}