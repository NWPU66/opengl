#version 460 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];
uniform mat4 projection;

uniform float screenWidth;
uniform float screenHeight;
// 屏幕的平铺噪声纹理会根据屏幕分辨率除以噪声大小的值来决定
vec2 noiseScale=vec2(screenWidth/4.,screenHeight/4.);

uniform float radius=1.f;

void main()
{
    vec3 fragPos=texture(gPositionDepth,TexCoords).xyz;
    vec3 normal=texture(gNormal,TexCoords).rgb;
    vec3 randomVec=texture(texNoise,TexCoords*noiseScale).xyz;
    
    vec3 tangent=normalize(randomVec-normal*dot(randomVec,normal));
    vec3 bitangent=cross(normal,tangent);
    mat3 TBN=mat3(tangent,bitangent,normal);
    /**FIXME - 
    唯一的疑点是为什么用随机向量构造tangentSpace，随机向量在viewSpace中是（x，y，0）格式的
    随机的切线空间，引起表面TBN坐标系的随机旋转
    */
    
    //接下来我们对每个核心样本进行迭代，将样本从切线空间变换到观察空间，
    //将它们加到当前像素位置上，并将片段位置深度与储存在原始深度缓冲中的样本深度进行比较。
    
    float occlusion=0.;
    for(int i=0;i<64;++i){
        // 获取样本位置
        vec3 sampleVec=TBN*samples[i];// 切线->观察空间
        sampleVec=fragPos+sampleVec*radius;//是viewspace的向量，z分量表示线性深度
        
        vec4 offset=vec4(sampleVec,1.);
        offset=projection*offset;// 观察->裁剪空间
        offset.xyz/=offset.w;// 透视划分
        offset.xyz=offset.xyz*.5+.5;// 变换到0.0 - 1.0的值域
        
        float sampleDepth=-texture(gPositionDepth,offset.xy).w;//是线性深度，n到f
        float rangeCheck=smoothstep(0.,1.,radius/abs(fragPos.z-sampleDepth));
        occlusion+=((sampleDepth>=sampleVec.z)?1.:0.)*rangeCheck;
    }
    
    occlusion=1.-(occlusion/64);
    FragColor=occlusion;
}

/**NOTE - Gramm-Schmidt
通过使用一个叫做Gramm-Schmidt处理(Gramm-Schmidt Process)的过程，我们创建了一个
正交基(Orthogonal Basis)，每一次它都会根据randomVec的值稍微倾斜。注意因为我们使用
了一个随机向量来构造切线向量，我们没必要有一个恰好沿着几何体表面的TBN矩阵，也就是
不需要逐顶点切线(和双切)向量。
*/

/**NOTE - 
生成的有噪声，而且是以4X4的规律重复的，因为噪声纹理周期是4X4像素。
*/