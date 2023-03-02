#pragma once

#include "engine.h"

class Headless : public Engine {
public:

    Headless();
    virtual ~Headless();

    void init();

    void draw();

    void close();

};