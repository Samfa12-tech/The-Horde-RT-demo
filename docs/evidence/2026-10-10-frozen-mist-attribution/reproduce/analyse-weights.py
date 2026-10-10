import json,struct,hashlib,math
from pathlib import Path
from PIL import Image,ImageDraw
root=Path.cwd();private=root/'reports/frozen-mist/owner-weights/captures';public=root/'docs/evidence/2026-10-10-frozen-mist-attribution'
def image(name,family='owner-weights'):return Image.open(root/'reports/frozen-mist'/family/'captures'/name/'192-rescue-journey-start.png').convert('RGBA')
def scalar(name,family='owner-weights'):
 values=[x[0] for x in struct.iter_unpack('<f',image(name,family).tobytes('raw','BGRA'))]
 assert all(math.isfinite(v) and (v==-1 or -0.0000002<=v<=1.0000002) for v in values)
 assert -1.0 in values, 'Expected exact no-medium sentinel confirms float32 channel decoding'
 return values
def stats(v):
 a=[x for x in v if x!=-1]
 return dict(active_pixels=len(a),minimum=min(a),maximum=max(a),mean=sum(a)/len(a),sum=sum(a),negative_roundoff_pixels=sum(x<0 for x in a),over_one_roundoff_pixels=sum(x>1 for x in a))
results=[]
for q in ['current','higher']:
 mid=scalar(q+'-midpoint-loss');local=scalar(q+'-sample-local-loss');rope=scalar(q+'-sample-local-rope-loss')
 midrope=scalar(q+'-midpoint-rope-loss','owner-midpoint-rope')
 masks=list(image(q+'-sample-local-masks').getdata());dims=image(q+'-sample-local-masks').size
 active=[i for i,x in enumerate(mid) if x!=-1]
 assert all((local[i]!=-1)==(mid[i]!=-1) and (rope[i]!=-1)==(mid[i]!=-1) for i in range(len(mid)))
 assert all(rope[i]<=local[i]+1e-6 for i in active)
 assert all(r==6 or r==0 for r,g,b,a in masks), 'Actual Authored density sample count'
 # PNG R=count,G=all committed opaque blockers,B=rope blockers, after native BGRA normalization.
 assert all((b&~g)==0 and b<=63 and g<=63 for r,g,b,a in masks)
 positions=[i for i in active if rope[i]>0]
 bounds=[min(i%dims[0] for i in positions),min(i//dims[0] for i in positions),max(i%dims[0] for i in positions),max(i//dims[0] for i in positions)]
 weighted_more=[i for i in active if local[i]>mid[i]+1e-6]
 weighted_less=[i for i in active if local[i]<mid[i]-1e-6]
 new_shadow=[i for i in active if mid[i]<=1e-6 and local[i]>1e-6]
 missed_rope=[i for i in active if mid[i]<=1e-6 and rope[i]>1e-6]
 both=[i for i in positions if mid[i]>0]
 midpositions=[i for i in active if midrope[i]>1e-6]
 overlap=[i for i in midpositions if rope[i]>1e-6]
 valid=[rope[i] for i in positions]
 rows=dict(shadow=q,midpoint=stats(mid),sample_local=stats(local),midpoint_rope=stats(midrope),sample_local_rope=stats(rope),local_more_shadow_pixels=len(weighted_more),local_less_shadow_pixels=len(weighted_less),midpoint_clear_local_shadow_pixels=len(new_shadow),midpoint_clear_local_rope_shadow_pixels=len(missed_rope),retained_local_rope_shadow_pixels=len(positions),retained_rope_with_midpoint_shadow_pixels=len(both),midpoint_rope_shadow_pixels=len(midpositions),midpoint_rope_without_local_rope_pixels=len(midpositions)-len(overlap),midpoint_and_local_rope_pixels=len(overlap),local_rope_without_midpoint_rope_pixels=len(positions)-len(overlap),valid_rope_loss_mean=sum(valid)/len(valid),valid_rope_loss_min=min(valid),valid_rope_loss_max=max(valid),rope_density_sample_intersections=sum(b.bit_count() for r,g,b,a in masks),all_opaque_density_sample_intersections=sum(g.bit_count() for r,g,b,a in masks),local_rope_bounds_inclusive=bounds)
 results.append(rows)
 # Evidence display only; maps are exact float32 payloads, these images are labelled linear grey visualizations.
 sheet=Image.new('RGB',(dims[0]*3,dims[1]+35),(18,18,18));draw=ImageDraw.Draw(sheet)
 for column,(name,values) in enumerate([('Midpoint rope-only loss',midrope),('Sample-local total mist shadow loss',local),('Sample-local rope-only retained loss',rope)]):
  grey=Image.new('RGB',dims)
  grey.putdata([(0,0,0) if x==-1 else (round(min(1,max(0,x))*255),)*3 for x in values])
  sheet.paste(grey,(column*dims[0],35));draw.text((column*dims[0]+8,8),q.upper()+': '+name,fill='white')
 sheet.save(public/'comparison'/('weighted-'+q+'.png'))
result=dict(encoding='Exact little-endian IEEE754 float32: decoded PNG BGRA bytes; -1 means no positive integrated raw sky contribution. Mask map R=density sample count,G=committed opaque mask,B=rope mask.',formula='loss = sum(T_before * density * 0.82 * ds * luminance(rawSky-visibleSky)) / sum(T_before * density * 0.82 * ds * luminance(rawSky)); transmittance evolves by original exp(-density*ds). RGB luminance weights 0.2126/0.7152/0.0722.',precision='float32 GPU accumulation, exact bit transport, CPU double aggregate. 1e-6 only classifies numerical equality in the diagnostic report, not an acceptance tolerance.',scope='Existing six density samples and physical helper/mask. Proves discrete sampled blockage, not a continuous-volume convergence reference or corrected production shader.',results=results)
(public/'weighted-shadow-metrics.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(results,indent=2))
