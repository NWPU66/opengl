#version 460 core

//input
in vec2 TexCoords;
uniform float exposure;
uniform sampler2D hdr_frameTexture;

//output
out vec4 frag_color;

const float gamma=2.2;

void main(){
    vec3 hdr_color=texture(hdr_frameTexture,TexCoords).rgb;
    
    // 曝光色调映射
    vec3 mapped=hdr_color*exposure;
    if(gl_FragCoord.x<960){mapped=vec3(1.)-exp(-mapped);}
    // Gamma校正
    mapped=pow(mapped,vec3(1./gamma));
    
    frag_color=vec4(mapped,1);
    // frag_color=vec4(pow(vec3(1)-exp(-texture(hdr_frameTexture,TexCoords).rgb*exposure),vec3(1/gamma)),1);
}

// Reinhard色调映射
// vec3 mapped = hdrColor / (hdrColor + vec3(1.0));