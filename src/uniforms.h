#pragma once

#include <map>
#include <vector>
#include <string>
#include <functional>

#include "gl/fbo.h"
#include "gl/shader.h"
#include "gl/texture.h"
#include "gl/textureStream.h"
#include "gl/textureAudio.h"
#include "gl/pyramid.h"

#include "scene/light.h"
#include "scene/camera.h"

#include "io/fs.h"

struct UniformData {
    std::string getType();

    float   value[4];
    int     size;
    bool    bInt = false;
    bool    change = false;
};

typedef std::map<std::string, UniformData> UniformDataList;

struct UniformFunction {
    UniformFunction();
    UniformFunction(const std::string &_type);
    UniformFunction(const std::string &_type, std::function<void(Shader&)> _assign);
    UniformFunction(const std::string &_type, std::function<void(Shader&)> _assign, std::function<std::string()> _print);

    std::function<void(Shader&)>    assign;
    std::function<std::string()>    print;
    std::string                     type;
    bool                            present = false;
};

typedef std::map<std::string, UniformFunction> UniformFunctionsList;
typedef std::map<std::string, Texture*> TextureList;
typedef std::map<std::string, TextureStream*> StreamsList;

class Uniforms {
public:
    Uniforms();
    virtual ~Uniforms();

    // Ingest new uniforms
    bool                    parseLine( const std::string &_line );

    bool                    addTexture( const std::string& _name, Texture* _texture );
    bool                    addTexture( const std::string& _name, const std::string& _path, WatchFileList& _files, bool _flip = true, bool _verbose = true );
    bool                    addBumpTexture( const std::string& _name, const std::string& _path, WatchFileList& _files, bool _flip = true, bool _verbose = true );
    bool                    addStreamingTexture( const std::string& _name, const std::string& _url, bool _flip = true, bool _device = false, bool _verbose = true );
    bool                    addAudioTexture( const std::string& _name, const std::string& device_id, bool _flip = false, bool _verbose = true );
    void                    updateStreammingTextures();

    void                    set( const std::string& _name, float _value);
    void                    set( const std::string& _name, float _x, float _y);
    void                    set( const std::string& _name, float _x, float _y, float _z);
    void                    set( const std::string& _name, float _x, float _y, float _z, float _w);
    
    void                    setCubeMap( TextureCube* _cubemap );
    void                    setCubeMap( const std::string& _filename, WatchFileList& _files, bool _verbose = true);

    // Check presence of uniforms on shaders
    void                    checkPresenceIn( const std::string &_vert_src, const std::string &_frag_src );


    // Feed uniforms to a specific shader
    bool                    feedTo( Shader &_shader );

    Camera&                 getCamera() { return cameras[0]; }

    // Debug
    void                    print(bool _all);
    void                    printBuffers();
    void                    printTextures();
    void                    printLights();

    // Change state
    void                    flagChange();
    void                    unflagChange();
    bool                    haveChange();

    void                    clear();

    // Manually defined uniforms (through console IN)
    UniformDataList         data;

    // Automatic uniforms
    UniformFunctionsList    functions;

    // Common 
    TextureList             textures;
    StreamsList             streams;

    TextureCube*            cubemap;
    std::vector<Fbo>        buffers;
    Pyramid                 poissonfill;

    // 3d Scene Uniforms 
    std::vector<Camera>     cameras;
    std::vector<Light>      lights;

protected:
    bool                    m_change;
    bool                    m_is_audio_init;
};


