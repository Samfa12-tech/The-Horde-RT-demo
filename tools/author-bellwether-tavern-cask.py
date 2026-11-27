"""Build the south-tavern coopered ale delivery cask for Bellwether.

This is a small Blender authoring helper for the combined south-square scene;
it deliberately does not import or edit a town GLB.  Run it from a Blender
authoring session after the terrain has been loaded, then call
``build_cask(ground)`` and add the returned mesh objects to the caller's own
source blend/export group.

Survey (Horde engine X/Z metres, +Y up; the existing mesh helpers convert to
Blender's Z-up coordinates):

* Cask centre: (85.85, 124.15), barrel axis along +Z, body length 1.34 m,
  maximum body radius 0.42 m.  Its body footprint is approximately
  X 85.43..86.27, Z 123.48..124.82.
* The existing south-ramp approach witness is (82.30, 131.70).  The cask is
  3.55 m east of that route centreline; its nearest body edge is about 1.8 m
  east of the existing rail at X 83.61.
* The existing frontage firewood rack is centred at (86.85, 119.90), the
  canopy lamp at (84.34, 121.03), and the south door at (82.30, 119.20).
  The cask's nearest footprint is more than 3.5 m from the rack, about
  3.1 m from the lamp, and over 6 m from the door threshold/sweep.
* The east-side patch is outside the route envelope and leaves the player
  approach open.

The caller supplies ``ground(x, z)`` from the admitted terrain.  Skids are
set from sampled terrain and the barrel rests on their common top plane; no
terrain mesh, runtime GLB, source blend, inventory or road data is changed.
Only the already-admitted HordeTavernTimber and HordeHearthChar finishes are
used.
"""
import math
from pathlib import Path
import runpy

from mathutils import Vector

import bpy


HERE = Path(__file__).resolve()
town = runpy.run_path(str(HERE.with_name("author-bellwether-mill-works.py")))
geometry = runpy.run_path(str(HERE.with_name("author-bellwether-mill-crossing.py")))

# Human-readable survey constants kept here so the combined author can audit
# the placement without opening another source asset.
CASK_CENTER_XZ = (85.85, 124.15)
CASK_AXIS_LENGTH = 1.34
CASK_BODY_RADIUS = 0.42

BOX = [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1),
       (6, 7, 3, 2), (7, 4, 0, 3), (7, 6, 5, 4)]


def _mesh(name, points, faces, finish):
    """Use the admitted world mesh helper and return the created object."""
    return geometry["mesh"](name, points, faces, finish)


def _admitted_material(name, colour, roughness):
    """Reuse the combined scene's admitted finish when it already exists."""
    return bpy.data.materials.get(name) or town["material"](name, colour, roughness)


def _lathed_z(name, centre, profile, sides, finish, angle_offset=0.0):
    """Create a closed rotational profile around the engine Z axis."""
    cx, cy, cz = centre
    points = []
    for z_offset, radius in profile:
        for index in range(sides):
            angle = angle_offset + math.tau * index / sides
            points.append((cx + radius * math.cos(angle), cy + radius * math.sin(angle), cz + z_offset))
    faces = []
    for ring in range(len(profile) - 1):
        following = ring + 1
        for index in range(sides):
            a = ring * sides + index
            b = ring * sides + (index + 1) % sides
            faces.append((a, b, following * sides + (index + 1) % sides, following * sides + index))
    faces.append(tuple(reversed(range(sides))))
    last = (len(profile) - 1) * sides
    faces.append(tuple(last + index for index in range(sides)))
    return _mesh(name, points, faces, finish)


def _wedge(name, x0, x1, z0, z1, base, top, finish):
    """A small timber chock with a sloped upper face, in engine coordinates."""
    points = [(x0, base, z0), (x1, base, z0), (x1, base, z1), (x0, base, z1),
              (x0, top[0], z0), (x1, top[0], z0), (x1, top[1], z1), (x0, top[1], z1)]
    return _mesh(name, points, BOX, finish)


def _hoop(name, centre, z_offset, radius, finish):
    """Create one narrow wrought hoop around the coopered barrel."""
    cx, cy, cz = centre
    sides, section = 16, 6
    tube = 0.034
    points = []
    for ring in range(sides):
        angle = math.tau * ring / sides
        for cross in range(section):
            around = math.tau * cross / section
            radial = radius + tube * math.cos(around)
            points.append((cx + radial * math.cos(angle),
                           cy + radial * math.sin(angle),
                           cz + z_offset + tube * math.sin(around)))
    faces = []
    for ring in range(sides):
        following = (ring + 1) % sides
        for cross in range(section):
            next_cross = (cross + 1) % section
            a = ring * section + cross
            faces.append((a, following * section + cross,
                          following * section + next_cross, ring * section + next_cross))
    return _mesh(name, points, faces, finish)


def build_cask(ground):
    """Return mesh objects for a grounded, delivery-ready ale cask.

    ``ground(x, z)`` must return the current terrain height in engine-space
    metres.  The function samples the complete skid footprint before creating
    geometry so the timber support cannot silently float on an old grade.
    """
    oak = _admitted_material("HordeTavernTimber", (.25, .115, .065), .84)
    char = _admitted_material("HordeHearthChar", (.06, .05, .04), .80)
    parts = []
    cx, cz = CASK_CENTER_XZ
    half_length = CASK_AXIS_LENGTH / 2
    skid_x_offsets = (-.30, .30)
    skid_z = (cz - half_length - .035, cz + half_length + .035)

    # The support is level across the cask but is buried only at the sampled
    # terrain envelope.  This is the smallest stable adjustment for the
    # measured ~8 cm cross-slope at this patch.
    sample_points = [(cx + dx, z) for dx in skid_x_offsets for z in skid_z]
    support_ground = max(ground(x, z) for x, z in sample_points)
    skid_bottom = min(ground(x, z) for x, z in sample_points) - .035
    skid_top = support_ground + .16
    for index, dx in enumerate(skid_x_offsets, 1):
        parts.append(town["block"](f"Ale cask oak skid {index}",
                                    cx + dx - .085, cx + dx + .085,
                                    skid_z[0], skid_z[1], skid_bottom, skid_top,
                                    oak, .018))

    # A cask bears on the two skids at x +/- .30, not on its mathematical
    # centreline.  Derive the body centre from that contact geometry.
    contact_radius = math.sqrt(CASK_BODY_RADIUS * CASK_BODY_RADIUS - .30 * .30)
    body_centre_y = skid_top + contact_radius
    body_centre = (cx, body_centre_y, cz)

    # Twelve slightly separated stave wedges retain a readable coopered
    # silhouette without a texture or a high-cost subdivision surface.
    stave_count = 12
    gap = .018
    profile = ((-half_length, .315), (-half_length + .11, .365),
               (-half_length + .27, .42), (half_length - .27, .42),
               (half_length - .11, .365), (half_length, .315))
    for index in range(stave_count):
        start = math.tau * index / stave_count + gap / 2
        end = math.tau * (index + 1) / stave_count - gap / 2
        points = []
        for z_offset, radius in profile:
            points.extend(((cx + radius * math.cos(angle), body_centre_y + radius * math.sin(angle), cz + z_offset)
                           for angle in (start, end)))
        faces = []
        for ring in range(len(profile) - 1):
            a = ring * 2
            b = (ring + 1) * 2
            faces.append((a, a + 1, b + 1, b))
        # Close the stave against a flat inner chord. The separate end-grain
        # disks meet the narrow stave ends at the two heads of the cask.
        faces.extend(((0, 2, 4, 6, 8, 10), (11, 9, 7, 5, 3, 1), (0, 10, 11, 1)))
        parts.append(_mesh(f"Ale cask stave {index + 1:02d}", points, faces, oak))

    parts.append(_lathed_z("Ale cask south end grain", body_centre, ((-half_length, .315), (-half_length + .03, .315)), stave_count, oak))
    parts.append(_lathed_z("Ale cask north end grain", body_centre, ((half_length - .03, .315), (half_length, .315)), stave_count, oak))

    for index, z_offset in enumerate((-half_length + .17, 0.0, half_length - .17), 1):
        ring_radius = .392 if abs(z_offset) < .01 else .358
        parts.append(_hoop(f"Ale cask iron hoop {index}", body_centre, z_offset, ring_radius, char))

    # The south-facing bung is oriented toward the lower-Z approach.  A short
    # turned plug projects from the end grain without becoming a gameplay cue.
    bung_z = cz - half_length - .018
    parts.append(_lathed_z("Ale cask south bung", (cx, body_centre_y, bung_z),
                           ((0.0, .098), (.028, .105), (.065, .082)), 10, oak,
                           angle_offset=math.pi / 10))
    parts.append(_lathed_z("Ale cask bung dark seat", (cx, body_centre_y, bung_z - .006),
                           ((-.008, .112), (.008, .112)), 10, char,
                           angle_offset=math.pi / 10))

    # End chocks are low, sloped timber wedges at each side of the two skids;
    # they read as a delivery cask set down for unloading rather than a barrel
    # floating on a decorative platform.
    for end_index, z in enumerate((cz - half_length - .025, cz + half_length + .025), 1):
        for side_index, dx in enumerate(skid_x_offsets, 1):
            side = -1 if end_index == 1 else 1
            z0, z1 = sorted((z, z + side * .12))
            chock_base = skid_top - .005
            parts.append(_wedge(f"Ale cask chock {end_index}-{side_index}",
                                cx + dx - .11, cx + dx + .11, z0, z1,
                                chock_base, (chock_base + .18, chock_base + .05), oak))
    return parts
