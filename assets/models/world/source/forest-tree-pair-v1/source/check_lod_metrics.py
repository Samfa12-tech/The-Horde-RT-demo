"""Quantify existing rendered masks and construction topology; does not edit images."""
import json,sys,math
from pathlib import Path
import numpy as np
from PIL import Image
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parent.parent
reports=[]
for tree in ['horde-upright-alder-v1','horde-irregular-pine-v1']:
 trace=json.loads((root/'qa'/f'{tree}-centerlines.json').read_text()); lodtrace=json.loads((root/'qa'/f'{tree}-lod1-centerlines.json').read_text())
 prev=[]; worst=-1e9; floating=[]; checked=0
 for item in trace:
  if item['kind']!='tube' or 'trunk roots branches' not in item['mesh']: continue
  if prev:
   start=np.array(item['points'][0]); best=1e9
   for path in prev:
    for i,(aa,bb) in enumerate(zip(path['points'],path['points'][1:])):
     aa,bb=np.array(aa),np.array(bb); d=bb-aa; t=np.clip(np.dot(start-aa,d)/max(np.dot(d,d),1e-20),0,1); nearest=aa+t*d; radius=path['radii'][i]*(1-t)+path['radii'][i+1]*t
     clearance=np.linalg.norm(start-nearest)-radius*.7 # conservative inscribed cross-section
     best=min(best,clearance)
   worst=max(worst,best); checked+=1
   if best>1e-5: floating.append({'branch_index':checked,'clearance_m':float(best)})
  prev.append(item)
 views=[]
 for angle in ['front','back']:
  aa=np.asarray(Image.open(root/'qa'/f'{tree}-{angle}-silhouette.png').convert('RGBA'))[:,:,3]>127
  bb=np.asarray(Image.open(root/'qa'/f'{tree}-lod1-{angle}-silhouette.png').convert('RGBA'))[:,:,3]>127
  iou=float(np.logical_and(aa,bb).sum()/np.logical_or(aa,bb).sum()); coverage=float(bb.sum()/aa.sum())
  crown_a=aa[:int(aa.shape[0]*.64)]; crown_b=bb[:int(bb.shape[0]*.64)]; ciou=float(np.logical_and(crown_a,crown_b).sum()/np.logical_or(crown_a,crown_b).sum())
  views.append({'view':angle,'full_silhouette_iou':iou,'crown_silhouette_iou':ciou,'lod_to_full_coverage_ratio':coverage,'mask_resolution':[512,600]})
 report={'asset':tree,'centerlines_and_leaf_placements_identical':trace==lodtrace,'tube_count':sum(x['kind']=='tube' for x in trace),'leaf_count':sum(x['kind']=='leaf' for x in trace),'wood_branch_attachment_starts_checked':checked,'wood_attachment_clearance_conservative_worst_m':float(worst),'detached_wood_starts':floating,'silhouette_views':views,'acceptance_thresholds':{'full_silhouette_iou':.85,'crown_silhouette_iou':.80},'pass':trace==lodtrace and not floating and all(v['full_silhouette_iou']>=.85 and v['crown_silhouette_iou']>=.80 for v in views),'notes':'Alpha masks compare actual exported geometry at identical front/back cameras. Tube centerlines, radii and leaf poses are unchanged. Surface side/perimeter counts differ.'}
 reports.append(report)
(root/'qa'/'lod-comparison.json').write_text(json.dumps(reports,indent=2)); print(json.dumps(reports,indent=2))
