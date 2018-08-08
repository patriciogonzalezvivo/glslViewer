#pragma once

#include "glm/glm.hpp"

//	GL Context
//----------------------------------------------
void initGL(glm::ivec4 &_viewport, bool _headless = false);
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
