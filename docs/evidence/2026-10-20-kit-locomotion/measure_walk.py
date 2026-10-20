"""Read-only Warden clip analysis; output only into this ignored reports folder.

Mirrors SkinnedMeshAsset Nlerp/TRS/parent*local/palette rules in Python double
precision. This is an independent offline estimate, not native runtime evidence.
"""
import argparse
import bisect
import csv
import hashlib
import json
import math
import pathlib
import statistics
import struct

IDENTITY = [1.,0.,0.,0., 0.,1.,0.,0., 0.,0.,1.,0., 0.,0.,0.,1.]

def multiply(a, b):
    return [sum(a[k*4+r]*b[c*4+k] for k in range(4)) for c in range(4) for r in range(4)]

def point(m, p):
    return [sum(m[k*4+r]*p[k] for k in range(3))+m[12+r] for r in range(3)]

def nlerp(a, b, fraction):
    if sum(x*y for x,y in zip(a,b)) < 0: b = [-x for x in b]
    v = [x+(y-x)*fraction for x,y in zip(a,b)]
    length = math.sqrt(sum(x*x for x in v))
    return [x/length for x in v] if length > .00001 else v

def local(t, q, s):
    x,y,z,w = q
    x2,y2,z2 = 2*x,2*y,2*z
    xx,xy,xz,yy,yz,zz,wx,wy,wz = x*x2,x*y2,x*z2,y*y2,y*z2,z*z2,w*x2,w*y2,w*z2
    return [(1-(yy+zz))*s[0],(xy+wz)*s[0],(xz-wy)*s[0],0.,
            (xy-wz)*s[1],(1-(xx+zz))*s[1],(yz+wx)*s[1],0.,
            (xz+wy)*s[2],(yz-wx)*s[2],(1-(xx+yy))*s[2],0., *t,1.]

class Rig:
    def __init__(self, path):
        self.path = path
        data = path.read_bytes()
        if data[:4] != b'glTF' or struct.unpack_from('<I',data,4)[0] != 2: raise ValueError('GLB2 required')
        length = struct.unpack_from('<I',data,12)[0]
        self.g = json.loads(data[20:20+length])
        self.binary = data[28+length:]
        self.sha256 = hashlib.sha256(data).hexdigest()
        self.nodes = self.g['nodes']
        self.parents = {child:i for i,n in enumerate(self.nodes) for child in n.get('children',[])}
        self.names = {n.get('name'):i for i,n in enumerate(self.nodes)}
        self.joints = self.g['skins'][0]['joints']
        self.inverse = self.accessor(self.g['skins'][0]['inverseBindMatrices'])
        self.clips = {}
        for animation in self.g['animations']:
            channels = []
            for ch in animation['channels']:
                sampler = animation['samplers'][ch['sampler']]
                # The current native reader ignores interpolation labels and
                # always interpolates TRS with linear/Nlerp, including STEP.
                if sampler.get('interpolation','LINEAR') not in ('LINEAR','STEP'): raise ValueError('unsupported interpolation')
                channels.append((ch['target']['node'],ch['target']['path'],
                                 [x[0] for x in self.accessor(sampler['input'])],self.accessor(sampler['output'])))
            self.clips[animation['name']] = channels
        primitive = self.g['meshes'][0]['primitives'][0]
        self.positions = self.accessor(primitive['attributes']['POSITION'])
        self.vertex_joints = self.accessor(primitive['attributes']['JOINTS_0'])
        self.weights = self.accessor(primitive['attributes']['WEIGHTS_0'])
        self.boots = {}
        for side in ('Left','Right'):
            leg = {i for i,n in enumerate(self.joints) if self.nodes[n]['name'] in
                   {side+'UpLeg',side+'Leg',side+'Foot',side+'ToeBase'}}
            subset = []
            for index,(p,j,w) in enumerate(zip(self.positions,self.vertex_joints,self.weights)):
                influences = [(int(joint),weight) for joint,weight in zip(j,w) if weight > .00001]
                if p[1] < .25 and influences and all(joint in leg for joint,weight in influences):
                    subset.append((index,p,influences))
            if not subset: raise ValueError('empty semantic boot subset '+side)
            self.boots[side] = subset

    def accessor(self, index):
        a = self.g['accessors'][index]
        if 'sparse' in a: raise ValueError('sparse unsupported')
        v = self.g['bufferViews'][a['bufferView']]
        component = {5121:'B',5123:'H',5125:'I',5126:'f'}[a['componentType']]
        count = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']]
        fmt = '<'+component*count
        size = struct.calcsize(fmt)
        offset = v.get('byteOffset',0)+a.get('byteOffset',0)
        stride = v.get('byteStride',size)
        return [struct.unpack_from(fmt,self.binary,offset+i*stride) for i in range(a['count'])]

    def duration(self, clip):
        return max(max(times) for _,_,times,_ in self.clips[clip])

    def pose(self, clip, time):
        duration = self.duration(clip)
        time = max(time,0) % duration
        pose = [[n.get('translation',[0.,0.,0.]),n.get('rotation',[0.,0.,0.,1.]),n.get('scale',[1.,1.,1.])]
                for n in self.nodes]
        for node,path,times,values in self.clips[clip]:
            nxt = bisect.bisect_right(times,time)
            if nxt == len(times): nxt = 0
            previous = len(times)-1 if nxt == 0 else nxt-1
            next_time = duration if nxt == 0 else times[nxt]
            sampled = time+duration if nxt == 0 and time < times[previous] else time
            fraction = max(0.,min(1.,(sampled-times[previous])/(next_time-times[previous]))) if next_time > times[previous] else 0.
            a,b = values[previous],values[nxt]
            pose[node][{'translation':0,'rotation':1,'scale':2}[path]] = nlerp(a,b,fraction) if path == 'rotation' else [x+(y-x)*fraction for x,y in zip(a,b)]
        globals_ = {}
        def world(index):
            if index not in globals_:
                matrix = local(*pose[index])
                globals_[index] = multiply(world(self.parents[index]),matrix) if index in self.parents else matrix
            return globals_[index]
        for i in range(len(self.nodes)): world(i)
        palette = [multiply(globals_[j],inv) for j,inv in zip(self.joints,self.inverse)]
        samples = {name:globals_[self.names[name]][12:15] for name in
                   ('Hips','Armature','LeftFoot','LeftToeBase','RightFoot','RightToeBase')}
        for side,subset in self.boots.items():
            points = []
            for _,p,influences in subset:
                xyz = [0.,0.,0.]
                for joint,weight in influences:
                    transformed = point(palette[joint],p)
                    xyz = [x+weight*y for x,y in zip(xyz,transformed)]
                points.append(xyz)
            samples[side+'SoleMinimumY'] = min(p[1] for p in points)
            samples[side+'BootCentroid'] = [statistics.fmean(p[axis] for p in points) for axis in range(3)]
        return samples

def slope(times, positions):
    tm,pm = statistics.fmean(times),statistics.fmean(positions)
    return sum((t-tm)*(p-pm) for t,p in zip(times,positions))/sum((t-tm)**2 for t in times)

def contacts(samples, duration, clearance, route_speed):
    n = len(samples)
    output = {}
    for side in ('Left','Right'):
        mask = [s[side+'SoleMinimumY']-min(s['LeftSoleMinimumY'],s['RightSoleMinimumY']) <= clearance for s in samples]
        intervals = []
        for start in range(n):
            if not mask[start] or mask[(start-1)%n]: continue
            count = 1
            while count < n and mask[(start+count)%n]: count += 1
            if count < 4: continue
            times = [(start+i)*duration/n for i in range(count)]
            # Trim first/last 15% of a stance to avoid heel/toe roll when fitting
            # the in-place rearward boot-centroid travel. Full intervals retained.
            trim = max(1,count*15//100)
            fit = range(trim,count-trim)
            fit_times = [times[i] for i in fit]
            z = [samples[(start+i)%n][side+'BootCentroid'][2] for i in fit]
            natural_speed = -slope(fit_times,z)
            residual = [p-(statistics.fmean(z)-natural_speed*(t-statistics.fmean(fit_times))) for t,p in zip(fit_times,z)]
            intervals.append({'start_seconds':times[0],'end_seconds':times[-1]+duration/n,
                              'wraps_cycle':start+count>n,'fraction':count/n,'fit_samples':len(z),
                              'rearward_travel_fit_metres':z[0]-z[-1],
                              'natural_speed_mps':natural_speed,
                              'fit_rms_metres':math.sqrt(statistics.fmean(x*x for x in residual)),
                              'current_world_slide_mps':route_speed-natural_speed})
        output[side] = {'contact_fraction':sum(mask)/n,'intervals':intervals,
                        'boot_centroid_z_range':max(s[side+'BootCentroid'][2] for s in samples)-min(s[side+'BootCentroid'][2] for s in samples),
                        'minimum_sole_height_range':max(s[side+'SoleMinimumY'] for s in samples)-min(s[side+'SoleMinimumY'] for s in samples),
                        'maximum_lift_above_lowest_boot':max(s[side+'SoleMinimumY']-min(s['LeftSoleMinimumY'],s['RightSoleMinimumY']) for s in samples)}
    speeds = [i['natural_speed_mps'] for side in output.values() for i in side['intervals'] if i['natural_speed_mps'] > 0]
    if speeds:
        speed = statistics.median(speeds)
        output['aggregate'] = {'natural_speed_mps':speed,'natural_stride_metres_per_cycle':speed*duration,
                               'suggested_visual_playback_multiplier':route_speed/speed,
                               'current_steps_per_minute':120/duration,
                               'matched_steps_per_minute':120/duration*route_speed/speed}
    return output

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--glb',type=pathlib.Path,required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    parser.add_argument('--samples',type=int,default=240)
    parser.add_argument('--route-speed',type=float,default=1.8)
    args = parser.parse_args()
    if not 60 <= args.samples <= 480: raise ValueError('bounded 60..480 samples required')
    root = pathlib.Path(__file__).resolve().parents[2]
    if args.output.resolve().parent != pathlib.Path(__file__).resolve().parent: raise ValueError('output must stay in this reports folder')
    rig = Rig(args.glb)
    clip = 'Warden|walk'
    duration = rig.duration(clip)
    samples = [rig.pose(clip,i*duration/args.samples) for i in range(args.samples)]
    idle = rig.pose('Warden|idle',0)
    first = samples[0]
    ranges = {name:{'range_xyz':[max(s[name][a] for s in samples)-min(s[name][a] for s in samples) for a in range(3)],
                   'first_xyz':first[name],'last_sample_xyz':samples[-1][name]}
              for name in ('Hips','Armature','LeftFoot','LeftToeBase','RightFoot','RightToeBase')}
    transition = {name:{'idle_zero_xyz':idle[name],'walk_zero_xyz':first[name],
                        'jump_metres':math.dist(idle[name],first[name])}
                  for name in ('Hips','LeftFoot','LeftToeBase','RightFoot','RightToeBase')}
    channels = []
    for node,path,times,values in rig.clips[clip]:
        if rig.nodes[node]['name'] in ('Hips','Armature'):
            channels.append({'node':rig.nodes[node]['name'],'path':path,'keys':len(times),
                             'start':times[0],'end':times[-1],
                             'component_ranges':[max(x[a] for x in values)-min(x[a] for x in values) for a in range(len(values[0]))]})
    result = {'schema':1,'evidence_class':'offline GLB analysis reproducing native interpolation in Python double precision; no game or RT execution',
              'head':'d2d24e3d','glb':str(args.glb),'sha256':rig.sha256,'clip':clip,'duration_seconds':duration,
              'samples_per_cycle':args.samples,'route_speed_mps':args.route_speed,
              'boot_subset_rule':'bind y<.25m; all nontrivial weights in side UpLeg/Leg/Foot/ToeBase; no other branch',
              'boot_subset_counts':{side:len(x) for side,x in rig.boots.items()},'node_ranges':ranges,
              'root_channels':channels,'idle_zero_to_walk_zero':transition,
              'stance_clearance_sensitivity':{str(mm):contacts(samples,duration,mm/1000,args.route_speed) for mm in (10,20,30)},
              'limitations':['Sole clearance is measured relative to the lowest semantic boot subset, not terrain contact.',
                             'Per-pose native all-vertex floor repack changes Y only; native output not executed or byte-validated here.',
                             'Centroid stance fit is an estimate affected by foot roll and chosen 10/20/30mm threshold.',
                             'Cadence alone does not prove foot locking, turns, transitions, variable terrain, visual quality or performance.']}
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
    with args.output.with_suffix('.csv').open('w',newline='',encoding='utf8') as f:
        writer = csv.writer(f)
        writer.writerow(['seconds','left_min_y','right_min_y','left_boot_z','right_boot_z','hip_x','hip_y','hip_z'])
        for i,s in enumerate(samples):writer.writerow([i*duration/args.samples,s['LeftSoleMinimumY'],s['RightSoleMinimumY'],
            s['LeftBootCentroid'][2],s['RightBootCentroid'][2],*s['Hips']])
    print(json.dumps({'duration':duration,'samples':args.samples,'boot_vertices':result['boot_subset_counts'],
                      'contacts_20mm':result['stance_clearance_sensitivity']['20'],'root':ranges['Hips'],
                      'transitions':transition},indent=2))

if __name__ == '__main__':main()
