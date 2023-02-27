import os
import numpy as np

import bpy
import bgl

from .gv_lib import GlslViewer as gv
from .gv_preferences import fetch_pref
from .gv_translate import bl2veraCamera, bl2veraMesh, bl2veraLight

from bpy.app.handlers import persistent


__GV_RENDER__ = None
def get_gv_render():
    global __GV_RENDER__
    return __GV_RENDER__


__GV_ENGINE__ = None
def get_gv_engine():
    global __GV_ENGINE__
    return __GV_ENGINE__


@persistent
def load_new_scene(dummy):
    print("Event: load_new_scene", bpy.data.filepath)

    global __GV_ENGINE__
    if __GV_ENGINE__:
        del __GV_ENGINE__ 
    __GV_ENGINE__ = None

    global __GV_RENDER__
    if __GV_RENDER__:
        del __GV_RENDER__ 
    __GV_RENDER__ = None


def file_exist(filename):
    try:
        file = open( bpy.path.abspath(filename) ,'r')
        file.close()
        return True
    except:
        return False


def update_areas():
    for window in bpy.context.window_manager.windows:
        for area in window.screen.areas:
            area.tag_redraw()


def fxaa_change(self, context):
    get_gv_engine().setFxaa( self.glsl_viewer_fxaa )
    get_gv_render().tag_redraw()


def show_cubemap_change(self, context):
    get_gv_engine().showCubemap( self.glsl_viewer_show_cubemap )
    get_gv_render().tag_redraw()


def show_textures_change(self, context):
    get_gv_engine().showTextures( self.glsl_viewer_show_textures )
    get_gv_render().tag_redraw()


def show_passes_change(self, context):
    get_gv_engine().showPasses( self.glsl_viewer_show_passes )
    get_gv_render().tag_redraw()


def show_histogram_change(self, context):
    get_gv_engine().showHistogram( self.glsl_viewer_show_histogram )
    get_gv_render().tag_redraw()


def show_skybox_turbidity_change(self, context):
    get_gv_engine().setSkyTurbidity( self.glsl_viewer_skybox_turbidity )
    get_gv_render().tag_redraw()


def show_debug_change(self, context):
    get_gv_engine().showBoudningBox( self.glsl_viewer_show_debug )
    get_gv_render().tag_redraw()


PROPS = [
    ('glsl_viewer_vert', bpy.props.StringProperty(name='Vertex Shader', default='main.vert')),
    ('glsl_viewer_frag', bpy.props.StringProperty(name='Fragment Shader', default='main.frag')),
    ('glsl_viewer_skybox_turbidity', bpy.props.FloatProperty(name='Skybox Turbidity', min=1, default=4.0, update=show_skybox_turbidity_change)),
    ('glsl_viewer_show_cubemap', bpy.props.BoolProperty(name='Show Cubemap', default=False, update=show_cubemap_change)),
    ('glsl_viewer_show_textures', bpy.props.BoolProperty(name='Show Textures', default=False, update=show_textures_change)),
    ('glsl_viewer_show_passes', bpy.props.BoolProperty(name='Show Passes', default=False, update=show_passes_change)),
    ('glsl_viewer_show_histogram', bpy.props.BoolProperty(name='Show Histogram', default=False, update=show_histogram_change)),
    ('glsl_viewer_show_debug', bpy.props.BoolProperty(name='Debug View', default=False, update=show_debug_change)),
    ('glsl_viewer_fxaa', bpy.props.BoolProperty(name='FXAA', default=False, update=fxaa_change)),
]


class GVRenderEngine(bpy.types.RenderEngine):
    '''
    These three members are used by blender to set up the
    RenderEngine; define its internal name, visible name and capabilities.
    '''

    bl_idname = "GLSLVIEWER_ENGINE"
    bl_label = "GlslViewer"
    bl_use_preview = True
    bl_use_image_save = True
    bl_use_gpu_context = True
    bl_use_postprocess = True
    bl_use_alembic_procedural = True
    bl_use_eevee_viewport = True
    bl_use_shading_nodes = False
    bl_use_shading_nodes_custom = False

    _frag_code = ""
    _vert_code = ""
    _render_started = False

    _preview_started = False
    _preview_skip = False

    def __init__(self):
        '''
        Init is called whenever a new render engine instance is created. Multiple
        instances may exist at the same time, for example for a viewport and final
        render.
        '''
        print("GVRenderEngine: __init__")
    
        global __GV_ENGINE__
        if __GV_ENGINE__ is None:
            __GV_ENGINE__ = gv.Engine()
            __GV_ENGINE__.include_folders = [ fetch_pref('include_folder') ]

        self.engine = __GV_ENGINE__

        global __GV_RENDER__
        if __GV_RENDER__ is None:
            __GV_RENDER__ = self


    def __del__(self):
        '''
        When the render engine instance is destroy, this is called. Clean up any
        render engine data here, for example stopping running render threads.
        '''
        print("GVRenderEngine: __del__")
        pass


    def start(self):
        if self._preview_started:
            return    
        print("GVRenderEngine: start")

        self._preview_started = True
        bpy.app.timers.register(self.event, first_interval=1)


    def event(self):
        # print("GVRenderEngine: event")
        if get_gv_render() == None:
            return

        if bpy.context.scene.render.engine != 'GLSLVIEWER_ENGINE':
            return

        self.update_shaders()

        return 1.0


    def reloadScene(self, depsgraph):
        # print("Reload entire scene")
        # view3d = context.space_data
        scene = depsgraph.scene

        self.engine.clearModels()
        for instance in depsgraph.object_instances:

            if instance.object.type == 'MESH':
                mesh = bl2veraMesh(instance.object)
                self.engine.loadMesh(instance.object.name_full, mesh)
                M = np.array(instance.object.matrix_world)
                self.engine.setMeshTransformMatrix( instance.object.name_full, 
                                                     M[0][0],  M[2][0],  M[1][0], M[3][0],
                                                     M[0][1],  M[2][1],  M[1][1], M[3][1],
                                                     M[0][2],  M[2][2],  M[1][2], M[3][2],
                                                    -M[0][3], -M[2][3], -M[1][3], M[3][3] )

            elif instance.object.type == 'LIGHT':
                sun = bl2veraLight(instance.object)
                self.engine.setSun(sun);    
            
            elif instance.object.type == 'CAMERA':
                self.engine.setCamera(bl2veraCamera(instance.object))
        
            elif not instance.object.name_full in bpy.data.objects:
                print("Don't know how to reload", instance.object.name_full, instance.object.type)

        self.update_images()
        self.update_shaders(True);

        self.engine.setFxaa( scene.glsl_viewer_fxaa )
        self.engine.enableCubemap( True )
        self.engine.showCubemap( scene.glsl_viewer_show_cubemap )
        self.engine.showTextures( scene.glsl_viewer_show_textures )
        self.engine.showPasses( scene.glsl_viewer_show_passes )
        clear_color = scene.world.color
        self.engine.setSkyGround( clear_color[0], clear_color[1], clear_color[2] )
        self.engine.setSkyTurbidity( scene.glsl_viewer_skybox_turbidity )
        self.engine.showBoudningBox( scene.glsl_viewer_show_debug )


    def view_update(self, context, depsgraph):
        '''
        For viewport renders. Blender calls view_update when starting viewport renders
        and/or something changes in the scene.
        This method is where data should be read from Blender in the same thread. 
        Typically a render thread will be started to do the work while keeping Blender responsive.
        '''
        # print("GVRenderEngine: view_update")
        scene = depsgraph.scene

        for update in depsgraph.updates:
            if update.id.name == "Scene":
                self._preview_skip = True
                # pass

            # else:
            if update.id.name in bpy.data.collections:
                self.reloadScene(depsgraph)
                self.tag_redraw()
                continue

            elif update.id.name == "World":
                clear_color = scene.world.color
                self.engine.setSkyGround( clear_color[0], clear_color[1], clear_color[2] )
                continue
    
            elif not update.id.name in bpy.data.objects:
                print("view_update: skipping update of", update.id.name)
                continue 
# 
            obj = bpy.data.objects[update.id.name]
            
            if obj.type == 'LIGHT':
                sun = bl2veraLight(obj)
                self.engine.setSun(sun);

            elif obj.type == 'MESH':
                if update.is_updated_geometry:
                    self.reloadScene(depsgraph)

                if update.is_updated_transform:
                    M = np.array(obj.matrix_world)
                    self.engine.setMeshTransformMatrix(  obj.name_full, 
                                                         M[0][0],  M[2][0],  M[1][0], M[3][0],
                                                         M[0][1],  M[2][1],  M[1][1], M[3][1],
                                                         M[0][2],  M[2][2],  M[1][2], M[3][2],
                                                        -M[0][3], -M[2][3], -M[1][3], M[3][3] )
                    
            elif obj.type == 'CAMERA':
                self.update_camera(context)

            else:
                print("GVRenderEngine: view_update -> obj type", obj.type)

        self.tag_redraw()


    def update_shaders(self, reload_shaders = False):
        context = bpy.context

        frag_filename = bpy.context.scene.glsl_viewer_frag
        vert_filename = bpy.context.scene.glsl_viewer_vert

        if not frag_filename in bpy.data.texts:
            print(f'File name {frag_filename} not found. Will create an internal one')

            if file_exist(frag_filename):
                bpy.ops.text.open(filepath=frag_filename)

            # else create a internal file with the default fragment code
            else:
                bpy.data.texts.new(frag_filename)
                # self._frag_code = self.engine.getSource(gv.FRAGMEN)
                self._frag_code = '''
#ifdef GL_ES
precision highp float;
#endif

uniform vec3        u_camera;

uniform vec2        u_resolution;
uniform float       u_time;
uniform int         u_frame;

varying vec4        v_position;

#ifdef MODEL_VERTEX_COLOR
varying vec4        v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3        v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2        v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

void main(void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 pos = v_position.xyz;

    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;

    color.rgb = pos;

    #ifdef MODEL_VERTEX_COLOR
    color = v_color;
    #endif

    #ifdef MODEL_VERTEX_NORMAL
    color.rgb = v_normal * 0.5 + 0.5;
    #endif

    #ifdef MODEL_VERTEX_TEXCOORD
    vec2 uv = v_texcoord;
    #else
    vec2 uv = st;
    #endif
    
    color.rg = uv;
    
    gl_FragColor = color;
}
                '''

                bpy.data.texts[frag_filename].write( self._frag_code )

        if not vert_filename in bpy.data.texts:
            print(f'File name {vert_filename} not found. Will create an internal one')

            if file_exist(vert_filename):
                bpy.ops.text.open(filepath=vert_filename)

            # else create a internal file with the default fragment code
            else:
                bpy.data.texts.new(vert_filename)
                # self._vert_code = self.engine.getSource(gv.VERTEX)
                self._vert_code = '''#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
uniform mat4    u_projectionMatrix;
uniform mat4    u_modelMatrix;
uniform mat4    u_viewMatrix;
uniform mat3    u_normalMatrix;

attribute vec4  a_position;
varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
attribute vec4  a_color;
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
attribute vec3  a_normal;
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
attribute vec2  a_texcoord;
varying vec2    v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
attribute vec4  a_tangent;
varying vec4    v_tangent;
varying mat3    v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
varying vec4    v_lightCoord;
#endif

void main(void) {
    
    v_position = u_modelMatrix * a_position;
    
#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = vec4(u_modelMatrix * vec4(a_normal, 0.0) ).xyz;
#endif
    
#ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
#endif
    
#ifdef MODEL_VERTEX_TANGENT
    v_tangent = a_tangent;
    vec3 worldTangent = a_tangent.xyz;
    vec3 worldBiTangent = cross(v_normal, worldTangent);// * sign(a_tangent.w);
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));
#endif
    
#ifdef LIGHT_SHADOWMAP
    v_lightCoord = u_lightMatrix * v_position;
#endif
    
    gl_Position = u_projectionMatrix * u_viewMatrix * v_position;
}'''
                bpy.data.texts[vert_filename].write( self._vert_code )

        for text in bpy.data.texts:

            # filename, file_extension = os.path.splitext(text.name_full)
            if text.name_full == frag_filename:
                if not text.is_in_memory and text.is_modified and context != None:
                    print(f'External file {frag_filename} have been modify. Reloading...')
                    ctx = context.copy()
                    #Ensure  context area is not None
                    ctx['area'] = ctx['screen'].areas[0]
                    oldAreaType = ctx['area'].type
                    ctx['area'].type = 'TEXT_EDITOR'
                    ctx['edit_text'] = text
                    bpy.ops.text.resolve_conflict(ctx, resolution='RELOAD')
                    #Restore context
                    ctx['area'].type = oldAreaType
            
                if text.as_string() != self._frag_code:
                    self._frag_code = text.as_string()
                    self.engine.setSource(gv.FRAGMENT, self._frag_code)
                    reload_shaders = True

            elif text.name_full == vert_filename:
                if not text.is_in_memory and text.is_modified and context != None:
                    print(f'External file {vert_filename} have been modify. Reloading...')
                    ctx = context.copy()
                    #Ensure  context area is not None
                    ctx['area'] = ctx['screen'].areas[0]
                    oldAreaType = ctx['area'].type
                    ctx['area'].type = 'TEXT_EDITOR'
                    ctx['edit_text'] = text
                    bpy.ops.text.resolve_conflict(ctx, resolution='RELOAD')
                    #Restore context
                    ctx['area'].type = oldAreaType

                if text.as_string() != self._vert_code:
                    self._vert_code = text.as_string()
                    self.engine.setSource(gv.VERTEX, self._vert_code)
                    reload_shaders = True

        if reload_shaders:
            self.engine.loadShaders()
            self.tag_redraw()


    def update_camera(self, context):
        region = context.region
        # Get viewport dimensions
        current_region3d: bpy.types.RegionView3D = None
        for area in context.screen.areas:
            area: bpy.types.Area
            if area.type == 'VIEW_3D':
                current_region3d = area.spaces.active.region_3d

                global __GV_ENGINE__
                if __GV_ENGINE__ is None or self.engine is None:
                    __GV_ENGINE__ = gv.Engine()
                    self.engine = __GV_ENGINE__
                    self.update_shaders(True);
                    self.tag_redraw()

                self.engine.setCamera( bl2veraCamera(current_region3d) )
                self.engine.resize(region.width, region.height)


    def update_images(self):
        for img in bpy.data.images:
            if img.name == 'Render Result' or img.name == 'Viewer Node':
                continue

            name, ext = os.path.splitext(img.name)
            name = name.replace(" ", "")

            if ext == '.hdr' or ext == '.HDR':
                if not self.engine.haveCubemap(name):
                    print("Add Cubemap", img.name, "as", name)
                    pixels = np.array(img.pixels)
                    self.engine.addCubemap(name, img.size[0], img.size[1], pixels)

            else:
                name = "u_" + name + "Tex"
                if not self.engine.haveTexture(name):
                    print("Add Texture", img.name, "as", name)
                    pixels = np.array(img.pixels)
                    self.engine.addTexture(name, img.size[0], img.size[1], pixels)


    def view_draw(self, context, depsgraph):
        '''
        For viewport renders. Blender calls view_draw whenever it redraws the 3D viewport.
        This is where we check for camera moves and draw pixels from our Blender display driver.
        The renderer is expected to quickly draw the render with OpenGL, and not perform other expensive work.
        Blender will draw overlays for selection and editing on top of the rendered image automatically.
        '''
        if self._preview_skip:
            self._preview_skip = False
            self.tag_redraw()
            return

        # print("GVRenderEngine: view_draw")
        scene = depsgraph.scene

        if not self._preview_started:
            self.start()
            self.reloadScene(depsgraph)

        for update in depsgraph.updates:
            print("GVRenderEngine: view_draw -> ", update.id.name)

        self.engine.setFrame(scene.frame_current)
        self.update_camera(context)
        self.update_images()

        bgl.glEnable(bgl.GL_DEPTH_TEST)
        bgl.glDepthMask(bgl.GL_TRUE)

        # # Clear background with the user's clear color
        clear_color = scene.world.color
        # bgl.glClearDepth(100000);
        bgl.glClearColor(clear_color[0], clear_color[1], clear_color[2], 1.0)
        bgl.glClear(bgl.GL_COLOR_BUFFER_BIT | bgl.GL_DEPTH_BUFFER_BIT)

        # bgl.glEnable(bgl.GL_BLEND)
        # bgl.glBlendFunc(bgl.GL_ONE, bgl.GL_ONE_MINUS_SRC_ALPHA)
        # self.bind_display_space_shader(scene)
        
        self.engine.draw()
        
        # self.unbind_display_space_shader()
        # bgl.glDisable(bgl.GL_BLEND)
        bgl.glDisable(bgl.GL_DEPTH_TEST)


    def render(self, depsgraph):
        '''
        Main render entry point. Blender calls this when doing final renders or preview renders.
        '''
        print("GVRenderEngine: render")

        scene = depsgraph.scene
        scale = scene.render.resolution_percentage / 100.0
        width = int(scene.render.resolution_x * scale)
        height = int(scene.render.resolution_y * scale)

        camera = self.camera_override

        if not self._preview_started:
            self._preview_started = True

        # Here we write the pixel values to the RenderResult
        filepath = bpy.context.scene.render.filepath
        absolutepath = bpy.path.abspath(filepath)
        out_image = os.path.join(absolutepath, '_render.png')

        # self.reloadScene(depsgraph)

        self.engine.setCamera(bl2veraCamera(camera))
        self.engine.resize(width, height)
        self.engine.setFrame(scene.frame_current)
        self.engine.setOutput(out_image)

        bgl.glEnable(bgl.GL_DEPTH_TEST)
        bgl.glDepthMask(bgl.GL_TRUE)
        self.engine.draw()
        bgl.glDisable(bgl.GL_DEPTH_TEST)

        result = self.begin_result(0, 0, width, height)
        lay = result.layers[0].passes["Combined"]
        try:
            lay.load_from_file(out_image)
        except:
            pass

        # layer = result.layers[0].passes["Combined"]
        # layer.rect = rect

        self.end_result(result)


    # def camera_model_matrix(self, camera, use_spherical_stereo):
    #     print("GVRenderEngine: camera_model_matrix")
    #     pass

    

def get_panels():
    '''
    RenderEngines also need to tell UI Panels that they are compatible with.
    We recommend to enable all panels marked as BLENDER_RENDER, and then
    exclude any panels that are replaced by custom panels registered by the
    render engine, or that are not supported.
    '''

    exclude_panels = {
        'VIEWLAYER_PT_filter',
        'VIEWLAYER_PT_layer_passes',
        'RENDER_PT_simplify',
        'RENDER_PT_color_management',
        'RENDER_PT_freestyle'
    }

    panels = []
    for panel in bpy.types.Panel.__subclasses__():
        if hasattr(panel, 'COMPAT_ENGINES') and 'BLENDER_RENDER' in panel.COMPAT_ENGINES:
            if panel.__name__ not in exclude_panels:
                panels.append(panel)

    return panels


def register_render_engine():
    # Register the RenderEngine
    bpy.utils.register_class(GVRenderEngine)
    bpy.app.handlers.load_pre.append(load_new_scene)

    for (prop_name, prop_value) in PROPS:
        setattr(bpy.types.Scene, prop_name, prop_value)

    for panel in get_panels():
        panel.COMPAT_ENGINES.add('GLSLVIEWER_ENGINE')


def unregister_render_engine():
    bpy.utils.unregister_class(GVRenderEngine)
    bpy.app.handlers.load_pre.remove(load_new_scene)

    for (prop_name, _) in PROPS:
        delattr(bpy.types.Scene, prop_name)

    for panel in get_panels():
        if 'GLSLVIEWER_ENGINE' in panel.COMPAT_ENGINES:
            panel.COMPAT_ENGINES.remove('GLSLVIEWER_ENGINE')


if __name__ == "__main__":
    register_render_engine()