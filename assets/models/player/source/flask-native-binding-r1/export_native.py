"""Append calibrated flask clips to exact player/viewmodel copies; no round trip.

Run with Blender 5.2 --background --python-exit-code 1 --python <this file>
-- --output reports/flask-native-binding-20261026/candidates-r1
All destinations must be new. Original assets, Blends and bindings are read only.
The fixed-seed Cancel candidate is not an arbitrary-phase cancellation solution.
"""
from pathlib import Path
import argparse
import copy
import hashlib
import json
import math
import struct
import sys

import bpy
from mathutils import Matrix, Quaternion, Vector

ROOT = next(p for p in Path(__file__).resolve().parents if (p/'src/scene/assets/SkinnedMeshAsset.h').is_file())
BLEND = ROOT/'assets/models/player/source/flask-use-tick-aligned-r2/flask-use-tick-aligned-r2.blend'
BODY = ROOT/'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb'
VIEW = ROOT/'assets/models/player/viewmodel/runtime/gothic-traveller-viewmodel.runtime.glb'
PINS = {BLEND:'1dee0e3e9d5b2c072117d70c36b0accca7dc91d14a3981c34b0d6f442515d699',
        BODY:'46a88dac9dd569117be5a17a35540fecf3e0e6026d3ad404dd5fdbc85a9ad54f',
        VIEW:'eaa0db3abb8ff2dae616247054c190031a08c542dd95396cc6dc1e18a7462488'}
RIGHT = ('RightArm','RightForeArm','RightHand')
C = Matrix.Rotation(math.pi/2,4,'X')
CI = C.inverted()

def require(condition,message):
    if not condition:raise RuntimeError(message)

def sha(data):return hashlib.sha256(data).hexdigest()
def columns(matrix):return [matrix[r][c] for c in range(4) for r in range(4)]
def from_columns(values):return Matrix(tuple(tuple(values[c*4+r] for c in range(4)) for r in range(4)))
def trs_matrix(t,q,s):return Matrix.LocRotScale(Vector(t),Quaternion((q[3],q[0],q[1],q[2])),Vector(s))
def delta(a,b):return max(abs(a[r][c]-b[r][c]) for r in range(4) for c in range(4))
def rigid(matrix):
    # Match the native socket's proper rigid basis, independent of imported cm scale.
    x=matrix.to_3x3().col[0].normalized();raw_y=matrix.to_3x3().col[1]
    y=(raw_y-x*x.dot(raw_y)).normalized();z=x.cross(y).normalized()
    if z.dot(matrix.to_3x3().col[2])<0:y=-y;z=-z
    out=Matrix.Identity(4)
    for row in range(3):out[row][0],out[row][1],out[row][2]=x[row],y[row],z[row]
    out.translation=matrix.translation
    return out

def unpack(path):
    raw=path.read_bytes();require(struct.unpack_from('<III',raw)==(0x46546c67,2,len(raw)),'Bad GLB')
    length,kind=struct.unpack_from('<II',raw,12);require(kind==0x4e4f534a,'Bad JSON')
    binary_length,kind=struct.unpack_from('<II',raw,20+length)
    binary=raw[28+length:];require(kind==0x004e4942 and len(binary)==binary_length,'Bad BIN')
    return raw,json.loads(raw[20:20+length]),binary

def accessor(doc,binary,index):
    a=doc['accessors'][index];view=doc['bufferViews'][a['bufferView']]
    n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']]
    code={5121:'B',5123:'H',5125:'I',5126:'f'}[a['componentType']]
    fmt='<'+code*n;stride=view.get('byteStride',struct.calcsize(fmt));start=view.get('byteOffset',0)+a.get('byteOffset',0)
    return [struct.unpack_from(fmt,binary,start+i*stride) for i in range(a['count'])]

def native_idle(doc,binary):
    local={i:[node.get('translation',[0,0,0]),node.get('rotation',[0,0,0,1]),node.get('scale',[1,1,1])]
           for i,node in enumerate(doc['nodes'])}
    idle=next(a for a in doc['animations'] if a['name']=='Idle')
    for channel in idle['channels']:
        sampler=idle['samplers'][channel['sampler']]
        require(accessor(doc,binary,sampler['input'])[0][0]>=0,'Idle begins before zero')
        local[channel['target']['node']][{'translation':0,'rotation':1,'scale':2}[channel['target']['path']]]=accessor(doc,binary,sampler['output'])[0]
    return local

def global_pose(doc,local):
    parents={child:i for i,node in enumerate(doc['nodes']) for child in node.get('children',[])}
    world={}
    def visit(i):
        if i not in world:
            node=doc['nodes'][i];matrix=from_columns(node['matrix']) if 'matrix' in node else trs_matrix(*local[i])
            world[i]=visit(parents[i])@matrix if i in parents else matrix
        return world[i]
    for i in range(len(doc['nodes'])):visit(i)
    return world

def points(obj):
    evaluated=obj.evaluated_get(bpy.context.evaluated_depsgraph_get());mesh=evaluated.to_mesh()
    try:return [evaluated.matrix_world@v.co for v in mesh.vertices]
    finally:evaluated.to_mesh_clear()

def skin_points(doc,binary,world):
    joints=doc['skins'][0]['joints'];binds=[from_columns(m) for m in accessor(doc,binary,doc['skins'][0]['inverseBindMatrices'])]
    matrices=[world[j]@bind for j,bind in zip(joints,binds)];result=[]
    for primitive in doc['meshes'][0]['primitives']:
        a=primitive['attributes']
        for p,js,ws in zip(accessor(doc,binary,a['POSITION']),accessor(doc,binary,a['JOINTS_0']),accessor(doc,binary,a['WEIGHTS_0'])):
            value=Vector((0,0,0))
            for joint,weight in zip(js,ws):
                if weight:value+=weight*(matrices[joint]@Vector(p))
            result.append(value)
    return result

def append(doc,binary,values,kind):
    binary.extend(b'\0'*(-len(binary)%4));offset=len(binary);n={'SCALAR':1,'VEC3':3,'VEC4':4}[kind]
    for row in values:binary.extend(struct.pack('<'+'f'*n,*row))
    view=len(doc['bufferViews']);doc['bufferViews'].append({'buffer':0,'byteOffset':offset,'byteLength':len(binary)-offset})
    item={'bufferView':view,'componentType':5126,'count':len(values),'type':kind}
    if kind=='SCALAR':item.update(min=[values[0][0]],max=[values[-1][0]])
    index=len(doc['accessors']);doc['accessors'].append(item);return index

def sample_tracks(tracks,seconds,times,idle):
    hi=next((i for i,t in enumerate(times) if t>=seconds),len(times)-1);lo=max(0,hi-1)
    alpha=0 if times[hi]==times[lo] else (seconds-times[lo])/(times[hi]-times[lo]);result=copy.deepcopy(idle)
    for joint,rows in tracks.items():
        t0,q0,s0=rows[lo];t1,q1,s1=rows[hi]
        a,b=list(q0),list(q1)
        if sum(x*y for x,y in zip(a,b))<0:b=[-v for v in b]
        values=[x+(y-x)*alpha for x,y in zip(a,b)]
        q=Quaternion((values[3],values[0],values[1],values[2]));q.normalize()
        result[joint]=[list(Vector(t0).lerp(Vector(t1),alpha)),[q.x,q.y,q.z,q.w],list(Vector(s0).lerp(Vector(s1),alpha))]
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--correct-native-interpolation',action='store_true',
                        help='New editable derivative only: hemisphere-correct right-arm keys and use native LINEAR/Nlerp interpolation')
    parser.add_argument('--body-only',action='store_true',help='Append only body clips; unchanged original viewmodel consumes the shared pose')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:]);out=args.output.resolve()
    allowed=(ROOT/'reports/flask-native-binding-20261026').resolve()
    require(out.is_relative_to(allowed) and out!=allowed and not out.exists(),'Use a new child of the named report folder')
    inputs={p:p.read_bytes() for p in PINS}
    for path,digest in PINS.items():require(sha(inputs[path])==digest,'Pinned source changed: '+str(path))
    body_raw,body_doc,body_bin=unpack(BODY);view_raw,view_doc,view_bin=unpack(VIEW)
    require(len(body_doc['skins'])==len(view_doc['skins'])==1,'Expected one skin per source asset')
    require({k:v for k,v in body_doc['skins'][0].items() if k!='inverseBindMatrices'}==
            {k:v for k,v in view_doc['skins'][0].items() if k!='inverseBindMatrices'} and
            accessor(body_doc,body_bin,body_doc['skins'][0]['inverseBindMatrices'])==
            accessor(view_doc,view_bin,view_doc['skins'][0]['inverseBindMatrices']),
            'Body/viewmodel skin joint roster or inverse-bind payload differs')
    joints=body_doc['skins'][0]['joints'];names={i:body_doc['nodes'][i]['name'] for i in joints}
    for i in joints:require(body_doc['nodes'][i]==view_doc['nodes'][i],'Body/viewmodel joint node differs')
    idle=native_idle(body_doc,body_bin);view_idle=native_idle(view_doc,view_bin)
    require(all(idle[i]==view_idle[i] for i in joints),'Body/viewmodel native Idle differs')
    idle_world=global_pose(body_doc,idle);parents={c:i for i,n in enumerate(body_doc['nodes']) for c in n.get('children',[])}
    bpy.ops.wm.open_mainfile(filepath=str(BLEND));rig=bpy.data.objects['Armature']
    meshes=(bpy.data.objects['char1'],bpy.data.objects['PlayerViewmodel']);props=(bpy.data.objects['StudySword'],bpy.data.objects['StudyFlask'])
    require(bpy.app.version[:2]==(5,2),'Use installed Blender5.2')
    rig.animation_data.action=bpy.data.actions['Idle'];bpy.context.scene.frame_set(1);bpy.context.view_layer.update()
    basis={p.name:p.matrix_basis.copy() for p in rig.pose.bones}
    initial={i:rig.matrix_world@rig.pose.bones[names[i]].matrix for i in joints}
    correction={i:initial[i].inverted()@C@idle_world[i] for i in joints}
    calibration=max((initial[i].translation-(C@idle_world[i]).translation).length for i in joints)
    require(calibration<0.0001,'Imported/native idle joint calibration exceeds0.1mm')
    # Both independently owned meshes must still represent the exact native pair.
    geometry_errors=[]
    for obj,doc,binary in zip(meshes,(body_doc,view_doc),(body_bin,view_bin)):
        blender=points(obj);native=skin_points(doc,binary,global_pose(doc,native_idle(doc,binary)))
        require(len(blender)==len(native),'Source/native mesh vertex addressing differs')
        error=max((CI@p-q).length for p,q in zip(blender,native))
        geometry_errors.append(error);require(error<0.0001,'Source/native idle geometry differs by more than0.1mm')
    require(all(any(m.type=='ARMATURE' and m.object==rig for m in obj.modifiers) for obj in meshes),'Source pair must use the same study rig')
    interpolation_correction=[]
    if args.correct_native_interpolation:
        for suffix,last in (('FlaskUseStudy_ONLY',76),('FlaskCancelStudy_ONLY',71)):
            action=bpy.data.actions['Armature_'+suffix];rig.animation_data.action=action
            original_keys={}
            for frame in range(1,last+1):
                bpy.context.scene.frame_set(frame);bpy.context.view_layer.update()
                original_keys[frame]={name:rig.pose.bones[name].matrix_basis.copy() for name in RIGHT}
            curves=[curve for layer in action.layers for strip in layer.strips for slot in action.slots
                    for curve in strip.channelbag(slot).fcurves]
            changed=0
            for name in RIGHT:
                prefix='pose.bones["'+name+'"].'
                selected=[curve for curve in curves if curve.data_path.startswith(prefix)]
                quaternion=sorted([curve for curve in selected if curve.data_path.endswith('.rotation_quaternion')],key=lambda curve:curve.array_index)
                require(len(quaternion)==4,'Expected four quaternion component curves')
                previous=None
                for index in range(last):
                    q=[curve.keyframe_points[index].co[1] for curve in quaternion]
                    if previous is not None and sum(x*y for x,y in zip(previous,q))<0:
                        q=[-v for v in q]
                        for curve,value in zip(quaternion,q):curve.keyframe_points[index].co[1]=value
                        changed+=1
                    previous=q
                for curve in selected:
                    for point in curve.keyframe_points:point.interpolation='LINEAR'
                    curve.update()
            key_error=0.
            for frame in range(1,last+1):
                bpy.context.scene.frame_set(frame);bpy.context.view_layer.update()
                key_error=max(key_error,max(delta(rig.pose.bones[name].matrix_basis,original_keys[frame][name]) for name in RIGHT))
            require(key_error<0.00001,'Interpolation correction changed an authored key pose')
            interpolation_correction.append({'action':action.name,'quaternionKeysNegated':changed,
                'authoredKeyPoseMatrixMaxError':key_error,'changedLocalJoints':list(RIGHT),
                'interpolation':'LINEAR component TRS with continuous quaternion hemisphere; matches native normalized shortest-path Nlerp',
                'originalSourceWritten':False})
    definitions=(('FlaskUse','FlaskUseStudy_ONLY',1,76,0.0),('FlaskCancel','FlaskCancelStudy_ONLY',37,71,0.6))
    clips=[];curves=[];summaries=[];reference=bytearray();reference.extend(struct.pack('<4sII',b'FNP1',1,0))
    reference_count=0
    reference_names=('Hips','LeftHand','LeftGrip','RightHand','RightGrip')
    name_to_joint={name:i for i,name in names.items()}
    for clip_name,action_suffix,start,end,source_offset in definitions:
        for obj in (rig,*props):obj.animation_data.action=bpy.data.actions[obj.name+'_'+action_suffix]
        for p in rig.pose.bones:p.matrix_basis=basis[p.name].copy()
        times=[(frame-start)/60 for frame in range(start,end+1)];tracks={i:[] for i in joints}
        source_unmodified_error=0.;scale_residual=0.;native_key_error=0.;native_midpoint_error=0.
        rows=[]
        def source_pose(frame):
            whole=math.floor(frame);bpy.context.scene.frame_set(whole,subframe=frame-whole);bpy.context.view_layer.update()
            return {i:CI@rig.matrix_world@rig.pose.bones[names[i]].matrix@correction[i] for i in joints}
        for frame in range(start,end+1):
            desired=source_pose(frame)
            for i in joints:
                if names[i] not in RIGHT:
                    # RightGrip is an unchanged local child of a moving hand;
                    # compare local authoring basis rather than its world pose.
                    source_unmodified_error=max(source_unmodified_error,
                        delta(rig.pose.bones[names[i]].matrix_basis,basis[names[i]]))
                    tracks[i].append(copy.deepcopy(idle[i]));continue
                parent=desired.get(parents.get(i),idle_world.get(parents.get(i),Matrix.Identity(4)))
                local=parent.inverted()@desired[i];t,q,s=local.decompose();q.normalize()
                scale_residual=max(scale_residual,max(abs(s[k]-idle[i][2][k]) for k in range(3)))
                # Keep exact native Idle local scale; source study has no stretch.
                values=[q.x,q.y,q.z,q.w]
                if tracks[i] and sum(a*b for a,b in zip(values,tracks[i][-1][1]))<0:values=[-v for v in values]
                tracks[i].append([list(t),values,list(idle[i][2])])
        require(source_unmodified_error<0.0001 and scale_residual<0.0002,
                f'Source changes untouched joints or scale materially: basis={source_unmodified_error}, scale={scale_residual}')
        # Export-side reconstruction and independent saved-source socket references
        # are sampled at every key and midpoint. Native CPU validation is separate.
        for sample in range(2*len(times)-1):
            seconds=sample/120;desired=source_pose(start+seconds*60)
            world=global_pose(body_doc,sample_tracks(tracks,seconds,times,idle))
            error=max((world[name_to_joint[n]].translation-desired[name_to_joint[n]].translation).length
                      for n in ('RightArm','RightForeArm','RightHand','RightGrip'))
            if sample%2:native_midpoint_error=max(native_midpoint_error,error)
            else:native_key_error=max(native_key_error,error)
            flask=CI@props[1].matrix_world@C;sword=CI@props[0].matrix_world@C
            grip=rigid(world[name_to_joint['RightGrip']]);hand=rigid(world[name_to_joint['RightHand']])
            left=rigid(world[name_to_joint['LeftGrip']]);flask_grip=flask@Matrix.Translation(Vector((0,.065,0)))
            rows.append({'seconds':seconds,'sourceSeconds':seconds+source_offset,
                         'modelFromRightHand':columns(hand),'modelFromRightGrip':columns(grip),
                         'modelFromLeftGrip':columns(left),'modelFromFlask':columns(flask),
                         'modelFromFlaskGrip':columns(flask_grip),'modelFromSword':columns(sword),
                         'rightGripFromFlask':columns(grip.inverted()@flask)})
            reference.extend(struct.pack('<Iff',len(clips),seconds,seconds+source_offset))
            for name in reference_names:reference.extend(struct.pack('<16f',*columns(desired[name_to_joint[name]])))
            reference_count+=1
        require(native_key_error<0.0001,'Native key reconstruction exceeds0.1mm')
        if args.correct_native_interpolation:
            require(native_midpoint_error<0.0001,'Corrected editable derivative still disagrees with native midpoint interpolation')
        changes={names[i]:{'translationMaxNativeUnits':max(max(abs(row[0][k]-idle[i][0][k]) for k in range(3)) for row in tracks[i]),
                            'rotationMaxComponent':max(max(abs(row[1][k]-idle[i][1][k]) for k in range(4)) for row in tracks[i]),
                            'scaleMaxComponent':max(max(abs(row[2][k]-idle[i][2][k]) for k in range(3)) for row in tracks[i])}
                 for i in joints}
        summary={'name':clip_name,'durationSeconds':times[-1],'keys':len(times),'sourceOffsetSeconds':source_offset,
                 'channels':len(joints)*3,'animatedLocalJoints':list(RIGHT),
                 'allOtherJointsUseExactNativeIdleValues':True,'perJointChangeVsIdle':changes,
                 'sourceUntouchedJointMatrixMaxError':source_unmodified_error,'localScaleResidualBeforeExactPreservation':scale_residual,
                 'nativeKeyJointPositionMaxErrorM':native_key_error,'nativeMidpointJointPositionMaxErrorM':native_midpoint_error,
                 'cancelSeedSeconds':0.6 if clip_name=='FlaskCancel' else None,
                 'arbitraryPhaseCancelSupported':False}
        clips.append((clip_name,times,tracks));curves.append({'clip':clip_name,'samples':rows});summaries.append(summary)
    struct.pack_into('<I',reference,8,reference_count)
    # The cancellation seed must match the complete-use clip at exactly0.60s.
    use=global_pose(body_doc,sample_tracks(clips[0][2],.6,clips[0][1],idle))
    cancel=global_pose(body_doc,sample_tracks(clips[1][2],0,clips[1][1],idle))
    seed_error=max(delta(use[i],cancel[i]) for i in joints)
    require(seed_error<0.00001,'Fixed cancellation seed does not join complete use')
    out.mkdir(parents=True,exist_ok=False);outputs=[]
    roles=[('body',BODY,body_raw,body_doc,body_bin)]
    if not args.body_only:roles.append(('viewmodel',VIEW,view_raw,view_doc,view_bin))
    for label,path,raw,original,old_binary in roles:
        doc=copy.deepcopy(original);binary=bytearray(old_binary)
        for clip_name,times,tracks in clips:
            time_accessor=append(doc,binary,[[t] for t in times],'SCALAR')
            animation={'name':clip_name,'samplers':[],'channels':[]}
            for i in joints:
                for kind,index in (('translation',0),('rotation',1),('scale',2)):
                    output=append(doc,binary,[row[index] for row in tracks[i]],'VEC4' if kind=='rotation' else 'VEC3')
                    animation['channels'].append({'sampler':len(animation['samplers']),'target':{'node':i,'path':kind}})
                    animation['samplers'].append({'input':time_accessor,'output':output,'interpolation':'LINEAR'})
            doc['animations'].append(animation)
        require(bytes(binary[:len(old_binary)])==old_binary,'Original binary prefix changed')
        for key in ('nodes','skins','meshes','materials','images','textures','samplers','scenes','scene','extensionsUsed','extensionsRequired'):
            require(doc.get(key)==original.get(key),'Static JSON changed: '+key)
        for key in ('animations','bufferViews','accessors'):
            require(doc[key][:len(original[key])]==original[key],'Original metadata changed: '+key)
        doc['buffers'][0]['byteLength']=len(binary);encoded=json.dumps(doc,separators=(',',':'),allow_nan=False).encode();encoded+=b' '*(-len(encoded)%4)
        binary.extend(b'\0'*(-len(binary)%4))
        result=struct.pack('<III',0x46546c67,2,28+len(encoded)+len(binary))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+struct.pack('<II',len(binary),0x004e4942)+binary
        target=out/label;target.mkdir();filename=('gothic-traveller-' if label=='body' else 'gothic-traveller-viewmodel-')+'flask-candidate.runtime.glb'
        with (target/filename).open('xb') as stream:stream.write(result)
        with (target/'asset.manifest.json').open('xb') as stream:stream.write((path.parent/'asset.manifest.json').read_bytes())
        outputs.append({'role':label,'file':str((target/filename).relative_to(ROOT)).replace('\\','/'),
                        'sha256':sha(result),'bytes':len(result),'sourceSha256':sha(raw),
                        'oldBinaryPrefixBytes':len(old_binary),'oldBinaryPrefixSha256':sha(old_binary),
                        'oldAnimations':len(original['animations']),'newAnimations':len(doc['animations']),
                        'oldAccessors':len(original['accessors']),'newAccessors':len(doc['accessors']),
                        'oldBufferViews':len(original['bufferViews']),'newBufferViews':len(doc['bufferViews'])})
    editable=None
    if args.correct_native_interpolation:
        for obj in (rig,*props):obj.animation_data.action=bpy.data.actions[obj.name+'_FlaskUseStudy_ONLY']
        bpy.context.scene.frame_set(1);bpy.context.preferences.filepaths.save_version=0
        edited=out/'flask-native-linear-candidate.blend'
        bpy.ops.wm.save_as_mainfile(filepath=str(edited))
        editable={'file':edited.name,'sha256':sha(edited.read_bytes()),'bytes':edited.stat().st_size}
    receipt={'schema':1,'status':'candidate-unadmitted','blenderVersion':bpy.app.version_string,
             'scriptSha256':sha(Path(__file__).read_bytes()),'sources':{str(p.relative_to(ROOT)).replace('\\','/'):sha(raw) for p,raw in inputs.items()},
             'coordinateSystem':'+Y up/+Z forward native player model; column-major matrices; no world placement',
             'calibration':'Ci * BlenderJointWorld * per-joint native Idle frame correction; prop roots Ci * BlenderRoot * C',
             'maxIdleJointOriginErrorM':calibration,'idleMeshMaxErrorM':dict(zip(('body','viewmodel'),geometry_errors)),
             'originalStaticDataBindsScalesAndClipsPreserved':True,'clips':summaries,'outputs':outputs,
             'editableInterpolationDerivative':editable,'interpolationCorrection':interpolation_correction,
             'nativeRotationInterpolation':'Normalized shortest-path Nlerp, as implemented in SkinnedMeshAsset.cpp',
             'cancelJoinMaxMatrixError':seed_error,'nativeReference':{'file':'native-reference.bin','magic':'FNP1','version':1,'records':reference_count,
                 'recordFormat':'uint32 logical clip0/1, float seconds, float sourceSeconds, then five column-major float[16] raw joint matrices',
                 'nodeOrder':list(reference_names)},
             'nativeAcceptance':'Pending actual native CPU importer/hand binding checks; no GPU, gameplay, cancellation-generalization or owner acceptance',
             'minimalConsumerInterface':'Explicit one-shot FlaskUse/FlaskCancel name bindings and utility time; right-arm pose/Grip ownership; shared body/viewmodel solved palette; preserve independent left-light solve; fixed Cancel seed=.60 only'}
    for name,value in (('export-receipt.json',receipt),('native-socket-curves.json',{'schema':1,'clips':curves})):
        with (out/name).open('x') as stream:json.dump(value,stream,indent=2,allow_nan=False)
    with (out/'native-reference.bin').open('xb') as stream:stream.write(reference)
    for path,raw in inputs.items():require(path.read_bytes()==raw,'Input changed during export: '+str(path))
    print('FLASK_NATIVE_EXPORT',json.dumps({'outputs':outputs,'clips':[{k:v for k,v in s.items() if k!='perJointChangeVsIdle'} for s in summaries]}),flush=True)

if __name__=='__main__':main()
