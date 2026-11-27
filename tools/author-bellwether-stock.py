"""Author Bellwether's first settled livestock pocket on the measured dry slope."""
import json
import math
from pathlib import Path
import runpy
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector


RECIPE = Path(__file__).resolve()
world = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-works.py")))
model = runpy.run_path(str(RECIPE.with_name("author-bellwether-mill-crossing.py")))
TOWN = Path("assets/models/world/runtime/bellwether-tavern-v1")
PARENT = "8a5afebeec0786ee704f02a5cd5fc2bd2d9f3d1b0f74226c98d406a1e6b63213"
BOUNDS = (28.90, 34.30, 140.82, 145.17)
PIG = (32.10, 143.56)
HENS = ((31.61, 141.55), (32.28, 142.35))
ROOST_SHIFT = 1.24
VIEW_ROUTE = ((39.35, 139.70), (38.20, 142.02), (35.80, 142.76))
ROOST_VIEW = (35.43, 141.21)
PIG_HEAD = ("Jowl", "Snout", "Rounded snout plate", "Dark eye", "Nostril", "Folded ear")
HEN_HEAD = ("Neck", "Drawn-in head", "Small comb", "Eye", "Beak tip")


def move_to_collection(obj, collection):
    for previous in tuple(obj.users_collection):
        previous.objects.unlink(obj)
    collection.objects.link(obj)


class Pocket:
    def __init__(self, ground):
        self.ground = ground
        self.parts = []
        self.collection = None
        self.timber = world["material"]("HordeTavernTimber", (.24, .14, .075), .87)
        self.stone = world["material"]("HordeTownStone", (.23, .21, .15), .95)
        self.clay = world["material"]("HordeTownRoofClay", (.26, .12, .065), .85)
        self.iron = world["material"]("HordeHearthChar", (.073, .055, .04), .80)

    def begin(self, name):
        self.collection = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(self.collection)

    def box(self, label, xlo, xhi, zlo, zhi, bottom, top, finish, bevel=0.):
        self.add(world["block"](label, xlo, xhi, zlo, zhi, bottom, top, finish, bevel))

    def add(self, obj):
        move_to_collection(obj, self.collection)
        self.parts.append(obj)

    def surface(self, label, points, faces, finish, smooth=False, closed=False):
        obj = model["mesh"](label, points, faces, finish)
        if closed:
            mesh = bmesh.new()
            mesh.from_mesh(obj.data)
            bmesh.ops.recalc_face_normals(mesh, faces=list(mesh.faces))
            if mesh.calc_volume(signed=True) < 0.:
                bmesh.ops.reverse_faces(mesh, faces=list(mesh.faces))
            if not all(edge.is_manifold and edge.is_contiguous for edge in mesh.edges) or mesh.calc_volume(signed=True)<=0.:
                raise ValueError(f"Closed animal detail is invalid: {label}")
            mesh.to_mesh(obj.data)
            mesh.free()
        if smooth:
            obj.data.polygons.foreach_set("use_smooth", [True] * len(faces))
        self.add(obj)

    def beam(self, name, start, end, tall, wide, finish):
        dx, dz = end[0] - start[0], end[2] - start[2]
        length = math.hypot(dx, dz)
        side_x, side_z = -dz / length * wide / 2, dx / length * wide / 2
        corners = [(p[0] + side * side_x, p[1] + vertical * tall / 2, p[2] + side * side_z)
                   for vertical, p, side in ((-1, start, -1), (-1, end, -1), (-1, end, 1), (-1, start, 1),
                                              (1, start, -1), (1, end, -1), (1, end, 1), (1, start, 1))]
        self.surface(name, corners, [(0, 1, 2, 3), (4, 5, 1, 0), (5, 6, 2, 1), (6, 7, 3, 2),
                                     (7, 4, 0, 3), (7, 6, 5, 4)], finish)

    def habits(self):
        self.begin("Stock fences and shelter")
        x0, x1, z0, z1 = BOUNDS
        gate_lo, gate_hi = 141.85, 143.17
        west = [(x0, z0), (x0, 142.95), (x0, z1)]
        north = [(x0, z0), (30.70, z0), (32.55, z0), (x1, z0)]
        south = [(x0, z1), (30.70, z1), (32.55, z1), (x1, z1)]
        east = [(x1, z0), (x1, gate_lo), (x1, gate_hi), (x1, z1)]
        def raised(point, height):
            x, z = point
            return x, self.ground(x, z) + height, z
        for i, (x, z) in enumerate(dict.fromkeys(west + north + south + east)):
            base = self.ground(x, z)
            self.box(f"Driven post {i}", x - .07, x + .07, z - .073, z + .073,
                     base - .13, base + .84 + .017 * (i % 3), self.timber, .014)
        for face, side in enumerate((west, north, south, east)):
            for i, (a, b) in enumerate(zip(side, side[1:])):
                for level, tall in ((.27, .173), (.60, .178)):
                    self.beam(f"Pegged rail {face}/{i}/{level}", raised(a, level), raised(b, level),
                              tall, .088, self.timber)
        # Deliberately closed gate, with independent leaf, hinges and latch.
        for i in range(6):
            z = gate_lo + .146 + i * .172
            y = self.ground(x1, z)
            self.box(f"Gate leaf stile {i}", x1 - .09, x1 + .012, z - .018, z + .11,
                     y + .094, y + .683, self.timber, .004)
        self.beam("Gate diagonal", raised((x1 + .038, gate_lo + .1), .20),
                  raised((x1 + .038, gate_hi - .1), .678), .091, .063, self.timber)
        gate_base = self.ground(x1, gate_lo)
        for height in (.27, .62):
            self.box(f"Wrought hinge {height}", x1 + .01, x1 + .067, gate_lo - .074, gate_lo + .24,
                     gate_base + height, gate_base + height + .046, self.iron)
        latch = self.ground(x1, gate_hi)
        self.box("Closed gate latch", x1 + .018, x1 + .095, gate_hi - .23, gate_hi + .034,
                 latch + .541, latch + .605, self.iron, .006)

        # The hens shelter above the ground on a board beside the straw.
        floor = self.ground(29.83 + ROOST_SHIFT, 141.7)
        start_of_roost = len(self.parts)
        for x in (29.18, 30.55):
            for z in (141.07, 142.20):
                self.box("Roost foot", x - .08, x + .08, z - .08, z + .08,
                         self.ground(x + ROOST_SHIFT, z) - .10, floor + 1.17, self.timber, .009)
        self.box("Deep roost shelf", 29.093, 30.78, 140.988, 142.288,
                 floor + .40, floor + .493, self.timber)
        self.box("Sleeping side", 29.10, 29.25, 141.00, 142.29,
                 floor + .44, floor + 1.19, self.timber)
        self.box("North roost screen", 29.17, 30.57, 140.977, 141.10,
                 floor + .47, floor + 1.18, self.timber)
        self.box("South roost screen", 29.17, 30.57, 142.17, 142.285,
                 floor + .47, floor + 1.18, self.timber)
        for i in range(5):
            x = 29.005 + .337 * i
            self.beam(f"Leaning roost cover {i}", (x, floor + 1.27 - .023*i, 140.87),
                      (x, floor + 1.24 - .023*i, 142.41), .10, .40, self.clay)
        for i in range(9):
            x, z = 30.08 + (i * .171) % .455, 141.18 + (i * .293) % .90
            self.beam(f"Loose straw {i}", (x, floor + .497, z),
                      (x + .20, floor + .502, z + .057 * ((i % 4) - 1)), .017, .03, self.clay)
        for obj in self.parts[start_of_roost:]:
            obj.location.x += ROOST_SHIFT

        for i in range(11):
            x, z = 31.37 + (i * .551) % 1.18, 143.13 + (i * .413) % .85
            end = (x + .23, z + .042 * ((i % 5) - 2))
            self.beam(f"Resting straw {i}", (x, self.ground(x,z) + .013, z),
                      (end[0], self.ground(*end) + .014, end[1]), .016, .038, self.clay)

        # The trough and its covered jug are outside the kitchen beds.
        x, z = 33.24, 144.27
        base = self.ground(x, z)
        self.box("Trough stone foot", x - .55, x + .55, z - .25, z + .25, base - .05, base + .14, self.stone, .03)
        for s in (-1, 1):
            cz = z + s * .216
            self.box("Hollow trough side", x - .55, x + .55, cz - .043, cz + .043,
                     base + .116, base + .306, self.timber, .012)
            cx = x + s * .507
            self.box("Hollow trough end", cx - .049, cx + .049, z - .24, z + .24,
                     base + .116, base + .305, self.timber, .012)
        self.box("Trough dark inner floor", x - .445, x + .445, z - .172, z + .172, base + .13, base + .157, self.iron)
        jug = (33.73, self.ground(33.73, 144.93), 144.93)
        self.ellipsoid("Covered clay water jug", (0., .205, 0.), (.148, .217, .153), self.clay,
                       lambda p: tuple(a + b for a, b in zip(p, jug)), 10, 6)
        self.box("Jug stopper", jug[0] - .065, jug[0] + .065, jug[2] - .068, jug[2] + .068,
                 jug[1] + .378, jug[1] + .415, self.timber, .014)
        return floor + .501

    def ellipsoid(self, name, centre, radii, finish, orient, sides=12, rows=6):
        cx, cy, cz = centre
        ax, ay, az = radii
        points = [(cx, cy - ay, cz)]
        for ring in range(1, rows):
            latitude = -math.pi / 2 + ring * math.pi / rows
            radius = math.cos(latitude)
            points.extend((cx + ax * radius * math.cos(2 * math.pi * i / sides),
                           cy + ay * math.sin(latitude),
                           cz + az * radius * math.sin(2 * math.pi * i / sides)) for i in range(sides))
        points.append((cx, cy + ay, cz))
        faces = [(0, 1 + j, 1 + (j + 1) % sides) for j in range(sides)]
        for ring in range(rows - 2):
            a, b = 1 + ring*sides, 1 + (ring + 1)*sides
            faces.extend((a + j, b + j, b + (j + 1) % sides, a + (j + 1) % sides) for j in range(sides))
        last = 1 + (rows - 2) * sides
        faces.extend((last + j, len(points) - 1, last + (j + 1) % sides) for j in range(sides))
        self.surface(name, [orient(point) for point in points], faces, finish, True)

    def animal_orientation(self, centre, floor, yaw):
        cosine, sine = math.cos(yaw), math.sin(yaw)
        def transform(point):
            x, y, z = point
            return centre[0] + x * cosine - z * sine, floor + y, centre[1] + z * cosine + x * sine
        return transform

    def hog(self):
        self.begin("Resting pig")
        orient = self.animal_orientation(PIG, self.ground(*PIG), .98)
        def ell(name, centre, radii, finish, sides=12, rows=6):
            self.ellipsoid(name, centre, radii, finish, orient, sides, rows)
        # A loose, pear-shaped silhouette with a heavy belly and tucked limbs.
        ell("Back and barrel", (-.15, .365, 0.), (.668, .341, .337), self.clay, 18, 9)
        ell("Shoulder and chest", (.255, .298, 0.), (.27, .255, .285), self.clay, 14, 7)
        ell("Jowl", (.501, .222, 0.), (.275, .201, .222), self.clay, 14, 7)
        ell("Snout", (.735, .168, 0.), (.16, .118, .138), self.clay, 12, 6)
        ell("Rounded snout plate", (.854, .159, .0), (.036, .098, .132), self.clay, 12, 6)
        for sign in (-1, 1):
            def flank(z):
                return sign * z
            ell("Foreleg folded", (.285, .087, flank(.182)), (.221, .08, .09), self.clay, 10, 5)
            ell("Hind leg tucked", (-.47, .122, flank(.217)), (.24, .127, .098), self.clay, 10, 5)
            ell("Fore hoof", (.469, .032, flank(.19)), (.089, .054, .065), self.iron, 8, 5)
            ell("Rear hoof", (-.361, .028, flank(.234)), (.095, .05, .066), self.iron, 8, 5)
            ell("Dark eye", (.567, .311, flank(.173)), (.032, .024, .023), self.iron, 8, 5)
            ell("Nostril", (.884, .18, flank(.063)), (.012, .022, .027), self.iron, 8, 5)
            outer = ((.391,.352,flank(.154)),(.55,.397,flank(.111)),
                     (.477,.51,flank(.154)),(.364,.577,flank(.237)),
                     (.319,.462,flank(.256)),(.34,.384,flank(.22)))
            inner = [(x + .015, y - .006, z - sign*.017) for x, y, z in outer]
            face = tuple(range(6)) if sign > 0 else tuple(reversed(range(6)))
            thickness = [(j, j + 6, ((j + 1) % 6) + 6, (j + 1) % 6) for j in range(6)]
            self.surface("Folded ear", [orient(p) for p in (*outer, *inner)],
                         [face, tuple(reversed(tuple(j+6 for j in face))), *thickness], self.clay, closed=True)
        tail_path = []
        for i in range(16):
            a = i / 15 * math.tau * 1.06
            cx, cy, cz = -.752 - i * .009, .432 + .079 * math.sin(a), .077 * (1. - math.cos(a))
            tail_path.append(Vector((cx, cy, cz)))
        tail = []
        for i, centre in enumerate(tail_path):
            tangent = (tail_path[min(15,i+1)] - tail_path[max(0,i-1)]).normalized()
            normal = tangent.cross(Vector((1.,0.,0.))).normalized()
            binormal = tangent.cross(normal)
            for j in range(6):
                a = math.tau * j / 6
                tail.append(orient(centre + .012 * (normal * math.cos(a) + binormal * math.sin(a))))
        faces = [(i*6+j,(i+1)*6+j,(i+1)*6+(j+1)%6,i*6+(j+1)%6) for i in range(15) for j in range(6)]
        faces += [tuple(range(6)), tuple(reversed(range(90,96)))]
        self.surface("Curled tail", tail, faces, self.clay, smooth=True, closed=True)
        return self.head_anchor("Pig head", PIG_HEAD, orient((.344, .245, 0.)), .98, 1.)

    def hen(self, name, centre, floor, yaw, size, wing):
        self.begin(name)
        orient = self.animal_orientation(centre, floor, yaw)
        def scale(point):
            return orient(tuple(value * size for value in point))
        def ell(label, centre, radii, finish, sides=10, rows=5):
            self.ellipsoid(label, centre, radii, finish, scale, sides, rows)
        ell("Settled feather body", (-.021, .122, 0.), (.182, .131, .119), self.timber, 14, 7)
        ell("Breast", (.105, .132, 0.), (.091, .119, .095), self.clay)
        ell("Neck", (.137, .215, 0.), (.067, .107, .065), self.clay)
        ell("Drawn-in head", (.168, .279, 0.), (.061, .065, .052), self.clay)
        ell("Small comb", (.169, .336, 0.), (.061, .028, .014), self.clay)
        for side in (-1, 1):
            ell("Closed wing", (-.041, .143, side * .081), (.148, .079, .053), wing, 10, 6)
            ell("Eye", (.200, .289, side * .044), (.01, .011, .009), self.iron, 8, 4)
            ell("Tucked shank", (.066, .028, side * .066), (.045, .023, .017), self.clay, 8, 4)
        ell("Tail feather roots", (-.177, .166, 0.), (.089, .061, .068), self.iron)
        for i in range(5):
            ell(f"Layered tail feather {i}", (-.215 - .006*(i % 2), .193 + .008*(i % 3), (i - 2) * .019),
                (.070, .025, .015), self.iron, 8, 4)
        self.surface("Beak tip",
                     [scale(p) for p in ((.213,.274,-.023),(.213,.273,.023),(.217,.301,0.),(.272,.277,0.))],
                     [(0,2,1),(0,1,3),(1,2,3),(2,0,3)], self.stone, closed=True)
        return self.head_anchor(name + " head", HEN_HEAD, scale((.122, .167, 0.)), yaw, size)

    def head_anchor(self, name, part_names, position, yaw, scale):
        parts = [obj for obj in self.collection.objects if obj.name.split('.')[0] in part_names]
        if not parts:
            raise ValueError(f"{name} lacks mesh parts")
        x, y, z = position
        mount = Matrix.Translation((x, -z, y)) @ Matrix.Rotation(-yaw, 4, 'Z') @ Matrix.Scale(scale, 4)
        anchor = bpy.data.objects.new(name + " pivot", None)
        self.collection.objects.link(anchor)
        anchor.empty_display_type = 'ARROWS'
        anchor.empty_display_size = .10
        anchor.matrix_world = mount
        for part in parts:
            part.parent = anchor
            part.matrix_parent_inverse = mount.inverted()
            self.parts.remove(part)
        return position, yaw, scale, parts, mount.inverted()


def export_stock_heads(output, anchors):
    heads = []
    for name, (_, _, _, parts, local_from_world) in zip(("StockPigHead", "StockHenHead"), anchors):
        local_parts = []
        for part in parts:
            copy = bpy.data.objects.new(part.name + " local", part.data.copy())
            bpy.context.scene.collection.objects.link(copy)
            copy.data.transform(local_from_world)
            local_parts.append(copy)
        heads.append(model["join_parts"](local_parts, name))
    bpy.ops.object.select_all(action="DESELECT")
    for head in heads:
        head.select_set(True)
    path = output / "stock-heads.glb"
    bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    glb, _ = world["read_glb"](path)
    nodes = [glb["nodes"][index] for index in glb["scenes"][0]["nodes"]]
    shapes = {}
    first = 0
    vertices = indices = 0
    for node in nodes:
        primitives = glb["meshes"][node["mesh"]]["primitives"]
        shapes[node["name"]] = (first, len(primitives))
        first += len(primitives)
        for primitive in primitives:
            vertices += glb["accessors"][primitive["attributes"]["POSITION"]]["count"]
            indices += glb["accessors"][primitive["indices"]]["count"]
    if len(nodes) != 2 or set(shapes) != {"StockPigHead", "StockHenHead"} or first != 5 or indices > 5100:
        raise ValueError("Stock head geometry exceeds the two-owner, five-primitive budget")
    manifest = json.loads((output / "asset.manifest.json").read_bytes())
    manifest.update(asset="bellwether-stock-heads-v1", requiredSockets=[], materialOverrides=[],
                    licenceStatus="Original Horde-owned untextured rigid animal heads and finishes.")
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices, maxPrimitives=first,
                               maxMaterials=len(glb["materials"]), maxTextureLayersPerKind=1)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    world["save_json"](output / "stock-heads.manifest.json", manifest)
    return shapes, indices // 3


def main(root, previous, output):
    terrain = root / world["TERRAIN"]
    if output.exists() or world["sha"](previous) != PARENT or world["sha"](terrain) != world["TERRAIN_HASH"]:
        raise ValueError("Choose a new output and supply the exact tally town and surveyed terrain.")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ground = world["terrain_probe"](terrain)
    bpy.data.orphans_purge(do_recursive=True)
    pocket = Pocket(ground)
    shelf = pocket.habits()
    mounts = (pocket.hog(), pocket.hen("Hen on the straw shelf", HENS[0], shelf, -.67, 1.09, pocket.timber),
              pocket.hen("Hen on dry ground", HENS[1], ground(*HENS[1]) - .009, .94, 1., pocket.clay))
    output.mkdir(parents=True)
    master = output / "bellwether-stock.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master), compress=True)
    model["join_parts"](pocket.parts, "BellwetherSettledStock")
    patch = output / "settled-stock.glb"
    bpy.ops.export_scene.gltf(filepath=str(patch), export_format="GLB", use_selection=True,
                             export_apply=True, export_normals=True, export_tangents=True, export_animations=False)
    town = output / "bellwether-tavern-v1.glb"
    added, vertices, primitives = world["join_town"](previous, patch, town)
    parent, _ = world["read_glb"](previous)
    indices = sum(parent["accessors"][p["indices"]]["count"] for mesh in parent["meshes"]
                  for p in mesh["primitives"]) + 3 * added
    if not 0 < added <= 6100 or primitives != 5 or vertices > 61500:
        raise ValueError(f"Stock pocket exceeds town budget: {added} new triangles, {vertices} vertices")
    manifest = json.loads((root / TOWN / "asset.manifest.json").read_bytes())
    manifest["budgets"].update(maxVertices=vertices, maxIndices=indices)
    manifest["lods"][0]["maxTriangles"] = indices // 3
    world["save_json"](output / "asset.manifest.json", manifest)
    head_shapes, head_triangles = export_stock_heads(output, mounts[:2])
    head_placements = ", ".join("{" + ", ".join(f"{value:.7f}f" for value in (*pivot, yaw, scale)) + "}"
                                for pivot, yaw, scale, _, _ in mounts)
    head_geometry = ", ".join("{" + f"{first}u, {count}u" + "}"
                              for first, count in (head_shapes["StockPigHead"], head_shapes["StockHenHead"]))
    view = ", ".join("{" + f"{x:.6f}f, {z:.6f}f" + "}" for x, z in VIEW_ROUTE)
    header = ("#pragma once\n#include <array>\n#include <cstddef>\n\n"
              "// Generated by tools/author-bellwether-stock.py.\nnamespace horde::gameplay::bellwether {\n"
              f"inline constexpr std::size_t kBellwetherStockSolidTriangles={added};\n"
              f"inline constexpr std::size_t kBellwetherStockHeadTriangles={head_triangles};\n"
              f"inline constexpr std::array<std::array<float,5>,3> kBellwetherStockHeadMounts{{{{{head_placements}}}}};\n"
              f"inline constexpr std::array<std::array<unsigned,2>,2> kBellwetherStockHeadGeometry{{{{{head_geometry}}}}};\n"
              f"inline constexpr std::array<std::array<float,2>,{len(VIEW_ROUTE)}> kBellwetherStockViewRoute{{{{{view}}}}};\n"
              f"inline constexpr std::array<float,2> kBellwetherStockPigXZ{{{{{PIG[0]:.6f}f, {PIG[1]:.6f}f}}}};\n"
              f"inline constexpr std::array<float,2> kBellwetherStockRoostXZ{{{{{30.55+ROOST_SHIFT:.6f}f, 141.718000f}}}};\n"
              f"inline constexpr std::array<float,2> kBellwetherStockRoostView{{{{{ROOST_VIEW[0]:.6f}f, {ROOST_VIEW[1]:.6f}f}}}};\n"
              "} // namespace horde::gameplay::bellwether\n")
    layout = output / "geometry.generated.h"
    layout.write_text(header, encoding="utf-8", newline="\n")
    items = (master, patch, town, layout, output / "asset.manifest.json", output / "stock-heads.glb",
             output / "stock-heads.manifest.json")
    world["save_json"](output / "authoring-receipt.json", dict(
        feature="Settled village stock", bounds=BOUNDS, pig=PIG, hens=HENS, approach=VIEW_ROUTE, roostView=ROOST_VIEW,
        originalTownSha256=PARENT, terrainSha256=world["TERRAIN_HASH"], runtimeTownSha256=world["sha"](town),
        addedSolidTriangles=added, headTriangles=head_triangles, headMeshes=head_shapes,
        townVertices=vertices, townTriangles=indices//3, blender=bpy.app.version_string,
        recipeSha256=world["sha"](RECIPE), files={item.name: world["sha"](item) for item in items}))


if __name__ == "__main__":
    arguments = sys.argv[sys.argv.index("--") + 1:]
    if len(arguments) != 3:
        raise ValueError("Expected ROOT, PREVIOUS_TOWN_GLB and NEW_OUTPUT.")
    main(*(Path(argument).resolve() for argument in arguments))
