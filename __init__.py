bl_info = {
    "name" : "glslViewer",
    "author" : "Patricio Gonzalez Vivo",
    "description" : "",
    "blender" : (2, 80, 0),
    "version" : (0, 0, 1),
    "location" : "",
    "warning" : "",
    "category" : "Generic"
}

from .scripts.gv_render_engine import register_render_engine, unregister_render_engine
from .scripts.gv_preferences import register_addon_preferences, unregister_addon_preferences
from .scripts.gv_operators import register_operators, unregister_operators
from .scripts.gv_render_ui import register_render_ui, unregister_render_ui

import importlib
from .scripts import developer_utility
importlib.reload(developer_utility)
modules = developer_utility.setup_addon_modules(
    __path__, __name__, "bpy" in locals()
)

def register():
    register_addon_preferences()
    register_render_engine()
    register_operators()
    register_render_ui()

def unregister():
    unregister_render_ui()
    unregister_operators()
    unregister_render_engine()
    unregister_addon_preferences()