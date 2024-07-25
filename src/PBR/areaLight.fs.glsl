// #version 460 core

// struct Light{
    //     float intensity;
    //     vec3 color;
    //     vec3 points[4];
    //     bool twoSided;
// };

// struct Material{
    //     sampler2D diffuse;// 纹理映射
    //     vec4 albedoRoughness;// (x,y,z) = 颜色, w = 粗糙度
// };

// //input
// in VS_OUT{
    //     vec3 globalPos;
    //     vec3 globalNormal;
    //     vec2 texCoord;
    //     vec3 globalTangent;
    //     vec3 globalBitangent;
    //     mat3 TBN;
// }fs_in;

// //output
// out vec4 fragColor;

// //uniform
// uniform Light areaLight;
// uniform vec3 areaLightTranslate;
// uniform Material material;

// uniform vec3 viewPosition;
// uniform sampler2D LTC1;// 用于构建逆变换矩阵M
// uniform sampler2D LTC2;// GGX预积分, 菲涅尔, 0(unused), 几何衰减

// //const
// const float LUT_SIZE=64.;// LUT大小
// const float LUT_SCALE=(LUT_SIZE-1.)/LUT_SIZE;
// const float LUT_BIAS=.5/LUT_SIZE;

// //function
// vec3 IntegrateEdge(vec3 v1,vec3 v2){
    //     float x=dot(v1,v2);
    //     float y=abs(x);
    //     float a=.8543985+(.4965155+.0145206*y)*y;
    //     float b=3.4175940+(4.1616724+y)*y;
    //     float v=a/b;
    //     float theta_sintheta=(x>0.)?v:.5*inversesqrt(max(1.-x*x,1e-7))-v;
    //     return cross(v1,v2)*theta_sintheta;
// }

// vec3 LTC_Evaluate(vec3 N,vec3 V,vec3 P,mat3 Minv,vec3 points[4],bool twoSided){
    //     // 构建TBN矩阵的三个基向量
    //     vec3 T1=normalize(V-N*dot(V,N));
    //     vec3 T2=cross(N,T1);
    //     mat3 TBN=mat3(T1,T2,N);
    
    //     // 依据TBN矩阵旋转光源
    //     Minv=Minv*transpose(TBN);
    
    //     // 多边形四个顶点
    //     vec3 L[4];
    //     for(int i=0;i<4;i++){
        //         // 通过逆变换矩阵将顶点变换于 受约余弦分布 中
        //         L[i]=normalize(Minv*(points[i]-P));//先转进切线空间，再应用Po矩阵
        //         // 投影至单位球面上
    //     }
    
    //     // use tabulated horizon-clipped sphere
    //     // 判断着色点是否位于光源之后
    //     vec3 dir=points[0]-P;// LTC 空间
    //     vec3 lightNormal=cross(points[1]-points[0],points[3]-points[0]);
    //     bool behind=(dot(dir,lightNormal)<0.);
    
    //     // 边缘积分
    //     vec3 vsum=vec3(0.);
    //     for(int i=0;i<4;i++){
        //         vsum+=IntegrateEdge(L[i],L[(i+1)%4]);
    //     }
    
    //     // 计算正半球修正所需要的的参数
    //     float len=length(vsum);
    
    //     float z=vsum.z/len;
    //     if(behind){z=-z;}
    
    //     vec2 uv=vec2(z*.5f+.5f,len);// range [0, 1]
    //     uv=uv*LUT_SCALE+LUT_BIAS;
    
    //     // 通过参数获得几何衰减系数
    //     float scale=texture(LTC2,uv).w;
    
    //     float sum=len*scale;
    //     if(!behind&&!twoSided){sum=0.;}
    
    //     return vec3(sum);
// }

// void main(){
    //     vec3 mDiffuse=vec3(.7f,.8f,.96f);
    //     vec3 mSpecular=vec3(.04);
    
    //     vec3 N=normalize(fs_in.globalNormal);
    //     vec3 P=fs_in.globalPos;
    //     vec3 V=normalize(viewPosition-P);
    //     float dotNV=clamp(dot(N,V),0.f,1.f);
    
    //     // 通过粗糙度和sqrt(1-cos_theta)采样M_texture
    //     vec2 uv=vec2(material.albedoRoughness.w,sqrt(1.f-dotNV));//为什么用这个参数来采样v
    //     uv=uv*LUT_SCALE+LUT_BIAS;
    
    //     // 获得inverse_M的四个参数
    //     vec4 t1=texture(LTC1,uv);
    //     // 获得用于计算菲涅尔的两个参数
    //     vec4 t2=texture(LTC2,uv);
    
    //     mat3 Minv=mat3(
        //         vec3(t1.x,0,t1.y),
        //         vec3(0,1,0),
        //         vec3(t1.z,0,t1.w)
    //     );
    
    //     // translate light source for testing
    //     vec3 translatedPoints[4];
    //     for(int i=0;i<4;i++){
        //         translatedPoints[i]=areaLight.points[i]+areaLightTranslate;
    //     }
    
    //     // Evaluate LTC shading
    //     vec3 diffuse=LTC_Evaluate(N,V,P,mat3(1),translatedPoints,areaLight.twoSided);
    //     vec3 specular=LTC_Evaluate(N,V,P,Minv,translatedPoints,areaLight.twoSided);
    
    //     // GGX BRDF shadowing and Fresnel
    //     // t2.x: shadowedF90 (F90 normally it should be 1.0)
    //     // t2.y: Smith function for Geometric Attenuation Term, it is dot(V or L, H).
    //     specular*=mSpecular*t2.x+(1.f-mSpecular)*t2.y;
    
    //     vec3 result=areaLight.color*areaLight.intensity*(specular+mDiffuse*diffuse);
    
    //     fragColor=vec4(result,1);
// }

#version 330 core

out vec4 fragColor;

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 texcoord;

struct Light
{
    float intensity;
    vec3 color;
    vec3 points[4];
    bool twoSided;
};
uniform Light areaLight;
uniform vec3 areaLightTranslate;

struct Material
{
    sampler2D diffuse;
    vec4 albedoRoughness;// (x,y,z) = color, w = roughness
};
uniform Material material;

uniform vec3 viewPosition;
uniform sampler2D LTC1;// for inverse M
uniform sampler2D LTC2;// GGX norm, fresnel, 0(unused), sphere

const float LUT_SIZE=64.;// ltc_texture size
const float LUT_SCALE=(LUT_SIZE-1.)/LUT_SIZE;
const float LUT_BIAS=.5/LUT_SIZE;

// Vector form without project to the plane (dot with the normal)
// Use for proxy sphere clipping
vec3 IntegrateEdgeVec(vec3 v1,vec3 v2)
{
    // Using built-in acos() function will result flaws
    // Using fitting result for calculating acos()
    float x=dot(v1,v2);
    float y=abs(x);
    
    float a=.8543985+(.4965155+.0145206*y)*y;
    float b=3.4175940+(4.1616724+y)*y;
    float v=a/b;
    
    float theta_sintheta=(x>0.)?v:.5*inversesqrt(max(1.-x*x,1e-7))-v;
    
    return cross(v1,v2)*theta_sintheta;
}

// P is fragPos in world space (LTC distribution)
vec3 LTC_Evaluate(vec3 N,vec3 V,vec3 P,mat3 Minv,vec3 points[4],bool twoSided)
{
    // construct orthonormal basis around N
    vec3 T1,T2;
    T1=normalize(V-N*dot(V,N));
    T2=cross(N,T1);
    
    // rotate area light in (T1, T2, N) basis
    Minv=Minv*transpose(mat3(T1,T2,N));
    
    // polygon (allocate 4 vertices for clipping)
    vec3 L[4];
    // transform polygon from LTC back to origin Do (cosine weighted)
    L[0]=Minv*(points[0]-P);
    L[1]=Minv*(points[1]-P);
    L[2]=Minv*(points[2]-P);
    L[3]=Minv*(points[3]-P);
    
    // use tabulated horizon-clipped sphere
    // check if the shading point is behind the light
    vec3 dir=points[0]-P;// LTC space
    vec3 lightNormal=cross(points[1]-points[0],points[3]-points[0]);
    bool behind=(dot(dir,lightNormal)<0.);
    
    // cos weighted space
    L[0]=normalize(L[0]);
    L[1]=normalize(L[1]);
    L[2]=normalize(L[2]);
    L[3]=normalize(L[3]);
    
    // integrate
    vec3 vsum=vec3(0.);
    vsum+=IntegrateEdgeVec(L[0],L[1]);
    vsum+=IntegrateEdgeVec(L[1],L[2]);
    vsum+=IntegrateEdgeVec(L[2],L[3]);
    vsum+=IntegrateEdgeVec(L[3],L[0]);
    
    // form factor of the polygon in direction vsum
    float len=length(vsum);
    
    float z=vsum.z/len;
    if(behind)
    z=-z;
    
    vec2 uv=vec2(z*.5f+.5f,len);// range [0, 1]
    uv=uv*LUT_SCALE+LUT_BIAS;
    
    // Fetch the form factor for horizon clipping
    float scale=texture(LTC2,uv).w;
    
    float sum=len*scale;
    if(!behind&&!twoSided)
    sum=0.;
    
    // Outgoing radiance (solid angle) for the entire polygon
    vec3 Lo_i=vec3(sum,sum,sum);
    return Lo_i;
}

// PBR-maps for roughness (and metallic) are usually stored in non-linear
// color space (sRGB), so we use these functions to convert into linear RGB.
vec3 PowVec3(vec3 v,float p)
{
    return vec3(pow(v.x,p),pow(v.y,p),pow(v.z,p));
}

const float gamma=2.2;
vec3 ToLinear(vec3 v){return PowVec3(v,gamma);}
vec3 ToSRGB(vec3 v){return PowVec3(v,1./gamma);}

void main()
{
    // gamma correction
    vec3 mDiffuse=vec3(.7f,.8f,.96f);// * vec3(0.7f, 0.8f, 0.96f);
    vec3 mSpecular=ToLinear(vec3(.23f,.23f,.23f));// mDiffuse
    
    vec3 result=vec3(0.f);
    
    vec3 N=normalize(worldNormal);
    vec3 V=normalize(viewPosition-worldPosition);
    vec3 P=worldPosition;
    float dotNV=clamp(dot(N,V),0.f,1.f);
    
    // use roughness and sqrt(1-cos_theta) to sample M_texture
    vec2 uv=vec2(material.albedoRoughness.w,sqrt(1.f-dotNV));
    uv=uv*LUT_SCALE+LUT_BIAS;
    
    // get 4 parameters for inverse_M
    vec4 t1=texture(LTC1,uv);
    
    // Get 2 parameters for Fresnel calculation
    vec4 t2=texture(LTC2,uv);
    
    mat3 Minv=mat3(
        vec3(t1.x,0,t1.y),
        vec3(0,1,0),
        vec3(t1.z,0,t1.w)
    );
    
    // translate light source for testing
    vec3 translatedPoints[4];
    translatedPoints[0]=areaLight.points[0]+areaLightTranslate;
    translatedPoints[1]=areaLight.points[1]+areaLightTranslate;
    translatedPoints[2]=areaLight.points[2]+areaLightTranslate;
    translatedPoints[3]=areaLight.points[3]+areaLightTranslate;
    
    // Evaluate LTC shading
    vec3 diffuse=LTC_Evaluate(N,V,P,mat3(1),translatedPoints,areaLight.twoSided);
    vec3 specular=LTC_Evaluate(N,V,P,Minv,translatedPoints,areaLight.twoSided);
    
    // GGX BRDF shadowing and Fresnel
    // t2.x: shadowedF90 (F90 normally it should be 1.0)
    // t2.y: Smith function for Geometric Attenuation Term, it is dot(V or L, H).
    specular*=mSpecular*t2.x+(1.f-mSpecular)*t2.y;
    
    result=areaLight.color*areaLight.intensity*(specular+mDiffuse*diffuse);
    
    fragColor=vec4(ToSRGB(result),1.f);
}
