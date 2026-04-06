"""
blender --background --python reassign_materials.py
"""

import bpy
import bmesh
import os
from mathutils import Vector
from mathutils.bvhtree import BVHTree

base_dir = os.path.dirname(os.path.abspath(__file__))
fbx_path = os.path.join(base_dir, "mielitrim.fbx")
obj_path = os.path.join(base_dir, "mielitrim.obj")
out_path = os.path.join(base_dir, "mieli_multimaterial.fbx")

bpy.ops.wm.read_factory_settings(use_empty=True)

# Import OBJ
print("Importing OBJ...")
bpy.ops.wm.obj_import(filepath=obj_path)
obj_mesh = None
for obj in bpy.data.objects:
    if obj.type == 'MESH':
        obj_mesh = obj
        break

# Apply transforms on OBJ, then scale to match FBX (FBX is 100x smaller)
bpy.context.view_layer.objects.active = obj_mesh
obj_mesh.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
obj_mesh.scale = (0.01, 0.01, 0.01)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
obj_mesh.select_set(False)

obj_materials = [slot.material for slot in obj_mesh.material_slots]
obj_mat_names = [m.name if m else "None" for m in obj_materials]
print(f"OBJ Materials: {obj_mat_names}")

# Import Mixamo FBX
print("Importing Mixamo FBX...")
bpy.ops.import_scene.fbx(filepath=fbx_path)

armature = None
fbx_mesh = None
for obj in bpy.data.objects:
    if obj.type == 'ARMATURE':
        armature = obj
    elif obj.type == 'MESH' and obj != obj_mesh:
        fbx_mesh = obj

# Apply transforms on FBX mesh (need to unparent first to apply properly)
bpy.context.view_layer.objects.active = fbx_mesh
fbx_mesh.select_set(True)

# Debug: print bounding boxes
obj_verts = [v.co for v in obj_mesh.data.vertices]
fbx_verts = [fbx_mesh.matrix_world @ v.co for v in fbx_mesh.data.vertices]
obj_min = Vector([min(v[i] for v in obj_verts) for i in range(3)])
obj_max = Vector([max(v[i] for v in obj_verts) for i in range(3)])
fbx_min = Vector([min(v[i] for v in fbx_verts) for i in range(3)])
fbx_max = Vector([max(v[i] for v in fbx_verts) for i in range(3)])
print(f"OBJ bounds: {obj_min} to {obj_max}")
print(f"FBX bounds: {fbx_min} to {fbx_max}")

# Build BVH from OBJ in world space
bm_obj = bmesh.new()
bm_obj.from_mesh(obj_mesh.data)
bm_obj.faces.ensure_lookup_table()
bvh = BVHTree.FromBMesh(bm_obj)
obj_face_materials = [f.material_index for f in bm_obj.faces]

# Reassign materials to FBX mesh
fbx_mesh.data.materials.clear()
for mat in obj_materials:
    fbx_mesh.data.materials.append(mat)

# Match faces using world-space coordinates
fbx_world = fbx_mesh.matrix_world
matched = 0
mat_counts = {}

for poly in fbx_mesh.data.polygons:
    # Face center in world space
    center = Vector((0, 0, 0))
    for vi in poly.vertices:
        center += fbx_world @ fbx_mesh.data.vertices[vi].co
    center /= len(poly.vertices)

    loc, normal, face_idx, dist = bvh.find_nearest(center)
    if face_idx is not None:
        mi = obj_face_materials[face_idx]
        poly.material_index = mi
        matched += 1
        name = obj_mat_names[mi]
        mat_counts[name] = mat_counts.get(name, 0) + 1

print(f"Matched {matched}/{len(fbx_mesh.data.polygons)} faces")
for name, count in sorted(mat_counts.items(), key=lambda x: -x[1]):
    print(f"  {name}: {count} faces")

bm_obj.free()
bpy.data.objects.remove(obj_mesh, do_unlink=True)

# Delete faces with "Material" (ground plane) material
ground_idx = obj_mat_names.index("Material") if "Material" in obj_mat_names else -1
if ground_idx >= 0:
    bpy.context.view_layer.objects.active = fbx_mesh
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='DESELECT')
    bpy.ops.object.mode_set(mode='OBJECT')
    for poly in fbx_mesh.data.polygons:
        poly.select = (poly.material_index == ground_idx)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.delete(type='FACE')
    bpy.ops.object.mode_set(mode='OBJECT')
    # Remove the empty material slot
    fbx_mesh.active_material_index = ground_idx
    bpy.ops.object.material_slot_remove()
    print(f"Removed ground plane ({ground_idx})")

# Export
bpy.ops.object.select_all(action='DESELECT')
armature.select_set(True)
fbx_mesh.select_set(True)
bpy.context.view_layer.objects.active = armature

print(f"Exporting {out_path}...")
bpy.ops.export_scene.fbx(
    filepath=out_path,
    use_selection=True,
    add_leaf_bones=False,
    bake_anim=True,
    path_mode='COPY',
    embed_textures=False,
)
print("Done!")
