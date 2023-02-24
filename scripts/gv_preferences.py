from bpy.types import AddonPreferences
from bpy.props import StringProperty
from bpy import utils

import bpy

PROPS = [
    ('glsl_viewer_vert', bpy.props.StringProperty(name='Vertex Shader Filename', default='main.vert')),
    ('glsl_viewer_frag', bpy.props.StringProperty(name='Fragment Shader Filename', default='main.frag')),
]

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
        default='.'
    )

    def draw(self, context):
        layout = self.layout
        row = layout.row()
        row.prop(self, 'include_folder')

def register_addon_preferences():
    for (prop_name, prop_value) in PROPS:
        setattr(bpy.types.Scene, prop_name, prop_value)

    utils.register_class(GlslViewerPreferences)


def unregister_addon_preferences():
    utils.unregister_class(GlslViewerPreferences)

    for (prop_name, _) in PROPS:
        delattr(bpy.types.Scene, prop_name)