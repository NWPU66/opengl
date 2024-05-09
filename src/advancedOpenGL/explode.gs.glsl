#version 460 core
layout(triangles)in;
layout(triangle_strip,max_vertices=3)out;

in VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 TexCoords;
}gs_in[];

out GS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 TexCoords;
}gs_out;

uniform float time;

void processSinglePoint(int idx);
vec4 explode(vec4 position,vec3 normal);
vec3 GetNormal();

void main()
{
    for(int i=0;i<3;i++){
        processSinglePoint(i);
        EmitVertex();
    }
    EndPrimitive();
}

void processSinglePoint(int idx)
{
    gs_out.globalPos=gs_in[idx].globalPos;
    gs_out.globalNormal=gs_in[idx].globalNormal;
    gs_out.TexCoords=gs_in[idx].TexCoords;
    
    gl_Position=explode(gl_in[idx].gl_Position,GetNormal());
}

vec4 explode(vec4 position,vec3 normal)
{
    float magnitude=1.f;
    vec3 direction=normal*((sin(time)+1.)/2.)*magnitude;
    return position+vec4(direction,0.);
}

vec3 GetNormal()
{
    vec3 a=vec3(gl_in[0].gl_Position)-vec3(gl_in[1].gl_Position);
    vec3 b=vec3(gl_in[2].gl_Position)-vec3(gl_in[1].gl_Position);
    return normalize(cross(a,b));//FIXME - 这里的法向量方向对吗？
    
    /**FIXME  - 错题本
    扣掉齐次向量的第四维，对于表示方向的向量而言没有影响。*/
}
