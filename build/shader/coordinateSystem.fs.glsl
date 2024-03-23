#version 460 core

//input
in vec4 vertexColor;
in vec2 texCoord;

//output
out vec4 fragColor;

//uniform
uniform vec4 outsideSetColor;
uniform sampler2D myTexture1;
uniform sampler2D myTexture2;

void main(){
    vec4 texColor1=texture(myTexture1,texCoord);
    vec4 texColor2=texture(myTexture2,2*vec2(-texCoord.x,texCoord.y)+vec2(.5,.5));
    fragColor=vertexColor*mix(texColor1,texColor2,outsideSetColor.y);
}