"""Render a lit, unbaked material preview for owner review with Blender."""
import bpy
import math
from pathlib import Path
from mathutils import Vector

root = Path(__file__).resolve().parents[5]
maps = Path(__file__).resolve().parent / "maps"
out = root / "reports/bellwether-roof-v1/roof-shaded-preview.png"
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.mesh.primitive_plane_add(size=2.4, location=(0, 0, 0))
plane = bpy.context.object
plane.name = "2.4m roof texture preview plane"
plane.data.name = "2.4m roof UV preview"
plane.data.uv_layers.active.data.foreach_set(
    "uv", [v for loop in plane.data.loops
           for v in (plane.data.vertices[loop.vertex_index].co.x/2.4 + .5,
                     plane.data.vertices[loop.vertex_index].co.y/2.4 + .5)])
mat = bpy.data.materials.new("Bellwether clay roof | unbaked PBR preview")
mat.use_nodes = True
n = mat.node_tree.nodes
l = mat.node_tree.links
n.clear()
out_node = n.new("ShaderNodeOutputMaterial")
bsdf = n.new("ShaderNodeBsdfPrincipled")
bsdf.inputs["IOR"].default_value = 1.46
base = n.new("ShaderNodeTexImage")
base.image = bpy.data.images.load(str(maps/"base-color.png"), check_existing=True)
base.image.colorspace_settings.name = "sRGB"
normal = n.new("ShaderNodeTexImage")
normal.image = bpy.data.images.load(str(maps/"normal.png"), check_existing=True)
normal.image.colorspace_settings.name = "Non-Color"
orm = n.new("ShaderNodeTexImage")
orm.image = bpy.data.images.load(str(maps/"orm.png"), check_existing=True)
orm.image.colorspace_settings.name = "Non-Color"
sep = n.new("ShaderNodeSeparateColor")
sep.mode = "RGB"
normal_map = n.new("ShaderNodeNormalMap")
l.new(base.outputs["Color"], bsdf.inputs["Base Color"])
l.new(orm.outputs["Color"], sep.inputs["Color"])
l.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
l.new(normal.outputs["Color"], normal_map.inputs["Color"])
l.new(normal_map.outputs["Normal"], bsdf.inputs["Normal"])
l.new(bsdf.outputs["BSDF"], out_node.inputs["Surface"])
plane.data.materials.append(mat)

def aim(obj, target):
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat("-Z", "Y").to_euler()

bpy.ops.object.camera_add(location=(2.4, -3.2, 3.2))
camera=bpy.context.object
camera.data.type="ORTHO"
camera.data.ortho_scale=3.35
aim(camera,(0,0,0))
scene=bpy.context.scene
scene.camera=camera
scene.render.engine="CYCLES"
scene.cycles.samples=48
scene.cycles.use_denoising=True
scene.render.resolution_x=1600
scene.render.resolution_y=1200
scene.render.resolution_percentage=100
scene.render.image_settings.file_format="PNG"
scene.render.image_settings.color_mode="RGB"
scene.view_settings.view_transform="AgX"
world=bpy.data.worlds.new("Neutral studio fill")
world.use_nodes=True
world.node_tree.nodes["Background"].inputs["Color"].default_value=(.22,.25,.28,1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value=.16
scene.world=world
for name, loc, power, size in [
    ("Soft grazing key",(1.8,-1.0,2.4),160,1.25),
    ("Cool broad fill",(-2.0,.8,2.2),80,1.8),
]:
    bpy.ops.object.light_add(type="AREA",location=loc)
    light=bpy.context.object
    light.name=name
    light.data.energy=power
    light.data.shape="DISK"
    light.data.size=size
    aim(light,(0,0,0))
scene.render.filepath=str(out)
bpy.ops.render.render(write_still=True)
