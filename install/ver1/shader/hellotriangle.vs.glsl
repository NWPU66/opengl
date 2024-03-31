#version 460 core
//版本声明和使用core模式

layout(location=0)in vec3 aPos;
//声明所有输入的顶点属性
//我们同样也通过layout (location = 0)设定了输入变量的位置值(Location)

out vec4 vertexColor;

void main(){
    gl_Position=vec4(aPos.x,aPos.y,aPos.z,1.);
    //vec4的最后一个分量用于齐次坐标系
    
    vertexColor=vec4(.5,0,0,1.);
}

//在真实的程序里输入数据通常都不是标准化设备坐标，
//所以我们首先必须先把它们转换至OpenGL的可视区域内。
