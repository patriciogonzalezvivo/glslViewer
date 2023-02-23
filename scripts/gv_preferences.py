from bpy.types import Panel, AddonPreferences
from bpy.props import StringProperty
from bpy import utils

import bpy

class HelloWorldOperator(bpy.types.Operator):
    bl_idname = "wm.hello_world"
    bl_label = "Minimal Operator"

    def execute(self, context):
        print("Hello World")
        return {'FINISHED'}

class RENDER_PT_glslViewer_render(Panel):
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

        split = layout.split(factor=0.7)
        col = split.column()
        col.label(text="NON-COMMERCIAL VERSION")

    

# Thank you https://devtalk.blender.org/t/how-to-save-custom-user-preferences-for-an-addon/10362 
def fetch_pref(name: str):
    prefs = bpy.context.preferences.addons['glslViewer'].preferences
    if prefs is None:
        return None
    return prefs[name]

class GlslViewerPreferences(AddonPreferences):
    bl_idname = "glslViewer"
    
    include_folder: StringProperty(
        name="Include Folder",
        description = "Folder use to resolve GLSL #include dependencies. For example: what ever folder containing LYGIA",
        subtype='DIR_PATH',
    )

    def draw(self, context):
        layout = self.layout
        row = layout.row()
        row.prop(self, 'include_folder')

def register_addon_preferences():
    utils.register_class(RENDER_PT_glslViewer_render)
    utils.register_class(GlslViewerPreferences)


def unregister_addon_preferences():
    utils.unregister_class(RENDER_PT_glslViewer_render)
    utils.unregister_class(GlslViewerPreferences)