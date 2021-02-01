#include "uniforms.h"

#include <regex>
#include <sstream>
#include <sys/stat.h>

#include "tools/text.h"

#include "gl/textureBump.h"
#include "gl/textureStreamSequence.h"
#ifdef SUPPORT_FOR_LIBAV 
#include "gl/textureStreamAV.h"
#endif

std::string UniformData::getType() {
    if (size == 1 && bInt) {
        return "int";
    }
    else if (size == 1 && !bInt) {
        return "float";
    }
    else {
        return (bInt ? "ivec" : "vec") + toString(size); 
    }
}

UniformFunction::UniformFunction() {
    type = "-undefined-";
}

UniformFunction::UniformFunction(const std::string &_type) {
    type = _type;
}

UniformFunction::UniformFunction(const std::string &_type, std::function<void(Shader&)> _assign) {
    type = _type;
    assign = _assign;
}

UniformFunction::UniformFunction(const std::string &_type, std::function<void(Shader&)> _assign, std::function<std::string()> _print) {
    type = _type;
    assign = _assign;
    print = _print;
}

// UNIFORMS

Uniforms::Uniforms(): cubemap(nullptr), m_change(false) {

    // set the right distance to the camera
    // Set up camera

    cameras.push_back( Camera() );

    functions["u_iblLuminance"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_iblLuminance", 30000.0f * getCamera().getExposure());
    },
    [this]() { return toString(30000.0f * getCamera().getExposure()); });
    
    // CAMERA UNIFORMS
    //
    functions["u_camera"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_camera", -getCamera().getPosition());
    },
    [this]() { return toString(-getCamera().getPosition(), ','); });

    functions["u_cameraDistance"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraDistance", getCamera().getDistance());
    },
    [this]() { return toString(getCamera().getDistance()); });

    functions["u_cameraNearClip"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraNearClip", getCamera().getNearClip());
    },
    [this]() { return toString(getCamera().getNearClip()); });

    functions["u_cameraFarClip"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraFarClip", getCamera().getFarClip());
    },
    [this]() { return toString(getCamera().getFarClip()); });

    functions["u_cameraEv100"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraEv100", getCamera().getEv100());
    },
    [this]() { return toString(getCamera().getEv100()); });

    functions["u_cameraExposure"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraExposure", getCamera().getExposure());
    },
    [this]() { return toString(getCamera().getExposure()); });

    functions["u_cameraAperture"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraAperture", getCamera().getAperture());
    },
    [this]() { return toString(getCamera().getAperture()); });

    functions["u_cameraShutterSpeed"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraShutterSpeed", getCamera().getShutterSpeed());
    },
    [this]() { return toString(getCamera().getShutterSpeed()); });

    functions["u_cameraSensitivity"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraSensitivity", getCamera().getSensitivity());
    },
    [this]() { return toString(getCamera().getSensitivity()); });
    
    functions["u_normalMatrix"] = UniformFunction("mat3", [this](Shader& _shader) {
        _shader.setUniform("u_normalMatrix", getCamera().getNormalMatrix());
    });

    // CAMERA MATRIX UNIFORMS
    functions["u_viewMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_viewMatrix", getCamera().getViewMatrix());
    });

    functions["u_projectionMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_projectionMatrix", getCamera().getProjectionMatrix());
    });

     // IBL UNIFORM
    functions["u_cubeMap"] = UniformFunction("samplerCube", [this](Shader& _shader) {
        if (cubemap) {
            _shader.setUniformTextureCube("u_cubeMap", (TextureCube*)cubemap);
        }
    });

    functions["u_SH"] = UniformFunction("vec3", [this](Shader& _shader) {
        if (cubemap) {
            _shader.setUniform("u_SH", cubemap->SH, 9);
        }
    });
}

Uniforms::~Uniforms(){

}

bool parseUniformData(const std::string &_line, UniformDataList *_uniforms) {
    bool rta = false;
    std::regex re("^(\\w+)\\,");
    std::smatch match;
    if (std::regex_search(_line, match, re)) {
        // Extract uniform name
        std::string name = std::ssub_match(match[1]).str();

        // Extract values
        int index = 0;
        std::stringstream ss(_line);
        std::string item;
        while (getline(ss, item, ',')) {
            if (index != 0) {
                (*_uniforms)[name].bInt = !isFloat(item);
                (*_uniforms)[name].value[index-1] = toFloat(item);
                (*_uniforms)[name].change = true;
            }
            index++;
        }

        // Set total amount of values
        (*_uniforms)[name].size = index-1;
        rta = true;
    }
    return rta;
}

bool Uniforms::parseLine( const std::string &_line ) {
    bool somethingChange = parseUniformData(_line, &data);
    m_change += somethingChange;
    return somethingChange;
}

bool Uniforms::addTexture( const std::string& _name, Texture* _texture) {
    if (textures.find(_name) == textures.end()) {
        textures[ _name ] = _texture;
        return true;
    }
    else {
        if (textures[ _name ])
            delete textures[ _name ];
        textures[ _name ] = _texture;
    }
    return false;
}

bool Uniforms::addTexture(const std::string& _name, const std::string& _path, WatchFileList& _files, bool _flip, bool _verbose) {
    if (textures.find(_name) == textures.end()) {
        struct stat st;

        // If we can not get file stamp proably is not a file
        if (stat(_path.c_str(), &st) != 0 )
            std::cerr << "Error watching for file " << _path << std::endl;
        
        // If we can lets proceed creating a texgure
        else {
            Texture* tex = new Texture();
            // load an image into the texture
            if (tex->load(_path, _flip)) {

                // the image is loaded finish add the texture to the uniform list
                textures[ _name ] = tex;

                // and the file to the watch list
                WatchFile file;
                file.type = IMAGE;
                file.path = _path;
                file.lastChange = st.st_mtime;
                file.vFlip = _flip;
                _files.push_back(file);

                if (_verbose) {
                    std::cout << "// " << _path << " loaded as: " << std::endl;
                    std::cout << "//    uniform sampler2D " << _name  << ";"<< std::endl;
                    std::cout << "//    uniform vec2 " << _name  << "Resolution;"<< std::endl;
                }

                return true;
            }
            else
                delete tex;
        }
    }
    return false;
}

bool Uniforms::addBumpTexture(const std::string& _name, const std::string& _path, WatchFileList& _files, bool _flip, bool _verbose) {
    if (textures.find(_name) == textures.end()) {
        struct stat st;

        // If we can not get file stamp proably is not a file
        if (stat(_path.c_str(), &st) != 0 )
            std::cerr << "Error watching for file " << _path << std::endl;
        
        // If we can lets proceed creating a texgure
        else {
            TextureBump* tex = new TextureBump();

            // load an image into the texture
            if (tex->load(_path, _flip)) {

                // the image is loaded finish add the texture to the uniform list
                textures[ _name ] = (Texture*)tex;

                // and the file to the watch list
                WatchFile file;
                file.type = IMAGE_BUMPMAP;
                file.path = _path;
                file.lastChange = st.st_mtime;
                file.vFlip = _flip;
                _files.push_back(file);

                if (_verbose) {
                    std::cout << "// " << _path << " loaded and transform to normalmap as: " << std::endl;
                    std::cout << "//    uniform sampler2D " << _name  << ";"<< std::endl;
                    std::cout << "//    uniform vec2 " << _name  << "Resolution;"<< std::endl;
                }

                return true;
            }
            else
                delete tex;
        }
    }
    return false;
}

bool Uniforms::addStreamingTexture( const std::string& _name, const std::string& _url, bool _vflip, bool _device, bool _verbose) {
    if (textures.find(_name) == textures.end()) {

        // Check if it's a PNG Sequence
        if (check_for_pattern(_url)) {
            TextureStreamSequence *tex = new TextureStreamSequence();

            if (tex->load(_url, _vflip)) {
                // the image is loaded finish add the texture to the uniform list
                textures[ _name ] = (Texture*)tex;
                streams[ _name ] = (TextureStream*)tex;

                if (_verbose) {
                    std::cout << "// " << _url << " sequence loaded as streaming texture: " << std::endl;
                    std::cout << "//    uniform sampler2D " << _name  << ";"<< std::endl;
                    std::cout << "//    uniform vec2 " << _name  << "Resolution;"<< std::endl;
                }

                return true;
            }
            else
                delete tex;
            
        }
        else {
#ifdef SUPPORT_FOR_LIBAV
        TextureStreamAV* tex = new TextureStreamAV();
        tex->device = _device;

        // load an image into the texture
        if (tex->load(_url, _vflip)) {
            // the image is loaded finish add the texture to the uniform list
            textures[ _name ] = (Texture*)tex;
            streams[ _name ] = (TextureStream*)tex;

            if (_verbose) {
                std::cout << "// " << _url << " loaded as streaming texture: " << std::endl;
                std::cout << "//    uniform sampler2D " << _name  << ";"<< std::endl;
                std::cout << "//    uniform vec2 " << _name  << "Resolution;"<< std::endl;
            }

            return true;
        }
        else
            delete tex;
#endif
        }


    }
    return false;
}

void Uniforms::setCubeMap( TextureCube* _cubemap ) {
    if (cubemap)
        delete cubemap;

    cubemap = _cubemap;
}

void Uniforms::setCubeMap( const std::string& _filename, WatchFileList& _files, bool _verbose ) {
    struct stat st;
    if ( stat(_filename.c_str(), &st) != 0 ) {
        std::cerr << "Error watching for cubefile: " << _filename << std::endl;
    }
    else {
        TextureCube* tex = new TextureCube();
        if ( tex->load(_filename, true) ) {

            setCubeMap(tex);

            WatchFile file;
            file.type = CUBEMAP;
            file.path = _filename;
            file.lastChange = st.st_mtime;
            file.vFlip = true;
            _files.push_back(file);

            std::cout << "// " << _filename << " loaded as: " << std::endl;
            std::cout << "//    uniform samplerCube u_cubeMap;"<< std::endl;
            std::cout << "//    uniform vec3        u_SH[9];"<< std::endl;
        }
        else {
            delete tex;
        }
    }
}

void Uniforms::checkPresenceIn( const std::string &_vert_src, const std::string &_frag_src ) {
    // Check active native uniforms
    for (UniformFunctionsList::iterator it = functions.begin(); it != functions.end(); ++it) {
        bool present = ( find_id(_vert_src, it->first.c_str()) != 0 || find_id(_frag_src, it->first.c_str()) != 0 );
        if ( it->second.present != present) {
            it->second.present = present;
            m_change = true;
        } 
    }
}

bool Uniforms::feedTo( Shader &_shader ) {
    bool update = false;

    // Pass Native uniforms 
    for (UniformFunctionsList::iterator it=functions.begin(); it!=functions.end(); ++it)
        if (it->second.present)
            if (it->second.assign)
                it->second.assign(_shader);


    // Pass User defined uniforms
    if (m_change) {
        for (UniformDataList::iterator it=data.begin(); it!=data.end(); ++it) {
            if (it->second.change) {
                if (it->second.bInt) {
                    if (it->second.size == 1)
                        _shader.setUniform(it->first, int(it->second.value[0]));
                    else if (it->second.size == 2)
                        _shader.setUniform(it->first, int(it->second.value[0]), int(it->second.value[1]));
                    else if (it->second.size == 3)
                        _shader.setUniform(it->first, int(it->second.value[0]), int(it->second.value[1]), int(it->second.value[2]));
                    else if (it->second.size == 4)
                        _shader.setUniform(it->first, int(it->second.value[0]), int(it->second.value[1]), int(it->second.value[2]), int(it->second.value[3]));
                }
                else
                    _shader.setUniform(it->first, it->second.value, it->second.size);
                update = true;
            }
        }
    }

    // Pass Textures Uniforms
    for (TextureList::iterator it = textures.begin(); it != textures.end(); ++it) {
        _shader.setUniformTexture(it->first, it->second, _shader.textureIndex++ );
        _shader.setUniform(it->first+"Resolution", float(it->second->getWidth()), float(it->second->getHeight()));
    }

    // Pass Buffers Uniforms
    for (unsigned int i = 0; i < buffers.size(); i++) {
        _shader.setUniformTexture("u_buffer" + toString(i), &buffers[i], _shader.textureIndex++ );
    }
    
    // Pass Light Uniforms
    if (lights.size() == 1) {
        if (lights[0].getType() != LIGHT_DIRECTIONAL)
            _shader.setUniform("u_light", lights[0].getPosition());
        _shader.setUniform("u_lightColor", lights[0].color);
        if (lights[0].getType() == LIGHT_DIRECTIONAL || lights[0].getType() == LIGHT_SPOT)
            _shader.setUniform("u_lightDirection", lights[0].direction);
        _shader.setUniform("u_lightIntensity", lights[0].intensity);
        if (lights[0].falloff > 0)
            _shader.setUniform("u_lightFalloff", lights[0].falloff);
        _shader.setUniform("u_lightMatrix", lights[0].getBiasMVPMatrix() );
        // _shader.setUniformDepthTexture("u_lightShadowMap", lights[0].getShadowMap() );
    }
    else {
        for (unsigned int i = 0; i < lights.size(); i++) {
            if (lights[i].getType() != LIGHT_DIRECTIONAL)
                _shader.setUniform("u_light", lights[i].getPosition());
            _shader.setUniform("u_lightColor", lights[i].color);
            if (lights[i].getType() == LIGHT_DIRECTIONAL || lights[i].getType() == LIGHT_SPOT)
                _shader.setUniform("u_lightDirection", lights[i].direction);
            _shader.setUniform("u_lightIntensity", lights[i].intensity);
            if (lights[i].falloff > 0)
                _shader.setUniform("u_lightFalloff", lights[i].falloff);
            _shader.setUniform("u_lightMatrix", lights[i].getBiasMVPMatrix() );
            // _shader.setUniformDepthTexture("u_lightShadowMap", lights[i].getShadowMap() );
        }
    }

    return update;
}

void Uniforms::flagChange() {
    // Flag all user uniforms as changed
    for (UniformDataList::iterator it = data.begin(); it != data.end(); ++it) {
        it->second.change = true;
    }
    m_change = true;
    getCamera().bChange = true;
}

void Uniforms::unflagChange() {
    if (m_change) {
        // Flag all user uniforms as NOT changed
        for (UniformDataList::iterator it = data.begin(); it != data.end(); ++it)
            it->second.change = false;

        m_change = false;
    }

    for (unsigned int i = 0; i < lights.size(); i++)
        lights[i].bChange = false;

    getCamera().bChange = false;
}

bool Uniforms::haveChange() { 
    bool lightChange = false;
    for (unsigned int i = 0; i < lights.size(); i++) {
        if (lights[i].bChange) {
            lightChange = true;
            break;
        }
    }

    for (StreamsList::iterator i = streams.begin(); i != streams.end(); ++i) {
        i->second->update();
    }

    // std::cout << "  change " << m_change << std::endl;
    // std::cout << "  lights " << lightChange << std::endl;
    // std::cout << "  u_time " << functions["u_time"].present << std::endl;
    // std::cout << "  u_delta " << functions["u_delta"].present << std::endl;
    // std::cout << "  u_mouse " << functions["u_mouse"].present << std::endl;
    // std::cout << "  u_camera " << getCamera().bChange << std::endl;

    return  m_change || 
            streams.size() > 0 ||
            functions["u_time"].present || 
            functions["u_delta"].present ||
            functions["u_mouse"].present ||
            functions["u_date"].present ||
            lightChange || getCamera().bChange;
}

void Uniforms::clear() {

    if (cubemap) {
        delete cubemap;
        cubemap = nullptr;
    }

    // Delete Textures
    for (TextureList::iterator i = textures.begin(); i != textures.end(); ++i) {
        if (i->second) {
            delete i->second;
            i->second = nullptr;
        }
    }
    textures.clear();

    // Streams are textures so it should be clear by now;
    // streams.clear();

}

void Uniforms::print(bool _all) {
    if (_all) {
        // Print all Native Uniforms (they carry functions)
        for (UniformFunctionsList::iterator it= functions.begin(); it != functions.end(); ++it) {                
            std::cout << it->second.type << ',' << it->first;
            if (it->second.print) {
                std::cout << "," << it->second.print();
            }
            std::cout << std::endl;
        }
    }
    else {
        // Print Native Uniforms (they carry functions) that are present on the shader
        for (UniformFunctionsList::iterator it= functions.begin(); it != functions.end(); ++it) {                
            if (it->second.present) {
                std::cout << it->second.type << ',' << it->first;
                if (it->second.print) {
                    std::cout << "," << it->second.print();
                }
                std::cout << std::endl;
            }
        }
    }
    
    // Print user defined uniform data
    for (UniformDataList::iterator it= data.begin(); it != data.end(); ++it) {
        std::cout << it->second.getType() << "," << it->first;
        for (int i = 0; i < it->second.size; i++) {
            std::cout << ',' << it->second.value[i];
        }
        std::cout << std::endl;
    }

    printBuffers();
    printTextures();
    printLights();
}

void Uniforms::printBuffers() {
    for (size_t i = 0; i < buffers.size(); i++)
        std::cout << "sampler2D,u_buffer" << i << std::endl;
    
    if (functions["u_scene"].present)
        std::cout << "sampler2D,u_scene" << std::endl;

    if (functions["u_sceneDepth"].present)
        std::cout << "sampler2D,u_sceneDepth" << std::endl;
}

void Uniforms::printTextures(){
     for (TextureList::iterator it = textures.begin(); it != textures.end(); ++it) {
        std::cout << "sampler2D," << it->first << ',' << it->second->getFilePath() << std::endl;
    }
}

void Uniforms::printLights() {
    if (lights.size() == 1) {
        if (lights[0].getType() != LIGHT_DIRECTIONAL)
            std::cout << "u_light," << toString( lights[0].getPosition() ) << std::endl;
        std::cout << "u_lightColor," << toString( lights[0].color )  << std::endl;
        if (lights[0].getType() == LIGHT_DIRECTIONAL || lights[0].getType() == LIGHT_SPOT)
            std::cout << "u_lightDirection," << toString( lights[0].direction ) << std::endl;
        std::cout << "u_lightIntensity," << toString( lights[0].intensity, 3) << std::endl;
        if (lights[0].falloff > 0.0)
            std::cout << "u_lightFalloff," << toString( lights[0].falloff, 3) << std::endl;
        // std::cout << "sampler2D,u_lightShadowMap" << std::endl;
    }
    else if (lights.size() > 1) {
        for (unsigned int i = 0; i < lights.size(); i++) {
            if (lights[i].getType() != LIGHT_DIRECTIONAL)
                std::cout << "u_light," << toString( lights[i].getPosition() ) << std::endl;
            std::cout << "u_lightColor," << toString( lights[i].color )  << std::endl;
            if (lights[i].getType() == LIGHT_DIRECTIONAL || lights[i].getType() == LIGHT_SPOT)
                std::cout << "u_lightDirection," << toString( lights[i].direction ) << std::endl;
            std::cout << "u_lightIntensity," << toString( lights[i].intensity, 3) << std::endl;
            if (lights[i].falloff > 0.0)
                std::cout << "u_lightFalloff," << toString( lights[i].falloff, 3) << std::endl;
            // std::cout << "sampler2D,u_lightShadowMap" << std::endl;
        }
    }
}
