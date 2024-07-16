#version 460 core
//input
in VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 texCoord;
    vec3 globalTangent;
    vec3 globalBitangent;
    mat3 TBN;
    
    vec3 viewPos;
    vec3 viewNormal;
}fs_in;

//output
layout(location=0)out vec4 gPositionDepth;
layout(location=1)out vec4 gNormal;
layout(location=2)out vec4 gAlbedoSpec;

const float NEAR=.1f;
const float FAR=100.f;

float LinearizeDepth(float depth)
{
    float z=depth*2.-1.;// 回到NDC
    return(2.*NEAR*FAR)/(FAR+NEAR-z*(FAR-NEAR));//between n to f
}

void main(){
    gPositionDepth=vec4(fs_in.viewPos,LinearizeDepth(gl_FragCoord.z));
    gNormal=vec4(fs_in.viewNormal,1);
    gAlbedoSpec=vec4(1,1,1,.5);
}
