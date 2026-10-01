// Closed light-region membership must not turn a reconstructed boundary hit
// into an outside receiver solely through origin + direction*t rounding.
// For finite inputs, E <= gamma(2) * (|origin| + |direction*t|).
// Eliminating the unavailable origin with the triangle inequality gives
// E <= gamma(2)/(1-gamma(2)) * (|computedPosition| + 2*|direction*t|).
// Accumulated hit distance is a conservative upper bound on the local segment.
// This bounds reconstruction arithmetic, NOT undocumented hardware hit-t error.
// Round each nonnegative bound operation outward rather than adding a spatial
// epsilon or changing the light/shadow ray, geometry or image-test tolerance.
float lightRegionRoundUp(float value)
{
    return uintBitsToFloat(floatBitsToUint(value) + 1u);
}

float lightRegionCoordinateError(float coordinate, float hitDistance,
                                 float directionComponent)
{
    const float unitRoundoff = 5.9604644775390625e-8f;
    // gamma(2)/(1-gamma(2)) simplifies to 2*u/(1-4*u).
    const float coefficient = (2.0f * unitRoundoff) / (1.0f - 4.0f * unitRoundoff);
    float travel = lightRegionRoundUp(abs(hitDistance * directionComponent));
    float magnitude = lightRegionRoundUp(abs(coordinate) + 2.0f * travel);
    return lightRegionRoundUp(lightRegionRoundUp(coefficient) * magnitude);
}

bool lightRegionContainsClosedCoordinate(float coordinate, float hitDistance,
                                         float directionComponent,
                                         float minimum, float maximum)
{
    float error = lightRegionCoordinateError(coordinate, hitDistance, directionComponent);
    // Comparing the gap avoids rounding the authored region endpoints outward.
    return (coordinate >= minimum || minimum - coordinate <= error) &&
           (coordinate <= maximum || coordinate - maximum <= error);
}
