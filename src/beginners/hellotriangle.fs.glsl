#version 330 core

in vec4 vertexColor;

out vec4 FragColor;
//声明输出的数据是一个4维向量
//色彩的格式是RGBA，每个分量从0到1

void main(){
    FragColor=vec4(1.f,.5f,.2f,1.f);
    FragColor=vertexColor;
}