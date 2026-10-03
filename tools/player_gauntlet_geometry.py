"""Pure offline mesh chirality rules shared by the Blender asset processor/tests."""

import math


def authored_face_and_uvs(face, uvs, *, mirror, legacy_uv_order=False):
    """Reverse winding AND its corner attributes when reflecting a hand mesh.

    legacy_uv_order only reproduces the historical, unaccepted mirrored-hand
    export. New anatomical candidates must never opt into that compatibility.
    """
    if len(face) != len(uvs):
        raise ValueError('Gauntlet face and UV corner counts disagree')
    if not mirror:
        return tuple(face), tuple(uvs)
    return tuple(reversed(face)), tuple(uvs if legacy_uv_order else reversed(uvs))


def mirror_for_hand(source_hand, target_hand):
    if source_hand not in ('Left', 'Right') or target_hand not in ('Left', 'Right'):
        raise ValueError('Gauntlet handedness must be explicit Left or Right')
    return source_hand != target_hand


def fit_cuff_to_forearm(points, wrist, elbow, cuff_axes, *, protected_indices=()):
    """Fit only the proximal cuff, never the grip-bearing hand, in bind space.

    The terminal tenth of the authored cuff defines its section centre. Move
    that centre onto the actual forearm axis, retaining its axial position and
    section shape. A smooth wrist-to-cuff field supplies the local displacement
    and ForeArm share; distal points remain exactly Hand-rigid. No pose/camera
    input, roll, topology, UV or gameplay target participates in this fit.
    """
    dot = lambda a, b: sum(x * y for x, y in zip(a, b))
    sub = lambda a, b: tuple(x - y for x, y in zip(a, b))
    vectors = (*points, wrist, elbow, *cuff_axes)
    if any(len(p) != 3 for p in vectors):
        raise ValueError('Cuff fit requires three-dimensional points and axes')
    values = [v for p in vectors for v in p]
    if not points or len(cuff_axes) != 3 or not all(math.isfinite(v) for v in values):
        raise ValueError('Cuff fit requires finite points and an orthonormal frame')
    if any(abs(dot(a, b) - (1.0 if i == j else 0.0)) > 1e-5
           for i, a in enumerate(cuff_axes) for j, b in enumerate(cuff_axes)):
        raise ValueError('Cuff fit frame must be orthonormal')
    lower = sub(elbow, wrist)
    length = math.sqrt(dot(lower, lower))
    if length < 0.01:
        raise ValueError('Cuff fit needs a nondegenerate forearm')
    forearm = tuple(v / length for v in lower)
    if dot(cuff_axes[0], forearm) < 0.5:
        raise ValueError('Authored cuff axis must point toward the elbow')
    local = [tuple(dot(sub(p, wrist), a) for a in cuff_axes) for p in points]
    low, high = min(p[0] for p in local), max(p[0] for p in local)
    terminal = [p for p in local if p[0] >= high - (high - low) * 0.1]
    centre_local = tuple((min(p[i] for p in terminal) + max(p[i] for p in terminal)) * 0.5
                         for i in range(3))
    if centre_local[0] <= 0.01:
        raise ValueError('Terminal cuff must be proximal to the wrist')
    centre = tuple(wrist[i] + sum(centre_local[j] * cuff_axes[j][i] for j in range(3))
                   for i in range(3))
    axial = dot(sub(centre, wrist), forearm)
    target = tuple(wrist[i] + axial * forearm[i] for i in range(3))
    delta = sub(target, centre)
    displacement = math.sqrt(dot(delta, delta))
    if displacement > 0.08:
        raise ValueError('Cuff mismatch exceeds this bounded local fit')
    fitted, shares = [], []
    for point, coordinate in zip(points, local):
        t = max(0.0, min(1.0, coordinate[0] / centre_local[0]))
        share = t * t * (3.0 - 2.0 * t)
        shares.append(share)
        fitted.append(tuple(point[i] + share * delta[i] for i in range(3))
                      if share else tuple(point))
    for index in protected_indices:
        if not isinstance(index, int) or not 0 <= index < len(points):
            raise ValueError('Protected grip vertex is outside the source roster')
        if shares[index] != 0.0 or fitted[index] != tuple(points[index]):
            raise ValueError('Cuff fit reached protected palm/finger grip contact')
    return fitted, shares, {
        'terminalCentreMetres': centre,
        'forearmAxisTargetMetres': target,
        'radialCorrectionMetres': displacement,
        'wristToCuffBlendMetres': centre_local[0],
        'proximalVertices': sum(s > 0.0 for s in shares),
        'distalHandVerticesPreserved': sum(s == 0.0 for s in shares),
        'construction': 'Bind-space proximal cuff fit and Hand/ForeArm articulation; no topology or grip change',
    }
