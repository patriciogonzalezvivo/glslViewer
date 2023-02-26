import bpy
import numpy as np

import mathutils

from .gv_lib import GlslViewer as gl

RENDER_CAM_TYPE_ID = "render_camera_type"
RENDER_CAM_TYPE_PERSPECTIVE = "perspective"
RENDER_CAM_TYPE_SPHERICAL_QUADRILATERAL = "spherical_quadrilateral"
RENDER_CAM_TYPE_QUADRILATERAL_HEXAHEDRON = "quadrilateral_hexahedron"

def view_plane(camd, winx, winy, xasp, yasp):    
    #/* fields rendering */
    ycor = yasp / xasp
    use_fields = False
    if (use_fields):
      ycor *= 2

    def BKE_camera_sensor_size(p_sensor_fit, sensor_x, sensor_y):
        #/* sensor size used to fit to. for auto, sensor_x is both x and y. */
        if (p_sensor_fit == 'VERTICAL'):
            return sensor_y;

        return sensor_x;

    if (camd.type == 'ORTHO'):
      #/* orthographic camera */
      #/* scale == 1.0 means exact 1 to 1 mapping */
      pixsize = camd.ortho_scale
    else:
      #/* perspective camera */
      sensor_size = BKE_camera_sensor_size(camd.sensor_fit, camd.sensor_width, camd.sensor_height)
      pixsize = (sensor_size * camd.clip_start) / camd.lens

    #/* determine sensor fit */
    def BKE_camera_sensor_fit(p_sensor_fit, sizex, sizey):
        if (p_sensor_fit == 'AUTO'):
            if (sizex >= sizey):
                return 'HORIZONTAL'
            else:
                return 'VERTICAL'

        return p_sensor_fit

    sensor_fit = BKE_camera_sensor_fit(camd.sensor_fit, xasp * winx, yasp * winy)

    if (sensor_fit == 'HORIZONTAL'):
      viewfac = winx
    else:
      viewfac = ycor * winy

    pixsize /= viewfac

    #/* extra zoom factor */
    pixsize *= 1 #params->zoom

    #/* compute view plane:
    # * fully centered, zbuffer fills in jittered between -.5 and +.5 */
    xmin = -0.5 * winx
    ymin = -0.5 * ycor * winy
    xmax =  0.5 * winx
    ymax =  0.5 * ycor * winy

    #/* lens shift and offset */
    dx = camd.shift_x * viewfac # + winx * params->offsetx
    dy = camd.shift_y * viewfac # + winy * params->offsety

    xmin += dx
    ymin += dy
    xmax += dx
    ymax += dy

    #/* fields offset */
    #if (params->field_second):
    #    if (params->field_odd):
    #        ymin -= 0.5 * ycor
    #        ymax -= 0.5 * ycor
    #    else:
    #        ymin += 0.5 * ycor
    #        ymax += 0.5 * ycor

    #/* the window matrix is used for clipping, and not changed during OSA steps */
    #/* using an offset of +0.5 here would give clip errors on edges */
    xmin *= pixsize
    xmax *= pixsize
    ymin *= pixsize
    ymax *= pixsize

    return xmin, xmax, ymin, ymax


def bl2veraCamera(source: bpy.types.RegionView3D | bpy.types.Object):
    cam = gl.Camera()

    view_matrix = None
    projection_matrix = None

    if isinstance(source, bpy.types.RegionView3D):
        projection_matrix = np.array(source.window_matrix)
        view_matrix = np.array(source.view_matrix.inverted())

    elif isinstance(source, bpy.types.Object):
        render = bpy.context.scene.render
        view_matrix = np.array(source.matrix_world.inverted())
        # projection_matrix = np.array(source.calc_matrix_camera(
        #     depsgraph=bpy.context.evaluated_depsgraph_get(),
        #     x=render.resolution_x,
        #     y=render.resolution_y,
        #     scale_x=render.pixel_aspect_x,
        #     scale_y=render.pixel_aspect_y,
        # ))

        left, right, bottom, top = view_plane(source.data, render.resolution_x, render.resolution_y, 1, 1)
        farClip, nearClip = source.data.clip_end, source.data.clip_start

        Xdelta = right - left
        Ydelta = top - bottom
        Zdelta = farClip - nearClip

        mat = [[0]*4 for i in range(4)]

        mat[0][0] = nearClip * 2 / Xdelta
        mat[1][1] = nearClip * 2 / Ydelta
        mat[2][0] = (right + left) / Xdelta #/* note: negate Z  */
        mat[2][1] = (top + bottom) / Ydelta
        mat[2][2] = -(farClip + nearClip) / Zdelta
        mat[2][3] = -1
        mat[3][2] = (-2 * nearClip * farClip) / Zdelta
        projection_matrix = mat
    
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

            # n = loop.normal
            n = bl_mesh.data.vertices[loop.vertex_index].normal
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

