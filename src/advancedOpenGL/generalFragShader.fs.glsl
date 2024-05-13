#version 460 core

in VS_OUT
{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 TexCoord;
}fs_in;

out vec4 FragColor;

uniform sampler2D texture_diffuse1;

void main()
{
    FragColor=vec4(vec3(texture(texture_diffuse1,fs_in.TexCoord)),1.);
}