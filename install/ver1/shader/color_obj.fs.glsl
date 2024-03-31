#version 460 core
//input
in vec3 attr_lightDisplacement;
in float deep_inverse;
//output
out vec4 fragColor;
//uniform
uniform vec3 objectColor;
uniform vec3 lightColor;

void main(){
    //把视空间中的属性解出来
    //距离不可以线性插值，但是距离的3轴分量可以
    vec3 lightDisplacement=attr_lightDisplacement/deep_inverse;
    float lightDistance=distance(lightDisplacement,vec3(0.f));
    float lightDropOff=1.f/pow(lightDistance,2.f);//光线强度随距离的平方成反比
    
    fragColor=vec4(objectColor*lightColor*lightDropOff,1.f);
}