#version 460 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D screenTexture;
void main()
{
    FragColor=vec4(texture(screenTexture,TexCoords).rgb,1);
}