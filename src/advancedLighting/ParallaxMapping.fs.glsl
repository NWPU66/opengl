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

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D depthMap;
uniform samplerCube skybox;
float height_scale=.1f;

//output
out vec4 fragColor;

vec3 Lighting(int i);
vec2 ParallaxMapping(vec3 viewDir);

//global value
vec3 tangent_dirToCamera=normalize(transpose(fs_in.TBN)*(cameraPos-fs_in.globalPos));
/**FIXME -
TBN用来将tangent space的向量转换到global space下
*/
vec2 texCoord=(gl_FragCoord.x<960)?ParallaxMapping(tangent_dirToCamera):fs_in.texCoord;
vec3 normal=normalize(fs_in.TBN*normalize(texture(normalMap,texCoord).xyz*2.f-1.f));

void main(){
    if(texCoord.x>1.||texCoord.y>1.||texCoord.x<0.||texCoord.y<0.){discard;}
    
    vec3 outputColor=vec3(0.f);
    for(int i=0;i<MAX_LIGHTS_NUM;i++){
        outputColor+=Lighting(i);
    }
    //ambient加上一个随视角的改变，正视的时候强度小，斜视强度大
    vec3 ambient=texture(skybox,reflect(fs_in.globalPos-cameraPos,normal)).xyz;
    vec3 fragToCamera=normalize(cameraPos-fs_in.globalPos);
    float fr=pow(1-max(dot(fragToCamera,normal),0.f),8);
    
    fragColor=vec4(outputColor+ambient*fr,1.f);
    
    // fragColor=vec4(texCoord,0,1);
}

vec2 ParallaxMapping(vec3 tangent_dirToCamera){
    // float height=texture(depthMap,fs_in.texCoord).r;
    /**FIXME -
    不需要反转，原因：我们使用“深度”而非“高度”的概念处理贴图
    在“高度”的概念中，突起的地方值接近1，凹陷的地方值接近0，
    在“深度”的概念中，突起的地方值接近0，凹陷的地方值接近1，
    我们的贴图本就是按照“深度”概念绘制的，所以不需要反转。
    */
    // vec2 p=tangent_dirToCamera.xy/tangent_dirToCamera.z*(height*height_scale);
    // return fs_in.texCoord-p;
    
    // vec3 p=tangent_dirToCamera*height_scale;
    // const float num_layers=100;
    // int i=1;
    // for(;i<num_layers;i++){
        //     vec3 current_p=p/num_layers*i;
        //     float current_height=texture(depthMap,fs_in.texCoord-current_p.xy).r;
        //     if(current_height<=current_p.z){
            //         break;
        //     }
    // }
    // return fs_in.texCoord-p.xy/num_layers*i;
    
    vec3 p=tangent_dirToCamera*height_scale;
    const float num_layers=mix(32,8,max(dot(vec3(0,0,1),tangent_dirToCamera),0));
    vec3 delta_p=p/num_layers;
    float current_p_depth=0;
    vec2 current_texcoord=fs_in.texCoord;
    while(true){
        current_p_depth+=delta_p.z;
        current_texcoord-=delta_p.xy;
        float current_depth=.1f*texture(depthMap,current_texcoord).r;
        if(current_depth<=current_p_depth){break;}
    }
    //Parallax Occlusion Mapping
    vec2 prev_texcoord=current_texcoord+delta_p.xy;
    float current_depth=current_p_depth-.1f*texture(depthMap,current_texcoord).r;
    float prev_depth=texture(depthMap,prev_texcoord).r-(current_p_depth-delta_p.z);
    float weight=current_depth/(current_depth+prev_depth);
    
    return(1-weight)*current_texcoord+weight*prev_texcoord;
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
    float diffuseFac=max(dot(dirToLight,normal),0.f);
    vec3 diffuseColor=lights[i].color*texture(diffuseMap,texCoord).rgb;
    vec3 diffuse=diffuseColor*diffuseFac;
    
    //specular
    vec3 halfVec=normalize(dirToLight+viewDir);
    float specularFac=pow(max(dot(halfVec,normal),0.f),64);
    vec3 specularColor=texture(diffuseMap,texCoord).rgb*lights[i].color;
    vec3 specular=specularColor*specularFac;
    
    //ambient
    // vec3 ambient=texture(skybox,reflect(-viewDir,normal)).xyz;
    
    //combine
    return(diffuse+specular)*lights[i].intensity*lightDistDropoff*spotLightCutOff;
}
