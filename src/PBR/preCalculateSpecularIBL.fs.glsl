#version 460 core
layout(location=0)out vec4 FragColor0;
layout(location=1)out vec4 FragColor1;
layout(location=2)out vec4 FragColor2;
layout(location=3)out vec4 FragColor3;
layout(location=4)out vec4 FragColor4;
layout(location=5)out vec4 FragColor5;

in vec3 globalPos;
in vec3 globalNormal;

uniform sampler2D equirectangularMap;
uniform float roughness;

const uint SAMPLE_COUNT=4096u;
const float PI=3.14159265359;
const vec3 faceNormals[6]={
    vec3(1,0,0),
    vec3(-1,0,0),
    vec3(0,1,0),
    vec3(0,-1,0),
    vec3(0,0,1),
    vec3(0,0,-1),
};
const vec2 invAtan=vec2(.1591,.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv=vec2(atan(v.z,v.x),asin(v.y));
    uv*=invAtan;
    uv+=.5;
    return uv;
}

float RadicalInverse_VdC(uint bits)
{
    bits=(bits<<16u)|(bits>>16u);
    bits=((bits&0x55555555u)<<1u)|((bits&0xAAAAAAAAu)>>1u);
    bits=((bits&0x33333333u)<<2u)|((bits&0xCCCCCCCCu)>>2u);
    bits=((bits&0x0F0F0F0Fu)<<4u)|((bits&0xF0F0F0F0u)>>4u);
    bits=((bits&0x00FF00FFu)<<8u)|((bits&0xFF00FF00u)>>8u);
    return float(bits)*2.3283064365386963e-10;// / 0x100000000
}
vec2 Hammersley(uint i,uint N)
{
    return vec2(float(i)/float(N),RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi,vec3 N,float roughness)
{
    float a=roughness*roughness;
    
    float phi=2.*PI*Xi.x;
    float cosTheta=sqrt((1.-Xi.y)/(1.+(a*a-1.)*Xi.y));
    float sinTheta=sqrt(1.-cosTheta*cosTheta);
    
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x=cos(phi)*sinTheta;
    H.y=sin(phi)*sinTheta;
    H.z=cosTheta;
    
    // from tangent-space vector to world-space sample vector
    vec3 up=abs(N.z)<.999?vec3(0.,0.,1.):vec3(1.,0.,0.);
    vec3 tangent=normalize(cross(up,N));
    vec3 bitangent=cross(N,tangent);
    
    vec3 sampleVec=tangent*H.x+bitangent*H.y+N*H.z;
    return normalize(sampleVec);
}

void main(){
    float directions[6]={0,0,0,0,0,0};
    for(int i=0;i<6;i++)
    {
        directions[i]=max(dot(normalize(globalNormal),faceNormals[i]),0);
    }
    
    vec3 N=normalize(globalPos);
    vec3 R=N;
    vec3 V=R;
    
    float totalWeight=0.;
    vec3 prefilteredColor=vec3(0.);
    for(uint i=0u;i<SAMPLE_COUNT;++i)
    {
        vec2 Xi=Hammersley(i,SAMPLE_COUNT);
        vec3 H=ImportanceSampleGGX(Xi,N,roughness);
        vec3 L=normalize(2.*dot(V,H)*H-V);
        // vec3 L=reflect(V,H);
        
        float NdotL=max(dot(N,L),0.);
        vec2 uv=SampleSphericalMap(L);
        prefilteredColor+=texture(equirectangularMap,uv).rgb*NdotL;
        totalWeight+=NdotL;
    }
    prefilteredColor=prefilteredColor/totalWeight;
    
    FragColor0=vec4(prefilteredColor,1)*directions[0];
    FragColor1=vec4(prefilteredColor,1)*directions[1];
    FragColor2=vec4(prefilteredColor,1)*directions[2];
    FragColor3=vec4(prefilteredColor,1)*directions[3];
    FragColor4=vec4(prefilteredColor,1)*directions[4];
    FragColor5=vec4(prefilteredColor,1)*directions[5];
}

/**NOTE -
当roughness=0时，切线空间的采样向量都是(0,0,1)，
即总是沿着世界空间的法向（globalPos）的方向采样
采样有唯一确定的方向，所以生成的贴图看起来一点都不模糊
*/

