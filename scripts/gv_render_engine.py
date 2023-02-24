import bpy
import bgl
import functools

from .pylib import GlslViewer as gv
from .tools import bl2npMatrix, bl2veraCamera, bl2veraMesh, bl2veraLight, file_exist
from .gv_preferences import fetch_pref

__GV_RENDER__ = None
__GV_ENGINE__ = None

def get_gv_render():
    global __GV_RENDER__
    return __GV_RENDER__

def get_gv_engine():
    global __GV_ENGINE__
    return __GV_ENGINE__

def update_areas():
    for window in bpy.context.window_manager.windows:
        for area in window.screen.areas:
            area.tag_redraw()

class GVRenderEngine(bpy.types.RenderEngine):
    '''
    These three members are used by blender to set up the
    RenderEngine; define its internal name, visible name and capabilities.
    '''

    bl_idname = "GLSLVIEWER_ENGINE"
    bl_label = "GlslViewer"
    bl_use_preview = True
    # bl_use_postprocess = True
    # bl_use_exclude_layers = True
    # bl_use_save_buffers = True
    bl_use_eevee_viewport = True
    # bl_use_shading_nodes = True
    # bl_use_custom_freestyle = True
    # bl_use_shading_nodes_custom = False

    _frag_filename = "main.frag"
    _frag_code = ""
    _vert_filename = "main.vert"
    _vert_code = ""
    _started = False
    _skip = False

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
        self.engine = __GV_ENGINE__
        self.engine.include_folders = [ fetch_pref('include_folder') ]

        global __GV_RENDER__
        if __GV_RENDER__ is None:
            __GV_RENDER__ = self

    def __del__(self):
        '''
        When the render engine instance is destroy, this is called. Clean up any
        render engine data here, for example stopping running render threads.
        '''
        print("GVRenderEngine: __del__")

        global __GV_RENDER__
        __GV_RENDER__ = None


    def start(self):
        # if self._started:
        #     return    
        print("GVRenderEngine: start")

        self._started = True

        if not self._frag_filename in bpy.data.texts:
            print(f'File name {self._frag_filename} not found. Will create an internal one')

            if file_exist(self._frag_filename):
                    bpy.ops.text.open(filepath=self._frag_filename)

            # else create a internal file with the default fragment code
            else:
                bpy.data.texts.new(self._frag_filename)
                self._frag_code = self.engine.getSource(gv.FRAGMENT)
                bpy.data.texts[self._frag_filename].write( self._frag_code )

        if not self._vert_filename in bpy.data.texts:
            print(f'File name {self._vert_filename} not found. Will create an internal one')

            if file_exist(self._vert_filename):
                    bpy.ops.text.open(filepath=self._vert_filename)

            # else create a internal file with the default fragment code
            else:
                bpy.data.texts.new(self._vert_filename)
                self._vert_code = self.engine.getSource(gv.VERTEX)
                bpy.data.texts[self._vert_filename].write( self._vert_code )

        bpy.app.timers.register(self.event, first_interval=1)


    @classmethod
    def getEngine(self):
        global __GV_ENGINE__
        if __GV_ENGINE__ is None:
            __GV_ENGINE__ = self.engine

        return __GV_ENGINE__

    def event(self):
        # print("GVRenderEngine: event")
        if get_gv_render() == None:
            return

        if bpy.context.scene.render.engine != 'GLSLVIEWER_ENGINE':
            return

        self.update_shaders()

        return 1.0


    def reloadScene(self, context, depsgraph):
        # print("Reload entire scene")
        self.engine.clearModels()
        for instance in depsgraph.object_instances:

            if instance.object.type == 'MESH':
                mesh = bl2veraMesh(instance.object)
                self.engine.loadMesh(instance.object.name_full, mesh)
                M = bl2npMatrix(instance.object.matrix_world)
                self.engine.setMeshTransformMatrix(  instance.object.name_full, 
                                                        M[0][0], M[2][0], M[1][0], M[3][0],
                                                        M[0][1], M[2][1], M[1][1], M[3][1],
                                                        M[0][2], M[2][2], M[1][2], M[3][2],
                                                        -M[0][3], -M[2][3], -M[1][3], M[3][3] )

            elif instance.object.type == 'LIGHT':
                sun = bl2veraLight(instance.object)
                self.engine.setSun(sun);

        self.update_camera(context)
        self.update_shaders(context, True);
        self.tag_redraw()


    def update(self, data, depsgraph):
        print("GVRenderEngine: update")
        scene = depsgraph.scene
        pass


    def view_update(self, context, depsgraph):
        '''
        For viewport renders. Blender calls view_update when starting viewport renders
        and/or something changes in the scene.
        This method is where data should be read from Blender in the same thread. 
        Typically a render thread will be started to do the work while keeping Blender responsive.
        '''
        print("GVRenderEngine: view_update")

        region = context.region
        view3d = context.space_data
        scene = depsgraph.scene

        for update in depsgraph.updates:
            if update.id.name == "Scene":
                self._skip = True
                # pass

            # else:
            if update.id.name in bpy.data.collections:
                self.reloadScene(context, depsgraph)
                continue

            elif not update.id.name in bpy.data.objects:
                print("Don't know how to update", update.id.name)
                # self.update_camera(context)
                continue 
# 
            obj = bpy.data.objects[update.id.name]
            
            if obj.type == 'LIGHT':
                sun = bl2veraLight(obj)
                self.engine.setSun(sun);

            elif obj.type == 'MESH':
                if update.is_updated_geometry:
                    self.reloadScene(context, depsgraph)

                if update.is_updated_transform:
                    M = bl2npMatrix(obj.matrix_world)
                    self.engine.setMeshTransformMatrix(  obj.name_full, 
                                                            M[0][0], M[2][0], M[1][0], M[3][0],
                                                            M[0][1], M[2][1], M[1][1], M[3][1],
                                                            M[0][2], M[2][2], M[1][2], M[3][2],
                                                            -M[0][3], -M[2][3], -M[1][3], M[3][3] )

            elif obj.type == 'CAMERA':
                self.update_camera(context)

                # dimensions = region.width, region.height
                # self.engine.resize(region.width, region.height)
                # self.engine.setCamera( bl2veraCamera(obj, dimensions) )

            else:
                print("GVRenderEngine: view_update -> obj type", obj.type)

        self.tag_redraw()


    def update_shaders(self, context = None, reload_shaders = False):
        # print("GVRenderEngine: update_shaders")
        for text in bpy.data.texts:

            # filename, file_extension = os.path.splitext(text.name_full)
            if text.name_full == self._frag_filename:
                if not text.is_in_memory and text.is_modified and context != None:
                    print(f'External file {self._frag_filename} have been modify. Reloading...')
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
                    # print("setSource FRAG")
                    self._frag_code = text.as_string()
                    self.engine.setSource(gv.FRAGMENT, self._frag_code)
                    reload_shaders = True

            elif text.name_full == self._vert_filename:
                if not text.is_in_memory and text.is_modified and context != None:
                    print(f'External file {self._vert_filename} have been modify. Reloading...')
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
                    # print("setSource VERT")
                    self._vert_code = text.as_string()
                    self.engine.setSource(gv.VERTEX, self._vert_code)
                    reload_shaders = True

        if reload_shaders:
            self.engine.reloadShaders()
            self.tag_redraw()


    def update_camera(self, context):
        region = context.region
        # Get viewport dimensions
        dimensions = region.width, region.height
        current_region3d: bpy.types.RegionView3D = None
        for area in context.screen.areas:
            area: bpy.types.Area
            if area.type == 'VIEW_3D':
                current_region3d = area.spaces.active.region_3d

                global __GV_ENGINE__
                if __GV_ENGINE__ is None or self.engine is None:
                    __GV_ENGINE__ = gv.Engine()
                    self.engine = __GV_ENGINE__
                    self.update_shaders(context, True);
                    self.tag_redraw()

                self.engine.resize(region.width, region.height)
                self.engine.setCamera( bl2veraCamera(current_region3d, dimensions) )


    def view_draw(self, context, depsgraph):
        '''
        For viewport renders. Blender calls view_draw whenever it redraws the 3D viewport.
        This is where we check for camera moves and draw pixels from our Blender display driver.
        The renderer is expected to quickly draw the render with OpenGL, and not perform other expensive work.
        Blender will draw overlays for selection and editing on top of the rendered image automatically.
        '''
        if self._skip:
            self._skip = False
            self.tag_redraw()
            return

        print("GVRenderEngine: view_draw")
        scene = depsgraph.scene

        if not self._started:
            self.start()
            self.reloadScene(context, depsgraph)

        for update in depsgraph.updates:
            print("GVRenderEngine: view_draw -> ", update.id.name)

        self.update_camera(context);
        self.engine.setFrame( scene.frame_current )

        bgl.glEnable(bgl.GL_DEPTH_TEST)
        bgl.glDepthMask(bgl.GL_TRUE)
        bgl.glClearDepth(100000);
        bgl.glClearColor(0.0, 0.0, 0.0, 0.0);
        bgl.glClear(bgl.GL_COLOR_BUFFER_BIT | bgl.GL_DEPTH_BUFFER_BIT)

        bgl.glEnable(bgl.GL_BLEND)
        bgl.glBlendFunc(bgl.GL_ONE, bgl.GL_ONE_MINUS_SRC_ALPHA)
        self.bind_display_space_shader(scene)
        
        self.engine.draw()
        
        self.unbind_display_space_shader()
        bgl.glDisable(bgl.GL_BLEND)
        bgl.glDisable(bgl.GL_DEPTH_TEST)

    # This is the method called by Blender for both final renders (F12) and
    # small preview for materials, world and lights.
    def render(self, depsgraph):
        print("GVRenderEngine: render")

        scene = depsgraph.scene
        scale = scene.render.resolution_percentage / 100.0
        width = int(scene.render.resolution_x * scale)
        height = int(scene.render.resolution_y * scale)

        # Fill the render result with a flat color. The framebuffer is
        # defined as a list of pixels, each pixel itself being a list of
        # R,G,B,A values.
        if self.is_preview:
            color = [0.1, 0.2, 0.1, 1.0]
        else:
            color = [0.2, 0.1, 0.1, 1.0]

        pixel_count = self.width * self.height
        rect = [color] * pixel_count

        # Here we write the pixel values to the RenderResult
        result = self.begin_result(0, 0, width, height)
        layer = result.layers[0].passes["Combined"]
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

    for panel in get_panels():
        panel.COMPAT_ENGINES.add('GLSLVIEWER_ENGINE')


def unregister_render_engine():
    bpy.utils.unregister_class(GVRenderEngine)

    for panel in get_panels():
        if 'GLSLVIEWER_ENGINE' in panel.COMPAT_ENGINES:
            panel.COMPAT_ENGINES.remove('GLSLVIEWER_ENGINE')


if __name__ == "__main__":
    register_render_engine()