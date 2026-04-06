"""
blender --background --python bake_atlas.py

1. Imports OBJ (materials) + Mixamo FBX (rig)
2. Reassigns materials to FBX faces by nearest-face matching
3. Creates a second UV map with non-overlapping islands
4. Bakes all material textures into one atlas
5. Replaces materials with single atlas material
6. Exports as mieli_final.fbx
"""

import bpy
import bmesh
import os
from mathutils import Vector
from mathutils.bvhtree import BVHTree

base_dir = os.path.dirname(os.path.abspath(__file__))
fbx_path = os.path.join(base_dir, "mielitrim.fbx")
obj_path = os.path.join(base_dir, "mielitrim.obj")
tex_dir = os.path.join(base_dir, "texture")
out_path = os.path.join(base_dir, "mieli_final.fbx")
atlas_path = os.path.join(base_dir, "mieli_atlas.png")

ATLAS_SIZE = 4096

texture_map = {
    "body": "body_diff_01.jpg",
    "cloth": "cloth_diff_01.jpg",
    "eyes_inner": "eyes_diff_02.jpg",
    "eyes_outer": "eyes_diff_02.jpg",
    "eyelash": "eyelash_alpha_01.jpg",
    "hair_inner": "hair_inner_alpha_01.jpg",
    "hair_outer": "hair_outer_alpha_01.jpg",
}

bpy.ops.wm.read_factory_settings(use_empty=True)

# --- Step 1: Import OBJ (material source) ---
print("=== Step 1: Import OBJ ===")
bpy.ops.wm.obj_import(filepath=obj_path)
obj_mesh = None
for obj in bpy.data.objects:
    if obj.type == 'MESH':
        obj_mesh = obj
        break

print(f"OBJ: {len(obj_mesh.material_slots)} materials, {len(obj_mesh.data.polygons)} faces")

# Build BVH from OBJ
bm_obj = bmesh.new()
bm_obj.from_mesh(obj_mesh.data)
bm_obj.faces.ensure_lookup_table()
bvh = BVHTree.FromBMesh(bm_obj)
obj_face_materials = [f.material_index for f in bm_obj.faces]
obj_materials = [slot.material for slot in obj_mesh.material_slots]
obj_mat_names = [m.name if m else "None" for m in obj_materials]
print(f"Materials: {obj_mat_names}")

# --- Step 2: Import Mixamo FBX ---
print("\n=== Step 2: Import Mixamo FBX ===")
bpy.ops.import_scene.fbx(filepath=fbx_path)

armature = None
fbx_mesh = None
for obj in bpy.data.objects:
    if obj.type == 'ARMATURE':
        armature = obj
    elif obj.type == 'MESH' and obj != obj_mesh:
        fbx_mesh = obj

print(f"FBX: {len(fbx_mesh.data.polygons)} faces, {len(fbx_mesh.data.vertices)} verts")

# Fix UV layer names (FBX may have broken encoding)
for uv in fbx_mesh.data.uv_layers:
    try:
        _ = uv.name
    except UnicodeDecodeError:
        pass
    uv.name = "original_uv"
    print(f"  Renamed UV layer to: {uv.name}")

# --- Step 3: Assign OBJ materials to FBX faces ---
print("\n=== Step 3: Reassign materials ===")
fbx_mesh.data.materials.clear()
for mat in obj_materials:
    fbx_mesh.data.materials.append(mat)

fbx_world = fbx_mesh.matrix_world
obj_world_inv = obj_mesh.matrix_world.inverted()
transform = obj_world_inv @ fbx_world

matched = 0
for poly in fbx_mesh.data.polygons:
    center = Vector((0, 0, 0))
    for vi in poly.vertices:
        center += fbx_mesh.data.vertices[vi].co
    center /= len(poly.vertices)
    center = transform @ center

    loc, normal, face_idx, dist = bvh.find_nearest(center)
    if face_idx is not None:
        poly.material_index = obj_face_materials[face_idx]
        matched += 1
    else:
        poly.material_index = 0

print(f"Matched {matched}/{len(fbx_mesh.data.polygons)} faces")
bm_obj.free()

# Delete OBJ mesh
bpy.data.objects.remove(obj_mesh, do_unlink=True)

# --- Step 4: Connect textures to materials ---
print("\n=== Step 4: Set up material textures ===")
for i, slot in enumerate(fbx_mesh.material_slots):
    mat = slot.material
    if not mat:
        continue
    mat.use_nodes = True
    tree = mat.node_tree
    nodes = tree.nodes
    links = tree.links

    principled = None
    for node in nodes:
        if node.type == 'BSDF_PRINCIPLED':
            principled = node
            break
    if not principled:
        principled = nodes.new('ShaderNodeBsdfPrincipled')

    if mat.name in texture_map:
        tex_file = os.path.join(tex_dir, texture_map[mat.name])
        if os.path.isfile(tex_file):
            img = bpy.data.images.load(tex_file, check_existing=True)
            src_node = nodes.new('ShaderNodeTexImage')
            src_node.image = img
            src_node.location = (-500, 300)
            # Connect to Base Color AND make it use the ORIGINAL UV map
            links.new(src_node.outputs['Color'], principled.inputs['Base Color'])
            print(f"  {mat.name} -> {texture_map[mat.name]}")

# --- Step 5: Create second UV map with non-overlapping islands ---
print("\n=== Step 5: Create atlas UV map ===")
bpy.context.view_layer.objects.active = fbx_mesh
fbx_mesh.select_set(True)

# The original UV map
orig_uv = fbx_mesh.data.uv_layers[0] if fbx_mesh.data.uv_layers else None
try:
    orig_name = orig_uv.name if orig_uv else 'NONE'
except UnicodeDecodeError:
    orig_name = "(bad encoding - renamed)"
print(f"Original UV map: {orig_name}")

# Create new UV map for atlas
atlas_uv = fbx_mesh.data.uv_layers.new(name="atlas_uv")

# Smart UV Project on the new UV map
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')

# Set the atlas UV as active
fbx_mesh.data.uv_layers.active = atlas_uv
bpy.ops.uv.smart_project(angle_limit=1.15192, margin_method='SCALED', island_margin=0.002)

bpy.ops.object.mode_set(mode='OBJECT')

# --- Step 6: Set up bake target ---
print("\n=== Step 6: Bake atlas ===")
atlas_img = bpy.data.images.new("mieli_atlas", ATLAS_SIZE, ATLAS_SIZE, alpha=False)

# For each material, add atlas target node using atlas UV map
for slot in fbx_mesh.material_slots:
    mat = slot.material
    if not mat:
        continue
    tree = mat.node_tree
    nodes = tree.nodes

    # Add UV Map node pointing to the atlas UV
    uv_node = nodes.new('ShaderNodeUVMap')
    uv_node.uv_map = "atlas_uv"
    uv_node.location = (-700, -100)

    # Add atlas target node
    atlas_node = nodes.new('ShaderNodeTexImage')
    atlas_node.image = atlas_img
    atlas_node.label = "bake_target"
    atlas_node.location = (-500, -100)

    # Connect UV map to atlas node
    tree.links.new(uv_node.outputs['UV'], atlas_node.inputs['Vector'])

    # Make atlas node active+selected
    for n in nodes:
        n.select = False
    atlas_node.select = True
    nodes.active = atlas_node

# Ensure source textures use the original UV map
if orig_uv:
    # Get the UV map name safely
    try:
        orig_uv_name = orig_uv.name
    except UnicodeDecodeError:
        # Fallback: find it by index
        orig_uv_name = None
        for idx, uv in enumerate(fbx_mesh.data.uv_layers):
            if uv != atlas_uv:
                # Rename it to something safe
                try:
                    uv.name = "original_uv"
                    orig_uv_name = "original_uv"
                except:
                    pass
                break
        if not orig_uv_name:
            orig_uv_name = "UVMap"

    for slot in fbx_mesh.material_slots:
        mat = slot.material
        if not mat:
            continue
        tree = mat.node_tree
        nodes = tree.nodes
        for node in nodes:
            if node.type == 'TEX_IMAGE' and node.label != "bake_target":
                uv_src = nodes.new('ShaderNodeUVMap')
                uv_src.uv_map = orig_uv_name
                uv_src.location = (-700, 300)
                tree.links.new(uv_src.outputs['UV'], node.inputs['Vector'])

# (atlas UV already set as active above)

# Bake settings
bpy.context.scene.render.engine = 'CYCLES'
bpy.context.scene.cycles.bake_type = 'DIFFUSE'
bpy.context.scene.render.bake.use_pass_direct = False
bpy.context.scene.render.bake.use_pass_indirect = False
bpy.context.scene.render.bake.use_pass_color = True

# Ensure only the mesh is selected for baking
bpy.ops.object.select_all(action='DESELECT')
fbx_mesh.select_set(True)
bpy.context.view_layer.objects.active = fbx_mesh

# Re-fetch atlas UV by name (references go stale after mode switches)
atlas_uv = fbx_mesh.data.uv_layers["atlas_uv"]
for uv_layer in fbx_mesh.data.uv_layers:
    print(f"  UV layer: '{uv_layer.name}'")
fbx_mesh.data.uv_layers.active = atlas_uv
fbx_mesh.data.uv_layers.active_index = list(fbx_mesh.data.uv_layers).index(atlas_uv)
print(f"Active UV set to atlas_uv")

print("Baking...")
bpy.ops.object.bake(type='DIFFUSE', pass_filter={'COLOR'})

atlas_img.filepath_raw = atlas_path
atlas_img.file_format = 'PNG'
atlas_img.save()
print(f"Atlas saved: {atlas_path}")

# --- Step 7: Replace all materials with single atlas material ---
print("\n=== Step 7: Finalize ===")

# Remove the original UV map, keep only atlas
try:
    orig_uv_ref = fbx_mesh.data.uv_layers.get("original_uv")
    if orig_uv_ref:
        fbx_mesh.data.uv_layers.remove(orig_uv_ref)
except:
    print("  Could not remove old UV layer, continuing anyway")

# Create single atlas material
atlas_mat = bpy.data.materials.new(name="mieli_atlas")
atlas_mat.use_nodes = True
tree = atlas_mat.node_tree
nodes = tree.nodes
links = tree.links

# Clear defaults
for n in nodes:
    nodes.remove(n)

output = nodes.new('ShaderNodeOutputMaterial')
output.location = (300, 0)
principled = nodes.new('ShaderNodeBsdfPrincipled')
principled.location = (0, 0)
links.new(principled.outputs['BSDF'], output.inputs['Surface'])

tex_node = nodes.new('ShaderNodeTexImage')
tex_node.image = atlas_img
tex_node.location = (-300, 0)
links.new(tex_node.outputs['Color'], principled.inputs['Base Color'])

# Replace all materials with single atlas material
fbx_mesh.data.materials.clear()
fbx_mesh.data.materials.append(atlas_mat)

# Set all faces to material 0
for poly in fbx_mesh.data.polygons:
    poly.material_index = 0

# --- Step 8: Export ---
print("\n=== Step 8: Export ===")
bpy.ops.object.select_all(action='DESELECT')
armature.select_set(True)
fbx_mesh.select_set(True)
bpy.context.view_layer.objects.active = armature

bpy.ops.export_scene.fbx(
    filepath=out_path,
    use_selection=True,
    add_leaf_bones=False,
    bake_anim=True,
    path_mode='COPY',
    embed_textures=False,
)

print(f"\nExported: {out_path}")
print(f"Atlas: {atlas_path}")
print("Done!")
