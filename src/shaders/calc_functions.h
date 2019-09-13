#pragma once

#include <string>

const std::string calc_functions = "\n\
void calcNormal(vec3 _viewDirection, out vec3 _normal, out vec3 _reflectDir) {\n\
    // if (enableNormalMap == 1) {\n\
    //     vec3 normalMapVec = texture(normalMap, mod(texCoordVarying * textureRepeatTimes, 1.0)).xyz;\n\
    //     if (enableDetailNormalMap == 1) {\n\
    //         vec3 detailNormalMapVec = texture(detailNormalMap, mod(texCoordVarying * detailTextureRepeatTimes, 1.0)).rgb;\n\
    //         normalMapVec = BlendSoftLight(normalMapVec, detailNormalMapVec);\n\
    //     }\n\
    //     normal = mix(mv_normal, PerturbNormal(normalMapVec, mv_normal, mv_positionVarying.xyz, texCoordVarying), vec3(normalValUniform));\n\
    //     vec3 relfect0 = reflect(normalize(mv_positionVarying.xyz), normal);\n\
    //     reflectDir = vec3(viewTranspose * vec4(relfect0, 0.0)) * vec3(1, 1, -1);\n\
    // }\n\
    // else {\n\
    //     normal = mv_normal;\n\
    //     vec3 relfect0 = reflect(normalize(mv_positionVarying.xyz), mv_normal);\n\
    //     reflectDir = vec3(viewTranspose * vec4(relfect0, 0.0)) * vec3(1, 1, -1);\n\
    // }\n\
\n\
    #ifdef MODEL_HAS_NORMALS\n\
    _normal = v_normal;\n\
    #if defined(MODEL_HAS_TANGENTS) && defined(MATERIAL_BUMPMAP_NORMALMAP)\n\
    _normal = v_tangentToWorld * (texture2D(MATERIAL_BUMPMAP_NORMALMAP, v_texcoord.xy).xyz * 2.0 - 1.0);\n\
    #endif\n\
    _normal = normalize(_normal);\n\
    _reflectDir = reflect(-_viewDirection, _normal);\n\
    #endif\n\
}\n\
\n\
vec3 calcColor(vec3 _baseColor, vec3 _normal, vec3 _viewDirection, vec3 _reflectDir, float _roughnessVal, float _metallicVal) {\n\
    float NoV = dot(_normal, _viewDirection);\n\
\n\
    vec3 diffuseColor = _baseColor * (1.0 - _metallicVal);\n\
    vec3 specularColor = mix(vec3(1.0), _baseColor, _metallicVal);\n\
    vec3 diffuse = diffuseColor;\n\
    #if defined(SH_ARRAY)\n\
    diffuse *= tonemap( sphericalHarmonics(_normal) ) * 0.5;\n\
    // #elif defined(CUBE_MAP)\n\
    // diffuse *= prefilterEnvMap(_normal, (numMips * 2. / 3.));\n\
    #endif\n\
    vec3 specular = approximateSpecularIBL(specularColor, _reflectDir, NoV, _roughnessVal) * _metallicVal;\n\
    vec3 frsnl = vec3(0.0);\n\
    frsnl = fresnel(_reflectDir, NoV, _roughnessVal, 0.2) * _metallicVal;\n\
\n\
    // for (int i = 0; i < numLights; i++) {\n\
    //     if (lights[i].isEnabled == true) {\n\
    //         vec3 lightDeffuse;\n\
    //         vec3 lightSpecular;\n\
    //         LightWithShadow(_normal, _viewDirection, diffuseColor, specularColor, roughnessVal, i, lightDeffuse, lightSpecular);\n\
    //         diffuse += lightDeffuse;\n\
    //         specular += lightSpecular;\n\
    //     }\n\
    // }\n\
\n\
    vec3 lightDeffuse = vec3(0.0);\n\
    vec3 lightSpecular = vec3(0.0);\n\
    lightWithShadow(_normal, _viewDirection, diffuseColor, specularColor, _roughnessVal, lightDeffuse, lightSpecular);\n\
    diffuse += lightDeffuse;\n\
    specular += lightSpecular;\n\
\n\
    return vec3(diffuse + specular + frsnl);\n\
}\n\
";
