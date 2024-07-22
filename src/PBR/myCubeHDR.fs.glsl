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

const vec2 invAtan=vec2(.1591,.3183);
const float PI=3.14159265359;

const vec3 faceNormals[6]={
    vec3(1,0,0),
    vec3(-1,0,0),
    vec3(0,1,0),
    vec3(0,-1,0),
    vec3(0,0,1),
    vec3(0,0,-1),
};

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv=vec2(atan(v.z,v.x),asin(v.y));
    uv*=invAtan;
    uv+=.5;
    return uv;
}

void main()
{
    float directions[6]={0,0,0,0,0,0};
    for(int i=0;i<6;i++)
    {
        directions[i]=max(dot(normalize(globalNormal),faceNormals[i]),0);
    }
    
    // pre-calculate irradiance
    float resolution=50;
    vec3 irradiance=vec3(0);
    //TBN
    vec3 N=normalize(globalPos);
    vec3 T=normalize(cross(N,vec3(0,1,0)));
    vec3 B=normalize(cross(N,T));
    mat3 TBN=mat3(T,B,N);
    for(float theta=0;theta<2*PI;theta+=2*PI/resolution){
        for(float phi=0;phi<PI/2;phi+=PI/2/resolution){
            vec3 tangentSpace_sampler=vec3(cos(theta)*sin(phi),sin(theta)*sin(phi),cos(phi));\
            vec3 globalSpace_sampler=TBN*tangentSpace_sampler;
            
            vec2 uv=SampleSphericalMap(normalize(globalSpace_sampler));
            vec3 color=texture(equirectangularMap,uv).rgb;
            
            irradiance+=color*cos(phi),0*sin(phi);
        }
    }
    irradiance/=resolution*resolution;
    
    FragColor0=vec4(irradiance,1)*directions[0];
    FragColor1=vec4(irradiance,1)*directions[1];
    FragColor2=vec4(irradiance,1)*directions[2];
    FragColor3=vec4(irradiance,1)*directions[3];
    FragColor4=vec4(irradiance,1)*directions[4];
    FragColor5=vec4(irradiance,1)*directions[5];
}