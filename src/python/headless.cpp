#include "headless.h"

#include "vera/shaders/defaultShaders.h"

WatchFileList   hfiles;

Headless::Headless() {
    verbose = true;
    help = false;
    cursor = false;
};

Headless::~Headless() {
};

void Headless::init() {
    vera::WindowProperties props;
    props.style = vera::HEADLESS;
    std::cout << "HEADLESS vera::initGL()" << std::endl;
    vera::initGL(props);

    addDefine("GLSLVIEWER", vera::toString(GLSLVIEWER_VERSION_MAJOR) + vera::toString(GLSLVIEWER_VERSION_MINOR) + vera::toString(GLSLVIEWER_VERSION_PATCH) );
    setSource(FRAGMENT, vera::getDefaultSrc(vera::FRAG_DEFAULT));
    setSource(VERTEX, vera::getDefaultSrc(vera::VERT_DEFAULT_SCENE));
    resetShaders( hfiles );

    m_sceneRender.dynamicShadows = true;
    uniforms.setSkyFlip(true);
    uniforms.setSkySize(2048/2);

    m_initialized = true;
}

void Headless::draw() {
    vera::updateGL();
    
    uniforms.update();

    renderPrep();
    render();
    renderPost();

    if (screenshotFile != "") {
        onScreenshot(screenshotFile);
        screenshotFile = "";
    }

    unflagChange();
    if (!m_initialized) {
        m_initialized = true;
        vera::updateViewport();
        flagChange();
    }

    vera::renderGL();
}

void Headless::close() {
    std::cout << "HEADLESS vera::closeGL()" << std::endl;
    vera::closeGL();
}

