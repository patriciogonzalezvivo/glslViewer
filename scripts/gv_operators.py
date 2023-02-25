import bpy
from .gv_render_engine import get_gv_engine

class GV_OT_print_defines(bpy.types.Operator):
    bl_idname = "glsl_viewer.print_defines"
    bl_label = "Print Defines"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.printDefines()

        return {'FINISHED'}


class GV_OT_print_buffers(bpy.types.Operator):
    bl_idname = "glsl_viewer.print_buffers"
    bl_label = "Print Defines"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.printBuffers()

        return {'FINISHED'}


class GV_OT_print_textures(bpy.types.Operator):
    bl_idname = "glsl_viewer.print_textures"
    bl_label = "Print Defines"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.printTextures()

        return {'FINISHED'}


class GV_OT_show_passes(bpy.types.Operator):
    bl_idname = "glsl_viewer.show_passes"
    bl_label = "Show Passes"
    bl_description = "Hide active render passes and buffers on the active viewport"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.showPasses( True )

        return {'FINISHED'}


class GV_OT_hide_passes(bpy.types.Operator):
    bl_idname = "glsl_viewer.hide_passes"
    bl_label = "Hide Passes"
    bl_description = "Hide active render passes and buffers on the active viewport"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.showPasses( False )

        return {'FINISHED'}
    

class GV_OT_show_textures(bpy.types.Operator):
    bl_idname = "glsl_viewer.show_textures"
    bl_label = "Show textures"
    bl_description = "Show loaded textures on the active viewport"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.showTextures( True )

        return {'FINISHED'}


class GV_OT_hide_textures(bpy.types.Operator):
    bl_idname = "glsl_viewer.hide_textures"
    bl_label = "Hide Textures"
    bl_description = "Hide loaded textures on the active viewport"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.showTextures( False )

        return {'FINISHED'}


class GV_OT_show_cubemap(bpy.types.Operator):
    bl_idname = "glsl_viewer.show_cubemap"
    bl_label = "Show Cubemap"
    bl_description = "Show cubemap on the active viewport"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.showCubemap( True )

        return {'FINISHED'}
    

class GV_OT_hide_cubemap(bpy.types.Operator):
    bl_idname = "glsl_viewer.hide_cubemap"
    bl_label = "Hide Cubempa"
    bl_description = "Hide cubemap on the active viewport"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.showCubemap( False )

        return {'FINISHED'}


class GV_OT_enable_cubemap(bpy.types.Operator):
    bl_idname = "glsl_viewer.enable_cubemap"
    bl_label = "Enable cubemaps"
    bl_description = "Enable cubemap on the active viewport"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.enableCubemap( True )

        return {'FINISHED'}

    
class GV_OT_disable_cubemap(bpy.types.Operator):
    bl_idname = "glsl_viewer.disable_cubemap"
    bl_label = "Disable Cubemaps"
    bl_description = "Disable cubemap on the active viewport"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.enableCubemap( False )

        return {'FINISHED'}
        


classes = [
    GV_OT_print_defines,
    GV_OT_print_buffers,
    GV_OT_print_textures,
    GV_OT_show_passes,
    GV_OT_hide_passes,
    GV_OT_show_textures,
    GV_OT_hide_textures,
    GV_OT_show_cubemap,
    GV_OT_hide_cubemap,
    GV_OT_enable_cubemap,
    GV_OT_disable_cubemap,
]

def register_operators():
    for cls in classes:
        bpy.utils.register_class(cls)
    
def unregister_operators():
    for cls in classes:
        try:
            bpy.utils.unregister_class(cls)
        except RuntimeError:
            pass  