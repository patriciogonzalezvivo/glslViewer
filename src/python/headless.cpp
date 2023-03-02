#include "headless.h"

#include "vera/shaders/defaultShaders.h"


Headless::Headless() {
};

Headless::~Headless() {
};

void Headless::init() {
    vera::WindowProperties props;
    props.style = vera::HEADLESS;
    vera::initGL(props);
    WatchFileList files;
    resetShaders( files );
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

    vera::renderGL();
}

void Headless::close() {
    vera::closeGL();
}

