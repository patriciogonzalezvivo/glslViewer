#pragma once

#include <string>

const std::string light_functions = "\n\
// https://github.com/stackgl/glsl-specular-cook-torrance/blob/master/index.glsl\n\
float beckmannDistribution(float _x, float _roughness) {\n\
    float NdotH = max(_x, 0.0001);\n\
    float cos2Alpha = NdotH * NdotH;\n\
    float tan2Alpha = (cos2Alpha - 1.0) / cos2Alpha;\n\
    float roughness2 = max(_roughness * _roughness, 0.0001);\n\
    float denom = 3.141592653589793 * roughness2 * cos2Alpha * cos2Alpha;\n\
    return exp(tan2Alpha / roughness2) / denom;\n\
}\n\
\n\
// https://github.com/stackgl/glsl-specular-cook-torrance\n\
float cookTorranceSpecular(vec3 _lightDirection, vec3 _viewDirection, vec3 _surfaceNormal, float _roughness) {\n\
    float NoV = max(dot(_surfaceNormal, _viewDirection), 0.0);\n\
    float NoL = max(dot(_surfaceNormal, _lightDirection), 0.0);\n\
\n\
    //Half angle vector\n\
    vec3 H = normalize(_lightDirection + _viewDirection);\n\
\n\
    //Geometric term\n\
    float NoH = max(dot(_surfaceNormal, H), 0.0);\n\
    float VoH = max(dot(_viewDirection, H), 0.000001);\n\
    float LoH = max(dot(_lightDirection, H), 0.000001);\n\
    float G1 = (2.0 * NoH * NoV) / VoH;\n\
    float G2 = (2.0 * NoH * NoL) / LoH;\n\
    float G = min(1.0, min(G1, G2));\n\
\n\
    //Distribution term\n\
    float D = beckmannDistribution(NoH, _roughness);\n\
\n\
    //Fresnel term\n\
    float F = pow(1.0 - NoV, 0.02);\n\
\n\
    //Multiply terms and done\n\
    return  G * F * D / max(3.14159265 * NoV, 0.000001);\n\
}\n\
\n\
// https://github.com/stackgl/glsl-diffenable-oren-nayar\n\
float orenNayarDiffuse(vec3 _lightDirection, vec3 _viewDirection, vec3 _surfaceNormal, float _roughness) {\n\
    float LdV = dot(_lightDirection, _viewDirection);\n\
    float NdL = dot(_lightDirection, _surfaceNormal);\n\
    float NdV = dot(_surfaceNormal, _viewDirection);\n\
\n\
    float s = LdV - NdL * NdV;\n\
    float t = mix(1.0, max(NdL, NdV), step(0.0, s));\n\
\n\
    float sigma2 = _roughness * _roughness;\n\
    float A = 1.0 + sigma2 * (1.0 / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));\n\
    float B = 0.45 * sigma2 / (sigma2 + 0.09);\n\
\n\
    return max(0.0, NdL) * (A + B * s / t) / 3.14159265358979;\n\
}\n\
\n\
void calcPointLight(vec3 _normal, vec3 _viewDirection, vec3 _diffuseColor, vec3 _specularColor, float _roughness, out vec3 _diffuse, out vec3 _specular) {\n\
    vec3 s = normalize(u_light - v_position.xyz);\n\
    float dif = orenNayarDiffuse(s, _viewDirection, _normal, _roughness);\n\
    float light_intensity = 1.0;\n\
    float spec = cookTorranceSpecular(s, _viewDirection, _normal, _roughness);\n\
    _diffuse = light_intensity * (_diffuseColor * u_lightColor * dif);\n\
    _specular = light_intensity * (_specularColor * u_lightColor * spec);\n\
}\n\
\n\
void lightWithShadow(vec3 _normal, vec3 _viewDirection, vec3 _diffuseColor, vec3 _specularColor, float _roughness, out vec3 _diffuse, out vec3 _specular) {\n\
    vec3 lightDiffuse = vec3(0.0);\n\
    vec3 lightSpecular = vec3(0.0);\n\
\n\
    calcPointLight(_normal, _viewDirection, _diffuseColor, _specularColor, _roughness, lightDiffuse, lightSpecular);\n\
\n\
    float shadow = 1.0;\n\
#if defined(SHADOW_MAP) && defined(SHADOW_MAP_SIZE) && !defined(PLATFORM_RPI)\n\
    float bias = 0.005;\n\
    shadow *= textureShadowPCF(u_ligthShadowMap, vec2(SHADOW_MAP_SIZE), v_lightcoord.xy, v_lightcoord.z - bias);\n\
    shadow = clamp(shadow, 0.0, 1.0);\n\
#endif\n\
\n\
    _diffuse = vec3(lightDiffuse * shadow);\n\
    _specular = clamp(lightSpecular, 0.0, 1.0) * shadow;\n\
}\n\
";
