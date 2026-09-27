"""Split proven coarse cloth edges only; keep source corners and view geometry."""
import bmesh
from mathutils import Vector


def split_segmented_cloth_seams(mesh, plans):
    if not plans:
        return dict(coarseEdges=0,verticesAdded=0,trianglesAdded=0,
                    construction='No coarse cloth edge required splitting')
    original_positions = [tuple(vertex.co) for vertex in mesh.vertices]
    original_weights = [tuple((g.group,g.weight) for g in vertex.groups) for vertex in mesh.vertices]
    uv_names = [layer.name for layer in mesh.uv_layers]
    packed_normals = mesh.attributes.get('custom_normal')
    if packed_normals is not None and (
            packed_normals.domain != 'CORNER' or packed_normals.data_type != 'INT16_2D'):
        raise RuntimeError('Review changed Blender custom-normal storage before splitting cloth')
    original_faces = {}
    for face in mesh.polygons:
        original_faces[face.index] = dict(material=face.material_index, corners={
            mesh.loops[loop].vertex_index: dict(
                normal=tuple(mesh.corner_normals[loop].vector),
                packedNormal=tuple(packed_normals.data[loop].value) if packed_normals else None,
                uvs={name:tuple(mesh.uv_layers[name].data[loop].uv) for name in uv_names})
            for loop in face.loop_indices})
    tags = ('hordeSeamSourceFace','hordeSeamSourceVertex','hordeSeamNewVertex')
    if any(mesh.attributes.get(name) is not None for name in tags):
        raise RuntimeError('Seam split bookkeeping attribute already exists')
    bm=bmesh.new()
    new_corners, new_positions = {}, {}
    affected_vertices = set()
    try:
        bm.from_mesh(mesh)
        bm.verts.ensure_lookup_table()
        bm.faces.ensure_lookup_table()
        face_tag=bm.faces.layers.int.new(tags[0])
        vertex_tag=bm.verts.layers.int.new(tags[1])
        new_tag=bm.verts.layers.int.new(tags[2])
        for face in bm.faces:
            face[face_tag]=face.index+1
        for vertex in bm.verts:
            vertex[vertex_tag]=vertex.index+1
            vertex[new_tag]=0
        # Allocating custom-data layers can invalidate earlier element handles.
        bm.verts.ensure_lookup_table()
        original_vertices=list(bm.verts)
        for plan in plans:
            a_index,b_index=plan['worldEdge']
            start,end=original_vertices[a_index],original_vertices[b_index]
            edge=bm.edges.get((start,end))
            if edge is None:
                raise RuntimeError('Planned coarse cloth edge is not a mesh edge')
            source_face_ids={face[face_tag]-1 for face in edge.link_faces}
            if not source_face_ids or any(len(face.verts)!=3 for face in edge.link_faces):
                raise RuntimeError('Segmented seam split requires existing triangular cloth faces')
            if any(mesh.materials[original_faces[index]['material']].name not in
                   ('NearFacePrimaryMasked','BodyRemainderPrimaryVisible')
                   for index in source_face_ids):
                raise RuntimeError('Planned cloth split touches a protected surface')
            for source_id in source_face_ids:
                affected_vertices.update(original_faces[source_id]['corners'])
            previous=0.
            for point in plan['points']:
                fraction=point['fraction']
                if not previous < fraction < 1.:
                    raise RuntimeError('Seam split fractions must be ordered and interior')
                edge=bm.edges.get((start,end))
                adjacent=[(face,next(vertex for vertex in face.verts if vertex not in (start,end)))
                          for face in edge.link_faces]
                _,midpoint=bmesh.utils.edge_split(edge,start,(fraction-previous)/(1-previous))
                midpoint.co=Vector(point['position'])
                midpoint[vertex_tag]=0
                token=len(new_positions)+1
                midpoint[new_tag]=token
                new_positions[token]=tuple(midpoint.co)
                for source_id in source_face_ids:
                    corners=original_faces[source_id]['corners']
                    first,last=corners[a_index],corners[b_index]
                    normal=(Vector(first['normal'])*(1-fraction)+Vector(last['normal'])*fraction)
                    if normal.length_squared < 1e-12:
                        raise RuntimeError('Cloth split produced an undefined interpolated normal')
                    new_corners[(token,source_id)]=dict(normal=tuple(normal.normalized()),
                        uvs={name:tuple(a*(1-fraction)+b*fraction for a,b in
                                       zip(first['uvs'][name],last['uvs'][name])) for name in uv_names})
                for face,opposite in adjacent:
                    source_id=face[face_tag]
                    child,_=bmesh.utils.face_split(face,midpoint,opposite)
                    child[face_tag]=source_id
                    if len(face.verts)!=3 or len(child.verts)!=3:
                        raise RuntimeError('Cloth split did not preserve explicit triangles')
                start,previous=midpoint,fraction
        bm.to_mesh(mesh)
    finally:
        bm.free()
    mesh.update()
    face_ids=mesh.attributes[tags[0]].data
    vertex_ids=mesh.attributes[tags[1]].data
    new_ids=mesh.attributes[tags[2]].data
    for vertex in mesh.vertices:
        original_id=vertex_ids[vertex.index].value-1
        if original_id>=0:
            if tuple(vertex.co)!=original_positions[original_id] or tuple(
                    (g.group,g.weight) for g in vertex.groups)!=original_weights[original_id]:
                raise RuntimeError('Cloth split changed an original vertex or deform weights')
        elif tuple(vertex.co)!=new_positions[new_ids[vertex.index].value]:
            raise RuntimeError('Cloth split moved a canonical seam point')
    normals=[None]*len(mesh.loops)
    for face in mesh.polygons:
        source_id=face_ids[face.index].value-1
        if face.material_index!=original_faces[source_id]['material']:
            raise RuntimeError('Cloth split changed a material identity')
        for loop in face.loop_indices:
            vertex_index=mesh.loops[loop].vertex_index
            original_id=vertex_ids[vertex_index].value-1
            if original_id>=0:
                corner=original_faces[source_id]['corners'][original_id]
                if any(tuple(mesh.uv_layers[name].data[loop].uv)!=corner['uvs'][name] for name in uv_names):
                    raise RuntimeError('Cloth split changed an original corner UV')
            else:
                corner=new_corners[(new_ids[vertex_index].value,source_id)]
                for name in uv_names:
                    mesh.uv_layers[name].data[loop].uv=corner['uvs'][name]
            normals[loop]=corner['normal']
    mesh.normals_split_custom_set(normals)
    if packed_normals is not None:
        # Blender re-quantizes even unchanged normals when setting the full
        # array. Restore exact packed values only in untouched normal fans;
        # split-face vertices retain normals encoded for their new topology.
        restored_normals = mesh.attributes['custom_normal']
        for face in mesh.polygons:
            source_id = face_ids[face.index].value - 1
            for loop in face.loop_indices:
                original_id = vertex_ids[mesh.loops[loop].vertex_index].value - 1
                if original_id >= 0 and original_id not in affected_vertices:
                    restored_normals.data[loop].value = original_faces[source_id]['corners'][original_id]['packedNormal']
    for name in tags:
        mesh.attributes.remove(mesh.attributes[name])
    return dict(coarseEdges=len(plans),verticesAdded=len(mesh.vertices)-len(original_positions),
                trianglesAdded=len(mesh.polygons)-len(original_faces),
                construction='Existing cloth triangles split at exact canonical sleeve vertices; no caps or new panels')
