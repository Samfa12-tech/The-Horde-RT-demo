"""Blender-only, reproducible topo derivative. Refuses existing destinations.

Run Blender --background --factory-startup --python this.py -- --root REPO
--source-npz FILE --output-directory NEW_DIRECTORY. Output is a source candidate;
native import, support correspondence and actual RT inspection remain required.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import sys

import bpy
import numpy as np


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def smooth01(t):
    t = np.clip(t, 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--source-npz', type=Path, required=True)
    parser.add_argument('--output-directory', type=Path, required=True)
    parser.add_argument('--entry-audit', type=Path, help='Audited chapter upper-return geometry; legacy mound otherwise')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    root = args.root.resolve()
    out = args.output_directory.resolve()
    version = out.name
    if out.exists():
        raise RuntimeError('Refusing to overwrite any existing terrain source or backup: ' + str(out))
    expected = '251126186fa4923f9902524ec4ac2cc76d0f7a1c82cb1feac9901cc7832bb0ba'
    if digest(args.source_npz) != expected:
        raise RuntimeError('Planning heightfield identity differs from the reviewed r2 bundle')
    data = np.load(args.source_npz, allow_pickle=False)
    east, north, original = data['x'], data['y'], data['height']
    if original.shape != (1002, 612) or not np.isfinite(original).all():
        raise RuntimeError('Invalid planning grid')

    # A symmetric five-tap separable smoothing of the planning grid removes
    # its pad/route junction spikes before grading the production trail.
    filtered = original.copy()
    weights = np.array([1., 4., 6., 4., 1.]) / 16.
    for axis in (0, 1):
        padding = [(0, 0), (0, 0)]
        padding[axis] = (2, 2)
        padded = np.pad(filtered, padding, mode='edge')
        filtered = sum(w * np.take(padded, np.arange(filtered.shape[axis]) + i, axis=axis)
                       for i, w in enumerate(weights))

    # Variable spacing retains a metre grid over the playable chapter and
    # progressively wider samples over real distant land. Exact interfaces are
    # inserted as grid lines so shaft holes cannot be rounded to a sample cell.
    xs = sorted(set(np.round(np.concatenate((np.arange(-65., -46., 2.),
        np.arange(-46., 25., 1.), np.arange(25., 81., 4.), np.arange(81., 237., 8.),
        [-36.9, -35.12, -34.9, -33.7, -32.5, -32.28, -30.5,
         -6.7, -4.3, -2.5, -1.58, 237.])), 5)))
    zs = sorted(set(np.round(np.concatenate((np.arange(-110., -30., 8.),
        np.arange(-30., 49., 1.), np.arange(49., 113., 4.), np.arange(113., 381., 8.),
        [-18.4, -16.82, -16.6, -16.1, -15.2, -14.95, -14.72, -13.8,
         -13.58, -12.8, -12., -11.92, 381.])), 5)))
    xx, zz = np.meshgrid(xs, zs)
    u = np.clip((xx - 78.3 - east[0]) / .5, 0, len(east) - 1.00001)
    v = np.clip((126.2 - zz - north[0]) / .5, 0, len(north) - 1.00001)
    ix, iz = u.astype(int), v.astype(int)
    a, b = u - ix, v - iz
    heights = ((1-b) * ((1-a)*filtered[iz, ix] + a*filtered[iz, ix+1]) +
               b * ((1-a)*filtered[iz+1, ix] + a*filtered[iz+1, ix+1])) - 31.55
    route = np.array([[-33.7, -12.8, 2.05], [-28.7, -5.8, 1.45],
        [-15.7, .2, .55], [-17.7, 15.2, -.55], [-3.7, 20.2, -.25], [13.3, 36.2, -1.15]])
    # Interpolate through the six approved landmarks, with bounded Hermite
    # tangents. Unlike a widened polyline this rounds bends without moving the
    # reunion, dip, clue or lookout anchors. Sampled stations are exported for
    # reproducible support/traversal checks, not used as an invisible floor.
    stations = []
    for i in range(len(route)-1):
        a, b = route[i], route[i+1]
        previous = route[max(0, i-1)]
        following = route[min(len(route)-1, i+2)]
        length = np.linalg.norm(b[:2]-a[:2])
        def tangent(delta):
            return delta * min(1., length / max(np.linalg.norm(delta[:2]), 1e-8))
        ta = tangent(b-previous) if i else b-a
        tb = tangent(following-a) if i+2<len(route) else b-a
        for t in np.linspace(0., 1., max(2, int(math.ceil(length/.25))), endpoint=False):
            stations.append((2*t**3-3*t*t+1)*a + (t**3-2*t*t+t)*ta +
                            (-2*t**3+3*t*t)*b + (t**3-t*t)*tb)
    stations.append(route[-1]); stations = np.array(stations)
    distance = np.full(xx.shape, np.inf)
    route_y = np.zeros(xx.shape)
    for start, end in zip(stations, stations[1:]):
        dx, dz = end[:2] - start[:2]
        t = np.clip(((xx-start[0])*dx + (zz-start[1])*dz)/(dx*dx+dz*dz), 0, 1)
        d = np.hypot(xx-start[0]-t*dx, zz-start[1]-t*dz)
        nearer = d < distance
        route_y = np.where(nearer, start[2] + t*(end[2]-start[2]), route_y)
        distance = np.minimum(distance, d)
    # 2.4m trail; clear reunion/lookout pockets broaden the grade locally.
    grade_distance = distance.copy()
    for x, z, radius in [(-28.7, -5.8, 3.2), (13.3, 36.2, 3.5), (-33.7, -14.3, 3.0)]:
        grade_distance = np.minimum(grade_distance, np.maximum(0., np.hypot(xx-x, zz-z)-radius+1.2))
    grade = 1.0-smooth01((grade_distance-1.2)/3.8)
    heights = heights*(1-grade) + route_y*grade
    # Gentle irregular banks give the trail a readable landform. All resulting
    # geometry remains real ray-visible ground; no visibility mask conceals it.
    banks = smooth01((grade_distance-2.0)/4.0)*(1-smooth01((grade_distance-10.)/9.))
    heights += banks*(.65 + .20*np.sin(xx*.17)*np.cos(zz*.11))

    rooms = [(-1.85,1.85,-6.4,3.4),(-1.2,1.2,-10.,-6.4),(0.,4.8,-11.2,-8.8),
             (3.6,6.,-15.2,-10.),(-2.5,4.8,-16.4,-14.),(-8.5,-2.5,-18.,-12.4),
             (-28.5,-8.5,-16.8,-13.6),(-30.5,-28.5,-16.8,-13.6),(-36.9,-30.5,-18.4,-12.)]
    for xmin, xmax, zmin, zmax in rooms:
        d = np.hypot(np.maximum(np.maximum(xmin-xx, xx-xmax), 0),
                     np.maximum(np.maximum(zmin-zz, zz-zmax), 0))
        heights = np.maximum(heights, 1.55 - .12*d*d)
    entry_envelopes=[]
    if args.entry_audit:
        entry=json.loads(args.entry_audit.read_text())
        if not entry.get('upperReturn') or entry.get('stairCount')!=18 or any(
            v['worldBoundaryMisses'] or v['visibleRemoteSealSamples']
            for v in entry['physicalEnclosureFrustumChecks']):
            raise RuntimeError('Entry envelope is not the passing upper-return derivative')
        for part in entry['geometry']:
            if ('roof' not in part['name'].lower() and 'enclosure' not in part['name'].lower()) or part['name']=='Existing chamber roof':
                continue
            low,high=part['boundsHorde']
            entry_envelopes.append((low[0],high[0],low[2],high[2],high[1]+.20))
        if not entry_envelopes or max(e[4] for e in entry_envelopes)>6.21:
            raise RuntimeError('Unexpected shortened-entry enclosure height')
        # A single rounded barrow covers the entire enclosed stair return.
        # Its corner minimum exceeds every roof top by .2m; unlike separate
        # rectangle caps it does not print the stairwell plan into the hillside.
        ex0=min(e[0] for e in entry_envelopes);ex1=max(e[1] for e in entry_envelopes)
        ez0=min(e[2] for e in entry_envelopes);ez1=max(e[3] for e in entry_envelopes)
        cx,cz=(ex0+ex1)/2,(ez0+ez1)/2
        rx,rz=(ex1-ex0)/2,(ez1-ez0)/2
        peak=max(e[4] for e in entry_envelopes)+1.3
        cover=peak-.55*(((xx-cx)/rx)**2+((zz-cz)/rz)**2)
        heights=np.maximum(heights,cover)
    else:
        # Original source revisions retain their tall 44-riser envelope.
        collapse_height = 2.1 + np.clip((zz-4.78)/12.7, 0, 1)*10.25
        collapse_d = np.hypot(np.maximum(abs(xx)-2.3, 0),
                             np.maximum(np.maximum(2.9-zz, zz-17.7), 0))
        heights = np.maximum(heights, collapse_height - 1.8*collapse_d*collapse_d)
    # The roof-covering mound must not overwrite the graded clue bend. Cut a
    # real trail into its outer shoulder, with a bank blend beyond the 2.4m
    # visible path. Reassert occupied roofs below; no invisible support ribbon
    # or cut through the retained rising entry stair is permitted.
    trail_cut = 1-smooth01((grade_distance-1.8)/1.5)
    heights = heights*(1-trail_cut)+route_y*trail_cut
    for xmin,xmax,zmin,zmax in rooms:
        inside=(xx>=xmin)&(xx<=xmax)&(zz>=zmin)&(zz<=zmax)
        heights=np.where(inside,np.maximum(heights,1.55),heights)
    if entry_envelopes:
        for xmin,xmax,zmin,zmax,top in entry_envelopes:
            # Include one grid cell of cover so interpolated boundary triangles
            # cannot intersect the top of the enclosed masonry.
            inside=(xx>=xmin-1)&(xx<=xmax+1)&(zz>=zmin-1)&(zz<=zmax+1)
            heights=np.where(inside,np.maximum(heights,top),heights)
    else:
        inside_entry=(abs(xx)<=2.3)&(zz>=2.9)&(zz<=17.7)
        heights=np.where(inside_entry,np.maximum(heights,collapse_height),heights)
    # Exact plateau connects the existing apron/rim. A smooth two-metre skirt
    # returns to the graded mound without moving the accepted rope geometry.
    apron_d = np.hypot(np.maximum(np.maximum(-35.12-xx, xx+32.28), 0),
                       np.maximum(np.maximum(-16.82-zz, zz+11.92), 0))
    apron_blend = 1-smooth01(apron_d/2.)
    heights = heights*(1-apron_blend) + 2.05*apron_blend
    heights = heights.astype(np.float32)
    # Rescue opening, impassable barred skylight and waterfall opening stay
    # distinct. The original closed entry skylight is deliberately NOT a hole.
    holes = [(-34.9,-32.5,-16.6,-13.8), (-6.7,-4.3,-16.6,-13.8),
             (-2.5,-1.58,-16.1,-14.72)]
    vertices = [(float(x), -float(z), float(y)) for x,z,y in zip(xx.flat,zz.flat,heights.flat)]
    faces, materials, cells = [], [], []
    split_vertices = {}
    def crossing(a, da, b, db):
        key = tuple(sorted((a,b)))
        if key not in split_vertices:
            t=da/(da-db)
            position=tuple(vertices[a][k]+t*(vertices[b][k]-vertices[a][k]) for k in range(3))
            split_vertices[key]=len(vertices);vertices.append(position)
        return split_vertices[key]
    def emit_surface(triangle, distances, cell):
        # Split along the signed 1.2m trail/pocket contour. Material boundaries
        # follow the graded path rather than jagged one-metre grid squares.
        for material in (0,1):
            polygon=[]
            for k in range(3):
                a,b=triangle[k],triangle[(k+1)%3]
                da,db=distances[k],distances[(k+1)%3]
                inside_a=(da<=0) if material==0 else (da>=0)
                inside_b=(db<=0) if material==0 else (db>=0)
                if inside_a:polygon.append(a)
                if inside_a!=inside_b:polygon.append(crossing(a,da,b,db))
            for k in range(1,len(polygon)-1):
                tri=(polygon[0],polygon[k],polygon[k+1])
                if len(set(tri))<3:continue
                p,q,r=(vertices[n] for n in tri)
                area=(q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])
                if area<=1e-9:continue
                faces.append(tri);materials.append(material);cells.append(cell)
    width = len(xs)
    for j in range(len(zs)-1):
        for i in range(len(xs)-1):
            x,z = (xs[i]+xs[i+1])*.5,(zs[j]+zs[j+1])*.5
            if any(lo<x<hi and back<z<front for lo,hi,back,front in holes):
                continue
            q = j*width+i
            # Blender coordinates X,-Z,Y produce engine +Y outward normals.
            signed=[grade_distance[j,i]-1.2,grade_distance[j+1,i]-1.2,
                    grade_distance[j+1,i+1]-1.2,grade_distance[j,i+1]-1.2]
            emit_surface((q,q+width,q+width+1),signed[:3],(i,j,0))
            emit_surface((q,q+width+1,q+1),(signed[0],signed[2],signed[3]),(i,j,1))
    surface_face_count = len(faces)
    # Close only the distant outer perimeter, never extrude every near cell
    # through the tomb. The underside is below all occupied tomb volumes.
    # Apertures still open into their existing authored shafts; no new cap is
    # placed across any skylight or rescue opening.
    bottom_y = float(math.floor(float(heights.min())) - 5)
    perimeter = list(range(width))
    perimeter += [j*width+width-1 for j in range(1,len(zs))]
    perimeter += list(range((len(zs)-1)*width+width-2,(len(zs)-1)*width-1,-1))
    perimeter += [j*width for j in range(len(zs)-2,0,-1)]
    bottom = []
    for index in perimeter:
        vx, vz, _ = vertices[index]
        bottom.append(len(vertices)); vertices.append((vx,vz,bottom_y))
    for i in range(len(perimeter)):
        j=(i+1)%len(perimeter)
        # Top perimeter is clockwise in Blender XY, so these faces point out.
        for triangle in ((perimeter[i],bottom[j],bottom[i]),
                         (perimeter[i],perimeter[j],bottom[j])):
            faces.append(triangle); materials.append(1); cells.append((-1,-1,-1))
    # The bottom is exactly planar. Two full-width triangles meet the collinear
    # segmented wall edges; there is no open underside. Do not fan the fine
    # interface samples hundreds of metres across this plane: those needle
    # triangles produced zero MikkTSpace tangents in the retained r05/r06.
    corners=[bottom[i] for i in (0,width-1,width+len(zs)-2,2*width+len(zs)-3)]
    for triangle in ((corners[0],corners[1],corners[2]),
                     (corners[0],corners[2],corners[3])):
        faces.append(triangle);materials.append(1);cells.append((-1,-1,-1))
    walkable = []
    for index, face in enumerate(faces):
        a,b,c=(np.array(vertices[n]) for n in face)
        normal=np.cross(b-a,c-a)
        walkable.append(bool(index<surface_face_count and normal[2]>0 and
                        np.linalg.norm(normal[:2])<=.65*normal[2]))
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bpy.context.preferences.filepaths.save_version = 0
    mesh = bpy.data.meshes.new('TopoTerrainSurface')
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    # Support must use Blender's actual float32 mesh positions, not the higher
    # precision intermediates used to locate contour intersections.
    vertices = [tuple(vertex.co) for vertex in mesh.vertices]
    obj = bpy.data.objects.new(version, mesh)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    uv = mesh.uv_layers.new(name='UV0')
    for poly in mesh.polygons:
        poly.use_smooth = poly.index < surface_face_count
        poly.material_index = materials[poly.index]
        repeats = .5 if poly.material_index == 1 else .42
        for loop in poly.loop_indices:
            vtx = mesh.vertices[mesh.loops[loop].vertex_index].co
            # Boundary walls use a vertical projection, avoiding collapsed UVs
            # and invalid tangents. Forest source spans 2 metres; trail retains its existing scale.
            if poly.index >= surface_face_count and abs(poly.normal.z)<.5:
                horizontal=vtx.x if abs(poly.normal.y)>.5 else -vtx.y
                uv.data[loop].uv=(horizontal*repeats,vtx.z*repeats)
            else:
                uv.data[loop].uv = (vtx.x*repeats, -vtx.y*repeats)
    source_textures = []
    for name, family in [('ChapterTrailDampSand','damp_sand'), ('ChapterForestGround','forrest_ground_01')]:
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True
        mat.diffuse_color = (.22,.20,.15,1)
        bsdf = mat.node_tree.nodes.get('Principled BSDF')
        bsdf.inputs['Metallic'].default_value = 0
        for filename, color, socket in [('diff.jpg','sRGB','Base Color'),('normal.jpg','Non-Color','Normal'),('arm.jpg','Non-Color','Roughness')]:
            path = (root/'assets/textures/polyhaven/source/forrest_ground_01-1k-v1'/filename.replace('.jpg','.png')
                    if family == 'forrest_ground_01' else root/'assets/textures/polyhaven/mobile_1k'/family/filename)
            if not path.is_file(): raise RuntimeError('Missing admitted material '+str(path))
            image = bpy.data.images.load(str(path), check_existing=True)
            image.colorspace_settings.name = color
            image.pack()
            # Keep packed editable masters portable and free of personal paths.
            # This points back to the admitted source if an artist unpacks it.
            image.filepath = '//' + Path(os.path.relpath(path, out)).as_posix()
            for packed in image.packed_files:
                packed.filepath = image.filepath
            tex = mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=image
            if socket=='Normal':
                normal=mat.node_tree.nodes.new('ShaderNodeNormalMap')
                mat.node_tree.links.new(tex.outputs['Color'],normal.inputs['Color'])
                mat.node_tree.links.new(normal.outputs['Normal'],bsdf.inputs['Normal'])
            elif socket=='Roughness':
                channels=mat.node_tree.nodes.new('ShaderNodeSeparateColor')
                mat.node_tree.links.new(tex.outputs['Color'],channels.inputs['Color'])
                mat.node_tree.links.new(channels.outputs['Green'],bsdf.inputs['Roughness'])
            else: mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs[socket])
            source_textures.append({'path':path.relative_to(root).as_posix(),'sha256':digest(path)})
        mesh.materials.append(mat)
    mesh.calc_tangents(uvmap='UV0')
    obj['source_heightfield_sha256'] = expected
    obj['map_to_engine'] = 'X=East+78.3; Y=Up-31.55; Z=126.2-North'
    obj['trail_clear_width_m'] = 2.4
    obj['acceptance'] = 'SOURCE CANDIDATE; native/support/RT visual gates pending'
    out.mkdir(parents=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(out/(version+'.blend')))
    bpy.ops.export_scene.gltf(filepath=str(out/(version+'.glb')), export_format='GLB',
        use_selection=True, export_tangents=True, export_animations=False,
        export_cameras=False, export_lights=False, export_extras=True)
    surface = {'schema':1,'axis':'engine X/Y/Z metres','x':xs,'z':zs,
        'vertices':[(x,y,-z) for x,z,y in vertices],
        'heights':heights.tolist(),'holes':holes,'faces':faces,'cells':cells,'undersideY':bottom_y,
        'surfaceMaterials':materials,'walkable':walkable,'surfaceFaceCount':surface_face_count,
        'routeStations':stations.tolist(),'maxWalkableSlope':.65,
        'entryEnvelopes':entry_envelopes,'sourceGlbSha256':digest(out/(version+'.glb'))}
    (out/'surface.json').write_text(json.dumps(surface,separators=(',',':'))+'\n')
    # Derive the candidate manifest from actual exporter output, not Blender's
    # pre-split vertex count. This is a new asset envelope, not a renderer cap.
    import struct
    raw=(out/(version+'.glb')).read_bytes()
    length=struct.unpack_from('<I',raw,12)[0]
    gltf=json.loads(raw[20:20+length]);primitives=[p for m in gltf['meshes'] for p in m['primitives']]
    manifest={'schema':1,'asset':version,'metresPerUnit':1.0,
        'coordinateSystem':{'up':'+Y','forward':'+Z'},
        'budgets':{'maxVertices':sum(gltf['accessors'][p['attributes']['POSITION']]['count'] for p in primitives),
            'maxIndices':len(faces)*3,'maxPrimitives':len(primitives),'maxMaterials':2,'maxTextureLayersPerKind':2},
        'lods':[{'name':'authored','maxTriangles':len(faces)}],'requiredSockets':[],
        'runtimeTextureProfile':{'android':'astc','windows':'rgba8','mipmapped':True},'materialOverrides':[],
        'distribution':'source-candidate','licenceStatus':'Original Horde terrain; admitted Poly Haven CC0 materials'}
    (out/'asset.manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    record = {'schema':1,'author':'Original Horde authoring from owner-supplied r2 topo',
        'entryAuditSha256':digest(args.entry_audit) if args.entry_audit else None,
        'sourceNpzSha256':expected,'recipeSha256':digest(Path(__file__)),
        'blender':bpy.app.version_string,'vertices':len(vertices),'triangles':len(faces),
        'materials':2,'uvRepeatsPerMetre':{'trail':.42,'forest':.5},'textures':source_textures,
        'boundsEngine':[[float(xx.min()),bottom_y,float(zz.min())],
                        [float(xx.max()),float(heights.max()),float(zz.max())]],
        'gridSpacing':'1m near; 2/4m mid; 8m far; exact shaft/skylight interfaces',
        'adjustments':['Five-tap separable planning-height smoothing','2.4m graded trail; 6.4m/7m pockets',
          'Clue trail cut into exterior mound shoulder; occupied roofs and rising stairs retained',
          'Banks and retained tomb roof/collapse mound clearance','Exact 2.05m apron and three separate open shafts'],
        'files':[{ 'path':p.name,'bytes':p.stat().st_size,'sha256':digest(p)}
                 for p in sorted(out.iterdir()) if p.is_file()],
        'status':'Authored source candidate; NOT runtime admission or visual acceptance'}
    (out/'authoring-receipt.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps({k:record[k] for k in ['vertices','triangles','materials','boundsEngine','status']}))


if __name__=='__main__':
    main()
