#version 460 core

layout(points)in;//从顶点着色器输入的图元类型
/*
points：绘制GL_POINTS图元时（1）。
lines：绘制GL_LINES或GL_LINE_STRIP时（2）
lines_adjacency：GL_LINES_ADJACENCY或GL_LINE_STRIP_ADJACENCY（4）
triangles：GL_TRIANGLES、GL_TRIANGLE_STRIP或GL_TRIANGLE_FAN（3）
triangles_adjacency：GL_TRIANGLES_ADJACENCY或GL_TRIANGLE_STRIP_ADJACENCY（6）
*/

layout(line_strip,max_vertices=2)out;//定义几何着色器输出的图元类型，最大能够输出的顶点数量为2
/*
points
line_strip
triangle_strip
*/

void main(){
    // gl_Position=gl_in[0].gl_Position+vec4(-.1,0.,0.,0.);
    // EmitVertex();
    // gl_Position=gl_in[0].gl_Position+vec4(.1,0.,0.,0.);
    // EmitVertex();
    // EndPrimitive();

    //start
}