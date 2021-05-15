#pragma once

#include <string>

const std::string holo_frag = R"(

#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform vec2        u_resolution;

uniform vec3        u_holoTile;
uniform vec4        u_holoCalibration; // dpi, pitch, slope, center
uniform vec2        u_holoRB;   // ri, bi

// GET CORRECT VIEW
vec2 quilt_map(vec3 tile, vec2 pos, float a) {
    vec2 tile2 = vec2(tile.x - 1.0, tile.y - 1.0);
    vec2 dir = vec2(-1.0, -1.0);

    a = fract(a) * tile.y;
    tile2.y += dir.y * floor(a);
    a = fract(a) * tile.x;
    tile2.x += dir.x * floor(a);
    return (tile2 + pos) / tile.xy;
}

void main (void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    float screen_aspect = u_resolution.x/u_resolution.y;
    
    // const vec3 tile = vec3(4., 8., 32.); // 2048x2048
    // const vec3 tile = vec3(5., 9., 45.); // 4096x4096
    // const vec3 tile = vec3(7., 11., 77.); //  4096x3576
    // const float dpi = 324.0;
    // const float pitch_f = 52.58737671470091;
    // const float slope = -7.196136200157333;
    // const float center = 0.4321881363063158;
    // const int ri = 0;
    // const int bi = 2;

    float pitch = -u_resolution.x / u_holoCalibration.x  * u_holoCalibration.y * sin(atan(abs(u_holoCalibration.z)));
    float tilt = u_resolution.y / (u_resolution.x * u_holoCalibration.z);
    float subp = 1.0 / (3.0 * u_resolution.x);
    float subp2 = subp * pitch;

    float a = (-st.x - st.y * tilt) * pitch - u_holoCalibration.w;
    color.r = texture2D(u_scene, quilt_map(u_holoTile, st, a-u_holoRB.x*subp2)).r;
    color.g = texture2D(u_scene, quilt_map(u_holoTile, st, a-subp2)).g;
    color.b = texture2D(u_scene, quilt_map(u_holoTile, st, a-u_holoRB.y*subp2)).b;

    #if defined(HOLO_DEBUG_CENTER)
    // Mark center line only in central view
    color.r = color.r * 0.001 + (st.x>0.49 && st.x<0.51 && fract(a)>0.48&&fract(a)<0.51 ?1.0:0.0);
    color.g = color.g * 0.001 + st.x;
    color.b = color.b * 0.001 + st.y;

    #elif defined(HOLO_DEBUG)
    // use quilt texture
    color = texture2D(u_scene, st).rgb;
    #endif

    gl_FragColor = vec4(color,1.0);
}
)";
