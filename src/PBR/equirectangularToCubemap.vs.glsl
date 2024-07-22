#version 460 core
layout(location=0)in vec3 aPos;
layout(location=1)out vec3 aNormal;

uniform mat4 projection;
uniform mat4 view[6];

out vec3 localPos;
out int faceIndex;

const vec3 faceNormals[6]={
    vec3(1,0,0),
    vec3(-1,0,0),
    vec3(0,1,0),
    vec3(0,-1,0),
    vec3(0,0,1),
    vec3(0,0,-1),
};

void main(){
    localPos=aPos;
    
    int i=0;
    while(i<6){
        if(dot(faceNormals[i],normalize(aNormal))>.99){
            break;
        }
        i++;
    }
    
    gl_Position=projection*view[i]*vec4(aPos,1);
    faceIndex=i;
}
