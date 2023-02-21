import bpy
import numpy as np

from .pylib import GlslViewer as gl

def bl2npMatrix(mat):
    return np.array(mat)

def bl2veraCamera(region_view_3d: bpy.types.RegionView3D, img_dims: tuple[int, int]):
    projection_matrix = np.array(region_view_3d.window_matrix)
    view_matrix = np.array(region_view_3d.view_matrix.inverted())

    cam = gl.Camera()
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

    cam.setViewport(img_dims[0], img_dims[1])
    cam.setScale(0.5)

    return cam

def bl2veraMesh(bl_mesh: bpy.types.Mesh):
    mesh = gl.Mesh()
    W = bl_mesh.matrix_world
    N = W.inverted_safe().transposed().to_3x3()

    bl_mesh.data.calc_loop_triangles()
    for v in bl_mesh.data.vertices:

        # https://blender.stackexchange.com/questions/6155/how-to-convert-coordinates-from-vertex-to-world-space
        
        p = v.co
        mesh.addVertex(-p[0], -p[1], -p[2])

        n = v.normal
        # n = N @ v.normal
        # mesh.addNormal(-n[0], n[2], -n[1])
        mesh.addNormal(-n[0], -n[1], -n[2])

        # mesh.addNormal(-n[0], -n[2], -n[1])

    # mesh.computeNormals()
    # mesh.invertNormals()

    for tri in bl_mesh.data.loop_triangles:
        mesh.addIndex(tri.vertices[0])
        mesh.addIndex(tri.vertices[1])
        mesh.addIndex(tri.vertices[2])

    if bl_mesh.data.uv_layers.active:
        for i in range(mesh.getVerticesTotal()):
            uv = bl_mesh.data.uv_layers.active.data[i].uv
            mesh.addTexCoord(uv[0], uv[1])
        mesh.computeTangents()

    return mesh

def bl2veraLight(bl_light):
    W = bl_light.matrix_world
    loc = bl_light.location 
    # loc = W @ loc
    light = gl.Light()
    light.setPosition(-loc[0], -loc[2], -loc[1]);

    return light

def file_exist(filename):
    try:
        file = open( bpy.path.abspath(filename) ,'r')
        file.close()
        return True
    except:
        return False