#version 460 core
//定义灯光组的最大灯光数量
#define MAX_LIGHTS_NUM 16

//input
in VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 texCoord;
    vec3 globalTangent;
    vec3 globalBitangent;
    mat3 TBN;
}fs_in;

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

uniform vec3 cameraPos;
uniform sampler2D texture0;
uniform sampler2D texture_normal;
uniform samplerCube skybox;

//output
out vec4 fragColor;

vec3 globalNormal=(gl_FragCoord.x<400)?normalize(fs_in.TBN*normalize(texture(texture_normal,fs_in.texCoord).xyz*2.f-1.f)):fs_in.globalNormal;
/**NOTE -
可行的改进：在vertexshader中把所有要在fragmentshader中使用的变量都转换进tangent空间
这样就不必再fragmentshader中使用昂贵的矩阵乘法来计算globalNormal

需要计算的量：
uniform vec3 dirToLight;
uniform vec3 dirToCamera;
其中dirToLight对于多光源管理来说有点困难
另外这两个量在vertexshader准备的时候最好使用“位移”而非“方向”
“位移”在globalspace中式线性的，“方向”则不是
*/

vec3 Lighting(int i);

void main(){
    vec3 outputColor=vec3(0.f);
    for(int i=0;i<MAX_LIGHTS_NUM;i++){
        outputColor+=Lighting(i);
    }
    
    //ambient加上一个随视角的改变，正视的时候强度小，斜视强度大
    vec3 ambient=texture(skybox,reflect(fs_in.globalPos-cameraPos,fs_in.globalNormal)).xyz;
    vec3 fragToCamera=normalize(cameraPos-fs_in.globalPos);
    float fr=pow(1-max(dot(fragToCamera,fs_in.globalNormal),0.f),8);
    fragColor=vec4(vec3(fr),1);
    
    fragColor=vec4(outputColor+ambient*fr,1.f);
    
    // fragColor=vec4(globalNormal,1);
}

vec3 Lighting(int i){
    if(lights[i].lightType==-1){
        return vec3(0);
    }
    
    vec3 dispToLight=lights[i].position-fs_in.globalPos;
    vec3 dirToLight=normalize(dispToLight);
    if(lights[i].lightType==1){
        //日光
        dirToLight=-normalize(lights[i].rotation);
    }
    vec3 viewDir=normalize(cameraPos-fs_in.globalPos);
    
    //光源衰减
    float lightDistDropoff=1;
    if(lights[i].lightType!=1){
        //日光，不计算距离
        float lightDist=distance(dispToLight,vec3(0));
        // float lightDistDropoff=1/dot(vec3(1,lightDist,pow(lightDist,2)),vec3(1,.09,.032));
        float lightDistDropoff=1/pow(lightDist,2);
    }
    // 聚光灯裁切
    float spotLightCutOff=1;
    if(lights[i].lightType==2){
        spotLightCutOff=dot(-dirToLight,lights[i].rotation);
        float cutOffRange=lights[i].innerCutOff-lights[i].outerCutOff;
        spotLightCutOff=clamp((spotLightCutOff-lights[i].outerCutOff)/cutOffRange,0.f,1.f);
    }
    
    //diffusion
    float diffuseFac=max(dot(dirToLight,globalNormal),0.f);
    vec3 diffuseColor=lights[i].color*texture(texture0,fs_in.texCoord).rgb;
    vec3 diffuse=diffuseColor*diffuseFac;
    
    //specular
    vec3 halfVec=normalize(dirToLight+viewDir);
    float specularFac=pow(max(dot(halfVec,globalNormal),0.f),64);
    vec3 specularColor=texture(texture0,fs_in.texCoord).rgb*lights[i].color;
    vec3 specular=specularColor*specularFac;
    
    //ambient
    // vec3 ambient=texture(skybox,reflect(-viewDir,globalNormal)).xyz;
    
    //combine
    return(diffuse+specular)*lights[i].intensity*lightDistDropoff*spotLightCutOff;
    // return vec3(dot(globalNormal,dirToLight))/5;
}
