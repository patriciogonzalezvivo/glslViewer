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

    virtual void init();

    virtual void loadMesh(const std::string& _name, const vera::Mesh& _mesh);
    virtual void loadImage(const std::string& _name, const std::string& _path, bool _flip);
    virtual void loadShaders();

    virtual void setCamera(const vera::Camera& _cam);
    
    virtual void setSun(const vera::Light& _light);
    virtual void setSkyTurbidity(float _turbidity) { uniforms.setSkyTurbidity(_turbidity); }
    virtual void setSkyGround(float _r, float _g, float _b);
    
    virtual void setMeshTransformMatrix(    const std::string& _name, 
                                    float x1, float y1, float z1, float w1,
                                    float x2, float y2, float z2, float w2,
                                    float x3, float y3, float z3, float w3,
                                    float x4, float y4, float z4, float w4 );
    virtual void setFps(int _fps);
    virtual void setFxaa(bool _value);
    virtual void setOutput(const std::string& _path) { screenshotFile = _path;}

    virtual int  getOutputFboId() const { return m_record_fbo.getId(); }
    
    virtual void printLights() { uniforms.printLights(); }
    virtual void printDefines() { m_sceneRender.printDefines(); uniforms.printDefines(); }
    virtual void printBuffers() { m_sceneRender.printBuffers(); uniforms.printBuffers(); }
    virtual void printMaterials() { uniforms.printMaterials(); }
    virtual void printModels() { uniforms.printModels(); }
    virtual void printShaders() { uniforms.printShaders(); }

    virtual void showHistogram(bool _value);
    virtual void showTextures(bool _value) { m_showTextures = _value; };
    virtual bool haveTexture(const std::string& _name);
    virtual bool loadTexture(const std::string& _name, int _width, int _height, int _channels, const std::vector<float>& _pixels);
    virtual bool addTexture(const std::string& _name, int _width, int _height, int _id);
    virtual void printTextures() { uniforms.printTextures(); }

    virtual void showCubemap(bool _value) { m_sceneRender.showCubebox = _value; };
    virtual void enableCubemap(bool _value);
    virtual bool haveCubemap(const std::string& _name);
    virtual bool addCubemap(const std::string& _name, int _width, int _height, int _channels, const std::vector<float>& _pixels);
    virtual void printCubemaps() { uniforms.printCubemaps(); }

    virtual void setUniform(const std::string& _name, const std::vector<float>& _values);

    virtual void showPasses(bool _value) { m_showPasses = _value; };
    virtual void showBoudningBox(bool _value) { m_sceneRender.showBBoxes = _value; }
    
    virtual void resize(const size_t& width, const size_t& height);

    virtual void clearModels();

    virtual void draw();

private:

    bool    m_enableCubemap;

};