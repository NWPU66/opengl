#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

const uint SAMPLE_COUNT=1024u;
const float PI=3.14159265359;

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

float G1(float NoV,float roughness){
    //GeometrySchlickGGX
    // float r=(roughness+1.);
    // float k=(r*r)/8.;
    float a=roughness;
    float k=(a*a)/2.;
    //与 IBL 一起使用时，BRDF 的几何项略有不同，因为 k 变量的含义稍有不同
    
    float num=NoV;
    float denom=NoV*(1.-k)+k;
    
    return num/denom;
}

float G_term(vec3 N,vec3 V,vec3 L,float roughness){
    //GeometrySmith
    float NoV=max(dot(N,V),0.);
    float NoL=max(dot(N,L),0.);
    float ggx2=G1(NoV,roughness);
    float ggx1=G1(NoL,roughness);
    return ggx1*ggx2;
}

vec2 IntegrateBRDF(float NdotV,float roughness){
    vec3 V=vec3(sqrt(1.-NdotV*NdotV),0,NdotV);
    vec3 N=vec3(0,0,1);
    vec2 DFG=vec2(0);
    
    for(uint i=0u;i<SAMPLE_COUNT;++i){
        vec2 Xi=Hammersley(i,SAMPLE_COUNT);
        vec3 H=ImportanceSampleGGX(Xi,N,roughness);
        vec3 L=normalize(2.*dot(V,H)*H-V);
        
        float NdotL=max(L.z,0.);
        float NdotH=max(H.z,0.);
        float VdotH=max(dot(V,H),0.);
        
        float G=G_term(N,V,L,roughness);
        float G_Vis=(G*VdotH)/(NdotH*NdotV);
        float Fc=pow(1.-VdotH,5.);
        
        DFG+=vec2((1.-Fc)*G_Vis,Fc*G_Vis);
    }
    
    return DFG/float(SAMPLE_COUNT);
}

void main()
{
    vec2 integratedBRDF=IntegrateBRDF(TexCoords.x,TexCoords.y);
    FragColor=vec4(integratedBRDF,0,1);
}