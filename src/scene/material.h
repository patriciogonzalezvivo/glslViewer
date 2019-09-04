#pragma once

#include <string>
#include "glm/glm.hpp"

#include "../tools/fs.h"
#include "../tools/uniform.h"
#include "../gl/shader.h"

struct Material {
    Material();
    virtual ~Material();

    std::string name;
    std::string folder;

    glm::vec3   ambient;                    // Ka
    glm::vec3   diffuse;                    // Kd
    glm::vec3   specular;                   // Ks
    glm::vec3   transmittance;              // Tf
    glm::vec3   emission;                   // Ke

    float       shininess;                  // Ns
    /*
    Specifies the specular exponent for the current material. 
    This defines the focus of the specular highlight.
    "exponent" is the value for the specular exponent. 
    A high exponent results in a tight, concentrated highlight. Ns values normally range from 0 to 1000.
    */

    float       ior;                        // Ni (optical density or index of refraction)
    /*
    Specifies the optical density for the surface. This is also known as index of refraction.
    "optical_density" is the value for the optical density. The values can range from 0.001 to 10. 
    A value of 1.0 means that light does not bend as it passes through an object. 
    Increasing the optical_density increases the amount of bending. 
    Glass has an index of refraction of about 1.5. Values of less than 1.0 produce bizarre results and are not recommended.
    */

    float       dissolve;
    /*
    Specifies the dissolve for the current material.
````"factor" is the amount this material dissolves into the background. 
    A factor of 1.0 is fully opaque. This is the default when a new material is created. 
    A factor of 0.0 is fully dissolved (completely transparent).
    */
   
    int         illum;
/*
    The "illum" statement specifies the illumination model to use in the material. Illumination models are mathematical equations that represent various material lighting and shading effects.

    "illum_#"can be a number from 0 to 10. The illumination models are summarized below; for complete descriptions see "Illumination models" on page 5-30.

    Illumination Properties that are turned on in themodel Property Editor

    0 Color on and Ambient off
    1 Color on and Ambient on
    2 Highlight on
    3 Reflection on and Ray trace on
    4 Transparency: Glass on

    Reflection: Ray trace on
    5 Reflection: Fresnel on and Ray trace on
    6 Transparency: Refraction on

    Reflection: Fresnel off and Ray trace on
    7 Transparency: Refraction on

    Reflection: Fresnel on and Ray trace on
    8 Reflection on and Ray trace off
    9 Transparency: Glass on
    
    Reflection: Ray trace off
    10 Casts shadows onto invisible surfaces
*/

    std::string ambientMap;             // map_Ka
    std::string diffuseMap;             // map_Kd
    std::string specularMap;            // map_Ks
    std::string specular_highlightMap;  // map_Ns
    std::string bumpMap;                // map_bump, map_Bump, bump
    std::string displacementMap;        // disp
    std::string alphaMap;               // map_d
    std::string reflectionMap;          // refl

    // PBR extension
    // http://exocortex.com/blog/extending_wavefront_mtl_to_support_pbr
    float       roughness;                  // [0, 1] default 0
    float       metallic;                   // [0, 1] default 0
    float       sheen;                      // [0, 1] default 0
    float       clearcoat_thickness;        // [0, 1] default 0
    float       clearcoat_roughness;        // [0, 1] default 0
    float       anisotropy;                 // aniso. [0, 1] default 0
    float       anisotropy_rotation;        // anisor. [0, 1] default 0

    std::string roughnessMap;          // map_Pr
    std::string metallicMap;           // map_Pm
    std::string sheenMap;              // map_Ps
    std::string emissiveMap;           // map_Ke
    std::string normalMap;             // norm. For normal mapping.

    void printProperties();
    bool feedProperties(Shader& _shader) const;
    bool loadTextures(Uniforms& _uniforms, WatchFileList& _files, const std::string& _folder) const;
};