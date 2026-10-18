"""Bounded source-only terrain/water planar constraint overlay.

Preserve all original vertices and nonintersecting triangles. Only admitted
outer triangles are split; inserted heights come from the unchanged regional
heightfield, never from a raised water surface or synthetic collision floor.
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


def water_footprints(regional):
    """Exact atlas lake and six-metre mitered ribbons, in engine X/Z."""
    engine=lambda p:(float(p[0])+78.3,126.2-float(p[1]))
    polygons=[list(map(engine,regional.lake_outline))]
    for river in regional.river_water:
        points=np.asarray(river,dtype=float)
        directions=np.diff(points[:,:2],axis=0)
        normals=np.stack((-directions[:,1],directions[:,0]),axis=1)
        normals/=np.linalg.norm(normals,axis=1)[:,None]
        left=[];right=[]
        for i,p in enumerate(points):
            before,after=normals[max(0,i-1)],normals[min(i,len(normals)-1)]
            m=before+after;m*=3./np.dot(m,before)
            left.append(engine(p[:2]+m));right.append(engine(p[:2]-m))
        # Separate quads match the renderer's shared miter vertices at bends.
        polygons.extend([[left[i],right[i],right[i+1],left[i+1]] for i in range(len(points)-1)])
    return [p if area(p)>0 else p[::-1] for p in polygons]


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
    constraints=[list(t) for polygon in footprints+[inset(footprints[0],2.)] for t in triangulate(polygon)]
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
    inserted=result_vertices[len(vertices):]
    if inserted:
        p=np.asarray(inserted)
        heights=regional.sample(p[:,0]-78.3,126.2-p[:,2])-31.55
        if not np.isfinite(heights).all():raise ValueError('Nonfinite water conformance sample')
        for i,h in enumerate(heights):result_vertices[len(vertices)+i]=(inserted[i][0],float(h),inserted[i][2])
    return result_vertices,result_faces,sources,dict(originalFaces=len(faces),faces=len(result_faces),
        insertedVertices=len(inserted),changedOuterFaces=len(changed),protectedFaces=sum(not v for v in outer))
