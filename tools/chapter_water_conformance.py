"""Bounded source-only terrain/water planar constraint overlay.

Preserve all original vertices and unaffected triangles. Only admitted outer
triangles and required edge neighbours are split. Ordinary inserts use the
unchanged regional heightfield. Deficient NEW five-metre bank crests have the
explicit bounded continuous-profile exception documented by bank_crest_heights.
No Blender, exporter, runtime dependency or filesystem mutation.
"""
import math
import numpy as np

EPS=1.e-7


def cross(a,b,c):
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])


def area(polygon):
    return sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(polygon,polygon[1:]+polygon[:1]))*.5


def clean(polygon):
    result=[]
    for point in polygon:
        if not result or math.dist(point,result[-1])>EPS:result.append(point)
    if len(result)>1 and math.dist(result[0],result[-1])<=EPS:result.pop()
    return result if len(result)>=3 and abs(area(result))>EPS else []


def triangulate(polygon):
    """Ear clipping retains collinear seam vertices; no skipped edge samples."""
    polygon=clean(list(polygon))
    if area(polygon)<0:polygon.reverse()
    result=[]
    while len(polygon)>3:
        for i,b in enumerate(polygon):
            a,c=polygon[i-1],polygon[(i+1)%len(polygon)]
            if cross(a,b,c)<=EPS:continue
            if any(cross(a,b,p)>=-EPS and cross(b,c,p)>=-EPS and cross(c,a,p)>=-EPS
                   for j,p in enumerate(polygon) if j not in ((i-1)%len(polygon),i,(i+1)%len(polygon))):continue
            result.append((a,b,c));polygon.pop(i);break
        else:raise ValueError('Water constraint polygon cannot be triangulated without dropping a seam')
    if len(polygon)==3 and area(polygon)>EPS:result.append(tuple(polygon))
    return result


def ribbon_geometry(regional,half_width):
    """Source X/Z constraints, crest edges and the renderer's 3D diagonal."""
    engine=lambda p:(float(p[0])+78.3,126.2-float(p[1]))
    polygons=[];edges=[];triangles=[]
    for river in regional.river_water:
        points=np.asarray(river,dtype=float)
        directions=np.diff(points[:,:2],axis=0)
        normals=np.stack((-directions[:,1],directions[:,0]),axis=1)
        normals/=np.linalg.norm(normals,axis=1)[:,None]
        left=[];right=[]
        for i,p in enumerate(points):
            before,after=normals[max(0,i-1)],normals[min(i,len(normals)-1)]
            m=before+after;m*=half_width/np.dot(m,before)
            x,z=engine(p[:2]+m);left.append((x,float(p[2])-31.55,z))
            x,z=engine(p[:2]-m);right.append((x,float(p[2])-31.55,z))
        # Separate quads match the renderer's shared miter vertices at bends.
        for i in range(len(points)-1):
            polygons.append([(p[0],p[2]) for p in (left[i],right[i],right[i+1],left[i+1])])
            edges.extend(((left[i],left[i+1]),(right[i],right[i+1])))
            # Our atlas normal is opposite the C++ engine normal, so these
            # use C++ left=right here and retain its exact shared diagonal.
            triangles.extend(((right[i],left[i+1],left[i]),(right[i],right[i+1],left[i+1])))
    return [p if area(p)>0 else p[::-1] for p in polygons],edges,triangles


def water_footprints(regional,half_width=3.):
    """Exact atlas lake and mitered ribbons, in engine X/Z."""
    lake=[(float(p[0])+78.3,126.2-float(p[1])) for p in regional.lake_outline]
    return [lake if area(lake)>0 else lake[::-1]]+ribbon_geometry(regional,half_width)[0]


def bank_crest_heights(points,old_heights,regional):
    """Raise only deficient newly inserted five-metre longitudinal crests.

    Preserve dry bilinear samples (>=1cm above actual widened ribbon faces).
    Otherwise use max(old, continuous unblended carve profile). Positive delta
    is capped at .50m, except TWO approved outflow endcap samples <=3.3m beyond
    the source grid, capped at1.25m. No original vertex or grid value changes.
    """
    from chapter_regional_terrain import nearest
    _,edges,triangles=ribbon_geometry(regional,5.)
    # Include the joined lake as well; a lower outflow face must not hide a
    # higher lake datum when checking the actual finite surface union.
    lake=water_footprints(regional,5.)[0]
    triangles.extend(tuple((x,-31.55,z) for x,z in face) for face in triangulate(lake))
    ids=[i for i,p in enumerate(points) if any(on_segment(tuple(p),(a[0],a[2]),(b[0],b[2])) for a,b in edges)]
    result=np.asarray(old_heights,dtype=float).copy();changes=[]
    if not ids:return result,dict(crestVertices=0,raisedVertices=0,maximumPositiveDelta=0.,minimumClearance=None,repairs=[])
    p=np.asarray(points)[ids];water=np.full(len(p),-np.inf)
    for triangle in triangles:
        a,b,c=np.asarray(triangle);den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
        u=((b[2]-c[2])*(p[:,0]-c[0])+(c[0]-b[0])*(p[:,1]-c[2]))/den
        v=((c[2]-a[2])*(p[:,0]-c[0])+(a[0]-c[0])*(p[:,1]-c[2]))/den
        inside=(u>=-1e-6)&(v>=-1e-6)&(u+v<=1.000001)
        water=np.where(inside,np.maximum(water,u*a[1]+v*b[1]+(1-u-v)*c[1]),water)
    distances=[];elevations=[]
    for river in regional.river_water:
        d,e=nearest(p[:,0]-78.3,126.2-p[:,1],river);distances.append(d);elevations.append(e)
    distances=np.asarray(distances);elevations=np.asarray(elevations)
    selected=np.argmin(distances,axis=0);rows=np.arange(len(p))
    d=distances[selected,rows];up=elevations[selected,rows]
    analytic=up-1+np.maximum(d-3,0)*.6-31.55
    allowed_endcaps=((-50.935615498,-512.936441114),(-51.178542583,-513.224354696))
    clearances=[]
    for j,i in enumerate(ids):
        old=float(old_heights[i]);datum=float(water[j])
        if not math.isfinite(datum):raise ValueError('Bank crest has no actual widened water face')
        if old-datum>=.01:new=old
        else:new=max(old,float(analytic[j]))
        delta=new-old;east=float(p[j,0]-78.3);north=float(126.2-p[j,1])
        terminal=any(math.hypot(east-x,north-y)<=1e-5 for x,y in allowed_endcaps)
        cap=1.25 if terminal else .50
        if delta<0 or delta>cap+1e-9:raise ValueError(f'Bank positive correction exceeds {cap}m at EN({east},{north}): {delta}')
        if new-datum<.01:raise ValueError(f'Bank crest cannot clear actual water by1cm at EN({east},{north})')
        result[i]=new;clearances.append(new-datum)
        if delta>0:changes.append(dict(insertedIndex=int(i),atlasEN=[east,north],oldBilinearY=old,newAnalyticY=new,positiveDelta=delta,actualWaterY=datum,clearance=new-datum,approvedOutflowEndcap=terminal))
    return result,dict(crestVertices=len(ids),raisedVertices=len(changes),maximumPositiveDelta=max((r['positiveDelta'] for r in changes),default=0.),minimumClearance=min(clearances),repairs=changes)


def inset(polygon,distance):
    """Mitered inner shoreline sampling contour; no change to water outline."""
    result=[]
    for i,p in enumerate(polygon):
        previous=np.asarray(p)-polygon[i-1]
        following=np.asarray(polygon[(i+1)%len(polygon)])-p
        before=np.array([-previous[1],previous[0]])/np.linalg.norm(previous)
        after=np.array([-following[1],following[0]])/np.linalg.norm(following)
        normal=before+after;scale=distance/np.dot(normal,before)
        result.append(tuple(np.asarray(p)+normal*scale))
    return result


def clip(polygon,a,b,inside):
    result=[]
    for p,q in zip(polygon,polygon[1:]+polygon[:1]):
        dp,dq=cross(a,b,p),cross(a,b,q)
        tolerance=EPS*math.dist(a,b)
        if abs(dp)<=tolerance:dp=0.
        if abs(dq)<=tolerance:dq=0.
        ip,iq=(dp>=0,dq>=0) if inside else (dp<=0,dq<=0)
        if ip:result.append(p)
        if ip!=iq:
            t=dp/(dp-dq)
            result.append((round(p[0]+t*(q[0]-p[0]),9),round(p[1]+t*(q[1]-p[1]),9)))
    return clean(result)


def partition(polygon,constraint):
    """Disjoint convex outside pieces plus intersection with one convex face."""
    remaining=polygon;outside=[]
    for a,b in zip(constraint,constraint[1:]+constraint[:1]):
        part=clip(remaining,a,b,False)
        if part:outside.append(part)
        remaining=clip(remaining,a,b,True)
        if not remaining:return [polygon],False
    if abs(area(remaining))<=EPS:return [polygon],False
    return outside+[remaining],True


def on_segment(point,a,b):
    length=math.dist(a,b)
    return length>EPS and abs(cross(a,b,point))<=EPS*length and \
        (point[0]-a[0])*(point[0]-b[0])+(point[1]-a[1])*(point[1]-b[1])<=EPS


def conform_water(vertices,faces,outer,regional,maximum_faces=39618):
    """Return new engine-XYZ vertices, faces, source-face IDs and compact facts.

    Fail before returning if scope/budget fails. Caller data is never mutated.
    New edge samples are propagated to both neighbours before triangulation.
    This avoids T-junctions even where a clipping-line extension meets an edge
    outside the finite water polygon. The original fine core is never split.
    """
    if len(faces)!=len(outer) or len(faces)>maximum_faces:raise ValueError('Invalid water conformance roster/budget')
    if not all(len(v)==3 and all(math.isfinite(x) for x in v) for v in vertices):raise ValueError('Nonfinite terrain vertex')
    points=[(float(v[0]),float(v[2])) for v in vertices]
    footprints=water_footprints(regional)
    # The original one-metre lake mask has a sharp shore. A two-metre inner
    # sample contour prevents triangles from interpolating a distant dry-bank
    # corner across the submerged interior; the exact outer shoreline stays.
    bank_footprints=ribbon_geometry(regional,5.)[0]
    constraints=[list(t) for polygon in footprints+bank_footprints+[inset(footprints[0],2.)] for t in triangulate(polygon)]
    bounds=lambda p:(min(q[0] for q in p),max(q[0] for q in p),min(q[1] for q in p),max(q[1] for q in p))
    constraint_bounds=[bounds(c) for c in constraints]
    overlap=lambda a,b:a[0]<=b[1]+EPS and a[1]>=b[0]-EPS and a[2]<=b[3]+EPS and a[3]>=b[2]-EPS
    polygons=[];split_edges={};edge_owners={};changed=set()
    for source,face in enumerate(faces):
        if len(face)!=3 or len(set(face))!=3 or any(i<0 or i>=len(points) for i in face):raise ValueError('Invalid terrain triangle')
        for a,b in zip(face,face[1:]+face[:1]):edge_owners.setdefault(tuple(sorted((a,b))),[]).append(source)
        polygon=[points[i] for i in face]
        # Source engine-XZ winding is downward; the overlay works CCW.
        if area(polygon)<0:polygon.reverse()
        pieces=[polygon]
        if outer[source]:
            box=bounds(polygon)
            for constraint,cb in zip(constraints,constraint_bounds):
                if not overlap(box,cb):continue
                next_pieces=[]
                for piece in pieces:
                    if not overlap(bounds(piece),cb):next_pieces.append(piece);continue
                    parts,intersects=partition(piece,constraint)
                    next_pieces.extend(parts)
                pieces=next_pieces
        local={p for piece in pieces for p in piece}
        for a,b in zip(face,face[1:]+face[:1]):
            samples={p for p in local if on_segment(p,points[a],points[b]) and p not in (points[a],points[b])}
            if samples:split_edges.setdefault(tuple(sorted((a,b))),set()).update(samples)
        polygons.append(pieces)
    for edge in split_edges:
        if any(not outer[i] for i in edge_owners[edge]):raise ValueError('Water constraint would change the protected core or boundary')
    result_vertices=[tuple(v) for v in vertices]
    # Weld numerically coincident clipping intersections, including intersections
    # reached through different adjacent source triangles. Keep original top
    # identities ahead of the coincident XY of the distant underside vertices.
    buckets={}
    bucket=lambda p:(math.floor(p[0]/EPS),math.floor(p[1]/EPS))
    for i,p in enumerate(points):buckets.setdefault(bucket(p),[]).append(i)
    def point_index(p):
        key=bucket(p);matches=[]
        for dx in (-1,0,1):
            for dz in (-1,0,1):
                for i in buckets.get((key[0]+dx,key[1]+dz),()):
                    v=result_vertices[i]
                    if math.dist(p,(v[0],v[2]))<=EPS:matches.append(i)
        if matches:return min(matches)
        i=len(result_vertices);result_vertices.append((p[0],math.nan,p[1]))
        buckets.setdefault(key,[]).append(i);return i
    result_faces=[];sources=[]
    for source,(face,pieces) in enumerate(zip(faces,polygons)):
        if not outer[source]:result_faces.append(tuple(face));sources.append(source);continue
        local={p for piece in pieces for p in piece}
        for a,b in zip(face,face[1:]+face[:1]):local.update(split_edges.get(tuple(sorted((a,b))),()))
        if len(pieces)==1 and local=={points[i] for i in face}:
            result_faces.append(tuple(face));sources.append(source);continue
        ids=[]
        for piece in pieces:
            expanded=[]
            for a,b in zip(piece,piece[1:]+piece[:1]):
                samples=[p for p in local if on_segment(p,a,b)]
                samples.sort(key=lambda p:(p[0]-a[0])**2+(p[1]-a[1])**2)
                expanded.extend(samples[:-1])
            try:triangles=triangulate(expanded)
            except ValueError as error:raise ValueError(f'{error}; source face {source}: piece={piece}; expanded={expanded}') from error
            for triangle in triangles:
                indices=[]
                for p in triangle:
                    indices.append(point_index(p))
                # Preserve upward engine winding.
                ids.append((indices[0],indices[2],indices[1]))
        if len(ids)==1 and set(ids[0])==set(face):ids=[tuple(face)]
        if ids!=[tuple(face)]:changed.add(source)
        result_faces.extend(ids);sources.extend([source]*len(ids))
        if len(result_faces)>maximum_faces:raise ValueError('Water conformance exceeds existing triangle budget')
    if len(result_faces)>maximum_faces:raise ValueError('Water conformance exceeds existing triangle budget')
    inserted=result_vertices[len(vertices):];bank_receipt=None
    if inserted:
        p=np.asarray(inserted)
        heights=regional.sample(p[:,0]-78.3,126.2-p[:,2])-31.55
        if not np.isfinite(heights).all():raise ValueError('Nonfinite water conformance sample')
        heights,bank_receipt=bank_crest_heights(p[:,[0,2]],heights,regional)
        for i,h in enumerate(heights):result_vertices[len(vertices)+i]=(inserted[i][0],float(h),inserted[i][2])
    return result_vertices,result_faces,sources,dict(originalFaces=len(faces),faces=len(result_faces),
        insertedVertices=len(inserted),changedOuterFaces=len(changed),protectedFaces=sum(not v for v in outer),bankCrest=bank_receipt)
