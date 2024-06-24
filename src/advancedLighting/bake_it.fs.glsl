#version 460 core
#define MAX_LIGHTS_NUM 16

//input
in VS_OUT{
    vec3 global_position;
    vec3 global_normal;
    vec2 texcoord;
}fs_in;

// output
out vec4 frag_color;

//uniform
struct Light{
    int lightType;
    vec3 color;
    float intensity;
    vec3 position,rotation;
    float innerCutOff,outerCutOff;// for spot light
};

layout(std140,binding=0)uniform lightGroup{
    int numLights;
    Light lights[MAX_LIGHTS_NUM];
};
uniform mat4 model;

void main(){
    // 计算我们想烘焙的属性
    
    // 距离片元最近的灯光的编号
    // 0号灯的位置是 vec3(1, 1.5, 1)
    // 2号灯的位置是 vec3(0, 1.5, 0)
    // 如果fragment距离0号灯近，为红色vec3(1,0,0)
    // 如果fragment距离2号灯近，为绿色vec3(0,1,0)
    
    float distance_to_0=distance(fs_in.global_position,lights[0].position);
    float distance_to_2=distance(fs_in.global_position,lights[2].position);
    vec3 direction_to_0=normalize(lights[0].position-fs_in.global_position);
    vec3 direction_to_2=normalize(lights[2].position-fs_in.global_position);
    float angle_to_0=dot(fs_in.global_normal,direction_to_0);
    float angle_to_2=dot(fs_in.global_normal,direction_to_2);
    frag_color=(distance_to_0<distance_to_2)?vec4(1,0,0,1):vec4(0,1,0,1);
    // frag_color=vec4(mat3(model)*vec3(1,0,0),1);
}