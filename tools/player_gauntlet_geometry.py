"""Pure offline mesh chirality rules shared by the Blender asset processor/tests."""


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
