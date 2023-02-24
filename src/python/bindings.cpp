#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>
#include <vector>

#include "engine.h"
#include "vera/types/node.h"
#include "vera/types/mesh.h"
#include "vera/types/light.h"
#include "vera/types/camera.h"
#include "vera/types/model.h"

namespace py = pybind11;

#define GET(type, name) \
    #name,\
    [](const type& obj) { return obj.name; }

#define GETSET(type, name) \
    #name,\
    [](const type& obj) { return obj.name; },\
    [](type& obj, const auto& value) { obj.name = value; }

PYBIND11_MODULE(PyGlslViewer, m) {
    m.doc() = "PyGlslViewer bindings";
    
    /**
     * Integration classes
     */

    py::class_<vera::Node>(m, "Node")
        .def(py::init<>())
        .def("setScale",py::overload_cast<float>(&vera::Node::setScale), py::arg("s"))
        .def("setScale",py::overload_cast<float,float,float>(&vera::Node::setScale), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setPosition",py::overload_cast<float,float,float>(&vera::Node::setPosition), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setOrientation",py::overload_cast<float,float,float>(&vera::Node::setOrientation), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setOrientation",py::overload_cast<float,float,float,float>(&vera::Node::setOrientation), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
        .def("setOrientation",py::overload_cast<float,float,float, float,float,float, float,float,float>(&vera::Node::setOrientation), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("x3"), py::arg("y3"), py::arg("z3") )
        .def("setOrientation",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Node::setOrientation), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        .def("setTransformMatrix",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Node::setTransformMatrix), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )

        .def("getPitch",&vera::Node::getPitch)
        .def("getHeading",&vera::Node::getHeading)
        .def("getRoll",&vera::Node::getRoll)

        .def("scale",py::overload_cast<float>(&vera::Node::scale), py::arg("s"))
        .def("scale",py::overload_cast<float,float,float>(&vera::Node::scale), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("truck",&vera::Node::truck, py::arg("_amount"))
        .def("boom",&vera::Node::boom, py::arg("_amount"))
        .def("dolly",&vera::Node::dolly, py::arg("_amount"))
        .def("translate",py::overload_cast<float,float,float>(&vera::Node::translate), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("rotate",py::overload_cast<float,float,float,float>(&vera::Node::rotate), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
        .def("apply",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Node::apply), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        .def("reset",&vera::Node::reset)
    ;

    py::class_<vera::Camera>(m, "Camera")
        .def(py::init<>())
        .def("setScale",py::overload_cast<float>(&vera::Camera::setScale), py::arg("s"))
        .def("setScale",py::overload_cast<float,float,float>(&vera::Camera::setScale), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setPosition",py::overload_cast<float,float,float>(&vera::Camera::setPosition), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setOrientation",py::overload_cast<float,float,float>(&vera::Camera::setOrientation), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setOrientation",py::overload_cast<float,float,float,float>(&vera::Camera::setOrientation), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
        .def("setOrientation",py::overload_cast<float,float,float, float,float,float, float,float,float>(&vera::Camera::setOrientation), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("x3"), py::arg("y3"), py::arg("z3") )
        .def("setOrientation",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Camera::setOrientation), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        .def("setTransformMatrix",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Camera::setTransformMatrix), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        .def("setProjection",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Camera::setProjection), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        .def("setFOV",&vera::Camera::setFOV, py::arg("_fov"))
        .def("setAspect",&vera::Camera::setAspect, py::arg("_aspect"))
        .def("setViewport",&vera::Camera::setViewport, py::arg("_width"), py::arg("_height"))
        .def("setClipping",&vera::Camera::setClipping, py::arg("_near_clip_distance"), py::arg("_far_clip_distance"))
        .def("setDistance",&vera::Camera::setDistance, py::arg("_distance"))
        .def("setVirtualOffset",&vera::Camera::setVirtualOffset, py::arg("_scale"), py::arg("_currentViewIndex"), py::arg("_totalViews"), py::arg("aspect"))
        .def("setExposure",&vera::Camera::setExposure, py::arg("_aperture"), py::arg("_shutterSpeed"), py::arg("_sensitivity"))
        .def("setTarget",py::overload_cast<float,float,float>(&vera::Camera::setTarget), py::arg("x"), py::arg("y"), py::arg("z"))

        .def("getPitch",&vera::Camera::getPitch)
        .def("getHeading",&vera::Camera::getHeading)
        .def("getRoll",&vera::Camera::getRoll)

        .def("scale",py::overload_cast<float>(&vera::Camera::scale), py::arg("s"))
        .def("scale",py::overload_cast<float,float,float>(&vera::Camera::scale), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("truck",&vera::Camera::truck, py::arg("_amount"))
        .def("boom",&vera::Camera::boom, py::arg("_amount"))
        .def("dolly",&vera::Camera::dolly, py::arg("_amount"))
        .def("translate",py::overload_cast<float,float,float>(&vera::Camera::translate), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("rotate",py::overload_cast<float,float,float,float>(&vera::Camera::rotate), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
        .def("apply",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Camera::apply), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        .def("reset",&vera::Camera::reset)
    ;

    py::class_<vera::Mesh>(m, "Mesh")
        .def(py::init<>())
        .def("addVertex",py::overload_cast<float,float,float>(&vera::Mesh::addVertex), py::arg("_x"), py::arg("_y"), py::arg("_z"))
        .def("getVerticesTotal",&vera::Mesh::getVerticesTotal)
        .def("addNormal",py::overload_cast<float,float,float>(&vera::Mesh::addNormal), py::arg("_x"), py::arg("_y"), py::arg("_z"))
        .def("computeNormals",&vera::Mesh::computeNormals)
        .def("smoothNormals",&vera::Mesh::smoothNormals, py::arg("_angle"))
        .def("invertNormals",&vera::Mesh::invertNormals)
        .def("flatNormals",&vera::Mesh::flatNormals)
        .def("computeTangents",&vera::Mesh::computeTangents)
        .def("addTexCoord",py::overload_cast<float,float>(&vera::Mesh::addTexCoord), py::arg("_u"), py::arg("_v"))
        .def("addIndex",&vera::Mesh::addIndex, py::arg("_i"))
        .def("addTriangleIndices", &vera::Mesh::addTriangleIndices, py::arg("index1"), py::arg("index2"), py::arg("index3"))
        .def("invertWindingOrder",&vera::Mesh::invertWindingOrder)
    ;

    py::class_<vera::Model>(m, "Model")
        .def(py::init<>())
        .def("setScale",py::overload_cast<float>(&vera::Model::setScale), py::arg("s"))
        .def("setScale",py::overload_cast<float,float,float>(&vera::Model::setScale), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setPosition",py::overload_cast<float,float,float>(&vera::Model::setPosition), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setOrientation",py::overload_cast<float,float,float>(&vera::Model::setOrientation), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setOrientation",py::overload_cast<float,float,float,float>(&vera::Model::setOrientation), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
        .def("setOrientation",py::overload_cast<float,float,float, float,float,float, float,float,float>(&vera::Model::setOrientation), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("x3"), py::arg("y3"), py::arg("z3") )
        .def("setOrientation",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Model::setOrientation), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        .def("setTransformMatrix",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Model::setTransformMatrix), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )

        .def("getPitch",&vera::Model::getPitch)
        .def("getHeading",&vera::Model::getHeading)
        .def("getRoll",&vera::Model::getRoll)

        .def("scale",py::overload_cast<float>(&vera::Model::scale), py::arg("s"))
        .def("scale",py::overload_cast<float,float,float>(&vera::Model::scale), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("truck",&vera::Model::truck, py::arg("_amount"))
        .def("boom",&vera::Model::boom, py::arg("_amount"))
        .def("dolly",&vera::Model::dolly, py::arg("_amount"))
        .def("translate",py::overload_cast<float,float,float>(&vera::Model::translate), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("rotate",py::overload_cast<float,float,float,float>(&vera::Model::rotate), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
        .def("apply",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Model::apply), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        .def("reset",&vera::Model::reset)

        .def("loaded",&vera::Model::loaded)
        .def("addDefine",&vera::Model::addDefine, py::arg("_define"), py::arg("_value"))
        .def("delDefine",&vera::Model::delDefine, py::arg("_define"))
        .def("clear",&vera::Model::clear)
        .def("setGeom",&vera::Model::setGeom, py::arg("_mesh"))
        .def("setName",&vera::Model::setName, py::arg("_str"))
        .def("setShader",&vera::Model::setShader, py::arg("_fragStr"), py::arg("_vertStr"))
        .def("setBufferShader",&vera::Model::setBufferShader, py::arg("_bufferName"), py::arg("_fragStr"), py::arg("_vertStr"))
        .def("getName",&vera::Model::getName)
        .def("getArea",&vera::Model::getArea)
        .def("printDefines",&vera::Model::printDefines)
        .def("printVboInfo",&vera::Model::printVboInfo)
        .def_readwrite("mesh", &vera::Model::mesh);
    ;

    py::enum_<vera::LightType>(m, "LightType")
        .value("LIGHT_DIRECTIONAL", vera::LIGHT_DIRECTIONAL)
        .value("LIGHT_POINT", vera::LIGHT_POINT)
        .value("LIGHT_SPOT", vera::LIGHT_SPOT)
        .export_values();
    ;

    py::class_<vera::Light>(m, "Light")
    .def(py::init<>())
        .def("setPosition",py::overload_cast<float,float,float>(&vera::Light::setPosition), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setOrientation",py::overload_cast<float,float,float>(&vera::Light::setOrientation), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("setOrientation",py::overload_cast<float,float,float,float>(&vera::Light::setOrientation), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
        .def("setOrientation",py::overload_cast<float,float,float, float,float,float, float,float,float>(&vera::Light::setOrientation), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("x3"), py::arg("y3"), py::arg("z3") )
        .def("setOrientation",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Light::setOrientation), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )

        .def("getPitch",&vera::Light::getPitch)
        .def("getHeading",&vera::Light::getHeading)
        .def("getRoll",&vera::Light::getRoll)

        .def("truck",&vera::Light::truck, py::arg("_amount"))
        .def("boom",&vera::Light::boom, py::arg("_amount"))
        .def("dolly",&vera::Light::dolly, py::arg("_amount"))
        .def("translate",py::overload_cast<float,float,float>(&vera::Light::translate), py::arg("x"), py::arg("y"), py::arg("z"))
        .def("rotate",py::overload_cast<float,float,float,float>(&vera::Light::rotate), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
        .def("apply",py::overload_cast<float,float,float,float, float,float,float,float, float,float,float,float, float,float,float,float>(&vera::Light::apply), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        .def("reset",&vera::Light::reset)

        .def("setType", &vera::Light::setType, py::arg("_type"))
        .def("setColor",&vera::Light::setColor, py::arg("_r"), py::arg("_g"), py::arg("_b"))
        .def("setDirection",&vera::Light::setDirection, py::arg("_x"), py::arg("_y"), py::arg("_z"))
        .def("setIntensity",&vera::Light::setIntensity, py::arg("_intensity"))
        .def("setFallOff",&vera::Light::setFallOff, py::arg("_falloff"))
    ;

    py::enum_<ShaderType>(m, "ShaderType")
        .value("FRAGMENT", FRAGMENT)
        .value("VERTEX", VERTEX)
        .export_values();
    ;

    py::class_<Uniforms>(m, "Uniforms")
        .def(py::init<>())

        .def("load",&Uniforms::load, py::arg("_name"), py::arg("_verbose"))
        .def("update", &Uniforms::update)

        .def("addTexture", py::overload_cast<const std::string&,const std::string&,bool,bool>(&Uniforms::addTexture), py::arg("_name"), py::arg("_path"), py::arg("_flip"), py::arg("_verbose"))
        // .def("addTexture", py::overload_cast<std::string,vera::Image,bool,bool>(&Uniforms::addTexture), py::arg("_name"), py::arg("_path"), py::arg("_flip"), py::arg("_verbose"))
        .def("addBumpTexture", &Uniforms::addBumpTexture, py::arg("_name"), py::arg("_path"), py::arg("_flip"), py::arg("_verbose"))
        .def("printTextures", &Uniforms::printTextures)
        .def("clearTextures", &Uniforms::clearTextures)

        .def("addCubemap", &Uniforms::addCubemap, py::arg("_name"), py::arg("_filename"), py::arg("_verbose"))
        .def("clearCubemaps", &Uniforms::clearCubemaps)
        .def("printCubemaps", &Uniforms::printCubemaps)


        .def("setSunPosition",py::overload_cast<float,float,float>(&Uniforms::setSunPosition), py::arg("_az"), py::arg("_elev"), py::arg("_distance"))
        .def("setSkyTurbidity", &Uniforms::setSkyTurbidity, py::arg("_turbidity"))
        // .def("setGroundAlbedo", &Uniforms::setGroundAlbedo, py::arg("_turbidity"))
        .def("getSunAzimuth", &Uniforms::getSunAzimuth)
        .def("getSunElevation", &Uniforms::getSunElevation)
        .def("getSkyTurbidity", &Uniforms::getSkyTurbidity)
        .def("getGroundAlbedo", &Uniforms::getGroundAlbedo)

        .def("clear", &Uniforms::clear)
        .def("flagChange", &Uniforms::flagChange)
        .def("unflagChange", &Uniforms::unflagChange)
        .def("haveChange", &Uniforms::haveChange)

        .def("addDefine",&Uniforms::addDefine, py::arg("_define"), py::arg("_value"))
        .def("delDefine",&Uniforms::delDefine, py::arg("_define"))
        .def("printDefines",&Uniforms::printDefines)

        .def("printBuffers",&Uniforms::printBuffers)
        .def("clearBuffers",&Uniforms::clearBuffers)

        .def("set",py::overload_cast<const std::string&,float>(&Uniforms::set), py::arg("_name"), py::arg("_value"))
        .def("set",py::overload_cast<const std::string&,float,float>(&Uniforms::set), py::arg("_name"), py::arg("_x"), py::arg("_y"))
        .def("set",py::overload_cast<const std::string&,float,float,float>(&Uniforms::set), py::arg("_name"), py::arg("_x"), py::arg("_y"), py::arg("_z"))
        .def("set",py::overload_cast<const std::string&,float,float,float,float>(&Uniforms::set), py::arg("_name"), py::arg("_x"), py::arg("_y"), py::arg("_z"), py::arg("_w"))
        .def("checkUniforms",&Uniforms::checkUniforms, py::arg("_vert_src"), py::arg("_frag_src"))
        .def("parseLine",&Uniforms::parseLine, py::arg("_line"))
        .def("clearUniforms",&Uniforms::clearUniforms)
        .def("printAvailableUniforms",&Uniforms::printAvailableUniforms, py::arg("_non_active"))
        .def("printDefinedUniforms",&Uniforms::printDefinedUniforms, py::arg("_csv"))

        .def("addCameraPath",&Uniforms::addCameraPath, py::arg("_name"))
    ;

    py::class_<Engine>(m, "Engine")
        .def(py::init<>())

        .def("loadMesh",&Engine::loadMesh, py::arg("_name"), py::arg("_mesh"))
        .def("loadImage",&Engine::loadImage, py::arg("_name"), py::arg("_path"), py::arg("_flip"))
        .def("loadShaders", &Engine::loadShaders)

        .def("setCamera", &Engine::setCamera, py::arg("_cam"))
        .def("setSun", &Engine::setSun, py::arg("_light"))
        .def("setSunPosition", &Engine::setSunPosition, py::arg("_az"), py::arg("_elev"), py::arg("_distance"))
        .def("setSource",&Engine::setSource, py::arg("_type"), py::arg("_source"))
        .def("setFrame",&Engine::setFrame, py::arg("_frame"))
        .def("setMeshTransformMatrix",&Engine::setMeshTransformMatrix, py::arg("_name"), py::arg("x1"), py::arg("y1"), py::arg("z1"), py::arg("w1"), py::arg("x2"), py::arg("y2"), py::arg("z2"), py::arg("w2"), py::arg("x3"), py::arg("y3"), py::arg("z3"), py::arg("w3"), py::arg("x4"), py::arg("y4"), py::arg("z4"), py::arg("w4") )
        
        .def("getSource",&Engine::getSource, py::arg("_type"))
        .def("getShowTextures", &Engine::getShowTextures)
        .def("getShowPasses", &Engine::getShowPasses)

        .def("printDefines", &Engine::printDefines)
        .def("printBuffers", &Engine::printBuffers)
        .def("printTextures", &Engine::printTextures)
        .def("printCubemaps", &Engine::printCubemaps)
        .def("printLights", &Engine::printLights)
        .def("printMaterials", &Engine::printMaterials)
        .def("printModels", &Engine::printModels)
        .def("printShaders", &Engine::printShaders)


        .def("clearModels", &Engine::clearModels)

        .def("showTextures",&Engine::showTextures, py::arg("_value"))
        .def("haveTexture",&Engine::haveTexture, py::arg("_name"))
        .def("addTexture",&Engine::addTexture, py::arg("_name"), py::arg("_width"), py::arg("_height"), py::arg("_pixels"))

        .def("showPasses",&Engine::showPasses, py::arg("_value"))
        .def("resize",&Engine::resize, py::arg("width"), py::arg("height"))

        .def("draw", &Engine::draw)
        
        .def_readwrite("include_folders", &Engine::include_folders)
        .def_readwrite("screenshotFile", &Engine::screenshotFile)

        .def_readwrite("frag_index", &Engine::frag_index)
        .def_readwrite("vert_index", &Engine::vert_index)
        .def_readwrite("geom_index", &Engine::geom_index)
        .def_readwrite("help", &Engine::help)
        .def_readwrite("fxaa", &Engine::fxaa)
    ;
}
