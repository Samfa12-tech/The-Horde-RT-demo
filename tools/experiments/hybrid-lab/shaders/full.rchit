#version 460
#extension GL_EXT_ray_tracing : require
layout(location=0) rayPayloadInEXT vec4 primary;
hitAttributeEXT vec2 bary;
void main(){primary=vec4(float(gl_PrimitiveID),bary,gl_HitTEXT);}
