#version 330 core
in GS_OUT{
    vec3 globalPos;
    vec2 texCoord;
}fs_in;

out vec4 FragColor;

void main()
{
    FragColor=vec4(1.,1.,0.,1.);
}