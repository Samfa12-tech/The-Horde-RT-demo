"""Finite Blender source head/contact observation; no native collision claim.

The default retains the original four complete-action witnesses. --sweep reads
both saved actions at their 60 Hz samples and halfway between them. This is a
discrete mesh observation, not continuous collision detection or a tolerance
for native contact. No Blend or production asset is written.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--blend',type=Path,default=Path(__file__).resolve().parent/'flask-use-study-r1.blend')
parser.add_argument('--intent',type=Path,default=Path(__file__).resolve().parent/'intent.json')
parser.add_argument('--sweep',action='store_true')
parser.add_argument('--motion-receipt',type=Path)
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
if args.output.exists():
    raise RuntimeError('Preserve prior source observation')
source=args.blend.resolve()
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
intent_bytes=args.intent.read_bytes()
spec=json.loads(intent_bytes)
receipt=None
if args.sweep:
    if args.motion_receipt is None:
        parser.error('--sweep requires the saved source --motion-receipt')
    receipt=json.loads(args.motion_receipt.read_text())
    assert receipt['blendSha256']==source_hash, 'Receipt must name this exact saved Blend'
    if 'effectiveIntentSha256' in receipt:
        assert hashlib.sha256(intent_bytes).hexdigest()==receipt['effectiveIntentSha256'], 'Use the matching effective intent'
    for path,digest in receipt['sourceHashes'].items():
        assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest, path
bpy.ops.wm.open_mainfile(filepath=str(source))
body=bpy.data.objects['char1']; flask=bpy.data.objects['HealingFlaskStudy']
rig=bpy.data.objects['Armature']
props=(bpy.data.objects['StudySword'],bpy.data.objects['StudyFlask'])
names=[m.name if m else '' for m in body.data.materials]
faces=[tuple(p.vertices) for p in body.data.polygons
       if any(n in names[p.material_index] for n in ('HeadPrimaryMasked','NearFacePrimaryMasked'))]
gauntlets=[tuple(p.vertices) for p in body.data.polygons if 'GauntletPrimaryVisible' in names[p.material_index]]
assert faces and gauntlets

def observe(frame):
    integer=math.floor(frame)
    bpy.context.scene.frame_set(integer,subframe=frame-integer)
    bpy.context.view_layer.update()
    obj=body.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=obj.to_mesh()
    try:
        # Armature evaluation preserves polygon topology for these two regions.
        assert len(mesh.polygons)==len(body.data.polygons)
        vertices=[obj.matrix_world@v.co for v in mesh.vertices]
        head=BVHTree.FromPolygons(vertices,faces)
        gloves=BVHTree.FromPolygons(vertices,gauntlets)
        flask_tree=BVHTree.FromPolygons([flask.matrix_world@v.co for v in flask.data.vertices],
                                       [tuple(p.vertices) for p in flask.data.polygons],all_triangles=True)
        lip=props[1].matrix_world@Vector((0,0,spec['flaskMouthHeightMetres']))
        return {'frame':frame,'sourceSeconds':(frame-1)/spec['sampleRateHz'],
                'headFlaskTriangleOverlaps':len(head.overlap(flask_tree)),
                'headGauntletTriangleOverlaps':len(head.overlap(gloves)),
                'lipNearestHeadDistanceMetres':head.find_nearest(lip)[3]}
    finally:
        obj.to_mesh_clear()

sequences={}
if args.sweep:
    for label,key,rows in (('FlaskUseStudy_ONLY','complete',receipt['samples']),
                           ('FlaskCancelStudy_ONLY','cancel',receipt['cancelStudy']['samples'])):
        for obj in (rig,*props):
            obj.animation_data.action=bpy.data.actions[obj.name+'_'+label]
        assert all(abs(row['time']-i/spec['sampleRateHz'])<1e-9 for i,row in enumerate(rows))
        samples=[observe(1+i/2) for i in range(2*len(rows)-1)]
        overlaps=[row for row in samples if row['headFlaskTriangleOverlaps'] or row['headGauntletTriangleOverlaps']]
        sequences[key]={'samples':samples,'observations':len(samples),
                        'overlapObservations':len(overlaps),
                        'firstOverlap':overlaps[0] if overlaps else None,
                        'minimumLipNearestHeadDistanceMetres':min(r['lipNearestHeadDistanceMetres'] for r in samples)}
    data={'scope':'Both saved source actions at 60 Hz samples and midpoints; selected head/gauntlet regions only',
          'sampleHz':120,'continuousCollisionDetection':False,'nativeCollisionAcceptance':False,
          'allSelectedContactObservationsClear':all(not s['overlapObservations'] for s in sequences.values()),
          'sequences':sequences}
else:
    data={'scope':'Four source samples of selected head materials only; not full swept/native collision or acceptance',
          'samples':[observe(frame) for frame in (31,34,37,43)]}
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
if receipt:
    for path,digest in receipt['sourceHashes'].items():
        assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest, path
data.update(blendSha256=source_hash,sourceBytesUnchanged=True,
            selectedHeadTriangles=len(faces),selectedGauntletTriangles=len(gauntlets))
args.output.write_text(json.dumps(data,indent=2)+'\n')
# Keep detailed pose rows in the receipt rather than flooding the console.
summary={key:value for key,value in data.items() if key not in ('samples','sequences')}
if sequences:
    summary['sequences']={key:{k:v for k,v in value.items() if k!='samples'} for key,value in sequences.items()}
print(json.dumps(summary,indent=2))
