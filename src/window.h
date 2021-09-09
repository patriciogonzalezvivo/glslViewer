#pragma once

#include "gl/gl.h"
#include "glm/glm.hpp"

enum WindowStyle {
    HEADLESS = 0,
    REGULAR,
    ALLWAYS_ON_TOP,
    FULLSCREEN,
    HOLOPLAY
};

//	GL Context
//----------------------------------------------
void initGL(glm::ivec4 &_viewport, WindowStyle _prop = REGULAR);
bool isGL();
void updateGL();
void renderGL();
void closeGL();

//	SET
//----------------------------------------------
void updateViewport();

void setFps(int _fps);
void setViewport(float _width, float _height);
void setWindowSize(int _width, int _height);
void setMousePosition(float _x, float _y);
void setMousePosition(glm::vec2 _pos);

//	GET
//----------------------------------------------
glm::ivec2  getScreenSize();
float       getPixelDensity();

glm::ivec4  getViewport();
glm::mat4   getOrthoMatrix();
int         getWindowWidth();
int         getWindowHeight();

glm::vec4   getDate();
double      getTime();
double      getDelta();
double      getFps();
float       getRestSec();
int         getRestMs();
int         getRestUs();

float       getMouseX();
float       getMouseY();
glm::vec2   getMousePosition();
float       getMouseVelX();
float       getMouseVelY();
glm::vec2   getMouseVelocity();
int         getMouseButton();
glm::vec4   getMouse4();
bool        getMouseEntered();

// EVENTS
//----------------------------------------------
void onKeyPress(int _key);
void onMouseMove(float _x, float _y);
void onMouseClick(float _x, float _y, int _button);
void onMouseDrag(float _x, float _y, int _button);
void onViewportResize(int _width, int _height);
void onScroll(float _yoffset);

#ifdef PLATFORM_RPI
EGLDisplay getEGLDisplay();
EGLContext getEGLContext();
#endif
