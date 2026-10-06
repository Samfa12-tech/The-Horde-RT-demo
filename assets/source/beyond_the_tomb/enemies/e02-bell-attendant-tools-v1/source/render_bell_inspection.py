"""Rerender the underside from the packed master with a labeled inspection fill.
Standalone review rerender; does not resave source or mutate exported geometry.
"""
import bpy,sys,argparse
from pathlib import Path
from mathutils import Vector
ap=argparse.ArgumentParser();ap.add_argument('--asset-dir',required=True);ROOT=Path(ap.parse_args(sys.argv[sys.argv.index('--')+1:]).asset_dir)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'abbey_bell_attendant_handbell.blend'))
s=bpy.context.scene;cam=s.camera;cam.location=(.23,-.38,-.45);cam.rotation_euler=(Vector((0,0,-.154))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=.215
ld=bpy.data.lights.new('Inspection_underfill','AREA');ld.energy=4;ld.shape='DISK';ld.size=.25;o=bpy.data.objects.new('Inspection_underfill',ld);bpy.context.collection.objects.link(o);o.location=(.1,-.28,-.40);o.rotation_euler=(Vector((0,0,-.1))-o.location).to_track_quat('-Z','Y').to_euler()
s.render.filepath=str(ROOT/'previews'/'03_bell_open_mouth_clapper.png');bpy.ops.render.render(write_still=True)
