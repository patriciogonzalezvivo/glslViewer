#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D u_scene;
uniform sampler2D u_scene_depth;

uniform vec2 u_resolution;
uniform float u_time;

varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

float LinearizeDepth(float zoverw) {
	float n = 1.0; //
	float f = 20000.0;
	return (2.0 * n) / (f + n - zoverw * (f - n));	
}

void main(void) {
   vec4 color = vec4(1.0);
   vec2 st = gl_FragCoord.xy/u_resolution.xy;

#ifdef POSTPROCESSING
    float depth = texture2D(u_scene_depth, st).r;
    color.rgb = texture2D(u_scene, st).rgb * depth;
#else
    color.rgb = v_color.rgb;
    float shade = dot(v_normal, normalize(vec3(0.0, 0.75, 0.75)));
    color.rgb *= smoothstep(-1.0, 1.0, shade);
#endif

    gl_FragColor = color;
}
