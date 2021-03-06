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

#ifndef __OPENSPACE_MODULE_IMGUI___GUI___H__
#define __OPENSPACE_MODULE_IMGUI___GUI___H__

#include <modules/imgui/include/guicomponent.h>
#include <modules/imgui/include/guifilepathcomponent.h>
#include <modules/imgui/include/guiglobebrowsingcomponent.h>
#include <modules/imgui/include/guihelpcomponent.h>
#include <modules/imgui/include/guiiswacomponent.h>
#include <modules/imgui/include/guimissioncomponent.h>
#include <modules/imgui/include/guiparallelcomponent.h>
#include <modules/imgui/include/guiperformancecomponent.h>
#include <modules/imgui/include/guipropertycomponent.h>
#include <modules/imgui/include/guispacetimecomponent.h>

#include <openspace/properties/property.h>
#include <openspace/scripting/scriptengine.h>
#include <openspace/util/keys.h>
#include <openspace/util/mouse.h>

namespace openspace::gui {

class GUI : public GuiComponent {
public:
    GUI();

    void initialize() override;
    void deinitialize() override;

    void initializeGL() override;
    void deinitializeGL() override;

    bool mouseButtonCallback(MouseButton button, MouseAction action);
    bool mouseWheelCallback(double position);
    bool keyCallback(Key key, KeyModifier modifier, KeyAction action);
    bool charCallback(unsigned int character, KeyModifier modifier);

    void startFrame(float deltaTime, const glm::vec2& windowSize,
        const glm::vec2& dpiScaling, const glm::vec2& mousePos, uint32_t mouseButtons);
    void endFrame();

    void render() override;

//protected:
    GuiHelpComponent _help;
    GuiFilePathComponent _filePath;
#ifdef GLOBEBROWSING_USE_GDAL
    GuiGlobeBrowsingComponent _globeBrowsing;
#endif //  GLOBEBROWSING_USE_GDAL
    GuiPerformanceComponent _performance;
    GuiPropertyComponent _globalProperty;
    GuiPropertyComponent _property;
    GuiPropertyComponent _screenSpaceProperty;
    GuiPropertyComponent _virtualProperty;
    GuiSpaceTimeComponent _spaceTime;
    GuiMissionComponent _mission;
#ifdef OPENSPACE_MODULE_ISWA_ENABLED
    GuiIswaComponent _iswa;
#endif // OPENSPACE_MODULE_ISWA_ENABLED
    GuiParallelComponent _parallel;
    GuiPropertyComponent _featuredProperties;

    bool _showInternals;

private:
    void renderAndUpdatePropertyVisibility();

    properties::Property::Visibility _currentVisibility;

};

void CaptionText(const char* text);

} // namespace openspace::gui

#endif // __OPENSPACE_MODULE_IMGUI___GUI___H__
