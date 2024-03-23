#version 460 core

out vec4 fragColor;
//如果你在片段着色器没有定义输出颜色，OpenGL会把你的物体渲染为黑色（或白色）。

in vec4 vertexColor;
//当类型和名字都一样的时候，OpenGL就会把两个变量链接到一起，
//它们之间就能发送数据了（这是在链接程序对象时完成的）。

uniform vec4 outsideSetColor;
//cpu中的程序可以直接与uniform类型的变量交流
//因为uniform是全局变量，我们可以在任何着色器中定义它们，而无需通过顶点着色器作为中介。

/*
如果你声明了一个uniform却在GLSL代码中没用过，编译器会静默移除这个变量，
导致最后编译出的版本中并不会包含它，这可能导致几个非常麻烦的错误，记住这点！
*/
in vec2 texCoord;
uniform sampler2D myTexture1;
uniform sampler2D myTexture2;
/*
片段着色器也应该能访问纹理对象，但是我们怎样能把纹理对象传给片段着色器呢？
GLSL有一个供纹理对象使用的内建数据类型，叫做采样器(Sampler)，
它以纹理类型作为后缀，比如sampler1D、sampler3D，或在我们的例子中的sampler2D。
我们可以简单声明一个uniform sampler2D把一个纹理添加到片段着色器中，
稍后我们会把纹理赋值给这个uniform。
*/

void main(){
    vec4 texColor1=texture(myTexture1,texCoord);
    vec4 texColor2=texture(myTexture2,2*vec2(-texCoord.x, texCoord.y)+vec2(.5,.5));
    /*
    我们使用GLSL内建的texture函数来采样纹理的颜色，
    它第一个参数是纹理采样器，第二个参数是对应的纹理坐标。
    texture函数会使用之前设置的纹理参数对相应的颜色值进行采样。
    */
    fragColor=vertexColor*mix(texColor1,texColor2,outsideSetColor.y);
    // fragColor=vec4(1.,1.,1.,texColor2.a);
}