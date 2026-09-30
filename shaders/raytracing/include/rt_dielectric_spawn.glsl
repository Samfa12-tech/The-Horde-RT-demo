// Numerical spawn guard, not an analytic replacement for RT intersections.
// Only loader-certified rectangular closed components may use this local
// correction. Arbitrary closed/concave volumes keep the ordinary failure path.
#if HORDE_GENERIC_TRANSMISSION_VARIANT
bool guardedRectangularDielectricSpawn(vec3 v0, vec3 v1, vec3 v2, vec2 bary,
    vec3 localPosition, vec3 surfacePosition, vec3 geometricNormal,
    mat4x3 objectToWorld, mat4x3 worldToObject, float minimumWidth, float geometryError,
    out vec3 spawnPosition, out float minimumNormalBias)
{
    spawnPosition = surfacePosition;
    minimumNormalBias = 0.0;
    // Certification is object-space only; reject non-finite runtime transforms
    // or derived normals instead of letting NaN ordered comparisons fail open.
    if (any(isnan(localPosition)) || any(isinf(localPosition)) ||
        any(isnan(surfacePosition)) || any(isinf(surfacePosition)) ||
        any(isnan(geometricNormal)) || any(isinf(geometricNormal))) return false;
    vec3 weights = vec3(1.0 - bary.x - bary.y, bary);
    if (any(lessThan(weights, vec3(0.0))) || any(isnan(weights)) ||
        minimumWidth <= 0.0 || isnan(minimumWidth) || isinf(minimumWidth) ||
        geometryError < 0.0 || isnan(geometryError) || isinf(geometryError)) return false;
    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    vec3 areaNormal = cross(e1, e2);
    float doubleArea = length(areaNormal);
    if (doubleArea <= 0.0 || isinf(doubleArea) || isnan(doubleArea)) return false;
    vec3 objectNormal = areaNormal / doubleArea;
    mat3 absoluteO2w = mat3(abs(objectToWorld[0]), abs(objectToWorld[1]), abs(objectToWorld[2]));
    mat3 absoluteW2o = mat3(abs(worldToObject[0]), abs(worldToObject[1]), abs(worldToObject[2]));
    const float unitRoundoff = 5.9604644775390625e-8;
    const float gamma5 = (5.0 * unitRoundoff) / (1.0 - 5.0 * unitRoundoff);
    // Affine dot3 + translation must be safe without assuming FMA contraction.
    const float gamma8 = (8.0 * unitRoundoff) / (1.0 - 8.0 * unitRoundoff);
    const float gamma16 = (16.0 * unitRoundoff) / (1.0 - 16.0 * unitRoundoff);
    const float gamma32 = (32.0 * unitRoundoff) / (1.0 - 32.0 * unitRoundoff);
    vec3 localError = gamma5 * (abs(v0) + abs(bary.x * e1) + abs(bary.y * e2));
    vec3 worldError = absoluteO2w * localError + gamma8 *
        (absoluteO2w * abs(localPosition) + abs(objectToWorld[3]));
    // Include rounded normal-offset addition and traversal's inverse transform.
    // Do not assume separately rounded transform matrices are exact inverses.
    vec3 inversePosition = mat3(worldToObject) * surfacePosition + worldToObject[3];
    vec3 objectError = localError + absoluteW2o * (worldError +
        unitRoundoff * (abs(surfacePosition) + vec3(0.00025))) + gamma8 *
        (absoluteW2o * abs(surfacePosition) + abs(worldToObject[3])) +
        abs(inversePosition - localPosition) + vec3(geometryError);
    // Inset endpoints and a clamped edge interpolation incur their own local
    // construction error. Any clamped interpolation is within the inset edge;
    // the closest-point dot/division only selects which interior point to use.
    objectError += gamma8 * (abs(v0) + abs(e1) + abs(e2));
    // Inflate positive bound arithmetic, then round each component upward by
    // one representable float. Do not let bound rounding shrink the safe inset.
    objectError = uintBitsToFloat(floatBitsToUint(objectError * (1.0 + gamma32)) + uvec3(1u));
    if (any(isnan(objectError)) || any(isinf(objectError))) return false;
    float lowerNormalBias = clamp(max(0.00002,
        max(max(abs(surfacePosition.x), abs(surfacePosition.y)), abs(surfacePosition.z)) * 0.000002),
        0.00002, 0.00025);
    vec3 objectOffsetDirection = mat3(worldToObject) * geometricNormal;
    if (any(isnan(objectOffsetDirection)) || any(isinf(objectOffsetDirection))) return false;
    // Absolute projection error covers the matrix-vector and dot operations;
    // multiplying a cancelled dot by a relative gamma is not conservative.
    float separationScaleLower = (abs(dot(objectNormal, objectOffsetDirection)) -
        gamma16 * dot(abs(objectNormal), absoluteW2o * abs(geometricNormal))) * (1.0 - gamma8);
    if (isnan(separationScaleLower) || isinf(separationScaleLower) || separationScaleLower <= 0.0) return false;
    float separationErrorUpper = dot(abs(objectNormal), objectError) * (1.0 + gamma8);
    // Derive a per-hit minimum only when the coordinate floor cannot separate
    // this face. Inflate division/multiplication and round upward by one ULP.
    if (separationScaleLower * lowerNormalBias <= separationErrorUpper)
    {
        float requiredBias = (separationErrorUpper / separationScaleLower) * (1.0 + gamma8);
        requiredBias = uintBitsToFloat(floatBitsToUint(requiredBias) + 1u);
        if (isnan(requiredBias) || isinf(requiredBias) || requiredBias > 0.00025) return false;
        lowerNormalBias = max(lowerNormalBias, requiredBias);
    }
    // Both source-face separation and the opposite-face clearance must fit.
    if (separationScaleLower * lowerNormalBias <= separationErrorUpper ||
        (length(objectOffsetDirection) * 0.00025 + 2.0 * length(objectError)) *
            (1.0 + gamma8) >= minimumWidth * 0.25)
        return false;
    vec3 oppositeEdges[3] = vec3[3](v2 - v1, -e2, e1);
    vec3 minimumWeights;
    for (int edge = 0; edge < 3; ++edge)
    {
        float edgeLength = length(oppositeEdges[edge]);
        if (edgeLength <= 0.0) return false;
        vec3 inward = cross(objectNormal, oppositeEdges[edge]) / edgeLength;
        float minimumWeight = dot(abs(inward), objectError) / (doubleArea / edgeLength);
        minimumWeights[edge] = uintBitsToFloat(floatBitsToUint(minimumWeight *
            (1.0 + gamma8)) + 1u);
    }
    float minimumSum = (minimumWeights.x + minimumWeights.y + minimumWeights.z) * (1.0 + gamma8);
    if (minimumSum >= 1.0) return false;
    vec3 guardedLocal = localPosition;
    if (any(lessThan(weights, minimumWeights)))
    {
        // Metric closest point, not barycentric renormalization/centroid mixing:
        // those can cause a large slide along narrow, long side-face triangles.
        vec3 insetBase = v0 + minimumWeights.y * e1 + minimumWeights.z * e2;
        vec3 insetVertices[3] = vec3[3](insetBase,
            insetBase + (1.0 - minimumSum) * e1,
            insetBase + (1.0 - minimumSum) * e2);
        float nearestSquared = 1e30;
        for (int edge = 0; edge < 3; ++edge)
        {
            vec3 start = insetVertices[edge];
            vec3 delta = insetVertices[(edge + 1) % 3] - start;
            float squaredLength = dot(delta, delta);
            if (squaredLength <= 0.0) return false;
            vec3 candidate = start + delta * clamp(dot(localPosition - start, delta) / squaredLength, 0.0, 1.0);
            float squared = dot(candidate - localPosition, candidate - localPosition);
            if (squared < nearestSquared) { nearestSquared = squared; guardedLocal = candidate; }
        }
        if (length(guardedLocal - localPosition) * (1.0 + gamma8) >
            2.0 * length(objectError) * (1.0 - gamma8)) return false;
    }
    precise vec3 guardedWorld = mat3(objectToWorld) * guardedLocal + objectToWorld[3];
    if (any(isnan(guardedWorld)) || any(isinf(guardedWorld))) return false;
    spawnPosition = guardedWorld;
    minimumNormalBias = lowerNormalBias;
    return true;
}
#endif
