#version 460
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"
layout(early_fragment_tests) in;
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) flat in vec4 material;
layout(location=0) out vec4 color;
void main() { color=display(shade(position,normalize(normal),material,rayDirection(gl_FragCoord.xy))); }
