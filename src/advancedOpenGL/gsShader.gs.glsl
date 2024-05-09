#version 460 core
layout(points)in;
layout(triangle_strip,max_vertices=5)out;

in VS_OUT{
    vec3 color;
}gs_in[];

out vec3 fcolor;

void build_house(vec4 position)
{
    fcolor=gs_in[0].color;
    gl_Position=position+vec4(-.1,-.1,0.,0.);// 1:左下
    EmitVertex();
    gl_Position=position+vec4(.1,-.1,0.,0.);// 2:右下
    EmitVertex();
    gl_Position=position+vec4(-.1,.1,0.,0.);// 3:左上
    EmitVertex();
    gl_Position=position+vec4(.1,.1,0.,0.);// 4:右上
    EmitVertex();
    gl_Position=position+vec4(0.,.2,0.,0.);// 5:顶部
    fcolor=vec3(1.f);
    EmitVertex();
    EndPrimitive();
}

void main(){
    // gl_Position=gl_in[0].gl_Position;
    // gl_PointSize=gl_in[0].gl_PointSize;
    // EmitVertex();
    // EndPrimitive();
    
    build_house(gl_in[0].gl_Position);
}