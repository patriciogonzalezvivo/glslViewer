#include "text.h"

#include <algorithm>
#include <array>
#include <functional>
#include <regex>
#include <tuple>
#include <cstring>

#include "vera/ops/string.h"

namespace {

template<typename T1> // Helper operator shorthand: [enum_t] -> [size_t].
constexpr size_t operator+(T1 some_enum) {
    return static_cast<size_t>(some_enum);
}

template<typename T1>
std::string create_regex_term(T1 regex_piece, const std::string& keyword) {
    std::ostringstream os;
    for(size_t i = 0; i < regex_piece.size()-1; ++i) {
        os << std::begin(regex_piece)[i] << keyword;
    }
    os << std::begin(regex_piece)[regex_piece.size()-1];
    return os.str();
};

template<typename T1, typename T2>
std::regex make_regex(T1 regex_pattern_check, T2 listings_keyword) {
    return std::regex{create_regex_term(regex_pattern_check, std::get<1>(listings_keyword))};
};

std::tuple<bool, std::smatch> does_any_of_the_regex_exist(const std::string& _source, std::regex re) {
    // Split Source code in lines
    const auto lines = vera::split(_source, '\n');
    std::smatch match;
    const auto match_found = std::any_of(std::begin(lines), std::end(lines)
                                         , [&](const std::string& line) { return std::regex_search(line, match, re); });
    return { match_found, match };
}

using regex_stringdata_t = const char * const;

template<typename T1>
using regex_string_t = std::tuple<T1, regex_stringdata_t>;

enum class regex_check_t {
    Convolution_Pyramid,
    Floor,
    Background,
    Post_Processing,
    MAX_KEYWORDS_CHECK_IDS
};
using regex_check_string_t = regex_string_t<regex_check_t>;
const auto valid_check_keyword_ids = std::array<regex_check_string_t, +(regex_check_t::MAX_KEYWORDS_CHECK_IDS)> {{
    {regex_check_t::Convolution_Pyramid, "CONVOLUTION_PYRAMID_ALGORITHM"}
    , {regex_check_t::Floor,"FLOOR"}
    , {regex_check_t::Background, "BACKGROUND"}
    , {regex_check_t::Post_Processing, "POSTPROCESSING"}
}};

bool generic_search_check(const std::string& _source, regex_check_t keyword_id ) {
    const auto regex_pattern_check = {
        R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*)"
        , R"()(?:\s*\))|(?:^\s*#ifdef\s+)"
        , R"()|(?:^\s*#ifndef\s+)"
        , R"())"
    };
    const auto re = make_regex(regex_pattern_check, valid_check_keyword_ids[+(keyword_id)]);
    return std::get<0>(does_any_of_the_regex_exist(_source, re));   //return only the "result" boolean.
}

enum class regex_count_t {
    Buffers,
    Double_Buffers,
    Convolution_Pyramid,
    Scene_Buffers,
    MAX_KEYWORDS_COUNT_IDS
};
using regex_count_string_t = regex_string_t<regex_count_t>;
const auto valid_count_keyword_ids = std::array<regex_count_string_t, +(regex_count_t::MAX_KEYWORDS_COUNT_IDS)> {{
    {regex_count_t::Buffers, "BUFFER"},
    {regex_count_t::Double_Buffers, "DOUBLE_BUFFER"},
    {regex_count_t::Convolution_Pyramid, "CONVOLUTION_PYRAMID"},
    {regex_count_t::Scene_Buffers, "SCENE_BUFFER"}
}};

struct is_not_duplicate_number_predicate {
    // Group results in a vector to check for duplicates
    std::vector<std::string> results = {};
    bool operator()(const std::string &line, const std::regex &re) {
        std::smatch match;
        // if there are matches
        if (std::regex_search(line, match, re)) {
            // Depending the case can be in the 2nd or 3rd group
            const auto case_group = [&](size_t index){return std::ssub_match(match[index]).str();};
            const auto number = (case_group(2).size() == 0)
                    ? case_group(3)
                    : case_group(2);
            // Check if it's already defined
            // If it's not add it
            if (!std::any_of(std::begin(results), std::end(results)
                             , [&](const std::string& index){ return index == number; })) {
                results.push_back(number);
                return true;
            }
        }
        return false;
    }
};

int generic_search_count(const std::string& _source, regex_count_t keyword_id ) {
    const auto regex_pattern_count  = {
        R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*)"
        , R"(_)(\d+)(?:\s*\))|(?:^\s*#ifdef\s+)"
        , R"(_)(\d+))"
    };
    // Split Source code in lines
    const auto lines = vera::split(_source, '\n');
    // Regext to search for #ifdef BUFFER_[NUMBER], #if defined( BUFFER_[NUMBER] ) and #elif defined( BUFFER_[NUMBER] ) occurences
    const auto re = make_regex(regex_pattern_count, valid_count_keyword_ids[+(keyword_id)]);
    // return the number of results
    auto predicate_op = is_not_duplicate_number_predicate{};
    return std::count_if(std::begin(lines), std::end(lines), [&](const std::string& line) {
        return std::ref(predicate_op)(line, re);
    });
}

enum class regex_get_t {
    BufferSize,
    MAX_KEYWORDS_GET_IDS
};

using regex_get_string_t = regex_string_t<regex_get_t>;
const auto valid_get_keyword_ids = std::array<regex_get_string_t, +(regex_get_t::MAX_KEYWORDS_GET_IDS)> {{
    {regex_get_t::BufferSize, R"(uniform\s*sampler2D\s*(\w*)\;\s*\/\/*\s(\d+)x(\d+))"}
}};

bool generic_search_get(const std::string& _source, const std::string& _name, glm::vec2& _size, regex_get_t keyword_id ) {
    bool result;
    std::smatch match;
    const auto re = std::regex{std::get<1>(valid_get_keyword_ids[+(keyword_id)])};
    std::tie(result, match) = does_any_of_the_regex_exist(_source, re); // capture both the "result" and the "match" info.
    if(result) {
        if (match[1] == _name) {    // regex-match result data is valid to spec.
            _size = {vera::toFloat(match[2]), vera::toFloat(match[3])};
        }
    }
    return result;
}
}  // Namespace {}

// Quickly determine if a shader program contains the specified identifier.
bool findId(const std::string& program, const char* id) {
    return std::strstr(program.c_str(), id) != 0;
}

// Count how many BUFFERS are in the shader
int countBuffers(const std::string& _source) {
    return generic_search_count(_source, regex_count_t::Buffers);
}

bool getBufferSize(const std::string& _source, const std::string& _name, glm::vec2& _size) {
    return generic_search_get(_source, _name, _size, regex_get_t::BufferSize);
}

// Count how many BUFFERS are in the shader
int countDoubleBuffers(const std::string& _source) {
    return generic_search_count(_source, regex_count_t::Double_Buffers);
}

// Count how many BUFFERS are in the shader
bool checkBackground(const std::string& _source) {
    return generic_search_check(_source, regex_check_t::Background);
}

// Count how many BUFFERS are in the shader
bool checkFloor(const std::string& _source) {
    return generic_search_check(_source, regex_check_t::Floor);
}

bool checkPostprocessing(const std::string& _source) {
    return generic_search_check(_source, regex_check_t::Post_Processing);
}

// Count how many CONVOLUTION_PYRAMID_ are in the shader
int countConvolutionPyramid(const std::string& _source) {
    return generic_search_count(_source, regex_count_t::Convolution_Pyramid);
}

bool checkConvolutionPyramid(const std::string& _source) {
    return generic_search_check(_source, regex_check_t::Convolution_Pyramid);
}

int countSceneBuffers(const std::string& _source) {
    return generic_search_count(_source, regex_count_t::Scene_Buffers);
}

std::string getUniformName(const std::string& _str) {
    std::vector<std::string> values = vera::split(_str, '.');
    return "u_" + vera::toLower( vera::toUnderscore( vera::purifyString( values[0] ) ) );
}

bool checkPattern(const std::string& _str) {
    return  (_str.find('*') != std::string::npos) ||
            (_str.find('?') != std::string::npos);
}
