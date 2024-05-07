#version 460 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D screenTexture;

const float offset=1./300.;

void main()
{
    vec2 offsets[9]=vec2[](
        vec2(-offset,offset),// 左上
        vec2(0.f,offset),// 正上
        vec2(offset,offset),// 右上
        vec2(-offset,0.f),// 左
        vec2(0.f,0.f),// 中
        vec2(offset,0.f),// 右
        vec2(-offset,-offset),// 左下
        vec2(0.f,-offset),// 正下
        vec2(offset,-offset)// 右下
    );
    float sharpenKernel[9]=float[](
        -1,-1,-1,//
        -1,9,-1,//
        -1,-1,-1//
    );
    float blurKernel[9]=float[](
        1./16,2./16,1./16,//
        2./16,4./16,2./16,//
        1./16,2./16,1./16//
    );
    float edgeKernel[9]=float[](
        1,1,1,//
        1,-8,1,//
        1,1,1//
    );
    
    vec3 sampleTex[9];
    for(int i=0;i<9;i++){
        sampleTex[i]=texture(screenTexture,TexCoords+offsets[i]).rgb;
    }
    vec3 col=vec3(0.);
    for(int i=0;i<9;i++){
        col+=sampleTex[i]*blurKernel[i];
    }
    FragColor=vec4(col,1.);
    
    vec3 color=texture(screenTexture,TexCoords).rgb;
    // FragColor=vec4(vec3(.2126*col.r+.7152*col.g+.0722*col.b),1.);
    FragColor=vec4(color,1.f);
}