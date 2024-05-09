#version 460 core
layout(triangles)in;
layout(line_strip,max_vertices=6)out;

in VS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec3 viewNormal;
    vec2 texCoord;
}gs_in[];

out GS_OUT{
    vec3 globalPos;
    vec2 texCoord;
}gs_out;

uniform mat4 projection;

const float MAGNITUDE=.15;

void ProcessSinglePoint(int idx);
vec3 GetNormal();

void main(){
    // ProcessSinglePoint(0);
    // ProcessSinglePoint(1);
    // ProcessSinglePoint(2);
    
    vec4 ViewTriangleCenter=(gl_in[0].gl_Position+gl_in[1].gl_Position+gl_in[2].gl_Position)/3.;
    vec3 ViewNormal=GetNormal();
    gl_Position=projection*ViewTriangleCenter;
    EmitVertex();
    gl_Position=projection*(ViewTriangleCenter+vec4(ViewNormal,0)*MAGNITUDE);
    EmitVertex();
    EndPrimitive();
}

void ProcessSinglePoint(int idx){
    gs_out.texCoord=gs_in[idx].texCoord;//texCoord
    
    //1st point
    gs_out.globalPos=gs_in[idx].globalPos;
    gl_Position=projection*gl_in[idx].gl_Position;
    EmitVertex();
    
    //2nd point
    gs_out.globalPos=gs_in[idx].globalPos+gs_in[idx].globalNormal*MAGNITUDE;
    gl_Position=projection*(gl_in[idx].gl_Position+vec4(gs_in[idx].viewNormal,0)*MAGNITUDE);
    EmitVertex();
    
    EndPrimitive();
}

vec3 GetNormal()
{
    vec3 a=vec3(gl_in[0].gl_Position)-vec3(gl_in[1].gl_Position);
    vec3 b=vec3(gl_in[2].gl_Position)-vec3(gl_in[1].gl_Position);
    return normalize(cross(b,a));//FIXME - 这里的法向量方向对吗？
}
