import os
import numpy as np

import bpy
import bgl
import gpu

from .gv_lib import GlslViewer as gv
from .gv_preferences import fetch_pref, default_vert_code, default_frag_code
from .gv_translate import bl2veraCamera, bl2veraMesh, bl2veraLight

from bpy.app.handlers import persistent


__GV_PREVIEW_RENDER__ = None

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

    global __GV_PREVIEW_RENDER__
    if __GV_PREVIEW_RENDER__:
        del __GV_PREVIEW_RENDER__ 
    __GV_PREVIEW_RENDER__ = None


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
    engine = get_gv_engine()
    if engine != None:
        engine.setFxaa( self.glsl_viewer_fxaa )
        engine.tag_redraw()


def enable_cubemap_change(self, context):
    engine = get_gv_engine()
    if engine != None:
        engine.enableCubemap( self.glsl_viewer_enable_cubemap )
        engine.tag_redraw()


def show_cubemap_change(self, context):
    engine = get_gv_engine()
    if engine != None:
        engine.showCubemap( self.glsl_viewer_show_cubemap )
        engine.tag_redraw()


def show_textures_change(self, context):
    engine = get_gv_engine()
    if engine != None:
        engine.showTextures( self.glsl_viewer_show_textures )
        engine.tag_redraw()


def show_passes_change(self, context):
    engine = get_gv_engine()
    if engine != None:
        engine.showPasses( self.glsl_viewer_show_passes )
        engine.tag_redraw()


def show_histogram_change(self, context):
    engine = get_gv_engine()
    if engine != None:
        engine.showHistogram( self.glsl_viewer_show_histogram )
        engine.tag_redraw()


def show_skybox_turbidity_change(self, context):
    engine = get_gv_engine()
    if engine != None:
        engine.setSkyTurbidity( self.glsl_viewer_skybox_turbidity )
        engine.tag_redraw()


def show_debug_change(self, context):
    engine = get_gv_engine()
    if engine != None:
        engine.showBoudningBox( self.glsl_viewer_show_debug )
        engine.tag_redraw()


PROPS = [
    ('glsl_viewer_vert', bpy.props.StringProperty(name='Vertex Shader', default='main.vert')),
    ('glsl_viewer_frag', bpy.props.StringProperty(name='Fragment Shader', default='main.frag')),
    ('glsl_viewer_skybox_turbidity', bpy.props.FloatProperty(name='Skybox Turbidity', min=1, default=4.0, update=show_skybox_turbidity_change)),
    ('glsl_viewer_enable_cubemap', bpy.props.BoolProperty(name='Enable Cubemap', default=True, update=enable_cubemap_change)),
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
    # bl_use_image_save = True
    bl_use_postprocess = True
    # bl_use_alembic_procedural = True
    bl_use_eevee_viewport = True
    bl_use_shading_nodes = False
    bl_use_shading_nodes_custom = False

    frag_code = ""
    vert_code = ""

    preview_engine = None
    preview_skip_draw = False
    preview_shader_checker = False


    def __init__(self):
        '''
        Init is called whenever a new render engine instance is created. Multiple
        instances may exist at the same time, for example for a viewport and final
        render.
        '''
        print("GVRenderEngine: __init__")


    def __del__(self):
        '''
        When the render engine instance is destroy, this is called. Clean up any
        render engine data here, for example stopping running render threads.
        '''
        print("GVRenderEngine: __del__")
        pass


    def initPreview(self, depsgraph):
        print("GVRenderEngine: initPreview")
        global __GV_ENGINE__
        if self.preview_engine != None and __GV_ENGINE__ != None:
            return 
    
        if __GV_ENGINE__ is None:
            __GV_ENGINE__ = gv.Engine()
            __GV_ENGINE__.init()
            if fetch_pref('include_folder') != '.':
                __GV_ENGINE__.include_folders = [ fetch_pref('include_folder') ]
        self.preview_engine = __GV_ENGINE__
        self.reloadScene(self.preview_engine, depsgraph)

        global __GV_PREVIEW_RENDER__
        if __GV_PREVIEW_RENDER__ is None:
            __GV_PREVIEW_RENDER__ = self


    def closePreview(self):
        print("GVRenderEngine: closePreview")

        global __GV_ENGINE__
        __GV_ENGINE__ = None
        self.preview_engine = None


    def start_checking_changes_on_shaders(self):
        print("GVRenderEngine: start")        
        if self.preview_shader_checker:
            return

        self.preview_shader_checker = True
        bpy.app.timers.register(self.check_for_changes_on_shaders, first_interval=1)


    def check_for_changes_on_shaders(self):
        # print("GVRenderEngine: check_for_changes_on_shaders")
        if bpy.context.scene.render.engine != 'GLSLVIEWER_ENGINE':
            return
        
        if self.preview_engine == None:
            return

        self.update_shaders(self.preview_engine)
        return 1.0


    def reloadScene(self, engine, depsgraph):
        print("Reload entire scene")

        engine.clearModels()
        for instance in depsgraph.object_instances:

            if instance.object.type == 'MESH':
                mesh = bl2veraMesh(instance.object)
                engine.loadMesh(instance.object.name_full, mesh)
                M = np.array(instance.object.matrix_world)
                engine.setMeshTransformMatrix( instance.object.name_full, 
                                                     M[0][0],  M[2][0],  M[1][0], M[3][0],
                                                     M[0][1],  M[2][1],  M[1][1], M[3][1],
                                                     M[0][2],  M[2][2],  M[1][2], M[3][2],
                                                    -M[0][3], -M[2][3], -M[1][3], M[3][3] )

            elif instance.object.type == 'LIGHT':
                sun = bl2veraLight(instance.object)
                engine.setSun(sun);    
            
            elif instance.object.type == 'CAMERA':
                engine.setCamera(bl2veraCamera(instance.object))
        
            elif not instance.object.name_full in bpy.data.objects:
                print("Don't know how to reload", instance.object.name_full, instance.object.type)

        self.update_shaders(engine, True)
        self.update_images(engine)

        scene = depsgraph.scene
        engine.setFxaa( scene.glsl_viewer_fxaa )
        engine.enableCubemap( True )
        engine.showCubemap( scene.glsl_viewer_show_cubemap )
        engine.showTextures( scene.glsl_viewer_show_textures )
        engine.showPasses( scene.glsl_viewer_show_passes )
        engine.setSkyGround( scene.world.color[0], scene.world.color[1], scene.world.color[2] )
        engine.setSkyTurbidity( scene.glsl_viewer_skybox_turbidity )
        engine.showBoudningBox( scene.glsl_viewer_show_debug )


    def update_images(self, engine):
        for img in bpy.data.images:
            if img.name == 'Render Result' or img.name == 'Viewer Node':
                continue

            name, ext = os.path.splitext(img.name)
            name = name.replace(" ", "")

            if ext == '.hdr' or ext == '.HDR':
                if not engine.haveCubemap(name):
                    print("Add Cubemap", img.name, "as", name)
                    pixels = np.array(img.pixels)
                    engine.addCubemap(name, img.size[0], img.size[1], pixels)

            else:
                name = "u_" + name + "Tex"
                if not engine.haveTexture(name):
                    print("Add Texture", img.name, "as", name)
                    pixels = np.array(img.pixels)
                    engine.addTexture(name, img.size[0], img.size[1], pixels)

    
    def update_shaders(self, engine, reload_shaders = False):
        context = bpy.context

        frag_filename = bpy.context.scene.glsl_viewer_frag
        vert_filename = bpy.context.scene.glsl_viewer_vert

        if not frag_filename in bpy.data.texts:
            print(f'File name {frag_filename} not found. Will create an internal one')

            if file_exist(frag_filename):
                bpy.ops.text.open(filepath=frag_filename)
            else:
                bpy.data.texts.new(frag_filename)
                self.frag_code = default_frag_code
                bpy.data.texts[frag_filename].write( self.frag_code )

        if not vert_filename in bpy.data.texts:
            print(f'File name {vert_filename} not found. Will create an internal one')

            if file_exist(vert_filename):
                bpy.ops.text.open(filepath=vert_filename)
            else:
                bpy.data.texts.new(vert_filename)
                self.vert_code = default_vert_code
                bpy.data.texts[vert_filename].write( self.vert_code )

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
            
                if text.as_string() != self.frag_code:
                    self.frag_code = text.as_string()
                    engine.setSource(gv.FRAGMENT, self.frag_code)
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

                if text.as_string() != self.vert_code:
                    self.vert_code = text.as_string()
                    engine.setSource(gv.VERTEX, self.vert_code)
                    reload_shaders = True

        if reload_shaders:
            engine.loadShaders()
            self.tag_redraw()


    def view_update(self, context, depsgraph):
        '''
        For viewport renders. Blender calls view_update when starting viewport renders
        and/or something changes in the scene.
        This method is where data should be read from Blender in the same thread. 
        Typically a render thread will be started to do the work while keeping Blender responsive.
        '''
        print("GVRenderEngine: view_update")

        # Make sure that the preview engine is running
        if self.preview_engine == None:
            self.initPreview(depsgraph)

        for update in depsgraph.updates:
            if update.id.name == "Scene":
                self.preview_skip_draw = True
                # pass

            # else:
            if update.id.name in bpy.data.collections:
                self.reloadScene(self.preview_engine, depsgraph)
                self.tag_redraw()
                continue

            elif update.id.name == "World":
                clear_color = depsgraph.scene.world.color
                self.preview_engine.setSkyGround( clear_color[0], clear_color[1], clear_color[2] )
                continue
    
            elif not update.id.name in bpy.data.objects:
                print("view_update: skipping update of", update.id.name)
                continue 

            obj = bpy.data.objects[update.id.name]
            
            if obj.type == 'LIGHT':
                sun = bl2veraLight(obj)
                self.preview_engine.setSun(sun);

            elif obj.type == 'MESH':
                if update.is_updated_geometry:
                    self.reloadScene(self.preview_engine, depsgraph)

                if update.is_updated_transform:
                    M = np.array(obj.matrix_world)
                    self.preview_engine.setMeshTransformMatrix(  obj.name_full, 
                                                         M[0][0],  M[2][0],  M[1][0], M[3][0],
                                                         M[0][1],  M[2][1],  M[1][1], M[3][1],
                                                         M[0][2],  M[2][2],  M[1][2], M[3][2],
                                                        -M[0][3], -M[2][3], -M[1][3], M[3][3] )
            
            elif obj.type == 'CAMERA':
                pass

            else:
                print("GVRenderEngine: view_update -> obj type", obj.type)

        # self.tag_redraw()


    def view_draw(self, context, depsgraph):
        '''
        For viewport renders. Blender calls view_draw whenever it redraws the 3D viewport.
        This is where we check for camera moves and draw pixels from our Blender display driver.
        The renderer is expected to quickly draw the render with OpenGL, and not perform other expensive work.
        Blender will draw overlays for selection and editing on top of the rendered image automatically.
        '''
        print("GVRenderEngine: view_draw")

        if self.preview_skip_draw:
            self.preview_skip_draw = False
            self.tag_redraw()
            return
        
        # Make sure that the preview engine is running
        if self.preview_engine == None:
            self.initPreview(depsgraph)

        if not self.preview_shader_checker:
            self.start_checking_changes_on_shaders()
            # self.reloadScene(self.preview_engine, depsgraph)

        # Get viewport camera
        region = context.region
        current_region3d: bpy.types.RegionView3D = None
        for area in context.screen.areas:
            area: bpy.types.Area
            if area.type == 'VIEW_3D':
                current_region3d = area.spaces.active.region_3d
                self.preview_engine.setCamera( bl2veraCamera(current_region3d) )
                self.preview_engine.resize(region.width, region.height)

        self.preview_engine.setFrame(depsgraph.scene.frame_current)
        self.update_images(self.preview_engine)

        bgl.glEnable(bgl.GL_DEPTH_TEST)
        bgl.glDepthMask(bgl.GL_TRUE)
        bgl.glDepthRange(0,1)
        bgl.glClearColor(depsgraph.scene.world.color[0], depsgraph.scene.world.color[1], depsgraph.scene.world.color[2], 1.0)
        bgl.glClear(bgl.GL_COLOR_BUFFER_BIT | bgl.GL_DEPTH_BUFFER_BIT)

        self.preview_engine.draw()

        bgl.glDisable(bgl.GL_DEPTH_TEST)


    def render(self, depsgraph):
        '''
        Main render entry point. Blender calls this when doing final renders or preview renders.
        '''
        print("GVRenderEngine: render")

        global __GV_PREVIEW_RENDER__
        if __GV_PREVIEW_RENDER__:
            __GV_PREVIEW_RENDER__.closePreview()
        
        scene = depsgraph.scene
        scale = scene.render.resolution_percentage / 100.0
        width = int(scene.render.resolution_x * scale)
        height = int(scene.render.resolution_y * scale)
        camera = self.camera_override
        filepath = bpy.context.scene.render.filepath
        absolutepath = bpy.path.abspath(filepath)
        out_image = os.path.join(absolutepath, '_render.png')

        final_engine = gv.Headless()
        if fetch_pref('include_folder') != '.':
            final_engine.include_folders = [ fetch_pref('include_folder') ]
        final_engine.init()
        self.reloadScene(final_engine, depsgraph)
        self.update_shaders(final_engine, True)
        final_engine.setSource(gv.FRAGMENT, self.frag_code)
        final_engine.setSource(gv.VERTEX, self.vert_code)
        final_engine.loadShaders()
        final_engine.setCamera( bl2veraCamera(camera) )
        final_engine.resize(width, height)
        final_engine.setFrame( scene.frame_current )
        final_engine.showTextures( False)
        final_engine.showPasses( False )
        final_engine.showBoudningBox( False )
        final_engine.setOutput( out_image )

        bgl.glClearColor(depsgraph.scene.world.color[0], depsgraph.scene.world.color[1], depsgraph.scene.world.color[2], 1.0)
        bgl.glClear(bgl.GL_COLOR_BUFFER_BIT)

        final_engine.draw()

        final_engine.close()

        result = self.begin_result(0, 0, width, height)

        layer = result.layers[0]
        layer.load_from_file(out_image)

        self.end_result(result)


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