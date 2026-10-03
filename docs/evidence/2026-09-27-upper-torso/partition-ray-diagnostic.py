"""Read-only CPU geometry diagnostic; not native RT/image acceptance."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import sys
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('world', type=Path)
parser.add_argument('view', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args(sys.argv[sys.argv.index('--')+1:])
if args.output.exists():
    raise RuntimeError('Use a new output directory')
args.output.mkdir(parents=True)

def read_obj(path):
    vertices, faces, groups = [], [], []
    group, matrix = None, None
    for line in path.read_text().splitlines():
        fields = line.split()
        if line.startswith('# model_to_world_row_major_3x4'):
            values = list(map(float, fields[2:]))
            matrix = Matrix([values[i:i+4] for i in range(0, 12, 4)] + [[0,0,0,1]])
        elif fields and fields[0] == 'v':
            vertices.append(Vector(list(map(float, fields[1:4]))))
        elif fields and fields[0] == 'g':
            group = fields[1]
        elif fields and fields[0] == 'f':
            faces.append(tuple(int(i.split('/')[0])-1 for i in fields[1:]))
            groups.append(group)
    assert matrix is not None
    return [matrix @ v for v in vertices], faces, groups

wv, wf, wg = read_obj(args.world)
vv, vf, vg = read_obj(args.view)
vertices = wv + vv
faces = wf + [tuple(i+len(wv) for i in face) for face in vf]
groups = wg + vg

def tree(allowed):
    selected = [i for i,g in enumerate(groups) if g in allowed]
    return BVHTree.FromPolygons(vertices, [faces[i] for i in selected], all_triangles=True), selected

current_groups = {'BodyRemainderPrimaryVisible', 'ViewmodelSleeves', 'ViewmodelGauntlets'}
trees = {
    'current': tree(current_groups),
    'plus_near_face': tree(current_groups | {'NearFacePrimaryMasked'}),
    'full_world': tree(set(wg)),
}
origin = Vector((0,.7,1.85))
forward = Vector((0,-4.05,-1)).normalized()
right = forward.cross(Vector((0,1,0))).normalized()
up = right.cross(forward)
width, height = 135,240
colors = {'BodyRemainderPrimaryVisible':(.1,.5,.8,1),
          'NearFacePrimaryMasked':(1,.3,.05,1), 'HeadPrimaryMasked':(.7,.1,.7,1),
          'ViewmodelSleeves':(.3,.7,.2,1),'ViewmodelGauntlets':(.8,.7,.1,1),
          'BodyPrimaryVisible':(.3,.7,.2,1),'GauntletPrimaryVisible':(.8,.7,.1,1)}
pixels = {name: [0.]*(width*height*4) for name in trees}
counts = {name: Counter() for name in trees}
changes, examples = Counter(), []
for y in range(height):
    for x in range(width):
        screen_x=((x+.5)/width*2-1)*(540/960)
        screen_y=((y+.5)/height*2-1)*-.74
        direction=(forward*1.22 + right*screen_x + up*screen_y).normalized()
        hits = {}
        for name,(bvh, indices) in trees.items():
            location, normal, index, distance=bvh.ray_cast(origin+direction*.002,direction,10.)
            semantic=groups[indices[index]] if index is not None else 'miss'
            counts[name][semantic]+=1
            # Blender image row order is bottom-to-top; game image row zero is top.
            offset=((height-1-y)*width+x)*4
            pixels[name][offset:offset+4]=colors.get(semantic,(.04,.04,.04,1))
            hits[name]=(semantic, location, normal, distance)
        if hits['plus_near_face'][0]=='NearFacePrimaryMasked':
            old=hits['current'][0]
            facing='back' if hits['plus_near_face'][2].dot(direction)>0 else 'front'
            changes[old+' -> NearFace ('+facing+')']+=1
            if x in (width//4,width//2,3*width//4) and y%20==0:
                examples.append(dict(pixel=[x*4+2,y*4+2],previous=old,facing=facing,
                    worldPosition=list(hits['plus_near_face'][1]),distance=hits['plus_near_face'][3]))
for name,data in pixels.items():
    image=bpy.data.images.new(name,width=width,height=height,alpha=True)
    image.pixels.foreach_set(data)
    image.filepath_raw=str(args.output/(name+'.png'))
    image.file_format='PNG'
    image.save()
report=dict(description=__doc__,world=str(args.world),view=str(args.view),
            sampleGrid=[width,height],camera=list(origin),pitch=-4.,
            counts={k:dict(v) for k,v in counts.items()},changes=dict(changes),examples=examples)
(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
