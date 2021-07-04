#pragma once

#include <string>

const std::string holoplay_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform vec2        u_resolution;

uniform vec3        u_holoPlayTile;
uniform vec4        u_holoPlayCalibration;  // dpi, pitch, slope, center
uniform vec2        u_holoPlayRB;           // ri, bi

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

    float pitch = -u_resolution.x / u_holoPlayCalibration.x  * u_holoPlayCalibration.y * sin(atan(abs(u_holoPlayCalibration.z)));
    float tilt = u_resolution.y / (u_resolution.x * u_holoPlayCalibration.z);
    float subp = 1.0 / (3.0 * u_resolution.x);
    float subp2 = subp * pitch;

    float a = (-st.x - st.y * tilt) * pitch - u_holoPlayCalibration.w;
    color.r = texture2D(u_scene, quilt_map(u_holoPlayTile, st, a-u_holoPlayRB.x*subp2)).r;
    color.g = texture2D(u_scene, quilt_map(u_holoPlayTile, st, a-subp2)).g;
    color.b = texture2D(u_scene, quilt_map(u_holoPlayTile, st, a-u_holoPlayRB.y*subp2)).b;

    #if defined(HOLOPLAY_DEBUG_CENTER)
    // Mark center line only in central view
    color.r = color.r * 0.001 + (st.x>0.49 && st.x<0.51 && fract(a)>0.48&&fract(a)<0.51 ?1.0:0.0);
    color.g = color.g * 0.001 + st.x;
    color.b = color.b * 0.001 + st.y;

    #elif defined(HOLOPLAY_DEBUG)
    // use quilt texture
    color = texture2D(u_scene, st).rgb;
    #endif

    gl_FragColor = vec4(color,1.0);
}
)";

const std::string holoplay_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform vec2        u_resolution;

uniform vec3        u_holoPlayTile;
uniform vec4        u_holoPlayCalibration;  // dpi, pitch, slope, center
uniform vec2        u_holoPlayRB;           // ri, bi

out     vec4        fragColor;

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

    float pitch = -u_resolution.x / u_holoPlayCalibration.x  * u_holoPlayCalibration.y * sin(atan(abs(u_holoPlayCalibration.z)));
    float tilt = u_resolution.y / (u_resolution.x * u_holoPlayCalibration.z);
    float subp = 1.0 / (3.0 * u_resolution.x);
    float subp2 = subp * pitch;

    float a = (-st.x - st.y * tilt) * pitch - u_holoPlayCalibration.w;
    color.r = texture(u_scene, quilt_map(u_holoPlayTile, st, a-u_holoPlayRB.x*subp2)).r;
    color.g = texture(u_scene, quilt_map(u_holoPlayTile, st, a-subp2)).g;
    color.b = texture(u_scene, quilt_map(u_holoPlayTile, st, a-u_holoPlayRB.y*subp2)).b;

    #if defined(HOLOPLAY_DEBUG_CENTER)
    // Mark center line only in central view
    color.r = color.r * 0.001 + (st.x>0.49 && st.x<0.51 && fract(a)>0.48&&fract(a)<0.51 ?1.0:0.0);
    color.g = color.g * 0.001 + st.x;
    color.b = color.b * 0.001 + st.y;

    #elif defined(HOLOPLAY_DEBUG)
    // use quilt texture
    color = texture(u_scene, st).rgb;
    #endif

    fragColor = vec4(color,1.0);
}
)";
