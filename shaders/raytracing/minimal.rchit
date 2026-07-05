#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec4 payload;

void main()
{
    payload = vec4(0.2, 0.2, 0.6, 1.0);
}
