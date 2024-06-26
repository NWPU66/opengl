#version 460 core

//input
in vec2 TexCoords;
// uniform float exposure;
uniform sampler2D screenTexture;
uniform sampler2D hdr_frameTexture;

//output
out vec4 frag_color;

const float gamma=2.2;

void main(){
    vec3 hdr_color=texture(screenTexture,TexCoords).rgb;
    
    // 曝光色调映射
    vec3 mapped=vec3(1.)-exp(-hdr_color*2.f);
    // Gamma校正
    mapped=pow(mapped,vec3(1./gamma));
    
    frag_color=vec4(mapped,1.);
    
    // frag_color=vec4(hdr_color,1);
}
/**FIXME - 问题记录
每次启动都不一样，还会黑屏，卡死
*/
