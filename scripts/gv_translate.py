import bpy
import numpy as np

from .gv_lib import GlslViewer as gl

RENDER_CAM_TYPE_ID = "render_camera_type"
RENDER_CAM_TYPE_PERSPECTIVE = "perspective"
RENDER_CAM_TYPE_SPHERICAL_QUADRILATERAL = "spherical_quadrilateral"
RENDER_CAM_TYPE_QUADRILATERAL_HEXAHEDRON = "quadrilateral_hexahedron"

def bl2veraCamera(source: bpy.types.RegionView3D | bpy.types.Object):
    cam = gl.Camera()

    view_matrix = None
    projection_matrix = None

    if isinstance(source, bpy.types.RegionView3D):
        projection_matrix = np.array(source.window_matrix)
        view_matrix = np.array(source.view_matrix.inverted())

    elif isinstance(source, bpy.types.Object):
        render = bpy.context.scene.render
        view_matrix = source.matrix_world.inverted()
        projection_matrix = source.calc_matrix_camera(
            render.resolution_x,
            render.resolution_y,
            render.pixel_aspect_x,
            render.pixel_aspect_y,
        )
    
    else:
        print(f"INVALID CAMERA SOURCE: {source}")
        return None

    cam.setTransformMatrix(
        view_matrix[0][0], view_matrix[2][0], view_matrix[1][0], view_matrix[3][0],
        view_matrix[0][1], view_matrix[2][1], view_matrix[1][1], view_matrix[3][1],
        view_matrix[0][2], view_matrix[2][2], view_matrix[1][2], view_matrix[3][2],
        view_matrix[0][3], view_matrix[2][3], view_matrix[1][3], view_matrix[3][3]
    )
    cam.setProjection(
        projection_matrix[0][0], projection_matrix[1][0], projection_matrix[2][0], projection_matrix[3][0],
        projection_matrix[0][1], projection_matrix[1][1], projection_matrix[2][1], projection_matrix[3][1],
        projection_matrix[0][2], projection_matrix[1][2], projection_matrix[2][2], projection_matrix[3][2],
        projection_matrix[0][3], projection_matrix[1][3], projection_matrix[2][3], projection_matrix[3][3]
    )

    # cam.setViewport(img_dims[0], img_dims[1])
    cam.setScale(0.5)

    return cam

def bl2veraMesh(bl_mesh: bpy.types.Mesh):
    mesh = gl.Mesh()
    W = bl_mesh.matrix_world
    N = W.inverted_safe().transposed().to_3x3()

    bl_mesh.data.calc_loop_triangles()
    bl_mesh.data.calc_normals_split()
    has_uv = bl_mesh.data.uv_layers != None and len(bl_mesh.data.uv_layers) > 0
    has_color = bl_mesh.data.vertex_colors != None and len(bl_mesh.data.vertex_colors) > 0

    for triangle_loop in bl_mesh.data.loop_triangles:
        for loop_index in triangle_loop.loops:
            loop = bl_mesh.data.loops[loop_index]

            v = bl_mesh.data.vertices[loop.vertex_index].co
            mesh.addVertex(-v[0], -v[1], -v[2])

            n = loop.normal
            mesh.addNormal(-n[0], -n[1], -n[2])

            if has_color:
                c = bl_mesh.data.vertex_colors.active.data[loop_index].color
                mesh.addColor(c[0], c[1], c[2], c[3])

            if has_uv:
                t = bl_mesh.data.uv_layers.active.data[loop_index].uv
                mesh.addTexCoord(t[0], t[1])

    if has_uv:
        mesh.computeTangents()

    return mesh

def bl2veraLight(bl_light):
    W = bl_light.matrix_world
    loc = bl_light.location 
    loc = W @ loc
    light = gl.Light()
    light.setPosition(-loc[0], -loc[2], -loc[1]);

    return light

