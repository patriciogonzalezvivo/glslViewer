#include "props.h"

GLenum getFilter( TextureFilter _filter ) {
    static GLenum filters[6] = {
        GL_LINEAR, GL_NEAREST 
        // GL_NEAREST_MIPMAP_NEAREST, 
        // GL_LINEAR_MIPMAP_NEAREST, 
        // GL_NEAREST_MIPMAP_LINEAR, 
        // GL_LINEAR_MIPMAP_LINEAR 
    };
    return filters[_filter];
}

GLenum getMagnificationFilter( TextureFilter _filter ) {
    // if(_filter == NEAREST_NEAREST || _filter == NEAREST_LINEAR) {
    //     return getFilter(NEAREST);
    // }
    // if(_filter == LINEAR_NEAREST || _filter == LINEAR_LINEAR) {
    //     return getFilter(LINEAR);
    // }
    return getFilter(_filter);
}

GLenum getMinificationFilter( TextureFilter _filter )  {
    return getFilter(_filter);
}

GLenum getWrap( TextureWrap _wrap ) {
    static GLenum wraps[3] = {GL_REPEAT, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT };
    return wraps[_wrap];
}