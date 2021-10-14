
#ifdef GL_ES
precision highp float;
#endif

// Author Patricio Gonzalez Vivo  @patriciogv 

uniform sampler2D   u_buffer0;  // pos

uniform mat4        u_modelMatrix;
uniform mat4        u_viewMatrix;
uniform mat4        u_projectionMatrix;
uniform mat4        u_modelViewProjectionMatrix;
uniform vec3        u_camera;
uniform vec2        u_resolution;
uniform float       u_time;

attribute vec4      a_position;
varying vec4        v_position;

varying vec4        v_color;
varying vec2        v_texcoord;

float population = 0.01; // 0 = none / 1 = 100%
#define decimation(value, presicion) (floor(value * presicion)/presicion)

void main(void) {
    v_position = a_position;
    v_texcoord = a_position.xy;
    vec2 pixel = 1.0/u_resolution;

    vec2 uv = v_texcoord;
    uv = decimation(uv, u_resolution) + 0.5 * pixel;

    vec4 data = texture2D(u_buffer0, uv);

    v_position.xy = data.xy * 2.0 - 1.0;

    float visible = step(a_position.z, population);
    vec2 dir = data.zw * 2.0 - 1.0;
    v_color = vec4(visible, dir.x, dir.y, 1.0 );

    gl_PointSize = visible;
    
    gl_Position = v_position;
}
