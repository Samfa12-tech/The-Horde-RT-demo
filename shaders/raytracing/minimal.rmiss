#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec4 payload;

void main()
{
    const vec3 d = normalize(gl_WorldRayDirectionEXT);
    const float moon = pow(max(dot(d, normalize(vec3(-0.28, 0.5, -0.82))), 0.0), 64.0);
    const float horizon = smoothstep(-0.22, 0.45, d.y);
    vec3 sky = mix(vec3(0.015, 0.018, 0.024), vec3(0.04, 0.065, 0.105), horizon);
    sky += vec3(0.25, 0.33, 0.48) * moon;
    sky += vec3(0.09, 0.035, 0.012) * smoothstep(-0.35, 0.15, -d.y);
    if (payload.w < -1.5)
    {
        payload = vec4(sky * 0.75 + vec3(0.02, 0.024, 0.03), 1.0);
        return;
    }
    if (payload.w < -0.5)
    {
        payload = vec4(1.0, 1.0, 1.0, 0.0);
        return;
    }
    payload = vec4(sky, 1.0);
}
