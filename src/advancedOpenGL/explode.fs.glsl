#version 460 core

in GS_OUT{
    vec3 globalPos;
    vec3 globalNormal;
    vec2 TexCoords;
}fs_in;

out vec4 FragColor;

void main(){
    vec3 sunlightDir=normalize(vec3(1.f,1.f,1.f));
    float diffusionFac=max(dot(sunlightDir,normalize(fs_in.globalNormal)),0.f);
    vec3 amibient=vec3(.1f,.2f,.5f)*(1.f-diffusionFac);
    
    vec3 finalCol=(vec3(diffusionFac)+amibient)*vec3(1.f,.5f,.31f);
    FragColor=vec4(finalCol,1.f);
    
    // if(gl_FrontFacing){
        //     FragColor=vec4(0,0,1,1);
    // }else{
        //     FragColor=vec4(1,0,0,1);
    // }
    
    //Debug程序
    // FragColor=vec4(vec3(gl_FragCoord.z),1);
    // if(gl_FragCoord.z<0){
    //     FragColor=vec4(1,0,0,1);
    // }
}