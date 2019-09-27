#pragma once

#include "glm/glm.hpp"

#ifndef REST_SEC
#define REST_SEC 0.016 // 60fps
#endif

enum windowStyle {
    HEADLESS = 0,
    REGULAR,
    ALLWAYS_ON_TOP,
    FULLSCREEN
};

//	GL Context
//----------------------------------------------
void initGL(glm::ivec4 &_viewport, windowStyle _prop = REGULAR);
bool isGL();
void updateGL();
void renderGL();
void closeGL();

//	SET
//----------------------------------------------
void setWindowSize(int _width, int _height);

//	GET
//----------------------------------------------
glm::ivec2 getScreenSize();
float getPixelDensity();

glm::ivec4 getViewport();
glm::mat4 getOrthoMatrix();
int getWindowWidth();
int getWindowHeight();


glm::vec4 getDate();
double getTime();
double getDelta();
double getFPS();

float getMouseX();
float getMouseY();
glm::vec2 getMousePosition();
float getMouseVelX();
float getMouseVelY();
glm::vec2 getMouseVelocity();
int getMouseButton();
glm::vec4 getMouse4();

// EVENTS
//----------------------------------------------
void onKeyPress(int _key);
void onMouseMove(float _x, float _y);
void onMouseClick(float _x, float _y, int _button);
void onMouseDrag(float _x, float _y, int _button);
void onViewportResize(int _width, int _height);
void onScroll(float _yoffset);
