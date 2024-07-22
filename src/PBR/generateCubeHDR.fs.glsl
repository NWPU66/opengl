#version 460 core
out vec4 FragColor;

out vec3 globalPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan=vec2(.1591,.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv=vec2(atan(v.z,v.x),asin(v.y));
    uv*=invAtan;
    uv+=.5;
    return uv;
}

void main()
{
    vec2 uv=SampleSphericalMap(normalize(globalPos));// make sure to normalize localPos
    vec3 color=texture(equirectangularMap,uv).rgb;
    FragColor=vec4(color,1.);
    FragColor=vec4(globalPos,1);
}