#version 460 core
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
layout(location=0)out vec4 gPosition;
layout(location=1)out vec4 gNormal;
layout(location=2)out vec4 gAlbedoSpec;

struct Material{
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_normal1;
};

uniform Material material;

void main(){
    gPosition=vec4(fs_in.globalPos,1);
    // gNormal=vec4(normalize(fs_in.TBN*normalize(texture(material.texture_normal1,fs_in.texCoord).xyz*2.f-1.f)),1);
    gNormal=vec4(fs_in.globalNormal,1);
    gAlbedoSpec.rgb=texture(material.texture_diffuse1,fs_in.texCoord).rgb;
    gAlbedoSpec.a=texture(material.texture_specular1,fs_in.texCoord).r;
}
/**FIXME - 模型切线不连续
不是兄弟，你这nanosuit的模型切线不连续，正确的讲，切线和模型完全对称

原因：
![](src/advancedLighting/QQ截图20240715164344.jpg)
左右半边模型公用一套UV，计算切线时需要局部空间坐标和点上的UV，
对于左半边模型XYZ和UV都是正确的，但是对于右半边模型，X是相反数，
这样计算出来的Tangent正好与YOZ平面对称
*/

/**NOTE -
因为我们使用了多渲染目标，这个布局指示符(Layout Specifier)告诉了OpenGL我们需要渲染到当前
的活跃帧缓冲中的哪一个颜色缓冲。注意我们并没有储存镜面强度到一个单独的颜色缓冲纹理中，
因为我们可以储存它单独的浮点值到其它颜色缓冲纹理的alpha分量中。

请记住，因为有光照计算，所以保证所有变量在一个坐标空间当中至关重要。
在这里我们在世界空间中存储(并计算)所有的变量。
*/
