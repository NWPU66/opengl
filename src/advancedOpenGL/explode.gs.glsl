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

void main(){
    for(int i=0;i<3;i++){
        processSinglePoint(i);
        EmitVertex();
    }
    EndPrimitive();
}

void processSinglePoint(int idx){
    gs_out.globalPos=gs_in[idx].globalPos;
    gs_out.globalNormal=gs_in[idx].globalNormal;
    gs_out.TexCoords=gs_in[idx].TexCoords;
    
    // gl_Position=explode(gs_out.globalPos,gs_out.globalNormal);
}

vec4 explode(vec4 position,vec3 normal){
    float magnitude=2.f;
    vec3 direction=normal*((sin(time)+1.f)/2.f)*magnitude;
    return position+vec4(direction,0.f);
}
