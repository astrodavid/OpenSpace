/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2017                                                               *
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

#ifndef __OPENSPACE_CORE___INTERACTIONHANDLER___H__
#define __OPENSPACE_CORE___INTERACTIONHANDLER___H__

#include <openspace/interaction/keyboardmousestate.h>
#include <openspace/documentation/documentationgenerator.h>
#include <openspace/properties/propertyowner.h>

#include <openspace/interaction/interactionmode.h>
#include <openspace/network/parallelconnection.h>
#include <openspace/properties/stringproperty.h>
#include <openspace/properties/scalar/boolproperty.h>
#include <openspace/properties/scalar/floatproperty.h>
#include <openspace/util/mouse.h>
#include <openspace/util/keys.h>

#include <ghoul/misc/boolean.h>

#include <list>

#include <mutex>

namespace openspace {

class Camera;
class SceneGraphNode;

namespace interaction {

class InteractionHandler : public properties::PropertyOwner, public DocumentationGenerator
{
public:
    InteractionHandler();
    ~InteractionHandler();

    void initialize();
    void deinitialize();

    // Mutators
    void setFocusNode(SceneGraphNode* node);
    void setCamera(Camera* camera);
    void resetCameraDirection();

    // Interaction mode setters
    void setCameraStateFromDictionary(const ghoul::Dictionary& cameraDict);
    InteractionMode* interactionMode();
    
    void goToChunk(int x, int y, int level);
    void goToGeo(double latitude, double longitude);
    
    void addKeyframe(double timestamp, KeyframeInteractionMode::CameraPose pose);
    void removeKeyframesAfter(double timestamp);
    void clearKeyframes();
    size_t nKeyframes() const;
    const std::vector<datamessagestructures::CameraKeyframe>& keyframes() const;

    void lockControls();
    void unlockControls();

    void updateCamera(double deltaTime);

    // Accessors
    ghoul::Dictionary getCameraStateDictionary();
    SceneGraphNode* focusNode() const;
    glm::dvec3 focusNodeToCameraVector() const;
    glm::quat focusNodeToCameraRotation() const;
    Camera* camera() const;

    /**
    * Returns the Lua library that contains all Lua functions available to affect the
    * interaction. The functions contained are
    * - openspace::luascriptfunctions::setOrigin
    * \return The Lua library that contains all Lua functions available to affect the
    * interaction
    */
    static scripting::LuaLibrary luaLibrary();

    void saveCameraStateToFile(const std::string& filepath);
    void restoreCameraStateFromFile(const std::string& filepath);

private:
    using Synchronized = ghoul::Boolean;

    struct KeyInformation {
        std::string command;
        Synchronized synchronization;
        std::string documentation;
    };
    
    std::string generateJson() const override;

    void setInteractionMode(InteractionMode* interactionMode);

    bool _cameraUpdatedFromScript = false;

    std::multimap<KeyWithModifier, KeyInformation> _keyLua;

    KeyboardMouseState* _inputState;
    Camera* _camera;

    InteractionMode* _currentInteractionMode;

    std::shared_ptr<OrbitalInteractionMode::MouseStates> _mouseStates;

    std::unique_ptr<OrbitalInteractionMode> _orbitalInteractionMode;
    std::unique_ptr<GlobeBrowsingInteractionMode> _globeBrowsingInteractionMode;
    std::unique_ptr<KeyframeInteractionMode> _keyframeInteractionMode;

    // Properties
    properties::StringProperty _origin;
    properties::OptionProperty _interactionModeOption;
    
    properties::BoolProperty _rotationalFriction;
    properties::BoolProperty _horizontalFriction;
    properties::BoolProperty _verticalFriction;

    properties::FloatProperty _sensitivity;
    properties::FloatProperty _rapidness;
};

} // namespace interaction
} // namespace openspace

#endif // __OPENSPACE_CORE___INTERACTIONHANDLER___H__
