#pragma once

#include "gl.h"

// GL Context
void initGL(int argc, char **argv);
bool isGL();
void updateGL();
void renderGL();
void closeGL();

// SET
void setWindowSize(int _width, int _height);

// GET
int getWindowWidth();
int getWindowHeight();
float getMouseX();
float getMouseY();
int getMouseButton();
float getMouseVelX();
float getMouseVelY();
unsigned char getKeyPressed();

// EVENTS
void onKeyPress(int _key);
void onMouseMove(float _x, float _y);
void onMouseClick(float _x, float _y, int _button);
void onMouseDrag(float _x, float _y, int _button);
void onViewportResize(int _width, int _height);
