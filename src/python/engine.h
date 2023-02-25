#pragma once

#include <cstddef>
#include <iostream>
#include <functional>

#include "vera/types/camera.h"
#include "../core/sandbox.h"

class Engine : public Sandbox {
public:

    Engine();
    virtual ~Engine();

    void loadMesh(const std::string& _name, const vera::Mesh& _mesh);
    void loadImage(const std::string& _name, const std::string& _path, bool _flip);
    void loadShaders();

    void setSun(const vera::Light& _light);
    void setSunPosition(float _az, float _elev, float _distance);
    void setCamera(const vera::Camera& _cam);
    void setMeshTransformMatrix(    const std::string& _name, 
                                    float x1, float y1, float z1, float w1,
                                    float x2, float y2, float z2, float w2,
                                    float x3, float y3, float z3, float w3,
                                    float x4, float y4, float z4, float w4 );

    void printLights() { uniforms.printLights(); }
    void printDefines() { m_sceneRender.printDefines(); uniforms.printDefines(); }
    void printBuffers() { m_sceneRender.printBuffers(); uniforms.printBuffers(); }
    void printMaterials() { uniforms.printMaterials(); }
    void printModels() { uniforms.printModels(); }
    void printShaders() { uniforms.printShaders(); }

    void showTextures(bool _value) { m_showTextures = _value; };
    bool getShowTextures() { return m_showTextures; };
    bool haveTexture(const std::string& _name);
    bool addTexture(const std::string& _name, int _width, int _height,const std::vector<float>& _pixels);
    void printTextures() { uniforms.printTextures(); }

    void showCubemap(bool _value) { m_sceneRender.showCubebox = _value; };
    bool getShowCubemap() { return m_sceneRender.showCubebox; };
    bool haveCubemap(const std::string& _name);
    bool addCubemap(const std::string& _name, int _width, int _height,const std::vector<float>& _pixels);
    void printCubemaps() { uniforms.printCubemaps(); }

    void showPasses(bool _value) { m_showPasses = _value; };
    bool getShowPasses() { return m_showPasses; };

    void resize(const size_t& width, const size_t& height);

    void clearModels();

    void draw();

private:

};