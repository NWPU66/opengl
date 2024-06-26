#version 460 core

//input
layout(location=0)in vec3 position;
layout(location=1)in vec3 normal;
layout(location=2)in vec2 texCoords;
layout(location=3)in vec3 tangent;
layout(location=4)in vec3 bitangent;

//output
out VS_OUT{
    vec3 global_position;
    vec3 tangent_position;
    vec2 texCoords;
    vec3 tangent_dirToCamera;
    
    vec3 global_normal;
    vec3 global_tangent;
    vec3 global_bitangent;
    mat3 TBN;
}vs_out;

//uniform
uniform mat4 model,view,projection;
uniform vec3 cameraPos;

void main(){
    vec4 global_position_v4=model*vec4(position,1);
    vs_out.global_position=vec3(global_position_v4);
    vs_out.texCoords=texCoords;
    
    vs_out.global_normal=normalize(mat3(transpose(inverse(model)))*normal);
    vs_out.global_tangent=normalize(mat3(transpose(inverse(model)))*tangent);
    vs_out.global_bitangent=-normalize(mat3(transpose(inverse(model)))*bitangent);
    vs_out.TBN=transpose(mat3(vs_out.global_tangent,vs_out.global_bitangent,vs_out.global_normal));
    
    vs_out.tangent_position=vs_out.TBN*vs_out.global_position;
    vs_out.tangent_dirToCamera=vs_out.TBN*(cameraPos-vs_out.global_position);
    
    gl_Position=projection*view*global_position_v4;
}
