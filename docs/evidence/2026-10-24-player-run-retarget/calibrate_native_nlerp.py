from pathlib import Path
import importlib.util,json,struct,statistics,hashlib,math,sys,types
sys.dont_write_bytecode=True
ROOT=Path.cwd();OUT=Path(__file__).resolve().parent
helper=ROOT/'docs/evidence/2026-10-20-kit-locomotion/measure_walk.py'
source=helper.read_text()
old="""        primitive = self.g['meshes'][0]['primitives'][0]
        self.positions = self.accessor(primitive['attributes']['POSITION'])
        self.vertex_joints = self.accessor(primitive['attributes']['JOINTS_0'])
        self.weights = self.accessor(primitive['attributes']['WEIGHTS_0'])"""
new="""        # The player has five semantic groups; Kit had one. Keep every
        # original group instead of measuring the player's chest-only group.
        self.positions=[]; self.vertex_joints=[]; self.weights=[]
        for primitive in self.g['meshes'][0]['primitives']:
            self.positions.extend(self.accessor(primitive['attributes']['POSITION']))
            self.vertex_joints.extend(self.accessor(primitive['attributes']['JOINTS_0']))
            self.weights.extend(self.accessor(primitive['attributes']['WEIGHTS_0']))"""
assert source.count(old)==1
m=types.ModuleType('player_contact_helper');m.__file__=str(helper);exec(source.replace(old,new),m.__dict__)
def analyze(file,clip):
 rig=m.Rig(file);duration=rig.duration(clip);samples=[rig.pose(clip,i*duration/120) for i in range(120)]
 return dict(duration=duration,contact={str(mm):m.contacts(samples,duration,mm/1000,3.2) for mm in (10,20,30)},
  rootRanges={n:[max(s[n][i] for s in samples)-min(s[n][i] for s in samples) for i in range(3)] for n in ('Hips','Armature')},
  rawSoleRange={side:[min(s[side+'SoleMinimumY'] for s in samples),max(s[side+'SoleMinimumY'] for s in samples)] for side in ('Left','Right')},
  bootCounts={s:len(v) for s,v in rig.boots.items()})
control=ROOT/'assets/models/player/runtime/gothic-traveller-lod0.runtime.glb'
candidate=OUT/'player-run-native-nlerp-proof.glb'
results={'evidenceClass':'offline native-rule pose/contact proxy, not a runtime or RT pass','samplesPerClip':120,'motorRunMetresPerSecond':3.2,'distanceClockBaseMetresPerSecond':1.9,'originalWalk':analyze(control,'Walking'),'unscaledRetarget':analyze(candidate,'Walking')}
stride=results['unscaledRetarget']['contact']['20']['aggregate']['natural_stride_metres_per_cycle']
new_duration=stride/1.9;assert .5<new_duration<2
data=candidate.read_bytes();n=struct.unpack_from('<I',data,12)[0];d=json.loads(data[20:20+n]);binary=bytearray(data[28+n:])
clip=next(a for a in d['animations'] if a['name']=='Walking');accessors=set(s['input'] for s in clip['samplers']);assert len(accessors)==1
i=next(iter(accessors));a=d['accessors'][i];v=d['bufferViews'][a['bufferView']];offset=v.get('byteOffset',0)+a.get('byteOffset',0)
old_duration=results['unscaledRetarget']['duration']
for j in range(a['count']):
 old=struct.unpack_from('<f',binary,offset+j*4)[0];struct.pack_into('<f',binary,offset+j*4,old/old_duration*new_duration)
a['max']=[new_duration]
js=json.dumps(d,separators=(',',':')).encode();js+=b' '*((-len(js))%4)
out=OUT/'player-run-native-calibrated-proof.glb';assert not out.exists()
out.write_bytes(struct.pack('<III',0x46546c67,2,28+len(js)+len(binary))+struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(binary),0x004e4942)+binary)
results['calibratedRetarget']=analyze(out,'Walking')
results['calibration']=dict(fittedCycleDistanceMetres=stride,diagnosticClipDurationSeconds=new_duration,
 wallSecondsPerCycleAtRun= stride/3.2,sourcePlaybackMultiplier=old_duration/(stride/3.2),
 runtimeSampleAdvanceAtRun=3.2/1.9,
 note='Choose test-clip period for the existing distance clock; no game speed/phase implementation change. 20mm is the retained measurement window, not a new pass tolerance.')
results['staticPrefixUnchanged']=binary[:2516028]==control.read_bytes()[28+struct.unpack_from('<I',control.read_bytes(),12)[0]:]
assert results['staticPrefixUnchanged']
results['calibratedProofSha256']=hashlib.sha256(out.read_bytes()).hexdigest()
(OUT/'calibration-native-nlerp.json').write_text(json.dumps(results,indent=2)+'\n')
for label in ('originalWalk','unscaledRetarget','calibratedRetarget'):
 r=results[label];print(label,'duration',r['duration'],'contact20',r['contact']['20'].get('aggregate'),'roots',r['rootRanges'],'sole',r['rawSoleRange'],flush=True)
print('CALIBRATION',results['calibration'],flush=True)
