import bpy
from .gv_render_engine import get_gv_engine

class GV_OT_print_defines(bpy.types.Operator):
    bl_idname = "wm.gv_print_defines"
    bl_label = "Print Defines"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.printDefines()

        return {'FINISHED'}

class GV_OT_print_buffers(bpy.types.Operator):
    bl_idname = "wm.gv_print_buffers"
    bl_label = "Print Defines"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.printBuffers()

        return {'FINISHED'}

class GV_OT_print_textures(bpy.types.Operator):
    bl_idname = "wm.gv_print_textures"
    bl_label = "Print Defines"
    bl_options = {'INTERNAL'}

    def execute(self, context):
        engine = get_gv_engine()
        if engine != None:
            engine.printTextures()

        return {'FINISHED'}

class GV_OT_listen(bpy.types.Operator):
    bl_idname = "wm.gv_listener"
    bl_label = "Listener"
    bl_options = {'INTERNAL'}

    def modal(self, context, event):
        print(event.type)
        return {'PASS_THROUGH'}

    def execute(self, context):
        wm = context.window_manager
        wm.modal_handler_add(self)
        return {'RUNNING_MODAL'}


classes = [
    GV_OT_print_defines,
    GV_OT_print_buffers,
    GV_OT_print_textures,
    GV_OT_listen
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