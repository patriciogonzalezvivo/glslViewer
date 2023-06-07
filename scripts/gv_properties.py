import bpy

from .gv_render_engine import get_gv_render, get_gv_engine

blend_mode = [
    ("NONE", "None", "", 0),
    ("ALPHA", "Alpha", "", 1),
    ("ADD", "Add", "", 2),
    ("MULTIPLY", "Multiply", "", 3),
    ("SCREEN", "Screen", "", 4),
    ("SUBSTRACT", "Substract", "", 5),
]

def get_blend(self):
    engine = get_gv_engine()
    if engine:
        return engine.getBlendMode()
    return 0


def set_blend(self, value):
    engine = get_gv_engine()
    if engine:
        engine.setBlendMode( value )

    render = get_gv_render()
    if render:
        render.tag_redraw()

def fxaa_change(self, context):
    engine = get_gv_engine()
    if engine:
        engine.setFxaa( self.glsl_viewer_fxaa )

    render = get_gv_render()
    if render:
        render.tag_redraw()


def enable_cubemap_change(self, context):
    engine = get_gv_engine()
    if engine:
        engine.enableCubemap( self.glsl_viewer_enable_cubemap )

    render = get_gv_render()
    if render:
        render.tag_redraw()


def show_cubemap_change(self, context):
    engine = get_gv_engine()
    if engine:
        engine.showCubemap( self.glsl_viewer_show_cubemap )
        
    render = get_gv_render()
    if render:
        render.tag_redraw()


def show_textures_change(self, context):
    engine = get_gv_engine()
    if engine:
        engine.showTextures( self.glsl_viewer_show_textures )

    render = get_gv_render()
    if render:
        render.tag_redraw()


def show_passes_change(self, context):
    engine = get_gv_engine()
    if engine:
        engine.showPasses( self.glsl_viewer_show_passes )

    render = get_gv_render()
    if render:
        render.tag_redraw()


def show_histogram_change(self, context):
    engine = get_gv_engine()
    if engine:
        engine.showHistogram( self.glsl_viewer_show_histogram )

    render = get_gv_render()
    if render:
        render.tag_redraw()


def show_skybox_turbidity_change(self, context):
    engine = get_gv_engine()
    if engine:
        engine.setSkyTurbidity( self.glsl_viewer_skybox_turbidity )

    render = get_gv_render()
    if render:
        render.tag_redraw()


def show_debug_change(self, context):
    engine = get_gv_engine()
    if engine:
        engine.showBoudningBox( self.glsl_viewer_show_debug )

    render = get_gv_render()
    if render:
        render.tag_redraw()


PROPS = [
    ('glsl_viewer_vert', bpy.props.StringProperty(name='Vertex Shader', default='main.vert')),
    ('glsl_viewer_frag', bpy.props.StringProperty(name='Fragment Shader', default='main.frag')),
    ('glsl_viewer_blend_mode', bpy.props.EnumProperty(name='Blend Mode', set=set_blend, get=get_blend, items=blend_mode)),
    ('glsl_viewer_skybox_turbidity', bpy.props.FloatProperty(name='Skybox Turbidity', min=1, default=4.0, update=show_skybox_turbidity_change)),
    ('glsl_viewer_enable_cubemap', bpy.props.BoolProperty(name='Enable Cubemap', default=True, update=enable_cubemap_change)),
    ('glsl_viewer_show_cubemap', bpy.props.BoolProperty(name='Show Cubemap', default=False, update=show_cubemap_change)),
    ('glsl_viewer_show_textures', bpy.props.BoolProperty(name='Show Textures', default=False, update=show_textures_change)),
    ('glsl_viewer_show_passes', bpy.props.BoolProperty(name='Show Passes', default=False, update=show_passes_change)),
    ('glsl_viewer_show_histogram', bpy.props.BoolProperty(name='Show Histogram', default=False, update=show_histogram_change)),
    ('glsl_viewer_show_debug', bpy.props.BoolProperty(name='Debug View', default=False, update=show_debug_change)),
    ('glsl_viewer_fxaa', bpy.props.BoolProperty(name='FXAA', default=False, update=fxaa_change)),
]

def register_properties():
    for (prop_name, prop_value) in PROPS:
        setattr(bpy.types.Scene, prop_name, prop_value)

def unregister_properties():
    for (prop_name, _) in PROPS:
        delattr(bpy.types.Scene, prop_name)

if __name__ == "__main__":
    register_properties()