/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2016                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <modules/newhorizons/rendering/renderableplanetprojection.h>

#include <modules/base/rendering/planetgeometry.h>
#include <modules/newhorizons/util/hongkangparser.h>
#include <modules/newhorizons/util/labelparser.h>
#include <modules/newhorizons/util/sequenceparser.h>

#include <openspace/rendering/renderengine.h>
#include <openspace/scene/scenegraphnode.h>
#include <openspace/util/factorymanager.h>
#include <openspace/util/time.h>

#include <ghoul/filesystem/filesystem.h>
#include <ghoul/io/texture/texturereader.h>
#include <ghoul/opengl/textureconversion.h>
#include <ghoul/opengl/textureunit.h>

namespace {
    const std::string _loggerCat = "RenderablePlanetProjection";
    const std::string keyProjObserver         = "Projection.Observer";
    const std::string keyProjTarget           = "Projection.Target";
    const std::string keyProjAberration       = "Projection.Aberration";
    const std::string keyInstrument           = "Instrument.Name";
    const std::string keyInstrumentFovy       = "Instrument.Fovy";
    const std::string keyInstrumentAspect     = "Instrument.Aspect";
    const std::string keyInstrumentNear       = "Instrument.Near";
    const std::string keyInstrumentFar        = "Instrument.Far";
    const std::string keySequenceDir          = "Projection.Sequence";
    const std::string keySequenceType         = "Projection.SequenceType";
    const std::string keyPotentialTargets     = "PotentialTargets";
    const std::string keyTranslation          = "DataInputTranslation";



    const std::string keyFrame = "Frame";
    const std::string keyGeometry = "Geometry";
    const std::string keyShading = "PerformShading";
    const std::string keyBody = "Body";
    const std::string _mainFrame = "GALACTIC";
    const std::string sequenceTypeImage = "image-sequence";
    const std::string sequenceTypePlaybook = "playbook";
    const std::string sequenceTypeHybrid = "hybrid";
}

namespace openspace {

RenderablePlanetProjection::RenderablePlanetProjection(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary)
    , _colorTexturePath("planetTexture", "RGB Texture")
    , _heightMapTexturePath("heightMap", "Heightmap Texture")
    , _rotation("rotation", "Rotation", 0, 0, 360)
    , _heightExaggeration("heightExaggeration", "Height Exaggeration", 1.f, 0.f, 100.f)
    , _programObject(nullptr)
    , _fboProgramObject(nullptr)
    , _baseTexture(nullptr)
    , _heightMapTexture(nullptr)
    , _capture(false)
{
    std::string name;
    bool success = dictionary.getValue(SceneGraphNode::KeyName, name);
    ghoul_assert(success, "");

    ghoul::Dictionary geometryDictionary;
    success = dictionary.getValue(
        keyGeometry, geometryDictionary);
    if (success) {
        geometryDictionary.setValue(SceneGraphNode::KeyName, name);
        using planetgeometry::PlanetGeometry;
        _geometry = std::unique_ptr<PlanetGeometry>(
            PlanetGeometry::createFromDictionary(geometryDictionary)
        );
    }

    dictionary.getValue(keyFrame, _frame);
    dictionary.getValue(keyBody, _target);
    if (_target != "")
        setBody(_target);

    bool b1 = dictionary.getValue(keyInstrument, _instrumentID);
    bool b2 = dictionary.getValue(keyProjObserver, _projectorID);
    bool b3 = dictionary.getValue(keyProjTarget, _projecteeID);
    std::string a = "NONE";
    bool b4 = dictionary.getValue(keyProjAberration, a);
    _aberration = SpiceManager::AberrationCorrection(a);
    bool b5 = dictionary.getValue(keyInstrumentFovy, _fovy);        
    bool b6 = dictionary.getValue(keyInstrumentAspect, _aspectRatio); 
    bool b7 = dictionary.getValue(keyInstrumentNear, _nearPlane);
    bool b8 = dictionary.getValue(keyInstrumentFar, _farPlane);

    // @TODO copy-n-paste from renderablefov ---abock
    ghoul::Dictionary potentialTargets;
    success = dictionary.getValue(keyPotentialTargets, potentialTargets);
    ghoul_assert(success, "");

    _potentialTargets.resize(potentialTargets.size());
    for (int i = 0; i < potentialTargets.size(); ++i) {
        std::string target;
        potentialTargets.getValue(std::to_string(i + 1), target);
        _potentialTargets[i] = target;
    }

    // TODO: textures need to be replaced by a good system similar to the geometry as soon
    // as the requirements are fixed (ab)
    std::string texturePath = "";
    success = dictionary.getValue("Textures.Color", texturePath);
    if (success){
        _colorTexturePath = absPath(texturePath); 
    }

    std::string heightMapPath = "";
    success = dictionary.getValue("Textures.Height", heightMapPath);
    if (success)
        _heightMapTexturePath = absPath(heightMapPath);

    addPropertySubOwner(_geometry.get());
    addProperty(_performProjection);
    addProperty(_clearAllProjections);


    addProperty(_colorTexturePath);
    _colorTexturePath.onChange(std::bind(&RenderablePlanetProjection::loadTextures, this));

    addProperty(_heightMapTexturePath);
    _heightMapTexturePath.onChange(std::bind(&RenderablePlanetProjection::loadTextures, this));

    addProperty(_projectionFading);
    addProperty(_heightExaggeration);

    SequenceParser* parser;

   // std::string sequenceSource;
    bool _foundSequence = dictionary.getValue(keySequenceDir, _sequenceSource);
    if (_foundSequence) {
        _sequenceSource = absPath(_sequenceSource);

        _foundSequence = dictionary.getValue(keySequenceType, _sequenceType);
        //Important: client must define translation-list in mod file IFF playbook
        if (dictionary.hasKey(keyTranslation)){
            ghoul::Dictionary translationDictionary;
            //get translation dictionary
            dictionary.getValue(keyTranslation, translationDictionary);

            if (_sequenceType == sequenceTypePlaybook) {
                parser = new HongKangParser(name,
                                            _sequenceSource,
                                            _projectorID,
                                            translationDictionary,
                                            _potentialTargets);
                openspace::ImageSequencer::ref().runSequenceParser(parser);
            }
            else if (_sequenceType == sequenceTypeImage) {
                parser = new LabelParser(name,
                                         _sequenceSource,
                                         translationDictionary);
                openspace::ImageSequencer::ref().runSequenceParser(parser);
            }
            else if (_sequenceType == sequenceTypeHybrid) {
                //first read labels
                parser = new LabelParser(name,
                                         _sequenceSource,
                                         translationDictionary);
                openspace::ImageSequencer::ref().runSequenceParser(parser);

                std::string _eventFile;
                bool foundEventFile = dictionary.getValue("Projection.EventFile", _eventFile);
                if (foundEventFile){
                    //then read playbook
                    _eventFile = absPath(_eventFile);
                    parser = new HongKangParser(name,
                                                _eventFile,
                                                _projectorID,
                                                translationDictionary,
                                                _potentialTargets);
                    openspace::ImageSequencer::ref().runSequenceParser(parser);
                }
                else{
                    LWARNING("No eventfile has been provided, please check modfiles");
                }
            }
        }
        else{
            LWARNING("No playbook translation provided, please make sure all spice calls match playbook!");
        }
    }
}

RenderablePlanetProjection::~RenderablePlanetProjection() {}

bool RenderablePlanetProjection::initialize() {
    bool completeSuccess = true;
    if (_programObject == nullptr) {
        // projection program

        RenderEngine& renderEngine = OsEng.renderEngine();
        _programObject = renderEngine.buildRenderProgram("projectiveProgram",
            "${MODULE_NEWHORIZONS}/shaders/projectiveTexture_vs.glsl",
            "${MODULE_NEWHORIZONS}/shaders/projectiveTexture_fs.glsl"
        );

        if (!_programObject)
            return false;
    }

    _fboProgramObject = ghoul::opengl::ProgramObject::Build("fboPassProgram",
        "${MODULE_NEWHORIZONS}/shaders/fboPass_vs.glsl",
        "${MODULE_NEWHORIZONS}/shaders/fboPass_fs.glsl"
    );

    loadTextures();
    completeSuccess &= (_baseTexture != nullptr);
    completeSuccess &= (_projectionTexture != nullptr);
    completeSuccess &= _geometry->initialize(this);

    if (completeSuccess) {
        completeSuccess &= auxiliaryRendertarget();
        // SCREEN-QUAD 
        const GLfloat size = 1.f;
        const GLfloat w = 1.f;
        const GLfloat vertex_data[] = {
            -size, -size, 0.f, w, 0.f, 0.f,
            size, size, 0.f, w, 1.f, 1.f,
            -size, size, 0.f, w, 0.f, 1.f,
            -size, -size, 0.f, w, 0.f, 0.f,
            size, -size, 0.f, w, 1.f, 0.f,
            size, size, 0.f, w, 1.f, 1.f,
        };

        glGenVertexArrays(1, &_quad);
        glBindVertexArray(_quad);
        glGenBuffers(1, &_vertexPositionBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, _vertexPositionBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, reinterpret_cast<void*>(sizeof(GLfloat) * 4));

        glBindVertexArray(0);
    }

    return completeSuccess;
}

bool RenderablePlanetProjection::deinitialize() {
    _baseTexture = nullptr;
    _geometry = nullptr;

    RenderEngine& renderEngine = OsEng.renderEngine();
    if (_programObject) {
        renderEngine.removeRenderProgram(_programObject);
        _programObject = nullptr;
    }

    _fboProgramObject = nullptr;

    return true;
}
bool RenderablePlanetProjection::isReady() const {
    return _geometry && _programObject && _baseTexture && _projectionTexture;
}

void RenderablePlanetProjection::imageProjectGPU(std::unique_ptr<ghoul::opengl::Texture> projectionTexture) {
    // keep handle to the current bound FBO
    GLint defaultFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFBO);

    GLint m_viewport[4];
    glGetIntegerv(GL_VIEWPORT, m_viewport);
    //counter = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, _fboID);

    glViewport(0, 0, static_cast<GLsizei>(_projectionTexture->width()), static_cast<GLsizei>(_projectionTexture->height()));
    _fboProgramObject->activate();

    ghoul::opengl::TextureUnit unitFbo;
    unitFbo.activate();
    projectionTexture->bind();
    _fboProgramObject->setUniform("projectionTexture", unitFbo);
        
    _fboProgramObject->setUniform("ProjectorMatrix", _projectorMatrix);
    _fboProgramObject->setUniform("ModelTransform" , _transform);
    _fboProgramObject->setUniform("_scaling"       , _camScaling);
    _fboProgramObject->setUniform("boresight"      , _boresight);

    if (_geometry->hasProperty("radius")){ 
        ghoul::any r = _geometry->property("radius")->get();
        if (glm::vec4* radius = ghoul::any_cast<glm::vec4>(&r)){
            _fboProgramObject->setUniform("_radius", radius);
        }
    }else{
        LERROR("Geometry object needs to provide radius");
    }
    if (_geometry->hasProperty("segments")){
        ghoul::any s = _geometry->property("segments")->get();
        if (int* segments = ghoul::any_cast<int>(&s)){
            _fboProgramObject->setUniform("_segments", segments[0]);
        }
    }else{
        LERROR("Geometry object needs to provide segment count");
    }

    glBindVertexArray(_quad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    _fboProgramObject->deactivate();

    //bind back to default
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
    glViewport(m_viewport[0], m_viewport[1],
                m_viewport[2], m_viewport[3]);
}

void RenderablePlanetProjection::attitudeParameters(double time) {
    // precomputations for shader
    _stateMatrix = SpiceManager::ref().positionTransformMatrix(_frame, _mainFrame, time);
    _instrumentMatrix = SpiceManager::ref().positionTransformMatrix(_instrumentID, _mainFrame, time);

    _transform = glm::mat4(1);
    //90 deg rotation w.r.t spice req. 
    glm::mat4 rot = glm::rotate(_transform, static_cast<float>(M_PI_2), glm::vec3(1, 0, 0));
    glm::mat4 roty = glm::rotate(_transform, static_cast<float>(M_PI_2), glm::vec3(0, -1, 0));
    glm::mat4 rotProp = glm::rotate(_transform, static_cast<float>(glm::radians(static_cast<float>(_rotation))), glm::vec3(0, 1, 0));

    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 3; j++){
            _transform[i][j] = static_cast<float>(_stateMatrix[i][j]);
        }
    }
    _transform = _transform * rot * roty * rotProp;

    glm::dvec3 bs;
    try {
        SpiceManager::FieldOfViewResult res = SpiceManager::ref().fieldOfView(_instrumentID);
        bs = std::move(res.boresightVector);
    }
    catch (const SpiceManager::SpiceException& e) {
        LERRORC(e.component, e.what());
        return;
    }

    double lightTime;
    glm::dvec3 p = SpiceManager::ref().targetPosition(_projectorID, _projecteeID, _mainFrame, _aberration, time, lightTime);
    psc position = PowerScaledCoordinate::CreatePowerScaledCoordinate(p.x, p.y, p.z);
   
    //change to KM and add psc camera scaling. 
    position[3] += (3 + _camScaling[1]);
    //position[3] += 3;
    glm::vec3 cpos = position.vec3();

    _projectorMatrix = computeProjectorMatrix(cpos, bs, _up, _instrumentMatrix, 
                                              _fovy, _aspectRatio, _nearPlane, _farPlane, _boresight);
}

void RenderablePlanetProjection::render(const RenderData& data) {
    if (!_programObject)
        return;
    
    if (_clearAllProjections)
        clearAllProjections();

    _camScaling = data.camera.scaling();
    _up = data.camera.lookUpVector();

    if (_capture && _performProjection) {
        for (const Image& img : _imageTimes) {
            RenderablePlanetProjection::attitudeParameters(img.startTime);
            imageProjectGPU(loadProjectionTexture(img.path));
        }
        _capture = false;
    }
    attitudeParameters(_time);
    _imageTimes.clear();

    double  lt;
    glm::dvec3 p =
        SpiceManager::ref().targetPosition("SUN", _projecteeID, "GALACTIC", {}, _time, lt);
    psc sun_pos = PowerScaledCoordinate::CreatePowerScaledCoordinate(p.x, p.y, p.z);

    // Main renderpass
    _programObject->activate();
    _programObject->setUniform("sun_pos", sun_pos.vec3());
    _programObject->setUniform("ViewProjection" ,  data.camera.viewProjectionMatrix());
    _programObject->setUniform("ModelTransform" , _transform);

    _programObject->setUniform("_hasHeightMap", _heightMapTexture != nullptr);
    _programObject->setUniform("_heightExaggeration", _heightExaggeration);
    _programObject->setUniform("_projectionFading", _projectionFading);

    setPscUniforms(*_programObject.get(), data.camera, data.position);
    
    ghoul::opengl::TextureUnit unit[3];
    unit[0].activate();
    _baseTexture->bind();
    _programObject->setUniform("baseTexture", unit[0]);

    unit[1].activate();
    _projectionTexture->bind();
    _programObject->setUniform("projectionTexture", unit[1]);

    if (_heightMapTexture) {
        unit[2].activate();
        _heightMapTexture->bind();
        _programObject->setUniform("heightTexture", unit[2]);
    }
    
    _geometry->render();
    _programObject->deactivate();
}

void RenderablePlanetProjection::update(const UpdateData& data) {
    _time = Time::ref().currentTime();
    _capture = false;

    if (openspace::ImageSequencer::ref().isReady() && _performProjection){
        openspace::ImageSequencer::ref().updateSequencer(_time);
        _capture = openspace::ImageSequencer::ref().getImagePaths(_imageTimes, _projecteeID, _instrumentID);
    }

    if (_fboProgramObject && _fboProgramObject->isDirty()) {
        _fboProgramObject->rebuildFromFile();
    }

    if (_programObject->isDirty())
        _programObject->rebuildFromFile();
}

void RenderablePlanetProjection::loadTextures() {
    using ghoul::opengl::Texture;
    _baseTexture = nullptr;
    if (_colorTexturePath.value() != "") {
        _baseTexture = ghoul::io::TextureReader::ref().loadTexture(_colorTexturePath);
        if (_baseTexture) {
            ghoul::opengl::convertTextureFormat(Texture::Format::RGB, *_baseTexture);
            _baseTexture->uploadTexture();
            _baseTexture->setFilter(Texture::FilterMode::Linear);
        }
    }

    generateProjectionLayerTexture();

    _heightMapTexture = nullptr;
    if (_heightMapTexturePath.value() != "") {
        _heightMapTexture = ghoul::io::TextureReader::ref().loadTexture(_heightMapTexturePath);
        if (_heightMapTexture) {
            ghoul::opengl::convertTextureFormat(Texture::Format::RGB, *_heightMapTexture);
            _heightMapTexture->uploadTexture();
            _heightMapTexture->setFilter(Texture::FilterMode::Linear);
        }
    }
}

}  // namespace openspace
