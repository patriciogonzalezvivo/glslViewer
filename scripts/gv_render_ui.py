import bpy

from .gv_render_engine import PROPS


class RENDER_PT_glslViewer_render(bpy.types.Panel):
    bl_idname = "RENDER_PT_glslViewer_render"
    bl_label = "Render"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "render"

    def draw(self, context):
        self.layout.use_property_split = True
        self.layout.use_property_decorate = False

        if context.scene.render.engine != "GLSLVIEWER_ENGINE":
            return

        layout = self.layout

        col = layout.column()
        col.label(text="Go to Scripts and search for the following files:")

        col = layout.column()
        for (prop_name, _) in PROPS:
            row = col.row()
            row.prop(context.scene, prop_name)


def register_render_ui():
    bpy.utils.register_class(RENDER_PT_glslViewer_render)


def unregister_render_ui():
    bpy.utils.unregister_class(RENDER_PT_glslViewer_render)