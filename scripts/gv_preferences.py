import bpy

# Thank you https://devtalk.blender.org/t/how-to-save-custom-user-preferences-for-an-addon/10362 
def fetch_pref(name: str):
    prefs = bpy.context.preferences.addons['glslViewer'].preferences
    if prefs is None:
        return None
    return prefs[name]


class GlslViewerPreferences(bpy.types.AddonPreferences):
    bl_idname = "glslViewer"
    
    include_folder: bpy.props.StringProperty(
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
    bpy.utils.register_class(GlslViewerPreferences)


def unregister_addon_preferences():
    bpy.utils.unregister_class(GlslViewerPreferences)