"""Author the carved fingerpost at Bellwether's south square decision point.

Run in Blender 5.2 from the repository root with::

  blender -b --factory-startup --python-exit-code 1 \
    --python tools/author-bellwether-square-waymarker.py -- ROOT PARENT_TOWN_GLB NEW_OUTPUT

The patch uses only the four admitted Bellwether material rows.  It is a
solid, non-walkable layer; route ribbons remain owned by the existing town
path layer.  The output town GLB is copied into the admitted runtime directory
by the integration step after the receipt and geometry checks pass.
"""
import json
import math
from pathlib import Path
import runpy
import sys

import bmesh
import bpy
from mathutils import Vector

HERE = Path(__file__).resolve()
town = runpy.run_path(str(HERE.with_name("author-bellwether-mill-works.py")))
geometry = runpy.run_path(str(HERE.with_name("author-bellwether-mill-crossing.py")))

PARENT = "d44bdbac5b5cea8c6807b4f86dfc28f920e44dcf73f74bb139eecc9a2bfba18e"
SOURCE = Path("assets/models/world/source/bellwether-square-waymarker-v1")
LAYOUT = SOURCE / "layout.json"
BOX = [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1), (6, 7, 3, 2),
       (7, 4, 0, 3), (7, 6, 5, 4)]


def beam(parts, name, start, end, width, depth, finish):
    """Add a cuboid beam between engine-coordinate points."""
    start, end = Vector(start), Vector(end)
    direction = (end - start).normalized()
    guide = Vector((0, 1, 0)) if abs(direction.y) < .9 else Vector((0, 0, 1))
    across = direction.cross(guide).normalized()
    up = across.cross(direction).normalized()
    corners = [across * (a * width / 2) + up * (b * depth / 2)
               for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    parts.append(geometry["mesh"](name,
        [tuple(point + offset) for point in (start, end) for offset in corners], BOX, finish))


def prism(parts, name, outline, thickness, finish):
    """Extrude an engine X/Y/Z polygon along its supplied normal."""
    points = [tuple(point) for point in outline]
    normal = (Vector(points[1]) - Vector(points[0])).cross(Vector(points[2]) - Vector(points[0])).normalized()
    offset = normal * (thickness / 2)
    front = [tuple(Vector(point) + offset) for point in points]
    back = [tuple(Vector(point) - offset) for point in points]
    faces = [tuple(reversed(range(len(points)))), tuple(range(len(points), 2 * len(points)))]
    count = len(points)
    faces.extend((i, (i + 1) % count, (i + 1) % count + count, i + count)
                 for i in range(count))
    parts.append(geometry["mesh"](name, front + back, faces, finish))


def face_box(parts, name, centre, along, length, height, depth, normal, finish):
    """A small box flush with a board face; along and normal are horizontal."""
    along = Vector(along).normalized()
    normal = Vector(normal).normalized()
    up = Vector((0., 1., 0.))
    half_a, half_h, half_n = along * length / 2, up * height / 2, normal * depth / 2
    ring = [(-half_a, -half_h), (half_a, -half_h), (half_a, half_h), (-half_a, half_h)]
    points = [tuple(Vector(centre) + a + h + n) for n in (-half_n, half_n)
              for a, h in ring]
    parts.append(geometry["mesh"](name, points, BOX, finish))


def face_beam(parts, name, start, end, width, depth, normal, finish):
    start, end = Vector(start), Vector(end)
    direction = (end - start).normalized()
    normal = Vector(normal).normalized()
    across = normal.cross(direction).normalized()
    half_w, half_d = width / 2, depth / 2
    corners = [normal * (-half_d) + across * (-half_w),
               normal * (-half_d) + across * half_w,
               normal * half_d + across * half_w,
               normal * half_d + across * (-half_w)]
    points = [tuple(point + offset) for point in (start, end) for offset in corners]
    parts.append(geometry["mesh"](name, points, BOX, finish))


def turned(parts, name, centre, profile, sides, finish):
    """Revolve a small cup or wick around the engine vertical axis."""
    x, y, z = centre
    points = []
    for height, radius in profile:
        for index in range(sides):
            angle = math.tau * index / sides
            points.append((x + radius * math.cos(angle), y + height, z + radius * math.sin(angle)))
    faces = []
    for level in range(len(profile) - 1):
        following = level + 1
        for index in range(sides):
            a, b = level * sides + index, level * sides + (index + 1) % sides
            faces.append((a, following * sides + index, following * sides + (index + 1) % sides, b))
    faces.extend((tuple(range(sides)), tuple(reversed(range(len(points) - sides, len(points))))))
    parts.append(geometry["mesh"](name, points, faces, finish))


def arrow_board(parts, name, post, direction, height, length, finish, stone, emblem, iron):
    """One tapered fingerboard, bolted across the post, with flush reliefs."""
    direction = Vector((direction[0], 0., direction[1])).normalized()
    side = Vector((-direction.z, 0., direction.x))
    centre = Vector((post[0], post[1] + height, post[2]))
    half_h = .20
    outline = [tuple(centre + Vector((0, -half_h, 0))),
               tuple(centre + Vector((0, half_h, 0))),
               tuple(centre + direction * (length - .31) + Vector((0, half_h, 0))),
               tuple(centre + direction * length),
               tuple(centre + direction * (length - .31) + Vector((0, -half_h, 0)))]
    prism(parts, name, outline, .12, finish)
    bolt = centre + direction * .11
    face_beam(parts, f"{name} iron through-bolt", bolt - side * .075, bolt + side * .075,
              .07, .08, direction, iron)
    # Relief is duplicated on both faces and overlaps the board skin by 6 mm.
    glyph_centre = centre + direction * ((length - .31) * .55)
    for sign in (-1., 1.):
        emblem(parts, glyph_centre + side * sign * .073, direction, side * sign, stone)


def tankard(parts, centre, direction, side, stone):
    # Tankard body, rim and a squared handle read as a cup in silhouette.
    face_box(parts, "Tankard inlay body", centre + Vector((0, -.03, 0)), direction,
             .24, .26, .045, side, stone)
    face_box(parts, "Tankard inlay rim", centre + Vector((0, .14, 0)), direction,
             .32, .055, .045, side, stone)
    # Three short rods leave the tankard handle open rather than filling it
    # with a solid plaque.
    face_beam(parts, "Tankard inlay handle upper", centre + direction*.12 + Vector((0, .08, 0)),
              centre + direction*.28 + Vector((0, .08, 0)), .055, .045, side, stone)
    face_beam(parts, "Tankard inlay handle outer", centre + direction*.28 + Vector((0, .08, 0)),
              centre + direction*.28 + Vector((0, -.10, 0)), .055, .045, side, stone)
    face_beam(parts, "Tankard inlay handle lower", centre + direction*.28 + Vector((0, -.10, 0)),
              centre + direction*.12 + Vector((0, -.10, 0)), .055, .045, side, stone)


def wheel(parts, centre, direction, side, stone):
    # An octagonal pale wheel with a dark void between the spokes is legible
    # without depending on a texture or a new light emitter.
    radius = .19
    ring = []
    for i in range(8):
        angle = math.tau * i / 8 + math.pi / 8
        ring.append(centre + direction * (radius * math.cos(angle)) +
                    Vector((0, radius * math.sin(angle), 0)))
    # An annulus keeps the board visible through the wheel; two full crossing
    # diameters make the destination emblem read at eye height.
    for index, start in enumerate(ring):
        face_beam(parts, f"Mill wheel inlay rim {index + 1}", start, ring[(index + 1) % len(ring)],
                  .045, .045, side, stone)
    face_beam(parts, "Mill wheel inlay horizontal diameter", centre - direction*.14,
              centre + direction*.14, .045, .045, side, stone)
    face_beam(parts, "Mill wheel inlay vertical diameter", centre + Vector((0, -.14, 0)),
              centre + Vector((0, .14, 0)), .045, .045, side, stone)


def scales(parts, centre, direction, side, stone):
    face_box(parts, "Market scales inlay upright", centre + Vector((0, -.01, 0)), direction,
             .065, .38, .045, side, stone)
    face_box(parts, "Market scales inlay beam", centre + Vector((0, .16, 0)), direction,
             .40, .055, .045, side, stone)
    for sign in (-1, 1):
        bowl = centre + direction * sign * .20 + Vector((0, .02, 0))
        face_box(parts, "Market scales inlay pan", bowl, direction, .15, .065, .045, side, stone)


def anvil(parts, centre, direction, side, stone):
    face_box(parts, "Smith anvil inlay body", centre + Vector((0, -.04, 0)), direction,
             .31, .16, .045, side, stone)
    face_box(parts, "Smith anvil inlay horn", centre + direction*.22 + Vector((0, .07, 0)), direction,
             .19, .10, .045, side, stone)
    face_box(parts, "Smith anvil inlay foot", centre + Vector((0, -.17, 0)), direction,
             .22, .08, .045, side, stone)


def build_marker(ground, layout):
    oak = town["material"]("HordeTavernTimber", (.25, .115, .065), .84)
    stone = town["material"]("HordeTownStone", (.43, .39, .30), .82)
    iron = town["material"]("HordeHearthChar", (.06, .05, .04), .80)
    clay = town["material"]("HordeTownRoofClay", (.28, .12, .06), .84)
    parts = []
    x, z = layout["postXZ"]
    base = ground(x, z)
    post_top = base + max(item["boardHeight"] + .25 for item in layout["destinations"])
    parts.append(town["block"]("Carved fingerpost", x-.115, x+.115, z-.115, z+.115,
                                base-.055, post_top, oak, .025))
    parts.append(town["block"]("Fingerpost socket", x-.22, x+.22, z-.22, z+.22,
                                base-.07, base+.16, stone, .035))

    # Small wrought collar and cap make the post feel assembled rather than
    # a floating sign bundle. The cap is stone so the marker catches sky light.
    parts.append(town["block"]("Fingerpost iron collar", x-.16, x+.16, z-.16, z+.16,
                                base+1.31, base+1.43, iron, .018))
    parts.append(town["block"]("Carved stone cap", x-.20, x+.20, z-.20, z+.20,
                                post_top-.09, post_top+.10, stone, .03))

    emblems = {"tavern": tankard, "dealer": scales, "mill": wheel, "smith": anvil}
    marks = {}
    approach = Vector((layout["cameraWitness"]["viewXZ"][0] - x, 0.,
                       layout["cameraWitness"]["viewXZ"][1] - z)).normalized()
    lamp_parts = []
    lamp_base = Vector((x, base + 2.84, z))
    lamp_end = lamp_base + approach * .72
    beam(lamp_parts, "Waymarker iron lamp arm", lamp_base, lamp_end, .075, .075, iron)
    beam(lamp_parts, "Waymarker iron lamp brace", Vector((x, base + 2.30, z)),
         lamp_base + approach * .30, .055, .055, iron)
    cup = tuple(lamp_end + Vector((0., -.18, 0.)))
    turned(lamp_parts, "Waymarker clay oil cup", cup,
           ((0., .07), (.06, .115), (.10, .115), (.10, .09), (.06, .07)), 8, clay)
    turned(lamp_parts, "Waymarker short wick", tuple(lamp_end + Vector((0., -.10, 0.))),
           ((0., .018), (.11, .014)), 6, iron)
    flame = tuple(lamp_end + Vector((0., .04, 0.)))
    light = tuple(lamp_end + Vector((0., .18, 0.)))
    parts.extend(lamp_parts)
    for index, destination in enumerate(layout["destinations"]):
        arrow_board(parts, f"{destination['label']} destination arrow", (x, base, z),
                    destination["direction"], destination["boardHeight"],
                    destination["shaftLength"], oak, stone,
                    emblems[destination["emblem"]], iron)
        marks[destination["label"]] = [x, base + destination["boardHeight"], z]
    return parts, base, marks, flame, light, triangle_count(lamp_parts)


def normalize_cask_materials(parts):
    """Route the paired helper's fresh material datablocks to admitted rows."""
    admitted = {}
    for material in bpy.data.materials:
        root_name = material.name.split(".", 1)[0]
        if root_name in {"HordeTavernTimber", "HordeHearthChar"} and root_name not in admitted:
            admitted[root_name] = material
    if set(admitted) != {"HordeTavernTimber", "HordeHearthChar"}:
        raise ValueError("The cask helper did not produce the two admitted material rows")
    for part in parts:
        for index, material in enumerate(part.data.materials):
            root_name = material.name.split(".", 1)[0]
            if root_name not in admitted:
                raise ValueError(f"Cask requested an unadmitted material: {material.name}")
            part.data.materials[index] = admitted[root_name]


def triangle_count(parts):
    total = 0
    for part in parts:
        part.data.calc_loop_triangles()
        total += len(part.data.loop_triangles)
    return total


def main(root, previous, output):
    terrain = root / town["TERRAIN"]
    layout = json.loads((root / LAYOUT).read_bytes())
    digest = town["sha"]
    if output.exists() or digest(previous) != PARENT or digest(terrain) != town["TERRAIN_HASH"]:
        raise ValueError("Use the admitted hearth-service town, pinned terrain and a fresh output directory.")

    # Route clearance is checked before geometry is built. This protects the
    # actual authored lane mouths rather than a visual approximation.
    post_x, post_z = layout["postXZ"]
    for destination in layout["destinations"]:
        a = Vector(destination["routeMouth"])
        d = Vector(destination["direction"]).normalized()
        # The post is measured against each route's local mouth and tangent.
        offset = Vector((post_x, post_z)) - a
        along = offset[0] * d[0] + offset[1] * d[1]
        if along >= -layout["clearanceMargin"] and abs(offset[0] * d[1] - offset[1] * d[0]) < destination["routeHalfWidth"] + layout["clearanceMargin"]:
            raise ValueError(f"Fingerpost overlaps {destination['label']} route envelope")

    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = town["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    parts, base, marks, flame, light, lamp_triangles = build_marker(ground, layout)
    waymarker_triangles = triangle_count(parts) - lamp_triangles
    cask_recipe = HERE.with_name("author-bellwether-tavern-cask.py")
    if not cask_recipe.is_file():
        raise ValueError("The paired tavern cask helper is required before the final exterior export.")
    cask = runpy.run_path(str(cask_recipe))
    cask_parts = cask["build_cask"](ground)
    normalize_cask_materials(cask_parts)
    cask_triangles = triangle_count(cask_parts)
    parts.extend(cask_parts)
    # Each component is a closed solid. Keep the outside face toward the lamp
    # on both sides of the boards, including the mirrored reliefs.
    for obj in parts:
        solid = bmesh.new()
        try:
            solid.from_mesh(obj.data)
            if any(not edge.is_manifold for edge in solid.edges):
                raise ValueError(f"Open solid in {obj.name}")
            bmesh.ops.recalc_face_normals(solid, faces=list(solid.faces))
            if solid.calc_volume(signed=True) <= 0:
                raise ValueError(f"Inverted solid in {obj.name}")
            solid.to_mesh(obj.data)
            obj.data.update()
        finally:
            solid.free()
    output.mkdir(parents=True)
    master = output / "square-waymarker.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    geometry["join_parts"](parts, "BellwetherSquareWaymarker")
    patch = output / "square-waymarker.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                               export_apply=True, export_tangents=True, export_normals=True,
                               export_animations=False)
    runtime = output / previous.name
    added, vertices, primitives = town["join_town"](previous, patch, runtime)
    if not 0 < added <= 2000 or primitives != 5:
        raise ValueError(f"Unexpected square waymarker geometry: {added} triangles, {primitives} primitives")
    if waymarker_triangles + lamp_triangles + cask_triangles != added:
        raise ValueError(f"Authored subtotals do not match merged triangle delta: {waymarker_triangles}+{lamp_triangles}+{cask_triangles}!={added}")
    doc, _ = town["read_glb"](runtime)
    indices = sum(doc["accessors"][p["indices"]]["count"] for m in doc["meshes"] for p in m["primitives"])
    manifest = json.loads((previous.parent / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    town["save_json"](output / "asset.manifest.json", manifest)
    header = output / "geometry.generated.h"
    header.write_text("#pragma once\n#include <array>\n#include <cstddef>\n\n"
                      "namespace horde::gameplay::bellwether {\n"
                      f"inline constexpr std::size_t kSquareWaymarkerSolidTriangles={added};\n"
                      f"inline constexpr std::size_t kSquareWaymarkerSignTriangles={waymarker_triangles};\n"
                      f"inline constexpr std::size_t kSquareWaymarkerCaskTriangles={cask_triangles};\n"
                      f"inline constexpr std::size_t kSquareWaymarkerLampTriangles={lamp_triangles};\n"
                      f"inline constexpr std::array<float,3> kSquareWaymarkerPost{{{{{post_x:.6f}f, {base:.6f}f, {post_z:.6f}f}}}};\n"
                      f"inline constexpr std::array<float,2> kSquareWaymarkerView{{{{{layout['cameraWitness']['viewXZ'][0]:.6f}f, {layout['cameraWitness']['viewXZ'][1]:.6f}f}}}};\n"
                      f"inline constexpr std::array<float,3> kSquareWaymarkerFlame{{{{{', '.join(f'{value:.6f}f' for value in flame)}}}}};\n"
                      f"inline constexpr std::array<float,3> kSquareWaymarkerLight{{{{{', '.join(f'{value:.6f}f' for value in light)}}}}};\n"
                      + "".join("inline constexpr std::array<float,3> kSquareWaymarker" + name +
                               "{{" + ", ".join(f"{value:.6f}f" for value in mark) + "}};\n"
                               for name, mark in marks.items()) +
                      "} // namespace horde::gameplay::bellwether\n", encoding="utf-8", newline="\n")
    town["save_json"](output / "authoring-receipt.json", dict(
        feature="Carved four-destination fingerpost and tavern cask at Bellwether south square",
        originalTownSha256=PARENT, terrainSha256=digest(terrain), layoutSha256=digest(root / LAYOUT),
        runtimeTownSha256=digest(runtime), addedSolidTriangles=added, townVertices=vertices,
        townTriangles=indices // 3, postXZ=layout["postXZ"], baseGroundY=base, destinationMarks=marks,
        waymarkerTriangles=waymarker_triangles, caskTriangles=cask_triangles,
        lampTriangles=lamp_triangles, waymarkerFlame=flame, waymarkerLight=light,
        caskRecipeSha256=digest(cask_recipe),
        clearanceMarginMetres=layout["clearanceMargin"], viewXZ=layout["cameraWitness"]["viewXZ"],
        recipeSha256=digest(HERE),
        blender=bpy.app.version_string,
        files={p.name: digest(p) for p in (master, patch, runtime, header, output / "asset.manifest.json")}))


if __name__ == "__main__":
    main(*(Path(arg).resolve() for arg in sys.argv[sys.argv.index("--") + 1:]))
