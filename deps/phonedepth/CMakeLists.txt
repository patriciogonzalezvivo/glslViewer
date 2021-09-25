project(phonedepth)

set(PHONEDEPTH_HEADERS    extract_depthmap.h)
set(PHONEDEPTH_SOURCES    extract_depthmap.cpp)

set_source_files_properties(${PHONEDEPTH_SOURCES} PROPERTIES LANGUAGE CXX)
add_library(phonedepth STATIC ${PHONEDEPTH_SOURCES} ${PHONEDEPTH_HEADERS})