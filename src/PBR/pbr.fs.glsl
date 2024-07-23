#version 460 core
#define MAX_LIGHTS_NUM 16
const float PI=3.14159265359;

//input
in VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 texCoord;
    vec3 globalTangent;
    vec3 globalBitangent;
    mat3 TBN;
}fs_in;

//output
out vec4 fragColor;

//uniform
uniform sampler2D albedoMap;
uniform sampler2D aoMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;

uniform vec3 cameraPos;
uniform float near,far;
uniform samplerCube shadowMaps[4];

uniform samplerCube irradianceMap;//IBL漫反射项
uniform samplerCube prefilterMap;//IBL镜面光照项
uniform sampler2D brdfLUT;//IBL镜面材质项

struct Light{
    int lightType;
    vec3 color;
    float intensity;
    vec3 position,rotation;
    float innerCutOff,outerCutOff;// for spot light
};

layout(std140,binding=0)uniform lightGroup{
    int numLights;
    Light lights[MAX_LIGHTS_NUM];
};

//function

vec3 F_term(float cosTheta,vec3 F0,float roughness){
    //fresnelSchlick
    // return F0+(1.-F0)*pow(clamp(1.-cosTheta,0.,1.),5.);
    return F0+(max(vec3(1.-roughness),F0)-F0)*pow(1.-cosTheta,5.);
}

float D_term(vec3 N,vec3 H,float roughness){
    //DistributionGGX
    float a=roughness*roughness;
    float a2=a*a;
    float NoH=max(dot(N,H),0.);
    float NoH2=NoH*NoH;
    
    float num=a2;
    float denom=(NoH2*(a2-1.)+1.);
    denom=PI*denom*denom;
    
    return num/denom;
}

float G1(float NoV,float roughness){
    //GeometrySchlickGGX
    float r=(roughness+1.);
    float k=(r*r)/8.;
    
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

float screenDepth2globalDepth(float screenDepth){
    screenDepth=screenDepth*2-1;
    return(2*near*far)/(near+far+screenDepth*(near-far));
}

float calculatePointShadow(int i){
    // vec3 displacementToLight=lights[i].position-fs_in.globalPos;
    // float distanceToLight=length(displacementToLight)/far;
    
    // float bias=.0001,s_col=0,l_col=1;
    // float offset=.01,samples=4.,shadow=0.;
    // for(float x=-offset;x<+offset;x+=offset/(samples*.5)){
        //     for(float y=-offset;y<offset;y+=offset/(samples*.5)){
            //         for(float z=-offset;z<offset;z+=offset/(samples*.5)){
                //             float cloestDepth=texture(shadowMaps[i],-displacementToLight+vec3(x,y,z)).r;
                //             shadow+=(distanceToLight+bias>cloestDepth)?s_col:l_col;
            //         }
        //     }
    // }
    // return shadow/pow(samples,3);
    
    // int i=2;
    vec3 displacementToLight=lights[i].position-fs_in.globalPos;
    float distanceToLight=length(displacementToLight)/far;
    float cloestDepth=texture(shadowMaps[i],-normalize(displacementToLight)).r;
    float shadow=(distanceToLight+.0001>cloestDepth)?0:1.;
    return shadow;
}

void main(){
    vec3 albedo=texture(albedoMap,fs_in.texCoord).rgb;
    float ao=texture(aoMap,fs_in.texCoord).r;
    float roughness=texture(roughnessMap,fs_in.texCoord).r;
    float metallic=texture(roughnessMap,fs_in.texCoord).r;
    vec3 normal=texture(normalMap,fs_in.texCoord).rgb;
    normal=normalize(fs_in.TBN*normalize(normal*2.-1));
    
    // vec3 N=normalize(fs_in.globalNormal);
    vec3 N=normal;
    vec3 V=normalize(cameraPos-fs_in.globalPos);
    vec3 R=reflect(-V,N);
    
    vec3 Lo=vec3(0.);
    for(int i=0;i<numLights;++i){
        vec3 L=normalize(lights[i].position-fs_in.globalPos);
        vec3 H=normalize(V+L);
        
        float distance=length(lights[i].position-fs_in.globalPos);
        float attenuation=1./(distance*distance);
        vec3 radiance=lights[i].color*attenuation*lights[i].intensity;
        
        //F_term
        vec3 F0=vec3(.04);
        F0=mix(F0,albedo,metallic);
        vec3 F=F_term(max(dot(H,V),0.),F0,roughness);
        
        //D_term
        float D=D_term(N,H,roughness);
        
        //G_term
        float G=G_term(N,V,L,roughness);
        
        //BRDF镜面项
        vec3 nominator=D*G*F;
        float denominator=4.*max(dot(N,V),0.)*max(dot(N,L),0.)+.001;//避免出现除零错误
        vec3 specular=nominator/denominator;
        
        //BRDF漫反射项
        vec3 kS=F;
        vec3 kD=vec3(1.)-kS;
        kD*=1.-metallic;
        
        //calculate shadow
        float shadowFac=calculatePointShadow(i);
        
        //渲染方程
        float NoL=max(dot(N,L),0.);
        Lo+=(kD*albedo/PI+specular)*radiance*NoL*shadowFac;
    }
    
    //环境光照
    vec3 F0=mix(vec3(.04),albedo,metallic);
    vec3 kS=F_term(max(dot(N,V),0.),F0,roughness);
    vec3 kD=1.-kS;
    kD*=1.-metallic;//金属没有漫射
    vec3 irradiance=texture(irradianceMap,N).rgb;
    vec3 ambient_diffuse=irradiance*albedo/PI;//环境照明漫射部分
    const float MAX_REFLECTION_LOD=4.;
    vec3 prefilteredColor=textureLod(prefilterMap,R,roughness*MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF=texture(brdfLUT,vec2(max(dot(N,V),0.),roughness)).rg;
    vec3 ambient_specular=prefilteredColor*(F0*envBRDF.x+envBRDF.y);//环境照明镜面部分
    vec3 ambient=(kD*ambient_diffuse+ambient_specular)*ao;
    
    vec3 color=(Lo+ambient);
    //色调映射
    color=color/(color+vec3(1.));
    //gamma矫正
    color=pow(color,vec3(1./2.2));
    
    fragColor=vec4(color,1.);
    
}

/**REVIEW -
根据迪士尼公司给出的观察以及后来被Epic Games公司采用的光照模型，
在几何遮蔽函数和法线分布函数中采用粗糙度的平方会让光照看起来更加自然。
*/

/**NOTE -
由于环境光来自半球内围绕法线 N 的所有方向，因此没有一个确定的半向量来计算菲涅耳效应。
为了模拟菲涅耳效应，我们用法线和视线之间的夹角计算菲涅耳系数。然而，之前我们是以受
粗糙度影响的微表面半向量作为菲涅耳公式的输入，但我们目前没有考虑任何粗糙度，表面的
反射率总是会相对较高。间接光和直射光遵循相同的属性，因此我们期望较粗糙的表面在边缘
反射较弱。由于我们没有考虑表面的粗糙度，间接菲涅耳反射在粗糙非金属表面上看起来有点
过强（为了演示目的略微夸大）：
*/
