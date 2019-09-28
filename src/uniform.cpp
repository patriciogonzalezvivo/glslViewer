#include "uniform.h"

#include <regex>
#include <sstream>
#include <sys/stat.h>

#include "tools/text.h"

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

Uniforms::Uniforms():
    m_change(false) {
}

Uniforms::~Uniforms(){

}

bool Uniforms::parseLine( const std::string &_line ) {
    bool somethingChange = parseUniformData(_line, &data);
    m_change += somethingChange;
    return somethingChange;
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
            if (tex->load(_path, true)) {

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
            Texture* tex = new Texture();

            // load an image into the texture
            if (tex->loadBump(_path, true)) {

                // the image is loaded finish add the texture to the uniform list
                textures[ _name ] = tex;

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
                    _shader.setUniform(it->first, int(it->second.value[0]));
                    update = true;
                }
                else {
                    _shader.setUniform(it->first, it->second.value, it->second.size);
                    update = true;
                }
                // it->second.change = false;
            }
        }
    }

    // Pass Textures Uniforms
    for (TextureList::iterator it = textures.begin(); it != textures.end(); ++it) {
        _shader.setUniformTexture(it->first, it->second, _shader.textureIndex++ );
        _shader.setUniform(it->first+"Resolution", it->second->getWidth(), it->second->getHeight());
    }

    // Pass Buffers Uniforms
    for (unsigned int i = 0; i < buffers.size(); i++)
        _shader.setUniformTexture("u_buffer" + toString(i), &buffers[i], _shader.textureIndex++ );
    
    return update;
}

void Uniforms::flagChange() {
    // Flag all user uniforms as changed
    for (UniformDataList::iterator it = data.begin(); it != data.end(); ++it) {
        it->second.change = true;
    }
    m_change = true;
}

void Uniforms::unflagChange() {
    if (m_change) {
        // Flag all user uniforms as NOT changed
        for (UniformDataList::iterator it = data.begin(); it != data.end(); ++it) {
            it->second.change = false;
        }
        m_change = false;
    }
}

bool Uniforms::haveChange() { 
    return  m_change || 
            functions["u_time"].present || 
            functions["u_delta"].present ||
            functions["u_mouse"].present ||
            functions["u_date"].present;
}

void Uniforms::clear() {
    // Delete Textures
    for (TextureList::iterator i = textures.begin(); i != textures.end(); ++i) {
        if (i->second) {
            delete i->second;
            i->second = nullptr;
        }
    }
    textures.clear();
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
