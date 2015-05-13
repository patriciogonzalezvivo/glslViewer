#pragma once

#include "gl.h"
#include "utils.h"

typedef struct {
    float   x,y;
    float   velX,velY;
    int     button;
} Mouse;
static Mouse mouse;
static unsigned char keyPressed;

typedef struct {
    uint32_t x, y, width, height;
} Viewport;
static Viewport viewport;

// Define events
void onKeyPress(int _key);
void onMouseMove();
void onMouseClick();
void onMouseDrag();
void onViewportResize(int _width, int _height);

void resizeViewport(uint _width, uint _height);

void initGL(int argc, char **argv);
bool isGL();
void updateGL();
void renderGL();
void closeGL();
