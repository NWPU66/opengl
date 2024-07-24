#version 460 core

struct Light{
    float intensity;
    vec3 color;
    vec3 points[4];
    bool twoSided;
};

struct Material{
    sampler2D diffuse;// 纹理映射
    vec4 albedoRoughness;// (x,y,z) = 颜色, w = 粗糙度
};

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
uniform Light areaLight;
uniform vec3 areaLightTranslate;
uniform Material material;

uniform vec3 viewPosition;
uniform sampler2D LTC1;// 用于构建逆变换矩阵M
uniform sampler2D LTC2;// GGX预积分, 菲涅尔, 0(unused), 几何衰减

//const
const float LUT_SIZE=64.;// LUT大小
const float LUT_SCALE=(LUT_SIZE-1.)/LUT_SIZE;
const float LUT_BIAS=.5/LUT_SIZE;

//function
vec3 IntegrateEdge(vec3 v1,vec3 v2){
    float x=dot(v1,v2);
    float y=abs(x);
    float a=.8543985+(.4965155+.0145206*y)*y;
    float b=3.4175940+(4.1616724+y)*y;
    float v=a/b;
    float theta_sintheta=(x>0.)?v:.5*inversesqrt(max(1.-x*x,1e-7))-v;
    return cross(v1,v2)*theta_sintheta;
}

vec3 LTC_Evaluate(vec3 N,vec3 V,vec3 P,mat3 Minv,vec3 points[4],bool twoSided){
    // 构建TBN矩阵的三个基向量
    vec3 T1=normalize(V-N*dot(V,N));
    vec3 T2=cross(N,T1);
    mat3 TBN=mat3(T1,T2,N);
    
    // 依据TBN矩阵旋转光源
    Minv=Minv*transpose(TBN);
    
    // 多边形四个顶点
    vec3 L[4];
    for(int i=0;i<4;i++){
        // 通过逆变换矩阵将顶点变换于 受约余弦分布 中
        L[i]=normalize(Minv*(points[i]-P));//先转进切线空间，再应用Po矩阵
        // 投影至单位球面上
    }
    
    // use tabulated horizon-clipped sphere
    // 判断着色点是否位于光源之后
    vec3 dir=points[0]-P;// LTC 空间
    vec3 lightNormal=cross(points[1]-points[0],points[3]-points[0]);
    bool behind=(dot(dir,lightNormal)<0.);
    
    // 边缘积分
    vec3 vsum=vec3(0.);
    for(int i=0;i<4;i++){
        vsum+=IntegrateEdge(L[i],L[(i+1)%4]);
    }
    
    // 计算正半球修正所需要的的参数
    float len=length(vsum);
    
    float z=vsum.z/len;
    if(behind){z=-z;}
    
    vec2 uv=vec2(z*.5f+.5f,len);// range [0, 1]
    uv=uv*LUT_SCALE+LUT_BIAS;
    
    // 通过参数获得几何衰减系数
    float scale=texture(LTC2,uv).w;
    
    float sum=len*scale;
    if(!behind&&!twoSided){sum=0.;}
    
    return vec3(sum);
}

void main(){
    vec3 mDiffuse=vec3(.7f,.8f,.96f);
    vec3 mSpecular=vec3(.04);
    
    vec3 N=normalize(fs_in.globalNormal);
    vec3 P=fs_in.globalPos;
    vec3 V=normalize(viewPosition-P);
    float dotNV=clamp(dot(N,V),0.f,1.f);
    
    // 通过粗糙度和sqrt(1-cos_theta)采样M_texture
    vec2 uv=vec2(material.albedoRoughness.w,sqrt(1.f-dotNV));//为什么用这个参数来采样v
    uv=uv*LUT_SCALE+LUT_BIAS;
    
    // 获得inverse_M的四个参数
    vec4 t1=texture(LTC1,uv);
    // 获得用于计算菲涅尔的两个参数
    vec4 t2=texture(LTC2,uv);
    
    mat3 Minv=mat3(
        vec3(t1.x,0,t1.y),
        vec3(0,1,0),
        vec3(t1.z,0,t1.w)
    );
    
    // translate light source for testing
    vec3 translatedPoints[4];
    for(int i=0;i<4;i++){
        translatedPoints[i]=areaLight.points[i]+areaLightTranslate;
    }
    
    // Evaluate LTC shading
    vec3 diffuse=LTC_Evaluate(N,V,P,mat3(1),translatedPoints,areaLight.twoSided);
    vec3 specular=LTC_Evaluate(N,V,P,Minv,translatedPoints,areaLight.twoSided);
    
    // GGX BRDF shadowing and Fresnel
    // t2.x: shadowedF90 (F90 normally it should be 1.0)
    // t2.y: Smith function for Geometric Attenuation Term, it is dot(V or L, H).
    specular*=mSpecular*t2.x+(1.f-mSpecular)*t2.y;
    
    vec3 result=areaLight.color*areaLight.intensity*(specular+mDiffuse*diffuse);
    
    fragColor=vec4(result,1);
}
