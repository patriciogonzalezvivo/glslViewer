#include "defines.h"

#include "tools/text.h"

HaveDefines::HaveDefines() : m_defineChange(false) {
}

HaveDefines::~HaveDefines() {
}

void HaveDefines::addDefine(const std::string &_define, const std::string &_value) {
    std::string define = toUpper( toUnderscore( purifyString(_define) ) );

    // if doesn't exist
    if (m_defines.find(define) == m_defines.end()) {
        // add it
        // std::cout << "Adding: " << define <<  " " << _value << std::endl;
        m_defines[define] = _value;
        m_defineChange = true;
    }
    // if its different
    else if ( m_defines[define] != _value){
        // change it
        // std::cout << "Changing: " << define <<  " " << _value << std::endl;
        m_defines[define] = _value;
        m_defineChange = true;
    }
}

void HaveDefines::addDefine( const std::string &_define, int _n ) {
    addDefine(_define, toString(_n));
}

void HaveDefines::addDefine( const std::string &_define, float _n ) {
    addDefine(_define, toString(_n, 3));
}

void HaveDefines::addDefine( const std::string &_define, double _n ) {
    addDefine(_define, toString(_n, 6));
}

void HaveDefines::addDefine( const std::string &_define, glm::vec2 _v ) {
    addDefine(_define, "vec2(" + toString(_v, ',') + ")");
}

void HaveDefines::addDefine( const std::string &_define, glm::vec3 _v ) {
    addDefine(_define, "vec3(" + toString(_v, ',') + ")");
}

void HaveDefines::addDefine( const std::string &_define, glm::vec4 _v ) {
    addDefine(_define, "vec4(" + toString(_v, ',') + ")");
}

void HaveDefines::addDefine( const std::string& _define, float* _value, int _nValues) {
    if (_nValues == 1) {
        addDefine(_define, _value[0]);
    }
    else if (_nValues == 2) {
        addDefine(_define, glm::vec2(_value[0], _value[1]));
    }
    else if (_nValues == 3) {
        addDefine(_define, glm::vec3(_value[0], _value[1], _value[2]));
    }
    else if (_nValues == 4) {
        addDefine(_define, glm::vec4(_value[0], _value[1], _value[2], _value[3]));
    }
}

void HaveDefines::addDefine( const std::string& _define, double* _value, int _nValues) {
    if (_nValues == 1) {
        addDefine(_define, _value[0]);
    }
    else if (_nValues == 2) {
        addDefine(_define, glm::vec2(_value[0], _value[1]));
    }
    else if (_nValues == 3) {
        addDefine(_define, glm::vec3(_value[0], _value[1], _value[2]));
    }
    else if (_nValues == 4) {
        addDefine(_define, glm::vec4(_value[0], _value[1], _value[2], _value[3]));
    }
}

void HaveDefines::delDefine(const std::string &_define) {
    std::string define = toUpper( toUnderscore( purifyString(_define) ) );

    if (m_defines.find(define) != m_defines.end()) {
        m_defines.erase(define);
        m_defineChange = true;
    }
}

void HaveDefines::printDefines() {
    for (DefinesList_cit it = m_defines.begin(); it != m_defines.end(); ++it) {
        std::cout << "#define " << it->first << " " << it->second << std::endl;
    }
}

void HaveDefines::mergeDefines( HaveDefines *_haveDefines ) {
    for (DefinesList_cit it = _haveDefines->m_defines.begin(); it != _haveDefines->m_defines.end(); ++it) {
        addDefine(it->first, it->second);
    }
}

void HaveDefines::mergeDefines( const HaveDefines *_haveDefines ) {
    for (DefinesList_cit it = _haveDefines->m_defines.begin(); it != _haveDefines->m_defines.end(); ++it) {
        addDefine(it->first, it->second);
    }
}

void HaveDefines::mergeDefines( const DefinesList &_defines ) {
    for (DefinesList_cit it=_defines.begin(); it != _defines.end(); ++it) {
        addDefine(it->first, it->second);
    }
}

void HaveDefines::replaceDefines( const DefinesList &_defines ) {
    m_defines = _defines;
    m_defineChange = true;
}