// Investigation-only exact source-plane qualification, NOT ray intersection.
// Mirrors DielectricContactGeometry.h. No float64/int64, Fma guarantee,
// rounded distance tolerance, or synthetic replacement for a native candidate.
struct ContactReceiver
{
    bool valid;
    int primitive;
    uint coordinateBits;
    uint flags;
};

ContactReceiver noContactReceiver()
{
    return ContactReceiver(false, -1, 0u, 0u);
}

bool contactEligibleOperand(float value)
{
    uint magnitude = floatBitsToUint(value) & 0x7fffffffu;
    return magnitude == 0u || (magnitude >= 0x35800000u && magnitude <= 0x49800000u);
}

int contactOrdering(float a, float b)
{
    return int(a > b) - int(a < b);
}

int contactAxisWinding(vec3 a, vec3 b, vec3 c, uint axis)
{
    uint u = (axis + 1u) % 3u;
    uint v = (axis + 2u) % 3u;
    for (int edge = 0; edge < 3; ++edge)
    {
        if (b[u] == a[u])
            return -contactOrdering(b[v], a[v]) * contactOrdering(c[u], a[u]);
        if (b[v] == a[v])
            return contactOrdering(b[u], a[u]) * contactOrdering(c[v], a[v]);
        vec3 oldA = a; a = b; b = c; c = oldA;
    }
    return 0;
}

bool contactExactAffine(float scale, float local, float translation, float plane)
{
    if (scale == 0.0 || !contactEligibleOperand(scale) || !contactEligibleOperand(local) ||
        !contactEligibleOperand(translation) || !contactEligibleOperand(plane)) return false;
    uint scaleBits = floatBitsToUint(scale);
    uint localBits = floatBitsToUint(local);
    uint planeBits = floatBitsToUint(plane);
    uint translationBits = floatBitsToUint(translation);
    if ((localBits & 0x7fffffffu) == 0u) return translation == plane;
    const uint fractionMask = 0x007fffffu;
    if ((scaleBits & fractionMask) != 0u && (localBits & fractionMask) != 0u) return false;
    int productExponent = int((scaleBits >> 23u) & 255u) +
        int((localBits >> 23u) & 255u) - 127;
    if (productExponent < 107 || productExponent > 147) return false;
    uint productSign = (scaleBits ^ localBits) & 0x80000000u;
    uint productMantissa = 0x00800000u | ((scaleBits | localBits) & fractionMask);
    if ((planeBits & 0x7fffffffu) == 0u ||
        (planeBits & 0x80000000u) != productSign) return false;
    int planeExponent = int((planeBits >> 23u) & 255u);
    int commonExponent = min(planeExponent, productExponent);
    if (abs(planeExponent - productExponent) > 2) return false;
    uint planeAligned = (0x00800000u | (planeBits & fractionMask)) <<
        uint(planeExponent - commonExponent);
    uint productAligned = productMantissa << uint(productExponent - commonExponent);
    bool negativeDifference = planeAligned < productAligned;
    uint difference = negativeDifference ? productAligned - planeAligned :
        planeAligned - productAligned;
    if (difference == 0u) return (translationBits & 0x7fffffffu) == 0u;
    int highestBit = findMSB(difference);
    if (highestBit > 23)
    {
        uint shift = uint(highestBit - 23);
        if ((difference & ((1u << shift) - 1u)) != 0u) return false;
        difference >>= shift;
    }
    else difference <<= uint(23 - highestBit);
    int differenceExponent = commonExponent - 23 + highestBit;
    if (differenceExponent <= 0 || differenceExponent >= 255) return false;
    uint differenceBits = (productSign ^ (negativeDifference ? 0x80000000u : 0u)) |
        (uint(differenceExponent) << 23u) | (difference & fractionMask);
    return translationBits == differenceBits;
}

bool contactInteriorBary(vec2 bary)
{
    // This is a native-candidate admission check, not an error-bound proof for
    // hardware barycentrics. Exact edges, nonfinite/out-of-range values reject.
    return !any(isnan(bary)) && !any(isinf(bary)) &&
        bary.x > 0.0 && bary.y > 0.0 && bary.x + bary.y < 1.0;
}

bool contactIdentityTransform(mat4x3 transform)
{
    return transform[0] == vec3(1.0, 0.0, 0.0) &&
        transform[1] == vec3(0.0, 1.0, 0.0) &&
        transform[2] == vec3(0.0, 0.0, 1.0) && transform[3] == vec3(0.0);
}

bool contactOpposedPlane(vec3 a, vec3 b, vec3 c, mat4x3 transform,
                         ContactReceiver receiver)
{
    uint axisCode = receiver.flags & kRtWorldSurfaceContactAxisMask;
    if (!receiver.valid || axisCode == 0u) return false;
    uint axis = axisCode - 1u;
    uvec3 localAxes = uvec3(3u);
    int determinantSign = 1;
    // Signed axis permutations/scales only. A qualifying plane row alone does
    // not exclude shear in other rows or establish the transformed winding.
    for (uint row = 0u; row < 3u; ++row)
    {
        if (!contactEligibleOperand(transform[3][row])) return false;
        for (uint column = 0u; column < 3u; ++column)
        {
            float value = transform[column][row];
            if (!contactEligibleOperand(value)) return false;
            if (value == 0.0) continue;
            if (localAxes[row] != 3u) return false;
            localAxes[row] = column;
            determinantSign *= value < 0.0 ? -1 : 1;
        }
        if (localAxes[row] == 3u) return false;
    }
    for (uint row = 0u; row < 3u; ++row)
        for (uint other = row + 1u; other < 3u; ++other)
        {
            if (localAxes[row] == localAxes[other]) return false;
            if (localAxes[row] > localAxes[other]) determinantSign = -determinantSign;
        }
    uint localAxis = localAxes[axis];
    for (uint component = 0u; component < 3u; ++component)
        if (!contactEligibleOperand(a[component]) || !contactEligibleOperand(b[component]) ||
            !contactEligibleOperand(c[component])) return false;
    if (a[localAxis] != b[localAxis] || a[localAxis] != c[localAxis]) return false;
    int winding = contactAxisWinding(a, b, c, localAxis);
    int scaleSign = transform[localAxis][axis] < 0.0 ? -1 : 1;
    int receiverSign = (receiver.flags & kRtWorldSurfaceContactNegativeWinding) != 0u ? -1 : 1;
    if (winding * determinantSign * scaleSign != -receiverSign) return false;
    return contactExactAffine(transform[localAxis][axis], a[localAxis], transform[3][axis],
                              uintBitsToFloat(receiver.coordinateBits));
}

bool contactOutgoingIntoReceiver(ContactReceiver receiver, vec3 direction)
{
    uint axisCode = receiver.flags & kRtWorldSurfaceContactAxisMask;
    if (!receiver.valid || axisCode == 0u) return false;
    float sign = (receiver.flags & kRtWorldSurfaceContactNegativeWinding) != 0u ? -1.0 : 1.0;
    return direction[axisCode - 1u] * sign < 0.0;
}

bool contactFallbackWorldWins(bool selectedHit, bool selectedIsDeferredExit,
                             float selectedT, float worldT)
{
    // Preserve a third confirmed blocker on a tie. Opaque tie precedence is
    // ONLY between the two deferred members of a rejected contact pair.
    return !selectedHit || worldT < selectedT ||
        (selectedIsDeferredExit && worldT == selectedT);
}
