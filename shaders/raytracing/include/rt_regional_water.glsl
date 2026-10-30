// Authored finite RT triangles, not an infinite displaced ocean. The immutable
// profile uses continuous downstream distance and bank coordinates, so phase
// travels through bends without resetting at a segment or triangle boundary.
int waterProfileIndex(uint code)
{
    uint index = code >> 16u;
    return all(bvec3((code & 0xffffu) == (10u | (7u << 8u)), index > 0u,
        index <= min(rtWaterContactRipple.surfaceMetadata.x, 42u)))
        ? int(index - 1u) : -1;
}

void waterProfileCoordinates(int index, vec3 p, out vec4 coordinates,
                              out vec3 alongGradient, out vec3 bankGradient)
{
    RtWaterSurfaceGpu profile = rtWaterContactRipple.surfaces[index];
    vec2 e1 = profile.bAlong.xz - profile.aAlong.xz;
    vec2 e2 = profile.cAlong.xz - profile.aAlong.xz;
    vec3 n = profile.normalKind.xyz;
    // All admitted surfaces are height-field triangles (normal.y > .65).
    // XZ barycentrics are exact on that plane and avoid a second 3D solve.
    float determinant = e1.x * e2.y - e1.y * e2.x;
    vec2 reciprocalB = vec2(e2.y, -e2.x) / determinant;
    vec2 reciprocalC = vec2(-e1.y, e1.x) / determinant;
    float b = dot(p.xz - profile.aAlong.xz, reciprocalB);
    float c = dot(p.xz - profile.aAlong.xz, reciprocalC);
    vec3 weight = vec3(1.0 - b - c, b, c);
    coordinates = profile.aFlow * weight.x + profile.bFlow * weight.y + profile.cFlow * weight.z;
    // x: continuous along distance; y: signed bank coordinate; z: speed;
    // w: standing/channel kind. Direction is encoded in the spatial gradient.
    coordinates = vec4(dot(vec3(profile.aAlong.w, profile.bAlong.w, profile.cAlong.w), weight),
                       coordinates.z, coordinates.w, profile.normalKind.w);
    vec2 along = reciprocalB * (profile.bAlong.w - profile.aAlong.w) +
                 reciprocalC * (profile.cAlong.w - profile.aAlong.w);
    vec2 bank = reciprocalB * (profile.bFlow.z - profile.aFlow.z) +
                reciprocalC * (profile.cFlow.z - profile.aFlow.z);
    alongGradient = vec3(along.x, 0.0, along.y);
    bankGradient = vec3(bank.x, 0.0, bank.y);
    alongGradient -= n * dot(n, alongGradient);
    bankGradient -= n * dot(n, bankGradient);
}
