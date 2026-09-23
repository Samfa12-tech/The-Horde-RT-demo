"""Close authored garment openings offline; preserve gauntlet corner data."""
import bmesh


def close_sleeve_openings(player):
    mesh = player.data
    sleeve = next(i for i, material in enumerate(mesh.materials) if material.name == 'ViewmodelSleeves')
    glove = next(i for i, material in enumerate(mesh.materials) if material.name == 'ViewmodelGauntlets')
    glove_normals = {(face.index, mesh.loops[loop].vertex_index): tuple(mesh.corner_normals[loop].vector)
                     for face in mesh.polygons if face.material_index == glove for loop in face.loop_indices}
    cloth_corners = {face.index: [(mesh.vertices[mesh.loops[loop].vertex_index].co.copy(),
                                  tuple(mesh.corner_normals[loop].vector)) for loop in face.loop_indices]
                     for face in mesh.polygons if face.material_index == sleeve}
    before_triangles = sum(len(face.vertices) - 2 for face in mesh.polygons if face.material_index == sleeve)
    bm = bmesh.new()
    try:
        bm.from_mesh(mesh)
        original_face = bm.faces.layers.int.new('hordeOriginalFace')
        original_vertex = bm.verts.layers.int.new('hordeOriginalVertex')
        for face in bm.faces:
            face[original_face] = face.index + 1
        for vertex in bm.verts:
            vertex[original_vertex] = vertex.index + 1
        sleeve_vertices = {v for f in bm.faces if f.material_index == sleeve for v in f.verts}
        if any(any(f.material_index != sleeve for f in v.link_faces) for v in sleeve_vertices):
            raise RuntimeError('Sleeve closure may not merge a gauntlet vertex')
        # Weld geometric seams, retaining per-corner UVs. Do not weld the glove.
        bmesh.ops.remove_doubles(bm, verts=sorted(sleeve_vertices, key=lambda v: v.index), dist=1e-6)
        edges = [e for e in bm.edges if e.is_boundary and e.link_faces[0].material_index == sleeve]
        cap_faces = bmesh.ops.holes_fill(bm, edges=edges, sides=0)['faces']
        if not cap_faces:
            raise RuntimeError('Expected authored sleeve openings to close')
        cap_count = len(cap_faces)
        for face in cap_faces:
            face.material_index = sleeve
            face.smooth = False
            face[original_face] = 0
        # A boundary-to-interior fan avoids reusing an existing garment edge as
        # a cap diagonal (ear clipping did so at three folded-source edges).
        # Blender interpolates UVs and deformation weights for each new centre.
        bmesh.ops.poke(bm, faces=cap_faces, offset=0.0, center_mode='MEAN_WEIGHTED')
        sleeve_faces = [f for f in bm.faces if f.material_index == sleeve]
        bmesh.ops.recalc_face_normals(bm, faces=sleeve_faces)
        sleeve_edges = {e for f in sleeve_faces for e in f.edges}
        if any(len(e.link_faces) != 2 for e in sleeve_edges):
            raise RuntimeError('Authored sleeve closure retained invalid edges: ' + str(dict(
                open=sum(len(e.link_faces) == 1 for e in sleeve_edges),
                overfull=sum(len(e.link_faces) > 2 for e in sleeve_edges), caps=cap_count,
                capBoundaryEdges=len(edges))))
        after_triangles = sum(len(f.verts) - 2 for f in sleeve_faces)
        bm.to_mesh(mesh)
    finally:
        bm.free()
    mesh.update()
    # Preserve authored source normals across a geometric seam weld. Averaging
    # cap normals into the cloth inverted nine bind-pose normal alignments.
    # New panels use their actual face normal; they cannot invert the old cloth.
    normals = [(0.0, 0.0, 0.0)] * len(mesh.loops)
    faces = mesh.attributes['hordeOriginalFace'].data
    vertices = mesh.attributes['hordeOriginalVertex'].data
    for face in mesh.polygons:
        original = faces[face.index].value - 1
        for loop in face.loop_indices:
            if face.material_index == glove:
                normals[loop] = glove_normals[(faces[face.index].value - 1,
                    vertices[mesh.loops[loop].vertex_index].value - 1)]
            elif original >= 0:
                point = mesh.vertices[mesh.loops[loop].vertex_index].co
                corner = min(cloth_corners[original], key=lambda c: (c[0] - point).length_squared)
                if (corner[0] - point).length > 1e-5:
                    raise RuntimeError('Sleeve closure moved an authored surface corner')
                normals[loop] = corner[1]
            else:
                normals[loop] = tuple(face.normal)
    mesh.normals_split_custom_set(normals)
    mesh.attributes.remove(mesh.attributes['hordeOriginalFace'])
    mesh.attributes.remove(mesh.attributes['hordeOriginalVertex'])
    return dict(boundaryEdgesBefore=len(edges), capFaces=cap_count,
                trianglesAdded=after_triangles-before_triangles, boundaryEdgesAfter=0,
                construction='Authored cloth opening fill; source surface positions and loop UVs retained')
