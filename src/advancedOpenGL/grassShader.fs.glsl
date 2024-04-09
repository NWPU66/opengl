#version 460 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D texture0;
void main(){
    vec4 textureColor=texture(texture0,uv);
    // if(textureColor.a<.1){discard;}
    fragColor=textureColor;
}