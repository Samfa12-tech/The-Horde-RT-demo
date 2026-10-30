import hashlib,json,math,sys
from pathlib import Path
from PIL import Image,ImageChops,ImageStat,ImageDraw
root=Path.cwd(); family='owner-view' if '--owner-view' in sys.argv else 'quality'; private=root/'reports/frozen-mist'/family/'captures'; public=root/'docs/evidence/2026-10-10-frozen-mist-attribution'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def image(name):return Image.open(private/name/'192-rescue-journey-start.png').convert('RGB')
def difference(a,b):
 aa=image(a);bb=image(b);dd=ImageChops.difference(aa,bb)
 count=bright=dark=equal_luma=0
 for pa,pb in zip(aa.getdata(),bb.getdata()):
  if pa==pb:continue
  count+=1; delta=sum(w*(y-x) for w,x,y in zip([.2126,.7152,.0722],pa,pb))
  if delta>0:bright+=1
  elif delta<0:dark+=1
  else:equal_luma+=1
 return dict(pair=[a,b],changed_pixels=count,second_brighter=bright,second_darker=dark,equal_luma_different_RGB=equal_luma,extrema=dd.getextrema(),mean_abs_rgb=ImageStat.Stat(dd).mean)
rows=[]
for q in ['current','higher']:
 for pair in [('midpoint-all','midpoint-sky'),('sample-local-all','sample-local-sky'),('midpoint-lantern','sample-local-lantern'),('midpoint-all','sample-local-all')]:
  rows.append(difference(q+'-'+pair[0],q+'-'+pair[1]))
for mode in ['midpoint-all','midpoint-sky','midpoint-lantern','sample-local-all','sample-local-sky','sample-local-lantern']:
 rows.append(difference('current-'+mode,'higher-'+mode))
frames=[]
expected_state=sha(private/'current-midpoint-all/frozen-state.json')
for p in sorted(private.iterdir()):
 if not p.is_dir():continue
 run=json.loads((p/'run.json').read_text());f=json.loads((p/'completed-frame.json').read_text())
 assert run['exit_code']==1 and f['presentation']['presented'] and f['dispatch']['rtDispatchRecorded']
 assert sha(p/'frozen-state.json')==expected_state
 frames.append(dict(name=p.name,shadow=f['shadowQuality'],shader=f['pipeline']['activeSha256'],exe=run['exe_sha256']))
assert len(frames)==12 and len(set(f['exe'] for f in frames))==1
result=dict(source='f2a095fcf85520d63aebc1a48547cf4a453e97ba',family=family,frame_sha256=expected_state,frames=frames,comparisons=rows,caveat='Displayed sRGB differences, not a physical shadow-strength metric. CURRENT alternates aperture source by parity; HIGHER uses source 0. Surface shading retained in every diagnostic. Owner-view is an approximation, not exact screenshot recovery.')
(public/(family+'-comparison.json')).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
names=['current-midpoint-all','current-sample-local-all','higher-midpoint-all','higher-sample-local-all']
sheet=Image.new('RGB',(1232*2,803*2+70),(18,18,18));draw=ImageDraw.Draw(sheet)
for i,name in enumerate(names):
 x=(i%2)*1232;y=(i//2)*838;draw.text((x+10,y+8),name,fill='white');sheet.paste(image(name),(x,y+35))
sheet.save(public/('comparison/'+family+'-current-higher.png'))
print(json.dumps([r for r in rows if r['pair'][0].endswith('midpoint-all')],indent=2))
