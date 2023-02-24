import bpy

from .gv_render_engine import get_gv_engine
from .gv_preferences import PROPS


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
        engine = get_gv_engine()

        # split = layout.split(factor=0.7)
        col = layout.column()
        col.label(text="Go to Scripts and search for main.frag/vert shaders.")

        col = layout.column()
        for (prop_name, _) in PROPS:
            row = col.row()
            row.prop(context.scene, prop_name)

        if engine != None:
            row2 = layout.row(align=True)
            if engine.getShowPasses():
                row2.operator('glsl_viewer.hide_passes', text="Hide Passes")  
            else:
                row2.operator('glsl_viewer.show_passes', text="Show Passes")

            if engine.getShowTextures():
                row2.operator('glsl_viewer.hide_textures', text="Hide Textures")  
            else:
                row2.operator('glsl_viewer.show_textures', text="Show Textures")

            


def register_render_ui():
    bpy.utils.register_class(RENDER_PT_glslViewer_render)


def unregister_render_ui():
    bpy.utils.unregister_class(RENDER_PT_glslViewer_render)