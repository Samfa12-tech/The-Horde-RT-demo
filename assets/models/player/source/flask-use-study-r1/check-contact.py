"""Finite Blender source head/contact observation; no native collision claim."""
import argparse
import json
from pathlib import Path
import sys

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
if args.output.exists():
    raise RuntimeError('Preserve prior source observation')
source=Path(__file__).resolve().parent/'flask-use-study-r1.blend'
spec=json.loads((source.parent/'intent.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(source))
body=bpy.data.objects['char1']; flask=bpy.data.objects['HealingFlaskStudy']
result=[]
for frame in (31,34,37,43):
    bpy.context.scene.frame_set(frame);bpy.context.view_layer.update()
    obj=body.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=obj.to_mesh()
    vertices=[obj.matrix_world@v.co for v in mesh.vertices]
    names=[m.name if m else '' for m in body.data.materials]
    faces=[tuple(p.vertices) for p in mesh.polygons
           if any(n in names[p.material_index] for n in ('HeadPrimaryMasked','NearFacePrimaryMasked'))]
    gauntlets=[tuple(p.vertices) for p in mesh.polygons if 'GauntletPrimaryVisible' in names[p.material_index]]
    assert faces and gauntlets
    head=BVHTree.FromPolygons(vertices,faces)
    gloves=BVHTree.FromPolygons(vertices,gauntlets)
    flask_tree=BVHTree.FromPolygons([flask.matrix_world@v.co for v in flask.data.vertices],
                                   [tuple(p.vertices) for p in flask.data.polygons],all_triangles=True)
    lip=bpy.data.objects['StudyFlask'].matrix_world@Vector((0,0,spec['flaskMouthHeightMetres']))
    result.append({'frame':frame,'headFlaskTriangleOverlaps':len(head.overlap(flask_tree)),
                   'headGauntletTriangleOverlaps':len(head.overlap(gloves)),
                   'lipNearestHeadDistanceMetres':head.find_nearest(lip)[3]})
    obj.to_mesh_clear()
data={'scope':'Four source samples of selected head materials only; not full swept/native collision or acceptance',
      'samples':result}
args.output.write_text(json.dumps(data,indent=2)+'\n');print(json.dumps(data,indent=2))
