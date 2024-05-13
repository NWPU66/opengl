#version 460 core

in VS_OUT
{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 TexCoord;
}fs_in;

out vec4 FragColor;

void main()
{
    FragColor=vec4(fs_in.TexCoord,0,1);
}