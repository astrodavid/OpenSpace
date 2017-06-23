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

#include <modules/fieldlinessequence/rendering/renderablefieldlinessequence.h>
#include <modules/fieldlinessequence/util/fieldlinessequencemanager.h>
#include <modules/fieldlinessequence/util/helperfunctions.h>

#include <openspace/engine/openspaceengine.h>
#include <openspace/rendering/renderable.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/rendering/raycastermanager.h>
#include <openspace/scene/scenegraphnode.h>
#include <openspace/util/time.h>
#include <openspace/util/updatestructures.h>

#include <ghoul/glm.h>
#include <ghoul/misc/assert.h>
#include <ghoul/opengl/ghoul_gl.h>
#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/textureunit.h>

#include <ccmc/Kameleon.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <thread>

#include <chrono>

namespace {
    const char* keyTracingMethod                = "TracingMethod";
    const char* keyTracingMethodPreTracedJson   = "PreTracedJson";
    const char* keyTracingMethodPreTracedBinary = "PreTracedBinary";
    const char* keyTracingMethodPreProcess      = "PreProcess";
    const char* keyTracingMethodLiveTrace       = "LiveTrace";

    const char* keyLoadBinariesAtRuntime        = "LoadBinariesAtRuntime";

    const char* keySourceFolder                 = "SourceFolder";
    const char* keyStartStateOffset             = "StartStateOffset";
    const char* keyStateStepSize                = "StateStepSize";
    const char* keyMaxNumStates                 = "MaxNumStates";

    const char* keySeedPointsInfo               = "SeedPointInfo";
    const char* keySeedPointsFile               = "File";

    const char* keyOutputLocationBinary         = "OutputLocationBinary";
    const char* keyOutputLocationJson           = "OutputLocationJson";

    const char* keyFieldlineMaxTraceSteps       = "MaximumTracingSteps";
    const char* keyTracingVariable              = "TracingVariable";
    const char* keyTracingStepLength            = "TracingStepLength";

    const char* keyExtraVariables               = "ExtraVariables";
    const char* keyExtraMagnitudeVariables      = "ExtraMagnitudeVariables";
    const char* keyExtraMinMax                  = "ExtraMinMax";
    const char* keyExtraMinMaxLimits            = "ExtraMinMaxLimits";
    const char* keyExtraColorTablePaths         = "ColorTablePaths";
    const char* keyExtraColorTableMinMaxs       = "ColorTableMinMax";

    const char* keyDefaultColor                 = "DefaultColor";

    const char* keyCartesianDomainLimits        = "CartesianDomainLimits";
    const char* keyRadialDomainLimits           = "RadialDomainLimits";

    // const char* keyVertexListFileType       = "FileType";
    // const char* keyVertexListFileTypeJson   = "Json";
    // const char* keyVertexListFileTypeBinary = "Binary";

    const char* keyVolume = "VectorVolume";
    const char* keyFieldlines = "Fieldlines";
    const char* keySeedPoints = "SeedPoints";

    const char* keyVolumeDirectory = "Directory";

    const char* keyFieldlineShouldMorph = "Morphing";
    const char* keyFieldlineResamples = "NumResamples";
    const char* keyFieldlineResampleOption = "ResamplingType";
    const char* keyFieldlineQuickMorphDistance = "QuickMorphDistance";

    const float R_E_TO_METER = 6371000.f; // Earth radius
    const float R_S_TO_METER = 695700000.f; // Sun radius
    const float A_U_TO_METER = 149597870700.f; // Astronomical Units
    // const char* keySeedPointsDirectory = "Directory"; // TODO: allow for varying seed points?

    enum colorMethod{ UNIFORM, QUANTITY_DEPENDENT, CLASSIFIED };
    enum TracingMethod{ PRE_PROCESS,
                        PRE_CALCULATED_JSON,
                        PRE_CALCULATED_BINARY,
                        LIVE_TRACE };

    #define fsManager (FieldlinesSequenceManager::ref())
}


namespace openspace {

RenderableFieldlinesSequence::RenderableFieldlinesSequence(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary),
      _isClampingColorValues("isClamping", "Clamp", true),
      _isMorphing("isMorphing", "Morphing", false),
      _show3DLines("show3DLines", "3D Lines", false),
      _showSeedPoints("showSeedPoints", "Show Seed Points", false),
      _showParticles("showParticles", "Show Particles", false),
      _useABlending("additiveBlending", "Additive Blending", false),
      _useNearestSampling("useNearestSampling", "Nearest Sampling", false),
      _usePointDrawing("togglePointDrawing", "Draw Points", false),
      _alphaFade("fieldlineAlpha", "Opacity Multiplier", 1.f, 0.f, 1.f),
      _lineWidth("fieldlineWidth", "Fieldline Width", 1.f, 0.01f, 10.f),
      _seedPointSize("seedPointSize", "Seed Point Size", 4.0, 0.0, 20.0),
      _fieldlineParticleSize("fieldlineParticleSize", "FL Particle Size", 0, 0, 1000),
      _modulusDivider("fieldlineParticleFrequency", "FL Particle Frequency (reversed)", 100, 1, 10000),
      _timeMultiplier("fieldlineParticleSpeed", "FL Particle Speed", 20, 1, 1000),
      _colorMethod("fieldlineColorMethod", "Color Method", properties::OptionProperty::DisplayType::Radio),
      _colorizingQuantity("fieldlineColorQuantity", "Quantity", properties::OptionProperty::DisplayType::Dropdown),
      _domainQuantity("fieldlineDomainQuantity", "Quantity", properties::OptionProperty::DisplayType::Dropdown),
      _colorGroup("Color"),
      _domainGroup("Domain Limits"),
      _particleGroup("Particles"),
      _seedGroup("Seed Points"),
      _transferFunctionPath("transferFunctionPath", "Transfer Function Path"),
      _transferFunctionMinVal("transferFunctionLimit1", "TF minimum", "0"),
      _transferFunctionMaxVal("transferFunctionLimit2", "TF maximum", "1"),
      _domainLimR("domainLimitsRadial", "Domain Limits Radial"),
      _domainLimX("domainLimitsX", "Domain Limits X"),
      _domainLimY("domainLimitsY", "Domain Limits Y"),
      _domainLimZ("domainLimitsZ", "Domain Limits Z"),
      _unitInterestRange("unitInterestRange", "Range Of Interest"),
      _fieldlineColor("fieldlineColor", "Fieldline Color", glm::vec4(0.7f,0.f,0.7f,0.75f),
                                                           glm::vec4(0.f),
                                                           glm::vec4(1.f)),
      _fieldlineParticleColor("fieldlineParticleColor", "FL Particle Color",
                                                           glm::vec4(1.f,0.f,1.f,0.5f),
                                                           glm::vec4(0.f),
                                                           glm::vec4(1.f)),
      _uniformSeedPointColor("seedPointColor", "SeedPoint Color",
                                                           glm::vec4(1.f,0.33f,0.f,0.85f),
                                                           glm::vec4(0.f),
                                                           glm::vec4(1.f)) {

    std::string name;
    dictionary.getValue(SceneGraphNode::KeyName, name);

    _loggerCat += " [" + name + "]";

    // TODO Check dictionary for main fields and set bools accordingly?
    _dictionary = dictionary;
}

bool RenderableFieldlinesSequence::isReady() const {
    return _program ? true : false;
}

bool RenderableFieldlinesSequence::initialize() {
    //--Extract common  info from LUA, applicable to all types of renderable fieldline sequences--//
    std::string sourceFileExt = "";
    // std::vector<std::string> validSourceFilePaths;
    TracingMethod tracingMethod;

    // Determine what type of tracing method to use.
    std::string tracingMethodString;
    if (!_dictionary.getValue(keyTracingMethod, tracingMethodString)) {
        LERROR("Renderable does not contain a key for '" << keyTracingMethod << "'");
        return false;
    } else {
        if (tracingMethodString == keyTracingMethodPreProcess) {
            sourceFileExt = "cdf";
            tracingMethod = PRE_PROCESS;
        } else if (tracingMethodString == keyTracingMethodPreTracedBinary) {
            sourceFileExt = "osfls";
            tracingMethod = PRE_CALCULATED_BINARY;
        } else if (tracingMethodString == keyTracingMethodPreTracedJson) {
            sourceFileExt = "json";
            tracingMethod = PRE_CALCULATED_JSON;
        } else if (tracingMethodString == keyTracingMethodLiveTrace) {
            sourceFileExt = "cdf";
            tracingMethod = LIVE_TRACE;
        } else {
            LERROR("\"" << tracingMethodString << "\" is not a recognised key!");
            return false;
        }
    }

    if (!getSourceFilesFromDictionary(sourceFileExt, _validSourceFilePaths)) {
        return false;
    }

    _numberOfStates = _validSourceFilePaths.size();
    LDEBUG("Found " << _numberOfStates << " valid files!");

    // TODO this makes no sense if binaries are loaded at runtime!
    _states.reserve(_numberOfStates);
    _startTimes.reserve(_numberOfStates);

    std::string outputJsonLoc;
    std::string outputBinaryLoc;
    // TODO: Use function that validate paths as well?
    bool saveJsonOutputs   = _dictionary.getValue(keyOutputLocationJson  , outputJsonLoc);
    bool saveBinaryOutputs = _dictionary.getValue(keyOutputLocationBinary, outputBinaryLoc);

    int numResamples;
    bool allowSeedPoints = false;

    switch (tracingMethod) {
        case PRE_PROCESS: {
            if (!getSeedPointsFromDictionary()) {
                return false;
            }

            allowSeedPoints = true;

            // Specify which quantity to trace
            std::string tracingVariable;
            if (!_dictionary.getValue(keyTracingVariable, tracingVariable)) {
                tracingVariable = "b"; //default: b = magnetic field.
                LWARNING(keyVolume << " isn't specifying a " <<
                         keyTracingVariable << ". Using default value: '" <<
                         tracingVariable << "' for magnetic field.");
            }

            int maxSteps = 1000; // Default value
            float f_maxSteps;
            if (!_dictionary.getValue(keyFieldlineMaxTraceSteps, f_maxSteps)) {
                LWARNING(keyFieldlines << " isn't specifying " << keyFieldlineMaxTraceSteps
                        << ". Using default value: " << maxSteps);
            } else {
                maxSteps = static_cast<int>(f_maxSteps);
            }

            float tracingStepLength = 0.2f;
            _dictionary.getValue(keyTracingStepLength, tracingStepLength);

            bool userWantsMorphing;
            if (!_dictionary.getValue(keyFieldlineShouldMorph, userWantsMorphing)) {
                _isMorphing = false; // Default value
                LWARNING(keyFieldlines << " isn't specifying " << keyFieldlineShouldMorph
                        << ". Using default: " << _isMorphing);
            } else {
                _isMorphing = userWantsMorphing;
            }

            int resamplingOption = 4; // Default
            numResamples = 2 * maxSteps + 3; // Default
            if (_isMorphing) {

                float f_resamplingOption;

                if(!_dictionary.getValue(keyFieldlineResampleOption, f_resamplingOption)) {
                    LWARNING(keyFieldlines << " isn't specifying " <<
                             keyFieldlineResampleOption << ". Default is set to " <<
                             resamplingOption);
                } else {
                    resamplingOption = static_cast<int>(f_resamplingOption);
                }

                float f_numResamples;

                if(_dictionary.getValue(keyFieldlineResamples, f_numResamples)) {
                    if (resamplingOption == 4) {
                        LWARNING(keyFieldlineResampleOption << " 4 does not allow a custom" <<
                                 " value for " << keyFieldlineResamples <<". Using (2 * " <<
                                 keyFieldlineMaxTraceSteps <<" + 3) = " << numResamples);
                    } else {
                        numResamples = static_cast<int>(f_numResamples);
                    }
                } else {
                    if (resamplingOption != 4) {
                        LWARNING(keyFieldlines << " isn't specifying " <<
                                 keyFieldlineResamples << ". Default is set to (2*" <<
                                 keyFieldlineMaxTraceSteps << "+3) = " << numResamples);
                    }
                }
            }

            std::string allExtraVars;
            std::string allExtraMagVars;
            _dictionary.getValue(keyExtraVariables,          allExtraVars);
            _dictionary.getValue(keyExtraMagnitudeVariables, allExtraMagVars);

            std::istringstream iss(allExtraVars);
            std::vector<std::string> extraFloatVars{std::istream_iterator<std::string>{iss},
                                                    std::istream_iterator<std::string>{}};

            iss.clear();
            iss.str(allExtraMagVars);
            std::vector<std::string> extraMagVars{std::istream_iterator<std::string>{iss},
                                                  std::istream_iterator<std::string>{}};

// //---------------------- HARDCODE DELUXEEE START!-------------------------------------//
    std::vector<std::string> sourcefiles;
    std::vector<std::string> tmpfiles;
    if (!fsManager.getAllFilePathsOfType(
            "/media/ccarlbau/ff854077-f4bf-4644-a94b-1fe2d8824c64/Oskar_Carlbaum_061617_SH_1/3D_CDF", ".cdf", tmpfiles)) {
            // "/media/ccarlbau/42a9eae3-e5e0-4d94-b439-f634d48b0d8c/Oskar_Carlbaum_060717_SH_4/3D_CDF", ".cdf", tmpfiles)) {
        LERROR("Failed to get valid "<< ".cdf" << " file paths from '"
                << "*pathToSourceFolder 061617_SH_1*" << "'" );
        return false;
    }
    sourcefiles.insert(sourcefiles.end(),tmpfiles.begin(),tmpfiles.end());
    // tmpfiles.clear();
    // if (!fsManager.getAllFilePathsOfType(
    //         "/media/ccarlbau/42a9eae3-e5e0-4d94-b439-f634d48b0d8c/Oskar_Carlbaum_060717_SH_3/3D_CDF", ".cdf", tmpfiles)) {
    //     LERROR("Failed to get valid "<< ".cdf" << " file paths from '"
    //             << "*pathToSourceFolder SH3*" << "'" );
    //     return false;
    // }
    // sourcefiles.insert(sourcefiles.end(),tmpfiles.begin(),tmpfiles.end());
    // tmpfiles.clear();
    // if (!fsManager.getAllFilePathsOfType(
    //         "/media/ccarlbau/42a9eae3-e5e0-4d94-b439-f634d48b0d8c/Oskar_Carlbaum_051717_SH_2/3D_CDF", ".cdf", tmpfiles)) {
    //     LERROR("Failed to get valid "<< ".cdf" << " file paths from '"
    //             << "*pathToSourceFolder SH2*" << "'" );
    //     return false;
    // }
    // sourcefiles.insert(sourcefiles.end(),tmpfiles.begin(),tmpfiles.end());


    std::string seeds_eq011_1 = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/equitorialslice_0.11AU_4deg_1.txt";
    std::string seeds_eq011_2 = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/equitorialslice_0.11AU_4deg_2.txt";
    std::string seeds_eq1_1 = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/equitorialslice_1AU_4deg_1.txt";
    std::string seeds_eq1_2 = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/equitorialslice_1AU_4deg_2.txt";

    std::string seeds_lat4_1 = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/enlil_1AU_lat4deg_4degIncrements_1.txt";
    std::string seeds_lat4_2 = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/enlil_1AU_lat4deg_4degIncrements_2.txt";
    std::string seeds_lat4_3 = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/enlil_011AU_lat4deg_4degIncrements_1.txt";
    std::string seeds_lat4_4 = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/enlil_011AU_lat4deg_4degIncrements_2.txt";

    std::vector<std::string> seedGroupFiles = { seeds_eq011_1,
                                                seeds_eq011_2,
                                                seeds_eq1_1,
                                                seeds_eq1_2,
                                                seeds_lat4_1,
                                                seeds_lat4_2,
                                                seeds_lat4_3,
                                                seeds_lat4_4
                                              };

    std::string blabla = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/new_dynamic/";
    std::vector<std::string> dynamic_seed_folders = {blabla + "earth/with_extra_seeds/",
                                                     blabla + "stereoa/with_extra_seeds/"};

    std::vector<std::vector<std::string>> dynamic_seed_files;
    std::vector<std::vector<std::vector<glm::vec3> > > dynamicSeeds;
    dynamicSeeds.resize(2);

    for (int j = 0 ; j < 2 ; j++ ) {
        std::vector<std::string> dsfTmp;
            if (!fsManager.getAllFilePathsOfType(dynamic_seed_folders[j], ".txt", dsfTmp)) {
            return false;
        }
        // dynamic_seed_files.push_back(dsfTmp);
        for (std::string str : dsfTmp) {
            std::vector<glm::vec3> tmpVec;
            if (!fsManager.getSeedPointsFromFile(str, tmpVec)) {
                LERROR("Failed to find seed points in'" << str << "'");
                return false;
            }
            dynamicSeeds[j].push_back(tmpVec);
        }
    }


    // std::string seeds_earth  = "${OPENSPACE_DATA}/scene/fieldlinessequence/seedpoints/earth_seeds_pre/";
    // std::vector<std::string> earthSeedFiles;
    // if (!fsManager.getAllFilePathsOfType(seeds_earth, ".txt", earthSeedFiles)) {
    //     return false;
    // }
    // std::vector<std::vector<glm::vec3>> earthSeeds;

    // for (int i = 0; i < earthSeedFiles.size(); ++i) {
    //     std::vector<glm::vec3> tmpVec;
    //     if (!fsManager.getSeedPointsFromFile(earthSeedFiles[i], tmpVec)) {
    //         LERROR("Failed to find seed points in'" << earthSeedFiles[i] << "'");
    //         return false;
    //     }
    //     earthSeeds.push_back(tmpVec);
    // }
    std::string binOut1  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/011AU_eq_1/";
    std::string binOut2  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/011AU_eq_2/";
    std::string binOut3  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/1AU_eq_1/";
    std::string binOut4  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/1AU_eq_2/";
    std::string binOut5  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/1AU_lat4_1/";
    std::string binOut6  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/1AU_lat4_2/";
    std::string binOut7  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/011AU_lat4_1/";
    std::string binOut8  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/011AU_lat4_2/";
    std::string binOut9  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/earth/";
    std::string binOut10  = "${OPENSPACE_DATA}/scene/fieldlinessequence/binary_enlil_2012_July/new_originals/stereoa/";

    std::vector<std::string> ssOutBins = {binOut1,
                                          binOut2,
                                          binOut3,
                                          binOut4,
                                          binOut5,
                                          binOut6,
                                          binOut7,
                                          binOut8,
                                          binOut9,
                                          binOut10};

    size_t numSeedGroups = seedGroupFiles.size();
    std::vector<std::vector<glm::vec3>> seedGroups;
    seedGroups.resize(numSeedGroups);

    // GET THE STATIC SEEDS
    for (size_t i = 0; i < numSeedGroups; ++i) {
        if (!fsManager.getSeedPointsFromFile(seedGroupFiles[i], seedGroups[i])) {
            LERROR("Failed to find seed points in'" << seedGroupFiles[i] << "'");
        }
        LERROR("DEBUG: Size of seedGroup" << i << seedGroups[i].size());
    }

    for (size_t i = 0; i < sourcefiles.size(); ++i) {
        std::unique_ptr<ccmc::Kameleon> kameleon =
                fsManager.createKameleonObject(sourcefiles[i]);
        if (kameleon == nullptr) {
            continue;
        }
        LDEBUG(sourcefiles[i] << " is now being traced.");
        // if ( (i > 235 && i < 235 + 92) || i == 347 || i == 348 || i == 346|| i > 457) {
            // TRACE ALL STATIC SEED GROUPS' STATE
            // if (i < 169) {
                for (size_t spf = 0 ; spf < numSeedGroups ; ++spf ) {
                    FieldlinesState fls(seedGroups[spf].size());
                    fsManager.getFieldlinesState(kameleon.get(),
                                                 tracingVariable,
                                                 seedGroups[spf],
                                                 tracingStepLength,
                                                 maxSteps,
                                                 _isMorphing,
                                                 numResamples,
                                                 resamplingOption,
                                                 extraFloatVars,
                                                 extraMagVars,
                                                 fls);
                    std::string filePath = fsManager.getAbsPath(ssOutBins[spf] +
                                   fsManager.timeToString(fls._triggerTime, true) + ".osfls");
                    fls.saveStateToBinaryFile(filePath);
                    LINFO("Saved state to " << filePath);
                }
            // }
        // }

        // TRACE FIELDLINES THROUGH EARTH
        for (int k = 0; k < 2 ; k++) {
            // if (i > 168 && k < 4 ) {continue;}
            FieldlinesState fls(dynamicSeeds[k][i].size());
            fsManager.getFieldlinesState(kameleon.get(),
                                         tracingVariable,
                                         dynamicSeeds[k][i],
                                         tracingStepLength,
                                         maxSteps,
                                         _isMorphing,
                                         numResamples,
                                         resamplingOption,
                                         extraFloatVars,
                                         extraMagVars,
                                         fls);
            std::string filePath = fsManager.getAbsPath(ssOutBins[seedGroupFiles.size()+k] +
                           fsManager.timeToString(fls._triggerTime, true) + ".osfls");
            fls.saveStateToBinaryFile(filePath);
            LINFO("Saved state to " << filePath);
        }

    // }

    // for (size_t i = 0; i < 100; ++i) {
    //     std::unique_ptr<ccmc::Kameleon> kameleon =
    //             fsManager.createKameleonObject(validSourceFilePaths[i]);
    //     if (kameleon == nullptr) {
    //         continue;
    //     }
    //     LDEBUG(validSourceFilePaths[i] << " is now being traced.");
    //     if (i > 21) {
    //         // Trace eq_plane_1
    //         FieldlinesState fls(seedGroups[0].size());
    //         fsManager.getFieldlinesState(kameleon.get(),
    //                                      tracingVariable,
    //                                      seedGroups[0],
    //                                      tracingStepLength,
    //                                      maxSteps,
    //                                      _isMorphing,
    //                                      numResamples,
    //                                      resamplingOption,
    //                                      extraFloatVars,
    //                                      extraMagVars,
    //                                      fls);
    //         std::string filePath = fsManager.getAbsPath(binOut1 +
    //                        fsManager.timeToString(fls._triggerTime, true) + ".osfls");
    //         fls.saveStateToBinaryFile(filePath);
    //     }
    //     // TRACE FIELDLINES THROUGH EARTH
    //     FieldlinesState fls(earthSeeds[i].size());
    //     fsManager.getFieldlinesState(kameleon.get(),
    //                                  tracingVariable,
    //                                  earthSeeds[i],
    //                                  tracingStepLength,
    //                                  maxSteps,
    //                                  _isMorphing,
    //                                  numResamples,
    //                                  resamplingOption,
    //                                  extraFloatVars,
    //                                  extraMagVars,
    //                                  fls);
    //     std::string filePath = fsManager.getAbsPath(binOut5 +
    //                    fsManager.timeToString(fls._triggerTime, true) + ".osfls");
    //     fls.saveStateToBinaryFile(filePath);
    //


    //         if (saveBinaryOutputs) {
    //             std::string filePath =
    //                     fsManager.getAbsPath(outputBinaryLoc +
    //                            fsManager.timeToString(_states[i]._triggerTime, true) +
    //                            ".osfls");
    //             _states[i].saveStateToBinaryFile(filePath);
    //             // filePath =
    //             //         fsManager.getAbsPath(outputBinaryLoc + "downsampled/" +
    //             //                fsManager.timeToString(_states[i]._triggerTime, true) +
    //             //                ".osfls");
    //             // _states[i].saveStateSubsetToBinaryFile(filePath, 4);

    //             // TODO REMOVE THIS OR ADD AS OPTION IN LUA MODFILE!
    //             // DELETE STATE TO CLEAR MEMORY FOR A PREPROCESS RUN
    //             _states.pop_back();
    //             // // add new empty state to not screw with for-loop calling _state[i]
    //             _states.push_back(FieldlinesState());
    //         }
    }
    return false;

//---------------------- HARDCODE DELUXEEE END!-------------------------------------//


            // TODO Only one loop.. Not one for each tracing method!!
            // for (size_t i = 0; i < _numberOfStates; ++i) {
            //     std::unique_ptr<ccmc::Kameleon> kameleon =
            //             fsManager.createKameleonObject(validSourceFilePaths[i]);
            //     if (kameleon == nullptr) {
            //         continue;
            //     }
            //     LDEBUG(validSourceFilePaths[i] << " is now being traced.");
            //     _states.push_back(FieldlinesState(_seedPoints.size()));
            //     fsManager.getFieldlinesState(
            //                                  // validSourceFilePaths[i],
            //                                  kameleon.get(),
            //                                  tracingVariable,
            //                                  _seedPoints,
            //                                  tracingStepLength,
            //                                  maxSteps,
            //                                  _isMorphing,
            //                                  numResamples,
            //                                  resamplingOption,
            //                                  extraFloatVars,
            //                                  extraMagVars,
            //                                  _states[i]);

            //     _startTimes.push_back(_states[i]._triggerTime);

            //     if (saveBinaryOutputs) {
            //         std::string filePath =
            //                 fsManager.getAbsPath(outputBinaryLoc +
            //                        fsManager.timeToString(_states[i]._triggerTime, true) +
            //                        ".osfls");
            //         _states[i].saveStateToBinaryFile(filePath);
            //         // filePath =
            //         //         fsManager.getAbsPath(outputBinaryLoc + "downsampled/" +
            //         //                fsManager.timeToString(_states[i]._triggerTime, true) +
            //         //                ".osfls");
            //         // _states[i].saveStateSubsetToBinaryFile(filePath, 4);

            //         // TODO REMOVE THIS OR ADD AS OPTION IN LUA MODFILE!
            //         // DELETE STATE TO CLEAR MEMORY FOR A PREPROCESS RUN
            //         _states.pop_back();
            //         // // add new empty state to not screw with for-loop calling _state[i]
            //         _states.push_back(FieldlinesState());
            //     }
            //     if (saveJsonOutputs) {
            //         std::string filePrefix = fsManager.timeToString(_states[i]._triggerTime, true);
            //         fsManager.saveFieldlinesStateAsJson(_states[i],
            //                                             outputJsonLoc,
            //                                             false,
            //                                             filePrefix,
            //                                             false);
            //     }
            // }
        } break;
        case PRE_CALCULATED_JSON: {
            allowSeedPoints = false;

            LERROR("TODO: allow Morphing for provided vertex lists!");
            _isMorphing = false;
            int resamplingOption = 0;   // TODO: implement morphing
            numResamples = 0;       // TODO: implement morphing

            for (size_t i = 0; i < _numberOfStates; ++i) {
                LDEBUG(_validSourceFilePaths[i] << " is now being processed.");
                FieldlinesState tmpState;
                _states.push_back(tmpState);
                fsManager.getFieldlinesState(_validSourceFilePaths[i],
                                             _isMorphing,
                                             numResamples,
                                             resamplingOption,
                                             _states[i]);
                {
                    // TODO: Delete this scope!!
                    _states[i]._extraVariables.pop_back();
                    _states[i]._extraVariableNames.pop_back();
                    _states[i].calculateTopologies(3.1f*R_E_TO_METER);
                }
                // TODO: Move elsewhere
                _startTimes.push_back(_states[i]._triggerTime);

                if (saveBinaryOutputs) {
                    std::string filePath =
                            fsManager.getAbsPath(outputBinaryLoc +
                                   fsManager.timeToString(_states[i]._triggerTime, true) +
                                   ".osfls");
                    LINFO("Saving file to: " << filePath);
                    _states[i].saveStateToBinaryFile(filePath);
                    // filePath =
                    //         fsManager.getAbsPath(outputBinaryLoc + "downsampled/" +
                    //                fsManager.timeToString(_states[i]._triggerTime, true) +
                    //                ".osfls");
                    // _states[i].saveStateSubsetToBinaryFile(filePath, 4);

                    // TODO REMOVE THIS OR ADD AS OPTION IN LUA MODFILE!
                    // DELETE STATE TO CLEAR MEMORY FOR A PREPROCESS RUN
                    // _states.pop_back();
                    // // add new empty state to not screw with for-loop calling _state[i]
                    // _states.push_back(FieldlinesState());
                }

                // std::string filePath = validSourceFilePaths[i];
                // auto lastPos = filePath.find_last_of("/\\");
                // filePath = filePath.substr(0,lastPos);
                // filePath += "/binary/" + fsManager.timeToString(tmpState._triggerTime, true) +
                //                        ".osfls";
                // tmpState.saveStateToBinaryFile(filePath);
            }
            // double tt = _states[19009]._triggerTime;
        } break;
        case PRE_CALCULATED_BINARY: {

            allowSeedPoints = false;

            if (!_dictionary.getValue(keyLoadBinariesAtRuntime, _loadBinariesAtRuntime)) {
                LWARNING("Modfile " << " isn't specifying " << keyLoadBinariesAtRuntime);
            }
            if (_loadBinariesAtRuntime) {
                LINFO("Binaries will be loaded during runtime.");

                const size_t filenameSize = 23; // number of  characters in filename (excluding '.osfls')
                const size_t extSize = 6; // size(".osfls")

                // for (size_t i = 0; i < _numberOfStates; ++i) {
                for (std::string fName : _validSourceFilePaths) {
                    const size_t ssize = fName.size();
                    // if (ssize != filenameSize + extSize) {
                    //     LWARNING("Unrecognised naming format of file: " << fName
                    //         << " Loading files at runtime requires each file to be named"
                    //         << " in the format 'YYYY-MM-DDTHH-MM-SS-XXX.osfls'. "
                    //         << "State will NOT be included");
                    //     // TODO: DELETE FILENAME FROM VALIDFILEPATHS AND REDUCE _NUMSTATES
                    //     continue;
                    // }
                    std::string tmpString = fName.substr(ssize - filenameSize - extSize, filenameSize-1);
                    tmpString.replace(13,1,":");
                    tmpString.replace(16,1,":");
                    tmpString.replace(19,1,".");
                    double tTime = Time::convertTime(tmpString);
                    _startTimes.push_back(tTime);
                }
                FieldlinesState tmpState;
                _states.push_back(tmpState);
                // Load first state, just for initialization purposes
                bool statuss = fsManager.getFieldlinesStateFromBinary(
                        _validSourceFilePaths[0], _states[0]);
                _drawingIndex = 0;
            } else {
                LINFO("Binaries will be loaded now (at startup) and stored in RAM");
                for (size_t i = 0; i < _numberOfStates; ++i) {
                    // LDEBUG(_validSourceFilePaths[i] << " is now being processed.");

                    // TODO: REMOVE.. JUST FOR DEBUG/PERFORMANCE TESTS
                    auto start = std::chrono::high_resolution_clock::now();

                    FieldlinesState tmpState;
                    _states.push_back(tmpState);
                    bool statuss = fsManager.getFieldlinesStateFromBinary(
                        _validSourceFilePaths[i],
                        _states[i]);

                    // TODO: REMOVE.. JUST FOR DEBUG/PERFORMANCE TESTS
                    auto end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> diff = end - start;
                    LDEBUG("TIME FOR ADDING STATE FROM BINARY: " << diff.count() << " milliseconds");

                    // TODO: Move elsewhere
                    _startTimes.push_back(_states[i]._triggerTime);
                    // TODO: Delete the rest in this scope!!
                    // _states[i]._extraVariables.pop_back();
                    // _states[i]._extraVariableNames.pop_back();
                    // _states[i].calculateTopologies(3.1f);
                    std::string filePath = _validSourceFilePaths[i];
                    std::string fileName = filePath.substr(filePath.size()-29, 29);

                    size_t dwnSmpl = 1;
                    std::string filePath0 = filePath.substr(0,filePath.size()-29) + "downsampled" + std::to_string(dwnSmpl) + "/" + fileName;
                    _states[i].saveStateSubsetToBinaryFile(fsManager.getAbsPath(filePath0), dwnSmpl);

                    dwnSmpl = 2;
                    std::string filePath1 = filePath.substr(0,filePath.size()-29) + "downsampled" + std::to_string(dwnSmpl) + "/" + fileName;
                    _states[i].saveStateSubsetToBinaryFile(fsManager.getAbsPath(filePath1), dwnSmpl);

                    dwnSmpl = 4;
                    std::string filePath2 = filePath.substr(0,filePath.size()-29) + "downsampled" + std::to_string(dwnSmpl) + "/" + fileName;
                    _states[i].saveStateSubsetToBinaryFile(fsManager.getAbsPath(filePath2), dwnSmpl);
                    // _states.pop_back();
                    // // // add new empty state to not screw with for-loop calling _state[i]
                    // _states.push_back(FieldlinesState());
                }
            }
        } break;
        case LIVE_TRACE: {
            allowSeedPoints = true;
            LERROR("NOT YET INCORPORATED INTO FIELDLINESSEQUENCE CLASS! TODO TODO!!!!!!");
            return false;
        } break;
        default:
            return false;
    }

    if (!_loadBinariesAtRuntime) {
        // _validSourceFilePaths are no longer needed since binaries are already in RAM
        _validSourceFilePaths.clear();
    }

    bool allowExtraVariables = false;
    size_t numExtraVariables = 0;
    if (_numberOfStates > 0) {
        // Approximate the end time of last state (and for the sequence as a whole)
        _seqStartTime = _startTimes[0];

        double lastStateStart = _startTimes[_numberOfStates - 1];
        if (_numberOfStates > 1) {
            double avgTimeOffset = (lastStateStart - _seqStartTime) /
                                   (static_cast<double>(_numberOfStates - 1));
            _seqEndTime =  lastStateStart + avgTimeOffset;
        } else {
            _isMorphing = false;
            // _seqEndTime = 631108800.f; // January 1st 2020
            _seqEndTime = FLT_MAX; // UNTIL THE END OF DAYS!
        }
        // Add seqEndTime as the last start time
        // to prevent vector from going out of bounds later.
        _startTimes.push_back(_seqEndTime); // =  lastStateStart + avgTimeOffset;

        float rmin, rmax, xmin, xmax, ymin, ymax, zmin, zmax;


        std::string model = _states[0]._modelName;
        if (model == "enlil") {
            _scalingFactor          = A_U_TO_METER;
            _scalingFactorLineWidth = R_S_TO_METER;
            _scalingFactorUnit = "AU";
            // TODO: change these hardcoded limits to something that makes sense
            // or allow user to specify in LUA
            // rmin =  0.09f;
            // xmin = ymin = zmin = -5.f;
            // xmax = ymax = zmax = 5.f;
            // rmax = std::max(xmax, std::max(ymax,zmax));

            _isSpherical = true;
        } else if (model == "pfss") {
            _scalingFactor          = R_S_TO_METER;
            _scalingFactorLineWidth = R_S_TO_METER;
            _scalingFactorUnit = "RS";
            // TODO: change these hardcoded limits to something that makes sense
            // or allow user to specify in LUA
            // rmin =  0.0f;
            // xmin = ymin = zmin = -4.f;
            // xmax = ymax = zmax = 4.f;
            // rmax = 5;
            _isSpherical = false; // this might have to change if pfss cdfs are provided

        } else if (model == "batsrus") {
            _scalingFactor          = R_E_TO_METER;
            _scalingFactorLineWidth = R_E_TO_METER;
            _scalingFactorUnit = "RE";
            // TODO: change these hardcoded limits to something that makes sense
            // or allow user to specify in LUA
            // rmin =  1.0f;
            // xmin = -250.f;
            // ymin = zmin = -150.f;
            // xmax = 50.f;
            // ymax = zmax = 150.f;

            // All corners of volume
            // std::vector<glm::vec3> corners{glm::vec3(xmax, ymin, zmin),
            //                                glm::vec3(xmax, ymin, zmax),
            //                                glm::vec3(xmax, ymax, zmax),
            //                                glm::vec3(xmax, ymax, zmax),
            //                                glm::vec3(xmin, ymin, zmin),
            //                                glm::vec3(xmin, ymin, zmax),
            //                                glm::vec3(xmin, ymax, zmax),
            //                                glm::vec3(xmin, ymax, zmin)};
            // rmax = 0;
            // for (glm::vec3 vec : corners) {
            //     float length = glm::length(vec);
            //     if (length > rmax) {
            //         rmax = length;
            //     }
            // }

            _isSpherical = false;
        } else {
            LERROR("OpenSpace's RenderableFieldlinesSequence class can only support the "
                << " batsrus and enlil models for the time being! CDF contains the "
                << model << " model.");
            return false;
        }

        ghoul::Dictionary cartesianLimits;
        glm::vec2 radialLimits;
        bool modfileSpecifiedCartesianLimits = _dictionary.getValue(keyCartesianDomainLimits, cartesianLimits);
        bool modfileSpecifiedRadialLimits = _dictionary.getValue(keyRadialDomainLimits, radialLimits);

        if (modfileSpecifiedCartesianLimits) {
            xmin = cartesianLimits.value<glm::vec2>("1").x /** _scalingFactor*/;
            xmax = cartesianLimits.value<glm::vec2>("1").y /** _scalingFactor*/;
            ymin = cartesianLimits.value<glm::vec2>("2").x /** _scalingFactor*/;
            ymax = cartesianLimits.value<glm::vec2>("2").y /** _scalingFactor*/;
            zmin = cartesianLimits.value<glm::vec2>("3").x /** _scalingFactor*/;
            zmax = cartesianLimits.value<glm::vec2>("3").y /** _scalingFactor*/;
        } else {
            xmin = ymin = zmin = -FLT_MAX;
            xmax = ymax = zmax = FLT_MAX;
        }

        if (modfileSpecifiedRadialLimits) {
            rmin = radialLimits.x /** _scalingFactor*/;
            rmax = radialLimits.y /** _scalingFactor*/;
        } else {
            rmin = 0;
            rmax = FLT_MAX;
        }

        _domainLimR.setMinValue(glm::vec2(rmin));
        _domainLimR.setMaxValue(glm::vec2(rmax));
        _domainLimX.setMinValue(glm::vec2(xmin));
        _domainLimX.setMaxValue(glm::vec2(xmax));
        _domainLimY.setMinValue(glm::vec2(ymin));
        _domainLimY.setMaxValue(glm::vec2(ymax));
        _domainLimZ.setMinValue(glm::vec2(zmin));
        _domainLimZ.setMaxValue(glm::vec2(zmax));

        _domainLimR.setValue(glm::vec2(rmin,rmax));
        _domainLimX.setValue(glm::vec2(xmin,xmax));
        _domainLimY.setValue(glm::vec2(ymin,ymax));
        _domainLimZ.setValue(glm::vec2(zmin,zmax));

        numExtraVariables = _states[0]._extraVariables.size();
        if (numExtraVariables > 0) {
            allowExtraVariables = (_states[0]._extraVariables[0].size() > 0) ? true : false;
            _hasExtraVariables = true;
        }

    } else {
        LERROR("Couldn't create any states!");
        return false;
    }

    if (_isMorphing) {
        LDEBUG("Optimising morphing!");
        float quickMorphDist;
        if(!_dictionary.getValue(keyFieldlineQuickMorphDistance, quickMorphDist)) {
            quickMorphDist = 20.f * R_E_TO_METER; // 2 times Earth's radius
            LWARNING(keyFieldlines << " isn't specifying " <<
                     keyFieldlineQuickMorphDistance <<
                     ". Default is set to 20 Earth radii.");
        }

        fsManager.setQuickMorphBooleans(_states, numResamples, quickMorphDist);
    }

    // GL_LINE width related constants dependent on hardware..
    // glGetFloatv(GL_LINE_WIDTH, &_maxLineWidthOpenGl);
    // glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, &_maxLineWidthOpenGl);
    // glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, &_maxLineWidthOpenGl);
    // glGetFloatv(GL_SMOOTH_LINE_WIDTH_GRANULARITY, &_maxLineWidthOpenGl);

    _usePointDrawing.onChange([this] {
        _drawingOutputType = _drawingOutputType == GL_LINE_STRIP ?
                             GL_POINTS : GL_LINE_STRIP;
    });

    addProperty(_useABlending);
    addProperty(_usePointDrawing);

    if (allowSeedPoints) {
        _seedPointProgram = OsEng.renderEngine().buildRenderProgram(
            "FieldlinesSequenceSeeds",
            "${MODULE_FIELDLINESSEQUENCE}/shaders/seedpoint_vs.glsl",
            "${MODULE_FIELDLINESSEQUENCE}/shaders/seedpoint_fs.glsl"
        );

        if (!_seedPointProgram) {
            LERROR("SeedPoint Shader program failed initialization!");
            return false;
        }
    }

    if (_isMorphing) {
        _program = OsEng.renderEngine().buildRenderProgram(
            "FieldlinesSequence",
            "${MODULE_FIELDLINESSEQUENCE}/shaders/fieldline_morph_flow_direction_vs.glsl",
            "${MODULE_FIELDLINESSEQUENCE}/shaders/fieldline_flow_direction_fs.glsl"
        );
    } else {
        _program = OsEng.renderEngine().buildRenderProgram(
            "FieldlinesSequence",
            "${MODULE_FIELDLINESSEQUENCE}/shaders/fieldline_flow_direction_vs.glsl",
            "${MODULE_FIELDLINESSEQUENCE}/shaders/fieldline_flow_direction_fs.glsl"
        );
    }
    _ropeProgram = OsEng.renderEngine().buildRenderProgram(
        "FieldlinesSequenceRope",
        "${MODULE_FIELDLINESSEQUENCE}/shaders/fieldline_rope_flow_direction_vs.glsl",
        "${MODULE_FIELDLINESSEQUENCE}/shaders/fieldline_rope_fs.glsl",
        "${MODULE_FIELDLINESSEQUENCE}/shaders/fieldline_rope_gs.glsl"
    );

    if (!_program) {
        LERROR("Shader program failed initialization!");
        return false;
    }

    _activeProgramPtr = &*_program;

    if (!_ropeProgram) {
        LERROR("Shader program 'ropeProgram' failed initialization!");
    } else {
        addProperty(_show3DLines);
        addProperty(_lineWidth);
        _show3DLines.onChange([this] {
            // TOGGLE ACTIVE SHADER PROGRAM
            _activeProgramPtr = (_activeProgramPtr == _program.get()) ? &*_ropeProgram : &*_program;
        });
    }

    addProperty(_alphaFade);

    // TODO: IT MAY BE BENEFICIAL IF SOME OF THESE PROPERTIES WERE DEPENDENT ON THE
    // NUMBER OF MAX TRACING STEPS THAT THE USER DEFINED IN LUA!
    // The fieldlineParticleSize and modulusDivider espacially
    addPropertySubOwner(_domainGroup);
    _domainGroup.addProperty(_domainLimR);
    _domainGroup.addProperty(_domainLimX);
    _domainGroup.addProperty(_domainLimY);
    _domainGroup.addProperty(_domainLimZ);

    addPropertySubOwner(_particleGroup);
    _particleGroup.addProperty(_fieldlineParticleColor);
    _particleGroup.addProperty(_fieldlineParticleSize);
    _particleGroup.addProperty(_modulusDivider);
    _particleGroup.addProperty(_timeMultiplier);
    _particleGroup.addProperty(_showParticles);

    if (allowSeedPoints) {
        addPropertySubOwner(_seedGroup);
        _seedGroup.addProperty(_seedPointSize);
        _seedGroup.addProperty(_showSeedPoints);
        _seedGroup.addProperty(_uniformSeedPointColor);
    }

    if (_isMorphing) {
        addProperty(_isMorphing);
    }

    if (allowExtraVariables) {
        _colorMethod.addOption(colorMethod::UNIFORM, "Uniform Color");
        _colorMethod.addOption(colorMethod::QUANTITY_DEPENDENT, "Quantity Dependent");
        _colorMethod = QUANTITY_DEPENDENT;

        // ASSUMING ALL STATES HAVE THE SAME EXTRA VARIABLES
        for (size_t i = 0; i < _states[0]._extraVariableNames.size(); ++i) {
            _colorizingQuantity.addOption(i, _states[0]._extraVariableNames[i]);
            _domainQuantity.addOption(i, _states[0]._extraVariableNames[i]);
        }
        // _domainQuantity.addOption(-1, "---none---");

        // Set tranferfunction min/max to min/max in the given range
        if (_dictionary.hasKeyAndValue<ghoul::Dictionary>(keyExtraColorTablePaths)) {
            // TODO: create helper function to extract all values of type from dictionary
            ghoul::Dictionary colTabPathsDic =
                    _dictionary.value<ghoul::Dictionary>(keyExtraColorTablePaths);
            size_t numColorTables = colTabPathsDic.size();
            if (numColorTables == 0) {
                LERROR("Must specify ColorTablePaths!");
                return false;
            }
            for (size_t i = 1; i <= numColorTables; ++i) {
                _colorTablePaths.push_back(
                        colTabPathsDic.value<std::string>( std::to_string(i) ) );
            }
            // If numberOfColorTables don't add up to numberOfExtraVariables extend or
            // remove to make sure they add up
            // if (numColorTables != numExtraVariables) {
                // _colorTablePaths.resize(numExtraVariables,
                        // _colorTablePaths[numColorTables-1]);
            // }

            if (_dictionary.hasKeyAndValue<ghoul::Dictionary>(keyExtraColorTableMinMaxs)) {
                ghoul::Dictionary colorTableMinMaxDic =
                        _dictionary.value<ghoul::Dictionary>(keyExtraColorTableMinMaxs);
                size_t numMinMaxs = colorTableMinMaxDic.size();
                // TODO: check if (numMinMaxs != ,jdfgh)
                for (size_t i = 1; i <= numMinMaxs; ++i) {
                    _transferFunctionLimits.push_back(
                            colorTableMinMaxDic.value<glm::vec2>(std::to_string(i)));
                }

            } else {
                LERROR("Must specify \"" << keyExtraColorTableMinMaxs << "\"!");
                return false;
            }

            if (_dictionary.hasKeyAndValue<ghoul::Dictionary>(keyExtraMinMaxLimits)) {
                ghoul::Dictionary extraVarMinMaxDic =
                        _dictionary.value<ghoul::Dictionary>(keyExtraMinMaxLimits);
                size_t numMinMaxs = extraVarMinMaxDic.size();
                // TODO: check if (numMinMaxs != ,jdfgh)
                for (size_t i = 1; i <= numMinMaxs; ++i) {
                    glm::vec2 tmp = extraVarMinMaxDic.value<glm::vec2>(std::to_string(i));
                    _tFInterestRangeLimits.push_back(tmp);
                    _tFInterestRange.push_back(tmp);
                }
            } else {
                // TODO: make some general function to handle all mod file exceptions messages
                LERROR("Must specify \"" << keyExtraMinMaxLimits << "\"!");
                return false;
            }

            // Make sure all vectors contain 'numExtraVariables' elements
            // Extend or remove to make sure they add up
            // TODO: Give warning if they actually change!
            _colorTablePaths.resize(numExtraVariables, _colorTablePaths[numColorTables-1]);
            _tFInterestRange.resize(numExtraVariables, _tFInterestRange[0]);
            _tFInterestRangeLimits.resize(numExtraVariables, _tFInterestRangeLimits[0]);
            _transferFunctionLimits.resize(numExtraVariables, _transferFunctionLimits[0]);


        } else {
            LERROR("Must specify \"" << keyExtraColorTablePaths << "\"!");
            return false;
        }
        // TODO: this should probably be determined some other way..
        // TODO: set in LUA when tracing variables are set there! Requires state to store min/max
        // for (size_t i = 0; i < _states[0]._extraVariables.size(); i++) {
        //     float minVal = FLT_MAX;
        //     float maxVal = FLT_MIN;
        //     for (size_t j = 0; j < _states.size(); ++j) {
        //         std::vector<float>& quantityVec = _states[j]._extraVariables[i];
        //     // for (std::vector<float>& quantityVec : _states[0]._extraVariables) {
        //         auto minMaxPos = std::minmax_element(quantityVec.begin(), quantityVec.end());
        //         minVal = *minMaxPos.first  < minVal ? *minMaxPos.first  : minVal;
        //         maxVal = *minMaxPos.second > maxVal ? *minMaxPos.second : maxVal;
        //     }
        //     _transferFunctionLimits.push_back(glm::vec2(minVal, maxVal));
        //     _tFInterestRange.push_back(glm::vec2(minVal, maxVal));
        //     _tFInterestRangeLimits.push_back(glm::vec2(minVal, maxVal));
        // }
            // TODO: REMOVE HARDCODED PATH
        _activeColorTable = &_colorTablePaths[0];
        _transferFunctionPath = *_activeColorTable;
        _transferFunction = std::make_shared<TransferFunction>(fsManager.getAbsPath(_transferFunctionPath));

        // INITIAL VALUES
        _transferFunctionMinVal = std::to_string(_transferFunctionLimits[0].x);
        _transferFunctionMaxVal = std::to_string(_transferFunctionLimits[0].y);

        _unitInterestRange = _tFInterestRange[0];
        _unitInterestRange.setMinValue(glm::vec3(_tFInterestRangeLimits[0].x));
        _unitInterestRange.setMaxValue(glm::vec3(_tFInterestRangeLimits[0].y));

        _colorizingQuantity.onChange([this] {
            LDEBUG("CHANGED COLORIZING QUANTITY");
            _updateColorBuffer = true;
            _transferFunctionMinVal = std::to_string(_transferFunctionLimits[_colorizingQuantity].x);
            _transferFunctionMaxVal = std::to_string(_transferFunctionLimits[_colorizingQuantity].y);
            _activeColorTable = &_colorTablePaths[_colorizingQuantity];
            _transferFunctionPath = *_activeColorTable;
        });

        _domainQuantity.onChange([this] {
            LDEBUG("CHANGED DOMAIN QUANTITY");
            _updateDomainBuffer = true;
            _unitInterestRange = _tFInterestRange[_domainQuantity];
            _unitInterestRange.setMinValue(glm::vec3(_tFInterestRangeLimits[_domainQuantity].x));
            _unitInterestRange.setMaxValue(glm::vec3(_tFInterestRangeLimits[_domainQuantity].y));
        });

        _transferFunctionPath.onChange([this] {
            // TOGGLE ACTIVE SHADER PROGRAM
            _transferFunction->setPath(_transferFunctionPath);
            *_activeColorTable = _transferFunctionPath;
        });

        _transferFunctionMinVal.onChange([this] {
            LDEBUG("CHANGED MIN VALUE");
            // TODO CHECK IF VALID NUMBER!
            // _updateTransferFunctionMin = true;
            _transferFunctionLimits[_colorizingQuantity].x = std::stof(_transferFunctionMinVal);
            _tFInterestRangeLimits[_colorizingQuantity].x =
                    std::min(_transferFunctionLimits[_colorizingQuantity].x,
                             _tFInterestRangeLimits[_colorizingQuantity].x);
        });

        _transferFunctionMaxVal.onChange([this] {
            LDEBUG("CHANGED MAX VALUE");
            // TODO CHECK IF VALID NUMBER!
            // _updateTransferFunctionMin = true;
            _transferFunctionLimits[_colorizingQuantity].y = std::stof(_transferFunctionMaxVal);
            _tFInterestRangeLimits[_colorizingQuantity].y =
                    std::max(_transferFunctionLimits[_colorizingQuantity].y,
                             _tFInterestRangeLimits[_colorizingQuantity].y);
        });

        // TODO: fix bug related to changing/updating of transfer function
        // TODO: Bug causes texture to get default FilterMode: linear
        _useNearestSampling.onChange([this] {
            LDEBUG("Changed TransferFunction sampling method!");

            ghoul::opengl::Texture& texture = _transferFunction->getTexture();
            // TOGGLE BETWEEN NEAREST AND LINEAR SAMPLING
            texture.setFilter(_useNearestSampling ? ghoul::opengl::Texture::FilterMode::Linear :
                                                    ghoul::opengl::Texture::FilterMode::Nearest);
        });

        _isClampingColorValues.onChange([this] {
            LDEBUG("Changed whether to Clamp or Discard values outside of TF range!");
        });

        _unitInterestRange.onChange([this] {
            LDEBUG("CHANGED TRANSFER FUNTION RANGE OF INTEREST");
            // Save the change even if domainQuantity is changed
            _tFInterestRange[_domainQuantity] = _unitInterestRange;
        });

        // _colorGroup.setPropertyGroupName(0,"FieldlineColorRelated");
        addPropertySubOwner(_colorGroup);

        _colorGroup.addProperty(_colorMethod);
        _colorGroup.addProperty(_colorizingQuantity);
        _colorGroup.addProperty(_isClampingColorValues);
        _colorGroup.addProperty(_transferFunctionPath);
        _colorGroup.addProperty(_transferFunctionMinVal);
        _colorGroup.addProperty(_transferFunctionMaxVal);
        _colorGroup.addProperty(_fieldlineColor);
        _colorGroup.addProperty(_useNearestSampling);

        _domainGroup.addProperty(_domainQuantity);
        _domainGroup.addProperty(_unitInterestRange);

    } else {
        addProperty(_fieldlineColor);
    }

    // TODO move to separate function
    //------------------ Initialize OpenGL VBOs and VAOs-------------------------------//
    glGenVertexArrays(1, &_vertexArrayObject);

    glGenBuffers(1, &_vertexPositionBuffer);
    glGenBuffers(1, &_vertexColorBuffer);
    glGenBuffers(1, &_vertexDomainBuffer);

    if (_isMorphing) {
        glGenBuffers(1, &_morphToPositionBuffer);
        glGenBuffers(1, &_quickMorphBuffer);
    }

    if (_seedPoints.size() > 0) {
        glGenVertexArrays(1, &_seedArrayObject);
        glBindVertexArray(_seedArrayObject);
        glGenBuffers(1, &_seedPositionBuffer);

        glBindBuffer(GL_ARRAY_BUFFER, _seedPositionBuffer);
        glBufferData(GL_ARRAY_BUFFER,
            _seedPoints.size() * sizeof(glm::vec3),
            &_seedPoints.front(),
            GL_STATIC_DRAW);
        GLuint seedLocation = 0;
        glEnableVertexAttribArray(seedLocation);
        glVertexAttribPointer(seedLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    setRenderBin(Renderable::RenderBin::Overlay);

    ////////////////////////////////////////
    // LAST MINUTE ADDITIONS!!!!!!!!!!!!!!!
    glm::vec4 tmpColor;
    if (_dictionary.getValue(keyDefaultColor, tmpColor)) {
        LDEBUG("Default color set through LUA modfile!");
        _fieldlineColor = tmpColor;
    }

    //////////////////////////////////////////////

    return true;
}

bool RenderableFieldlinesSequence::deinitialize() {
    glDeleteVertexArrays(1, &_vertexArrayObject);
    _vertexArrayObject = 0;

    glDeleteBuffers(1, &_vertexPositionBuffer);
    _vertexPositionBuffer = 0;

    glDeleteBuffers(1, &_vertexColorBuffer);
    _vertexColorBuffer = 0;

    glDeleteBuffers(1, &_vertexDomainBuffer);
    _vertexDomainBuffer = 0;

    glDeleteBuffers(1, &_morphToPositionBuffer);
    _morphToPositionBuffer = 0;

    glDeleteBuffers(1, &_quickMorphBuffer);
    _quickMorphBuffer = 0;

    RenderEngine& renderEngine = OsEng.renderEngine();
    if (_program) {
        renderEngine.removeRenderProgram(_program);
        _program = nullptr;
    }

    if (_ropeProgram) {
        renderEngine.removeRenderProgram(_ropeProgram);
        _ropeProgram = nullptr;
    }

    if (_seedPointProgram) {
        renderEngine.removeRenderProgram(_seedPointProgram);
        _seedPointProgram = nullptr;
    }

    return true;
}

void RenderableFieldlinesSequence::render(const RenderData& data) {
    // if (_isWithinTimeInterval) {
    if (_shouldRender) {
        _activeProgramPtr->activate();

        glm::dmat4 rotationTransform = glm::dmat4(data.modelTransform.rotation);
        glm::mat4 scaleTransform = glm::mat4(1.0); // TODO remove if no use
        glm::dmat4 modelTransform =
                glm::translate(glm::dmat4(1.0), data.modelTransform.translation) *
                rotationTransform *
                glm::dmat4(glm::scale(glm::dmat4(1.0), glm::dvec3(data.modelTransform.scale))) *
                glm::dmat4(scaleTransform);
        glm::dmat4 modelViewTransform = data.camera.combinedViewMatrix() * modelTransform;

        int testTime = static_cast<int>(data.time.deltaTime() * OsEng.runTime() * 100) / 5;
        // double testTime = _;

        // Set uniforms for shaders
        // _activeProgramPtr->setUniform("time", testTime * _timeMultiplier);
        _activeProgramPtr->setUniform("timeD", _currentTime * _timeMultiplier);
        _activeProgramPtr->setUniform("flParticleSize", _fieldlineParticleSize);
        _activeProgramPtr->setUniform("modulusDivider", _modulusDivider);
        _activeProgramPtr->setUniform("colorMethod", _colorMethod);
        _activeProgramPtr->setUniform("fieldlineColor", _fieldlineColor);
        _activeProgramPtr->setUniform("fieldlineParticleColor", _fieldlineParticleColor);
        _activeProgramPtr->setUniform("usingParticles", _showParticles);
        _activeProgramPtr->setUniform("domainLimR", _domainLimR.value() * _scalingFactor);
        _activeProgramPtr->setUniform("domainLimX", _domainLimX.value() * _scalingFactor);
        _activeProgramPtr->setUniform("domainLimY", _domainLimY.value() * _scalingFactor);
        _activeProgramPtr->setUniform("domainLimZ", _domainLimZ.value() * _scalingFactor);
        _activeProgramPtr->setUniform("alphaMultiplier", _alphaFade);
        if (_show3DLines) {
            _activeProgramPtr->setUniform("width", _lineWidth * _scalingFactorLineWidth);
            // _activeProgramPtr->setUniform("camDirection",
            //                             glm::vec3(data.camera.viewDirectionWorldSpace()));
            // _activeProgramPtr->setUniform("modelTransform", modelTransform);
            // _activeProgramPtr->setUniform("viewTransform", data.camera.combinedViewMatrix());
            _activeProgramPtr->setUniform("modelViewTransform", glm::mat4(modelViewTransform));
            _activeProgramPtr->setUniform("projectionTransform", data.camera.sgctInternal.projectionMatrix());
        } else {
            glLineWidth(_lineWidth);
            _activeProgramPtr->setUniform("modelViewProjection",
                    data.camera.sgctInternal.projectionMatrix() * glm::mat4(modelViewTransform));
            if (_isMorphing) { // TODO ALLOW MORPHING AND 3D LINES/ROPES
                _activeProgramPtr->setUniform("state_progression", _stateProgress);
                _activeProgramPtr->setUniform("isMorphing", _isMorphing);
            }
        }

        if (_hasExtraVariables) {
            if (_colorMethod == colorMethod::QUANTITY_DEPENDENT) {
                // TODO MOVE THIS TO UPDATE AND CHECK
                ghoul::opengl::TextureUnit textureUnit;
                textureUnit.activate();
                _transferFunction->bind(); // Calls update internally
                _activeProgramPtr->setUniform("colorMap", textureUnit);
                _activeProgramPtr->setUniform("isClamping", _isClampingColorValues);
                _activeProgramPtr->setUniform("transferFunctionLimits",
                                              _transferFunctionLimits[_colorizingQuantity]);
            }
            _activeProgramPtr->setUniform("tFIterestRange", _tFInterestRange[_domainQuantity]);
        }

        bool additiveBlending = false;
        if (_useABlending) {
            bool usingFramebufferRenderer = OsEng.renderEngine().rendererImplementation() ==
                                            RenderEngine::RendererImplementation::Framebuffer;

            bool usingABufferRenderer = OsEng.renderEngine().rendererImplementation() ==
                                        RenderEngine::RendererImplementation::ABuffer;

            if (usingABufferRenderer) {
                _activeProgramPtr->setUniform("useABlending", _useABlending);
            }

            additiveBlending = usingFramebufferRenderer;
            if (additiveBlending) {
                glDepthMask(false);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            }
        }

        // _activeProgramPtr->setUniform("classification", _classification);
        // if (!_classification)
        //     _activeProgramPtr->setUniform("fieldLineColor", _fieldlineColor);

        glBindVertexArray(_vertexArrayObject);
        glMultiDrawArrays(
                _drawingOutputType,
                &_states[_drawingIndex]._lineStart[0],
                &_states[_drawingIndex]._lineCount[0],
                static_cast<GLsizei>(_states[_drawingIndex]._lineStart.size())
        );

        glBindVertexArray(0);
        _activeProgramPtr->deactivate();

        if (_showSeedPoints) {
            glBindVertexArray(_seedArrayObject);
            _seedPointProgram->activate();
            _seedPointProgram->setUniform("color", _uniformSeedPointColor);
            _seedPointProgram->setUniform("isSpherical", _isSpherical);
            _seedPointProgram->setUniform("scaleFactor", _scalingFactor);
            _seedPointProgram->setUniform("modelViewProjection",
                data.camera.sgctInternal.projectionMatrix() * glm::mat4(modelViewTransform));
            glPointSize(_seedPointSize);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>( _seedPoints.size() ) );

            glBindVertexArray(0);
            _seedPointProgram->deactivate();
        }

        if (additiveBlending) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(true);
        }
    }
}

void RenderableFieldlinesSequence::update(const UpdateData& data) {
    _currentTime = data.time.j2000Seconds();
    if (_activeProgramPtr->isDirty()) {
        _activeProgramPtr->rebuildFromFile();
    }

    if (_showSeedPoints) {
        if (_seedPointProgram->isDirty()) {
            _seedPointProgram->rebuildFromFile();
        }
    }

    // Check if current time in OpenSpace is within sequence interval
    if (isWithinSequenceInterval()) {
        // if NOT in the same state as in the previous update..
        if ( _activeStateIndex < 0 || _currentTime < _startTimes[_activeStateIndex] ||
                // This next line requires/assumes seqEndTime to be last position in _startTimes
                _currentTime >= _startTimes[_activeStateIndex + 1]) {

            updateActiveStateIndex();

            if (_loadBinariesAtRuntime) {
                _mustProcessNewState = true;
            } else {
                _needsUpdate = true;
            }
        } else if (_isMorphing) {
            double stateDuration = _startTimes[_activeStateIndex + 1] -
                                   _startTimes[_activeStateIndex]; // TODO? could be stored
            double stateTimeElapsed = _currentTime - _startTimes[_activeStateIndex];
            _stateProgress = static_cast<float>(stateTimeElapsed / stateDuration);
            // ghoul_assert(_stateProgress >= 0.0f, "_stateProgress is NEGATIVE!!");
        } // else {we're still in same state as previous update (no changes needed)}
    } else {
        // Not in interval => set everything to false
        _activeStateIndex = -1;
        _shouldRender = false;
        _needsUpdate = false;
        _mustProcessNewState = false;
    }

    if (_mustProcessNewState) {
        if (!_isProcessingState && !_newStateIsReady) {
                _isProcessingState = true;
                _mustProcessNewState = false;
                std::thread readBinaryThread([this] {
                    this->readNewState(this->_activeStateIndex);
                });
                // std::thread readBinaryThread(&RenderableFieldlinesSequence::readNewState, _activeStateIndex, this);
                readBinaryThread.detach();
        }
    }

    if(_needsUpdate || _newStateIsReady) {
        if (_loadBinariesAtRuntime) {
            _states[0] = std::move(_newState);
        } else {
            _drawingIndex = _activeStateIndex;
        }

        glBindVertexArray(_vertexArrayObject);

        updateVertexPosBuffer();

        if (_isMorphing) {
            updateMorphingBuffers();
            double stateDuration = _startTimes[_activeStateIndex + 1] -
                                       _startTimes[_activeStateIndex]; // TODO? could be stored
            double stateTimeElapsed = _currentTime - _startTimes[_activeStateIndex];
            _stateProgress = static_cast<float>(stateTimeElapsed / stateDuration);
        }

        // TODO: makes no sense in having _hasExtraVariables as a member variable if it is
        // only used here..
        _hasExtraVariables = (_states[_drawingIndex]._extraVariables.size() > 0) ? true : false;
        if (_hasExtraVariables) {
            _updateColorBuffer = true;
            _updateDomainBuffer = true;
        }

        // UNBIND
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        _needsUpdate = false;
        _newStateIsReady = false;
        _shouldRender = true;
    }
    if (_activeStateIndex > -1) {
        if (_updateColorBuffer) {
            _updateColorBuffer = false;
            updateColorBuffer();
        }
        if (_updateDomainBuffer) {
            _updateDomainBuffer = false;
            updateDomainBuffer();
        }
    }
}

bool RenderableFieldlinesSequence::isWithinSequenceInterval() {
    return (_currentTime >= _seqStartTime) &&
           (_isMorphing ? _currentTime < _startTimes[_numberOfStates-1] // nothing to morph to after last state
                        : _currentTime < _seqEndTime);
}

// Assumes we already know that _currentTime is within the sequence interval
void RenderableFieldlinesSequence::updateActiveStateIndex() {
    auto iter = std::upper_bound(_startTimes.begin(), _startTimes.end(), _currentTime);
    //
    if (iter != _startTimes.end()) {
        if ( iter != _startTimes.begin()) {
            _activeStateIndex = std::distance(_startTimes.begin(), iter) - 1;
        } else {
            _activeStateIndex = 0;
        }
    } else {
        _activeStateIndex = _numberOfStates - 1;
    }
}

void RenderableFieldlinesSequence::updateColorBuffer() {
    glBindVertexArray(_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, _vertexColorBuffer);

    auto& quantityVec = _states[_drawingIndex]._extraVariables[_colorizingQuantity];

    glBufferData(GL_ARRAY_BUFFER, quantityVec.size() * sizeof(float),
            &quantityVec.front(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(_vertAttrColorQuantity);
    glVertexAttribPointer(_vertAttrColorQuantity, 1, GL_FLOAT, GL_FALSE, 0, 0);
}

// TODO updateDomainBuffer() and updateColorBuffer() are identical except for a few variables
// TODO Make a generalized function
void RenderableFieldlinesSequence::updateDomainBuffer() {
    glBindVertexArray(_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, _vertexDomainBuffer);

    auto& quantityVec = _states[_drawingIndex]._extraVariables[_domainQuantity];

    glBufferData(GL_ARRAY_BUFFER, quantityVec.size() * sizeof(float),
            &quantityVec.front(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(_vertAttrDomainQuantity);
    glVertexAttribPointer(_vertAttrDomainQuantity, 1, GL_FLOAT, GL_FALSE, 0, 0);
}

void RenderableFieldlinesSequence::updateMorphingBuffers() {
    /// TODO SWAP BUFFERING IF MORPHING BUFFER CONTAINS THE POINT OF THE NEW _activeIndex
    glBindBuffer(GL_ARRAY_BUFFER, _morphToPositionBuffer);

    glBufferData(GL_ARRAY_BUFFER,
        _states[_drawingIndex+1]._vertexPositions.size() * sizeof(glm::vec3),
        &_states[_drawingIndex+1]._vertexPositions.front(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(_vertAttrMorphToPos);
    glVertexAttribPointer(_vertAttrMorphToPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, _quickMorphBuffer);

    glBufferData(GL_ARRAY_BUFFER,
            _states[_drawingIndex]._quickMorph.size() * sizeof(GLfloat),
            &_states[_drawingIndex]._quickMorph.front(),
            GL_STATIC_DRAW);

    glEnableVertexAttribArray(_vertAttrMorphQuick);
    glVertexAttribPointer(_vertAttrMorphQuick, 1, GL_FLOAT, GL_FALSE, 0, 0);
}

void RenderableFieldlinesSequence::updateVertexPosBuffer() {
    glBindBuffer(GL_ARRAY_BUFFER, _vertexPositionBuffer);

    auto& vertexPosVec = _states[_drawingIndex]._vertexPositions;

    glBufferData(GL_ARRAY_BUFFER, vertexPosVec.size() * sizeof(glm::vec3),
            &vertexPosVec.front(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(_vertAttrVertexPos);
    glVertexAttribPointer(_vertAttrVertexPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

bool RenderableFieldlinesSequence::getUnsignedIntFromModfile(const std::string& key,
                                                             unsigned int& val) {
    float f_val;
    if (!_dictionary.getValue(key, f_val)) {
        LDEBUG("Key \"" << key << "\" wasn't found!");
        return false;
    } else if (f_val < 0.f) {
        LWARNING(key << " doesn't accept negative numbers. Converting to absolute value!");
        f_val *= -1.f;
    }
    val = static_cast<unsigned int>(f_val);
    return true;
}

bool RenderableFieldlinesSequence::getSourceFilesFromDictionary(
        const std::string& fileExt, std::vector<std::string>& validSourceFilePaths) {

    // ----------------- GET SOURCE FOLDER AS SPECIFIED IN MOD FILE --------------------
    std::string pathToSourceFolder;
    if (!_dictionary.getValue(keySourceFolder, pathToSourceFolder)) {
        LERROR("Mod file doesn't specify a '" << keySourceFolder);
        return false;
    }

    // ------------- GET ALL FILES OF SPECIFIED TYPE FROM SOURCE FOLDER ----------------
    if (!fsManager.getAllFilePathsOfType(
            pathToSourceFolder, fileExt, validSourceFilePaths) ||
            validSourceFilePaths.empty() ) {

        LERROR("Failed to get valid "<< fileExt << " file paths from '"
                << pathToSourceFolder << "'" );
        return false;
    }

    // Set default values
    unsigned int startStateOffset = 0;  // 0 => start at first file in folder
    unsigned int stateStepSize    = 1;  // 1 => every state (no state skipping)
    unsigned int numMaxStates     = 0;  // 0 => means no limit

    // Update default values if provided in mod file
    getUnsignedIntFromModfile(keyStartStateOffset, startStateOffset);
    getUnsignedIntFromModfile(keyStateStepSize,    stateStepSize);
    getUnsignedIntFromModfile(keyMaxNumStates,     numMaxStates);

    // Extract the file paths that fulfill the specification
    vector_extraction::extractChosenElements(startStateOffset, stateStepSize, numMaxStates, validSourceFilePaths);

    LDEBUG("Found " << validSourceFilePaths.size() << " valid ." << fileExt << " files in " << pathToSourceFolder);
    // LDEBUG("Found the following valid ." << fileExt << " files in " << pathToSourceFolder);
    // for (std::string str : validSourceFilePaths) {
    //     LDEBUG("\t" << str);
    // }
    return true;
}

// TODO: GENERALIZE!
// TODO: allow more than one seed file.. allow folder!
bool RenderableFieldlinesSequence::getSeedPointsFromDictionary() {
    ghoul::Dictionary seedInfo;
    if (!_dictionary.getValue(keySeedPointsInfo, seedInfo)) {
        LERROR("Mod-file doesn't specify a '" << keySeedPointsInfo << "' field.");
        return false;
    }

    // SeedPoints Info. Needs a .txt file containing seed points.
    // Each row should have 3 floats seperated by spaces
    std::string pathToSeedPointFile;
    if (!seedInfo.getValue(keySeedPointsFile, pathToSeedPointFile)) {
        LERROR(keySeedPointsInfo << " doesn't specify a '" << keySeedPointsFile << "'" <<
            "\n\tRequires a path to a .txt file containing seed point data." <<
            "Each row should have 3 floats seperated by spaces.");
        return false;
    } else if (!fsManager.getSeedPointsFromFile(pathToSeedPointFile, _seedPoints)) {
        LERROR("Failed to find seed points in'" << pathToSeedPointFile << "'");
        return false;
    }
    return true;
}

void RenderableFieldlinesSequence::readNewState(const int activeStateIndex) {
    if (_states.size() > 1) {
        LERROR("ALREADY MORE THAN ONE STATE IN '_states' VECTOR");
    }
    FieldlinesState tmpState;
    // _states.push_back(tmpState);
    // size_t numS = _states.size();
    auto start = std::chrono::high_resolution_clock::now();

    bool statuss = fsManager.getFieldlinesStateFromBinary(
            _validSourceFilePaths[activeStateIndex],
            tmpState);

    _newState = std::move(tmpState);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    auto ms = diff.count();
    if (ms > 40) {
        LWARNING("TIME FOR ADDING STATE FROM BINARY: " << ms << " milliseconds. (" << _validSourceFilePaths[activeStateIndex] << ")");
    }

    _newStateIsReady = true;
    _isProcessingState = false;
}

} // namespace openspace
