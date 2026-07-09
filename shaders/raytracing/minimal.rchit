#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec4 payload;

void main()
{
    payload = vec4(0.02, 0.06, 0.12, 1.0);
}
