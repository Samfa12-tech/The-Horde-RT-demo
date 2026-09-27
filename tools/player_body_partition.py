"""Bounds for retaining torso cloth without exposing the existing head mask."""
import math


def upper_torso_partition_limits(minimum_height, maximum_height, head_origin_height):
    """Return (head_start, near_face_start) for the opt-in anatomical partition.

    The historical head region starts at 86% of the source height. Keep that
    entire region masked, splitting its lower hood/neck from its upper head at
    the authored Head origin. Only the old 79-86% torso band becomes primary.
    This is not a skin-weight threshold or a camera-dependent surface cut.
    """
    values = (minimum_height, maximum_height, head_origin_height)
    if not all(math.isfinite(value) for value in values):
        raise ValueError('Body partition landmarks must be finite')
    if maximum_height <= minimum_height:
        raise ValueError('Body partition requires positive height')
    original_head_start = minimum_height + (maximum_height - minimum_height) * .86
    if not original_head_start < head_origin_height < maximum_height:
        raise ValueError('Authored Head origin must lie inside the retained head region')
    return head_origin_height, original_head_start
