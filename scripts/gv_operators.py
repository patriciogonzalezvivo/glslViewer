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


class GV_OT_show_hide_passes(bpy.types.Operator):
    bl_idname = "glsl_viewer.show_hide_passes"
    bl_label = "Print Defines"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.showTextures( not engine.getShowTextures() )

        return {'FINISHED'}

classes = [
    GV_OT_print_defines,
    GV_OT_print_buffers,
    GV_OT_print_textures,
    GV_OT_show_hide_passes
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