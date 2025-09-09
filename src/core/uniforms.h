#pragma once

#include <map>
#include <queue>
#include <mutex>
#include <array>
#include <vector>
#include <string>
#include <functional>

#include "tools/files.h"
#include "tools/tracker.h"

#include "vera/gl/flood.h"
#include "vera/types/scene.h"
#include "vera/types/image.h"

typedef std::array<float, 16> UniformValue;

struct UniformData {
    std::string getType();

    void        set(const UniformValue &_value, size_t _size, bool _int = false, bool _queue = true);
    void        parse(const std::vector<std::string>& _command, size_t _start = 1, bool _queue = true);
    std::string print();
    bool        check();

    std::queue<UniformValue>            queue;
    UniformValue                        value;
    size_t                              size    = 0;
    bool                                bInt    = false;
    bool                                change  = false;
};

struct UniformFunction {
    UniformFunction();
    UniformFunction(const std::string &_type);
    UniformFunction(const std::string &_type, std::function<void(vera::Shader&)> _assign);
    UniformFunction(const std::string &_type, std::function<void(vera::Shader&)> _assign, std::function<std::string()> _print);

    std::function<void(vera::Shader&)>   assign;
    std::function<std::string()>        print;
    std::string                         type;
    bool                                present = false;
};

// Uniforms values types (float, vecs and functions)
typedef std::map<std::string, UniformFunction>          UniformFunctionsMap;
typedef std::map<std::string, UniformData>              UniformDataMap;
typedef std::map<std::string, std::vector<UniformData>> UniformSequenceMap;

// Buffers types
typedef std::vector<vera::Fbo*>                 BuffersList;
typedef std::vector<vera::PingPong*>            DoubleBuffersList;
typedef std::vector<vera::Pyramid>              PyramidsList;
typedef std::vector<vera::Flood>                FloodList;

typedef std::map<std::string, vera::Image>      ImagesMap;

class Uniforms : public vera::Scene {
public:
    Uniforms();
    virtual ~Uniforms();

    virtual void        clear();
    
    // Change state
    virtual void        flagChange();
    virtual void        resetChange();
    virtual bool        haveChange();

    // Feed uniforms to a specific shader
    virtual bool        feedTo( vera::Shader *_shader, bool _lights = true, bool _buffers = true);

    // defines
    virtual void        addDefine(const std::string& _define, const std::string& _value);
    virtual void        delDefine(const std::string& _define);
    virtual void        printDefines();

    // Buffers
    BuffersList         buffers;
    DoubleBuffersList   doubleBuffers;
    PyramidsList        pyramids;
    FloodList           floods;
    virtual void        printBuffers();
    virtual void        clearBuffers();

    std::mutex          loadMutex;
    ImagesMap           loadQueue;

    // Uniforms that trigger functions (u_time, u_data, etc.)
    UniformFunctionsMap functions;
    virtual void        checkUniforms( const std::string &_vert_src, const std::string &_frag_src );

    // Manually added uniforms
    UniformDataMap      data;
    virtual void        set( const std::string& _name, float _value);
    virtual void        set( const std::string& _name, float _x, float _y);
    virtual void        set( const std::string& _name, float _x, float _y, float _z);
    virtual void        set( const std::string& _name, float _x, float _y, float _z, float _w);
    virtual void        set( const std::string& _name, const std::vector<float>& _data, bool _queue = true);
    virtual bool        parseLine( const std::string &_line );

    UniformSequenceMap  sequences;
    virtual bool        addSequence( const std::string& _name, const std::string& _filename);
    virtual void        setStreamsPlay();
    virtual void        setStreamsFrame(size_t _frame);
    virtual void        setStreamsStop();
    virtual void        setStreamsRestart();

    virtual bool        addCameras( const std::string& _filename );

    virtual void        clearUniforms();
    virtual void        printAvailableUniforms(bool _non_active);
    virtual void        printDefinedUniforms(bool _csv = false);

    Tracker             tracker;

    void                update();
    void                setFrame(size_t _frame) { m_frame = _frame; }
    size_t              getFrame() const { return m_frame; }
    bool                isPlaying() const { return m_play; }

protected:
    size_t              m_frame;
    bool                m_play;
};


