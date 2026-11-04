#version 460
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(location=0) out vec3 position;
layout(location=1) out vec3 normal;
layout(location=2) flat out vec4 material;
void main() {
    Vertex vert=mesh.v[gl_VertexIndex];
    position=vert.p.xyz;normal=vert.n.xyz;material=vert.material;
    vec3 delta=position-pc.camera.xyz;
    float z=dot(delta,pc.forward.xyz);
    float nearPlane=0.025,farPlane=50.0;
    gl_Position=vec4(dot(delta,pc.right.xyz)/(0.72*pc.config.x/pc.config.y),
        -dot(delta,pc.up.xyz)/0.72,farPlane/(farPlane-nearPlane)*z-farPlane*nearPlane/(farPlane-nearPlane),z);
}
