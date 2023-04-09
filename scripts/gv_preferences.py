import bpy

# Thank you https://devtalk.blender.org/t/how-to-save-custom-user-preferences-for-an-addon/10362 
def fetch_pref(name: str):
    prefs = bpy.context.preferences.addons['glslViewer'].preferences
    if prefs is None:
        return None
    return prefs[name]


default_vert_code = '''#version 100
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
uniform mat4    u_projectionMatrix;
uniform mat4    u_modelMatrix;
uniform mat4    u_viewMatrix;
uniform mat3    u_normalMatrix;

attribute vec4  a_position;
varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
attribute vec4  a_color;
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
attribute vec3  a_normal;
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
attribute vec2  a_texcoord;
#endif
varying vec2    v_texcoord;

#ifdef MODEL_VERTEX_TANGENT
attribute vec4  a_tangent;
varying vec4    v_tangent;
varying mat3    v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
varying vec4    v_lightCoord;
#endif

void main(void) {
    v_position = u_modelMatrix * a_position;
    v_texcoord = a_position.xy * 0.5 + 0.5;
    
#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = vec4(u_modelMatrix * vec4(a_normal, 0.0) ).xyz;
#endif
    
#ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
#endif
    
#ifdef MODEL_VERTEX_TANGENT
    v_tangent = a_tangent;
    vec3 worldTangent = a_tangent.xyz;
    vec3 worldBiTangent = cross(v_normal, worldTangent);// * sign(a_tangent.w);
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));
#endif
    
#ifdef LIGHT_SHADOWMAP
    v_lightCoord = u_lightMatrix * v_position;
#endif
    
    gl_Position = u_projectionMatrix * u_viewMatrix * v_position;
}'''

default_frag_code = '''#version 100
#ifdef GL_ES
precision highp float;
#endif

uniform vec3        u_camera;

uniform vec2        u_resolution;
uniform float       u_time;
uniform int         u_frame;

varying vec4        v_position;

#ifdef MODEL_VERTEX_COLOR
varying vec4        v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3        v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2        v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

void main(void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 pos = v_position.xyz;

    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;

    color.rgb = pos;

    #ifdef MODEL_VERTEX_COLOR
    color = v_color;
    #endif

    #ifdef MODEL_VERTEX_NORMAL
    color.rgb = v_normal * 0.5 + 0.5;
    #endif

    #ifdef MODEL_VERTEX_TEXCOORD
    vec2 uv = v_texcoord;
    #else
    vec2 uv = st;
    #endif
    
    color.rg = uv;
    
    gl_FragColor = color;
}'''


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