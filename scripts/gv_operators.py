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

classes = [
    GV_OT_print_defines,
    GV_OT_print_buffers,
    GV_OT_print_textures,
    GV_OT_show_passes,
    GV_OT_hide_passes,
    GV_OT_show_textures,
    GV_OT_hide_textures,
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