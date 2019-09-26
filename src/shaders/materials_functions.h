#pragma once

#include <string>

const std::string materials_functions = R"(
#ifndef FNC_GETMATERIAL

float checkBoard(vec2 _scale) {
#ifdef MODEL_VERTEX_TEXCOORD
    vec2 uv = v_texcoord;
#else
    vec2 uv = gl_FragCoord.xy/u_resolution;
#endif
    uv = floor(fract(uv * _scale) * 2.0);
    return min(1.0, uv.x + uv.y) - (uv.x * uv.y);
}

const float GAMMA = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;

// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 LINEARtoSRGB(vec3 color) { return pow(color, vec3(INV_GAMMA)); }
// vec4 SRGBtoLINEAR(vec4 srgbIn) { return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w); }
vec4 SRGBtoLINEAR(vec4 srgbIn) { return vec4(srgbIn.xyz * srgbIn.xyz, srgbIn.w); }

#ifdef MATERIAL_BASECOLORMAP
uniform sampler2D MATERIAL_BASECOLORMAP;
#endif

vec3 getMaterialAlbedo() {
    vec3 base = vec3(1.0);
#if defined(MATERIAL_BASECOLORMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_BASECOLORMAP_OFFSET)
    uv += (MATERIAL_BASECOLORMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_BASECOLORMAP_SCALE)
    uv *= (MATERIAL_BASECOLORMAP_SCALE).xy;
    #endif
    base = SRGBtoLINEAR(texture2D(MATERIAL_BASECOLORMAP, uv)).rgb;
#elif defined(MATERIAL_BASECOLOR)
    base = MATERIAL_BASECOLOR;
#elif !defined(MODEL_VERTEX_COLOR)
    base *= 0.5 + checkBoard(vec2(8.0)) * 0.5;
#endif

#if defined(MODEL_VERTEX_COLOR)
    base *= v_color.rgb;
#endif
    return base;
}

#ifdef MATERIAL_SPECULARMAP
uniform sampler2D MATERIAL_SPECULARMAP;
#endif

vec3 getMaterialSpecular() {
    vec3 spec = vec3(0.04);
#if defined(MATERIAL_SPECULARMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_SPECULARMAP_OFFSET)
    uv += (MATERIAL_SPECULARMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_SPECULARMAP_SCALE)
    uv *= (MATERIAL_SPECULARMAP_SCALE).xy;
    #endif
    spec = SRGBtoLINEAR(texture2D(MATERIAL_SPECULARMAP, uv)).rgb;
#elif defined(MATERIAL_SPECULAR)
    spec = MATERIAL_SPECULAR;
#endif
    return spec;
}

#ifdef MATERIAL_EMISSIVEMAP
uniform sampler2D MATERIAL_EMISSIVEMAP;
#endif

vec3 getMaterialEmission() {
    vec3 emission = vec3(0.0);
#if defined(MATERIAL_EMISSIVEMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_EMISSIVEMAP_OFFSET)
    uv += (MATERIAL_EMISSIVEMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_EMISSIVEMAP_SCALE)
    uv *= (MATERIAL_EMISSIVEMAP_SCALE).xy;
    #endif
    emission = SRGBtoLINEAR(texture2D(MATERIAL_EMISSIVEMAP, uv)).rgb;
#elif defined(MATERIAL_EMISSIVE)
    emission = MATERIAL_EMISSIVE;
#endif
    return emission;
}

#ifdef MATERIAL_NORMALMAP
uniform sampler2D MATERIAL_NORMALMAP;
#endif

#ifdef MATERIAL_BUMPMAP_NORMALMAP
uniform sampler2D MATERIAL_BUMPMAP_NORMALMAP;
#endif

vec3 getMaterialNormal() {
    vec3 normal = vec3(0.0, 0.0, 1.0);
#ifdef MODEL_VERTEX_NORMAL
    normal = v_normal;
    #if defined(MODEL_VERTEX_TANGENT) && defined(MODEL_VERTEX_TEXCOORD) && defined(MATERIAL_NORMALMAP)
    vec2 uv = v_texcoord.xy;
        #if defined(MATERIAL_NORMALMAP_OFFSET)
    uv += (MATERIAL_NORMALMAP_OFFSET).xy;
        #endif
        #if defined(MATERIAL_NORMALMAP_SCALE)
    uv *= (MATERIAL_NORMALMAP_SCALE).xy;
        #endif
    normal = texture2D(MATERIAL_NORMALMAP, uv).xyz;
    normal = v_tangentToWorld * (normal * 2.0 - 1.0);
    #elif defined(MODEL_VERTEX_TANGENT) && defined(MODEL_VERTEX_TEXCOORD) && defined(MATERIAL_BUMPMAP_NORMALMAP)
    vec2 uv = v_texcoord.xy;
        #if defined(MATERIAL_BUMPMAP_OFFSET)
    uv += (MATERIAL_BUMPMAP_OFFSET).xy;
        #endif
        #if defined(MATERIAL_BUMPMAP_SCALE)
    uv *= (MATERIAL_BUMPMAP_SCALE).xy;
        #endif
    normal = v_tangentToWorld * (texture2D(MATERIAL_BUMPMAP_NORMALMAP, uv).xyz * 2.0 - 1.0);
    #endif
#endif
    return normal;
}


#ifdef MATERIAL_ROUGHNESSMAP
uniform sampler2D MATERIAL_ROUGHNESSMAP;
#endif

const float c_MinReflectance = 0.04;
// Gets metallic factor from specular glossiness workflow inputs 
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {
    float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
    float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
    if (perceivedSpecular < c_MinReflectance) {
        return 0.0;
    }
    float a = c_MinReflectance;
    float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - c_MinReflectance) + perceivedSpecular - 2.0 * c_MinReflectance;
    float c = c_MinReflectance - perceivedSpecular;
    float D = max(b * b - 4.0 * a * c, 0.0);
    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

float getMaterialMetallic() {
    float metallic = 0.0;
#if defined(MATERIAL_METALLICMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_METALLICMAP_OFFSET)
    uv += (MATERIAL_METALLICMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_METALLICMAP_SCALE)
    uv *= (MATERIAL_METALLICMAP_SCALE).xy;
    #endif
    metallic = SRGBtoLINEAR(texture2D(MATERIAL_METALLICMAP, uv)).r;
#elif defined(MATERIAL_METALLIC)
    metallic = MATERIAL_METALLIC;
#else
    vec3 diffuse = getMaterialAlbedo();
    vec3 specular = getMaterialSpecular();
    float maxSpecula = max(max(specular.r, specular.g), specular.b);
    metallic = convertMetallic(diffuse, specular, maxSpecula);
#endif
    return metallic;
}


#ifdef MATERIAL_ROUGHNESSMAP
uniform sampler2D MATERIAL_ROUGHNESSMAP;
#endif

float getMaterialRoughness() {
    float roughness = 0.1;
#if defined(MATERIAL_ROUGHNESSMAP) && defined(MODEL_VERTEX_TEXCOORD)
    vec2 uv = v_texcoord.xy;
    #if defined(MATERIAL_ROUGHNESSMAP_OFFSET)
    uv += (MATERIAL_ROUGHNESSMAP_OFFSET).xy;
    #endif
    #if defined(MATERIAL_ROUGHNESSMAP_SCALE)
    uv *= (MATERIAL_ROUGHNESSMAP_SCALE).xy;
    #endif
    roughness = SRGBtoLINEAR(texture2D(MATERIAL_ROUGHNESSMAP, uv)).r;
#elif defined(MATERIAL_ROUGHNESS)
    roughness = MATERIAL_ROUGHNESS;
#elif defined(DEBUG)
    roughness = checkBoard(vec2(20.0, 0.0)) * 0.25;
#endif
    return roughness;
}

float getMaterialShininess() {
    float shininess = 15.0;
#ifdef MATERIAL_SHININESS
    shininess = MATERIAL_SHININESS;
#elif defined(MATERIAL_METALLIC) && defined(MATERIAL_ROUGHNESS)
    float smooth = .95 - MATERIAL_ROUGHNESS * 0.5;
    smooth *= smooth;
    smooth *= smooth;
    shininess = smooth * (80. + 160. * (1.0-MATERIAL_METALLIC));
#endif
    return shininess;
}
#endif
)";
