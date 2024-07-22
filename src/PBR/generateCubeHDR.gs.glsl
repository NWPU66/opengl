#version 460 core

layout(triangles)in;
layout(triangle_strip,max_vertices=3)out;

in vec3 globalPosition[];
in vec3 globalNormal[];

uniform mat4 projection;
uniform mat4 view[6];

out vec3 globalPos;

const vec3 faceNormals[6]={
    vec3(1,0,0),
    vec3(-1,0,0),
    vec3(0,1,0),
    vec3(0,-1,0),
    vec3(0,0,1),
    vec3(0,0,-1),
};

void main(){
    int best_i=-1;
    // float best_dot=-100;
    // for(int i=0;i<6;i++){
        //     float current_dot=dot(faceNormals[i],normalize(globalNormal[0]));
        //     if(current_dot>best_dot){
            //         best_dot=current_dot;
            //         best_i=i;
        //     }
    // }
    best_i=0;
    
    for(int j=0;j<3;j++){
        gl_Layer=best_i;
        globalPos=globalPosition[j];
        gl_Position=projection*view[best_i]*vec4(globalPosition[j],1);
        EmitVertex();
    }
    EndPrimitive();
}
