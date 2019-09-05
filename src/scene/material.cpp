#include "material.h"

#include "../tools/text.h"

Material::Material(): 
    name("default"),
    ambient(0.0),                   // Ka
    diffuse(0.0),                   // Kd
    specular(0.0),                  // Ks
    transmittance(1.0),             // Tf
    emission(0.0),                  // Ke
    shininess(0.0),                 // Ns
    ior(1.5),                       // Ni (optical density or index of refraction)
    dissolve(1.0),                  // 1 == opaque; 0 == fully transparent
    illum(-1),                      // illumination model (see http://www.fileformat.info/format/material/)
    ambientMap(""),                 // map_Ka
    diffuseMap(""),                 // map_Kd
    specularMap(""),                // map_Ks
    specular_highlightMap(""),      // map_Ns
    bumpMap(""),                    // map_bump, map_Bump, bump
    displacementMap(""),            // disp
    alphaMap(""),                   // map_d
    reflectionMap(""),              // refl
    roughness(0.0),                 // [0, 1] default 0
    metallic(0.0),                  // [0, 1] default 0
    sheen(0.0),                     // [0, 1] default 0
    clearcoat_thickness(0.0),       // [0, 1] default 0
    clearcoat_roughness(0.0),       // [0, 1] default 0
    anisotropy(0.0),                // aniso. [0, 1] default 0
    anisotropy_rotation(0.0),       // anisor. [0, 1] default 0
    roughnessMap(""),               // map_Pr
    metallicMap(""),                // map_Pm
    sheenMap(""),                   // map_Ps
    emissiveMap(""),                // map_Ke
    normalMap("")                   // norm. For normal mapping.
{
}

Material::~Material() {
}

std::string getUniformName(const std::string& _str){
    std::vector<std::string> values = split(_str, '.');
    return "u_" + toLower( toUnderscore( purifyString( values[0] ) ) );
}

void Material::printProperties() {
    if (illum != -1)
        std::cout << " Ilumination model    = " << illum << std::endl;

    if (ambient != glm::vec3(0.0))
        std::cout << " Ambient Color    = " << toString(ambient) << std::endl;
    if (!ambientMap.empty())
        std::cout << " Ambient Texture  = " << ambientMap << std::endl;

    if (diffuse != glm::vec3(0.0))
        std::cout << " Diffuse Color    = " << toString(diffuse) << std::endl;
    if (!diffuseMap.empty())
        std::cout << " DiffuseMap       = " << diffuseMap << std::endl;

    if (specular != glm::vec3(0.0))
        std::cout << " Specular Color   = " << toString(specular) << std::endl;
    if (!specularMap.empty())
        std::cout << " SpecularMap      = " << specularMap << std::endl;

    if (roughness != 0.0)
        std::cout     << " Roughness        = " << roughness << std::endl;
    if (!roughnessMap.empty())
        std::cout << " RoughnessMap     = " << roughnessMap << std::endl;

    if (metallic != 0.0)
        std::cout << " Metallic         = " << metallic << std::endl;
    if (!metallicMap.empty())
        std::cout << " MetallicMap      = " << metallicMap << std::endl;

    if (shininess != 0.0)
        std::cout << " Shininess        = " << shininess << std::endl;

    if (sheen != 1.0)
        std::cout << " Sheen            = " << sheen << std::endl;
    if (!sheenMap.empty())
        std::cout << " SheenMap         = " << sheenMap << std::endl;

    if (!specular_highlightMap.empty())
        std::cout << " HighlightMap     = " << specular_highlightMap << std::endl;
    if (!reflectionMap.empty())
        std::cout << " ReflectionMap    = " << reflectionMap << std::endl;

    if (clearcoat_thickness != 0.0)
        std::cout << " C. Coat Thickness= " << clearcoat_thickness << std::endl;
    if (clearcoat_roughness != 0.0)
        std::cout << " C. Coat Roughness= " << clearcoat_roughness << std::endl;
    if (anisotropy != 0.0)
        std::cout << " Anisotropy       = " << anisotropy << std::endl;
    if (anisotropy_rotation != 0.0)
        std::cout << " Anisotropy Rot.  = " << anisotropy_rotation << std::endl;

    if (emission != glm::vec3(0.0))
        std::cout << " Emission Color   = " << toString(emission) << std::endl;
    if (!emissiveMap.empty())
        std::cout << " EmissiveMap      = " << emissiveMap << std::endl;

    if (!normalMap.empty())
        std::cout << " NormalMap        = " << normalMap << std::endl;
    if (!bumpMap.empty())
        std::cout << " BumpMap          = " << bumpMap << std::endl;
    if (!displacementMap.empty())
        std::cout << " DisplacementMap  = " << displacementMap << std::endl;

    if (!alphaMap.empty())
        std::cout << " AlphaMap         = " << alphaMap << std::endl;
    if (transmittance != glm::vec3(1.0))
        std::cout << " Transmittance    = " << toString(transmittance) << std::endl;

    if (dissolve != 1.0)
        std::cout << " Dissolve         = " << dissolve << std::endl;

    if (ior != 0.0)
        std::cout     << " Index of refr.   = " << ior << std::endl; 
    
}

bool Material::feedProperties(Shader& _shader) const {
    _shader.addDefine( "MATERIAL_NAME_" + toUpper(name) );

    if (illum != -1)
        _shader.addDefine( "MATERIAL_ILLUM", illum );

    if (ambient != glm::vec3(0.0))
        _shader.addDefine( "MATERIAL_AMBIENT", ambient );

    if (!ambientMap.empty())
        _shader.addDefine( "MATERIAL_AMBIENTMAP", getUniformName( ambientMap ) );

    if (diffuse != glm::vec3(0.0))
        _shader.addDefine( "MATERIAL_DIFFUSE", diffuse );

    if (!diffuseMap.empty())
        _shader.addDefine( "MATERIAL_DIFFUSEMAP", getUniformName( diffuseMap ) );

    if (specular != glm::vec3(0.0))
        _shader.addDefine( "MATERIAL_SPECULAR", specular );
        
    if (!specularMap.empty())
        _shader.addDefine( "MATERIAL_SPECULARMAP", getUniformName( specularMap ) );

    if (roughness != 0.0)
        _shader.addDefine( "MATERIAL_ROUGHNESS", roughness );

    if (!roughnessMap.empty())
        _shader.addDefine( "MATERIAL_ROUGHNESSMAP", getUniformName( roughnessMap ) );

    if (metallic != 0.0)
        _shader.addDefine( "MATERIAL_METALLIC", metallic );

    if (!metallicMap.empty())
        _shader.addDefine( "MATERIAL_METALLICMAP", getUniformName( metallicMap ) );

    if (shininess != 0.0)
        _shader.addDefine( "MATERIAL_SHININESS", shininess );

    if (sheen != 0.0)
        _shader.addDefine( "MATERIAL_SHEEN", sheen );

    if (!sheenMap.empty())
        _shader.addDefine( "MATERIAL_SHEENMAP", getUniformName( sheenMap ) );

    if (!specular_highlightMap.empty())
        _shader.addDefine( "MATERIAL_SPECULAR_HIGHLIGHTMAP", getUniformName( specular_highlightMap ) );

    if (!reflectionMap.empty())
        _shader.addDefine( "MATERIAL_REFLECTIONMAP", getUniformName( reflectionMap ) );

    if (clearcoat_thickness != 0.0)
        _shader.addDefine( "MATERIAL_CLEARCOAT_THICKNESS", clearcoat_thickness );

    if (clearcoat_roughness != 0.0)
        _shader.addDefine( "MATERIAL_CLEARCOAT_ROUGHNESS", clearcoat_roughness );
        
    if (anisotropy != 0.0)
        _shader.addDefine( "MATERIAL_ANISOTROPY", anisotropy );

    if (anisotropy_rotation != 0.0)
        _shader.addDefine( "MATERIAL_ANISOTROPY_ROTATION", anisotropy_rotation );

    if (emission != glm::vec3(0.0))
        _shader.addDefine( "MATERIAL_EMISSION", emission );

    if (!emissiveMap.empty())
        _shader.addDefine( "MATERIAL_EMISSIVEMAP", getUniformName( emissiveMap ) );

    if (!normalMap.empty())
        _shader.addDefine( "MATERIAL_NORMALMAP", getUniformName( normalMap ) );

    if (!bumpMap.empty())
        _shader.addDefine( "MATERIAL_BUMPMAP", getUniformName( bumpMap ) );

    if (!displacementMap.empty())
        _shader.addDefine( "MATERIAL_DISPLACEMENTMAP", getUniformName( displacementMap ) );

    if (!alphaMap.empty())
        _shader.addDefine( "MATERIAL_ALPHAMAP", getUniformName( alphaMap ) );

    if (transmittance != glm::vec3(0.0))
        _shader.addDefine( "MATERIAL_TRANSMITTANCE", transmittance );

    if (dissolve != 1.0)
        _shader.addDefine( "MATERIAL_DISSOLVE", dissolve );
    
    if (ior != 0.0)
        _shader.addDefine( "MATERIAL_IOR", ior );

    return true;
}

bool Material::loadTextures(Uniforms& _uniforms, WatchFileList& _files, const std::string& _folder) const {

    if (!ambientMap.empty())
        _uniforms.addTexture(getUniformName( ambientMap ), _folder + ambientMap, _files);

    if (!diffuseMap.empty())
        _uniforms.addTexture(getUniformName( diffuseMap ), _folder + diffuseMap, _files);
    
    if (!specularMap.empty())
        _uniforms.addTexture(getUniformName( specularMap ), _folder + specularMap, _files);

    // specular_highlightMap,
    if (!specular_highlightMap.empty())
        _uniforms.addTexture(getUniformName( specular_highlightMap ), _folder + specular_highlightMap, _files);

    // bumpMap,
    if (!bumpMap.empty())
        _uniforms.addTexture(getUniformName( bumpMap ), _folder + bumpMap, _files);

    // displacementMap,
    if (!displacementMap.empty())
        _uniforms.addTexture(getUniformName( displacementMap ), _folder + displacementMap, _files);

    // alphaMap,
    if (!alphaMap.empty())
        _uniforms.addTexture(getUniformName( alphaMap ), _folder + alphaMap, _files);

    // reflectionMap,
    if (!reflectionMap.empty())
        _uniforms.addTexture(getUniformName( reflectionMap ), _folder + reflectionMap, _files);

    // roughnessMap,
    if (!roughnessMap.empty())
        _uniforms.addTexture(getUniformName( roughnessMap ), _folder + roughnessMap, _files);

    // metallicMap,
    if (!metallicMap.empty())
        _uniforms.addTexture(getUniformName( metallicMap ), _folder + metallicMap, _files);

    // sheenMap,
    if (!sheenMap.empty())
        _uniforms.addTexture(getUniformName( sheenMap ), _folder + sheenMap, _files);

    // emissiveMap,
    if (!emissiveMap.empty())
        _uniforms.addTexture(getUniformName( emissiveMap ), _folder + emissiveMap, _files);

    // normalMap
    if (!normalMap.empty())
        _uniforms.addTexture(getUniformName( normalMap ), _folder + normalMap, _files);

    return true;
}
