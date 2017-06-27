-- Load the common helper functions
dofile(openspace.absPath('${SCRIPTS}/common.lua'))

openspace.clearKeys()
helper.setCommonKeys()
helper.setDeltaTimeKeys({
--  1           2           3           4           5           6           7           8           9           0
--------------------------------------------------------------------------------------------------------------------------
--  1s          2s          5s          10s         30s         1m          2m          5m          10m         30m
    1,          2,          5,          10,         30,         60,         120,        300,        600,        1800,

--  1h          2h          3h          6h          12h         1d          2d          4d          1w          2w
    3600,       7200,       10800,      21600,      43200,      86400,      172800,     345600,     604800,     1209600,

--  1mo         2mo         3mo         6mo         1yr         2y          5y          10y         20y         50y
    2592000,    5184000,    7776000,    15552000,   31536000,   63072000,   157680000,  315360000,  630720000,  1576800000
})
--  OBS: One month (1mo) is approximated by 30 days.


-- Spacecraft imagery stuff
openspace.bindKey("a", "openspace.setPropertyValue('Interaction.origin', 'Sun')", "Sets the focus of the camera to the Sun")
openspace.bindKey("e", "openspace.setPropertyValue('Interaction.origin', 'Earth')", "Sets the focus of the camera to the Earth")
openspace.bindKey("s", "openspace.setPropertyValue('Interaction.origin', 'SDO')", "Sets the focus of the camera to SDO")
--openspace.bindKey("d", "openspace.setPropertyValue('Interaction.origin', 'Stereo A')", "Sets the focus of the camera to Stereo A")
--openspace.bindKey("f", "openspace.setPropertyValue('Interaction.origin', 'SOHO')", "Sets the focus of the camera to SOHO")
--openspace.bindKey("g", "openspace.setPropertyValue('Interaction.origin', 'ISS')", "Sets the focus of the camera to ISS")
-- Time
openspace.bindKey("h", "openspace.time.setDeltaTime(1500)", "Set delta time to 1500")
openspace.bindKey("j", "openspace.time.setTime('2012 JUL 01 00:00:00.000')", "Sets time to 2012 07 01 00:00:00.000")
openspace.bindKey("k", "openspace.time.setTime('2012 JUL 04 00:00:00.000')", "Sets time to 2012 07 04 00:00:00.000")
openspace.bindKey("l", "openspace.time.setTime('2012 JUL 12 19:10:15.000')", "Sets time to 2012 07 12 19:10:15.000")


-- openspace.bindKey("x",
--     "openspace.setPropertyValue("FL_BATSRUS_Artificial_SingleStep*.renderable.showParticles", true)"
--     helper.property.invert('FL_BATSRUS_Artificial_SingleStepOpenClosed.renderable.'),
--     ""
-- )




-- TODO : MOON AND GEOSYNCHRONIZE TOGGLES!
-- TODO : ADD TIME STEP JAN-2000-01 04.00 and JAN-2000-01 06.57
-- TODO Everything colored by topology
-- TODO : SET PARTICLES AND COLORS CORRECTLY
-- OPENCLOSED COLOR ALPHA 0.18, PARTICLE ALPHA 0.5
-- OPENCLOSED PARTICLE FREQUENCY 31
-- OPENCLOSED PARTICLE SIZE 2
-- OPENCLOSED PARTICLE SPEED 15000000
-- THROUGHOUT PARTICLE FREQUENCY 31
-- THROUGHOUT PARTICLE SIZE 2
-- THROUGHOUTS COLOR ALPHA 0.33, PARTICLE ALPHA 1.0
-- PARTICLE SPEED = 15000000
-- DELTA TIME 0.001





openspace.bindKey("z", "openspace.time.setDeltaTime(0.001)", "Sets delta time to 0.001")
openspace.bindKey("x", "openspace.time.setTime('2011 JAN 01 00:00:00.000')", "Sets time to 2011 01 12 00:00:00.000")
openspace.bindKey("c", "openspace.time.setTime('2000 JAN 01 04:00:00.000')", "Sets time to 2000 01 01 04:00:00.000")
openspace.bindKey("v", "openspace.time.setTime('2000 JAN 01 06:57:00.000')", "Sets time to 2000 01 01 06:57:00.000")
openspace.bindKey("b", "openspace.time.setTime('2012 JUL 14 06:00:00.000')", "Sets time to 2012 07 14 06:00:00.000")


-- openspace.bindKey("Ctrl+d",
--     "openspace.setPropertyValue('SolarImagery_Stereo_StereoA_Image_EUV.renderable.currentActiveInstrumentProperty', 'SECCHI_EUVI_195');" ..
--     "openspace.setPropertyValue('SolarImagery_Stereo_StereoB_Image_EUV.renderable.currentActiveInstrumentProperty', 'SECCHI_EUVI_195');" ..
--     "openspace.setPropertyValue('SolarImagery_SDO_Image_AIA.renderable.currentActiveInstrumentProperty', 'AIA_AIA_193');",
--     "Sets all EUV to 195"
--     --"openspace.setPropertyValue('Pluto.renderable.clearAllProjections', true);",
-- )

openspace.bindKey("Alt+d",
    "openspace.setPropertyValue('SolarImagery_Stereo_StereoA_Image_EUV.renderable.currentActiveInstrumentProperty', 'SECCHI_EUVI_304');" ..
    "openspace.setPropertyValue('SolarImagery_Stereo_StereoB_Image_EUV.renderable.currentActiveInstrumentProperty', 'SECCHI_EUVI_304');" ..
    "openspace.setPropertyValue('SolarImagery_SDO_Image_AIA.renderable.currentActiveInstrumentProperty', 'AIA_AIA_304');",
    "Sets all EUV to 304"
    --"openspace.setPropertyValue('Pluto.renderable.clearAllProjections', true);",
)

-- Activate Stereo A + B EUV + Orbits + Labels + Frustums
openspace.bindKey("Shift+d",
    --"openspace.setPropertyValue('SolarImagery_Stereo_O*.renderable.enabled', true);",
    --"openspace.setPropertyValue('SolarImagery_Stereo_L*.renderable.enabled', true);",
    --helper.renderable.toggle('SolarImagery_Stereo_*_Image_*')
    helper.renderable.toggle('SolarImagery_Stereo_O_StereoA_Trail') ..
    helper.renderable.toggle('SolarImagery_Stereo_O_StereoB_Trail') ..
    helper.renderable.toggle('SolarImagery_Stereo_L_StereoA_Marker') ..
    helper.renderable.toggle('SolarImagery_Stereo_L_StereoB_Marker') ..
    "openspace.setPropertyValue('SolarImagery_Stereo_StereoA_Image_EUV.renderable.currentActiveInstrumentProperty', 'SECCHI_EUVI_195');" ..
    "openspace.setPropertyValue('SolarImagery_Stereo_StereoB_Image_EUV.renderable.currentActiveInstrumentProperty', 'SECCHI_EUVI_195');" ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoA_Image_EUV') ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoB_Image_EUV') ..
    helper.property.invert('SolarImagery_Stereo_StereoA_Image_EUV.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Stereo_StereoB_Image_EUV.renderable.enableFrustum'),
    --helper.renderable.toggle('SolarImagery_Stereo_StereoA_Image_COR'),
    --helper.rendearble.toggle('SolarImagery_Stereo_StereoB_Image_COR'),
    "Activate Stereo A + B + EUV + Orbits + Labels + Frustums"
)

-- PFSS ON
-- Turn off SDO Frustum, stereo frustums, switch to HMI magnetogram, set plane opacity to 1
openspace.bindKey("Ctrl+d",
    helper.property.invert('SolarImagery_SDO_Image_AIA.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Stereo_StereoA_Image_EUV.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Stereo_StereoB_Image_EUV.renderable.enableFrustum') ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoA_Image_EUV') ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoB_Image_EUV') ..
    "openspace.setPropertyValue('SolarImagery_SDO_Image_AIA.renderable.currentActiveInstrumentProperty', 'HMI_HMI_magnetogram');" ..
    "openspace.setPropertyValue('SolarImagery_*_Image*.renderable.planeOpacity', 100)",
    "Turn off SDO Frustum, stereo frustums, switch to HMI magnetigram, set plane opacity to 1"
)

-- Frustums
openspace.bindKey("Ctrl+h",
    helper.property.invert('SolarImagery_SDO_Image_AIA.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Stereo_StereoA_Image_EUV.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Stereo_StereoA_Image_COR.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Stereo_StereoB_Image_EUV.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Stereo_StereoB_Image_COR.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Soho_Image_C2.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Soho_Image_C3.renderable.enableFrustum'),
    "Invert display frustums"
)

-- SDO
openspace.bindKey("Shift+s",
    --helper.renderable.toggle('SolarImagery_SDO_Image_AIA') ..
    helper.renderable.toggle('SolarImagery_SDO_Trail') ..
    helper.renderable.toggle('SolarImagery_SDO_Marker'),
    "Toggle SDO Trail and marker SDO"
)

-- Toggle SDO Marker, turn on SDO Frustum, move out planes, turn on plane opacity,  set to continuum
openspace.bindKey("Ctrl+s",
    --helper.renderable.toggle('SolarImagery_SDO_Marker') ..
    --helper.renderable.toggle('SolarImagery_EarthMarker_Marker') ..
    helper.property.invert('SolarImagery_SDO_Image_AIA.renderable.enableFrustum') ..
    --"openspace.setPropertyValue('SolarImagery_*_Image*.renderable.moveFactor', 0.85)" ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoA_Image_EUV') ..
     "openspace.setPropertyValue('SolarImagery_*_Image*.renderable.moveFactor', 0.9)" ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoB_Image_EUV') ..
    "openspace.setPropertyValue('SolarImagery_*_Image*.renderable.planeOpacity', 100)" ..
    "openspace.setPropertyValue('SolarImagery_SDO_Image_AIA.renderable.currentActiveInstrumentProperty', 'HMI_HMI_continuum');",
    "Enable SDO frustum and switch to Continuum"
)

-- Toggle SOHO
openspace.bindKey("Shift+h",
    helper.renderable.toggle('SolarImagery_Soho_Image_C2') ..
    helper.renderable.toggle('SolarImagery_Soho_Image_C3') ..
    helper.property.invert('SolarImagery_Soho_Image_C2.renderable.enableFrustum') ..
    helper.property.invert('SolarImagery_Soho_Image_C3.renderable.enableFrustum') ..
    --helper.property.invert('SolarImagery_SDO_Image_AIA.renderable.enableFrustum') ..
    helper.renderable.toggle('SolarImagery_SDO_Marker') ..
    helper.renderable.toggle('SolarImagery_Soho_Marker') ..
    helper.renderable.toggle('SolarImagery_Soho_Trail'),
    "Toggle SOHO"
)

-- Set
openspace.bindKey("Ctrl+l",
    -- "openspace.setPropertyValue('SolarImagery_SDO_Image_AIA.renderable.minRealTimeUpdateInterval', 1000);" ..
    -- "openspace.setPropertyValue('SolarImagery_Stereo_StereoA_Image_EUV.renderable.minRealTimeUpdateInterval', 1000);" ..
    -- "openspace.setPropertyValue('SolarImagery_Stereo_StereoB_Image_EUV.renderable.minRealTimeUpdateInterval', 1000);",
    helper.renderable.toggle('SolarImagery_SDO_Image_AIA') ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoA_Image_EUV') ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoB_Image_EUV'),
    "Set EUV to slow update"
)

-- toggle
openspace.bindKey("Ctrl+k",
    helper.renderable.toggle('SolarImagery_Soho_Image_C2') ..
    helper.renderable.toggle('SolarImagery_Soho_Image_C3') ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoA_Image_COR') ..
    helper.renderable.toggle('SolarImagery_Stereo_StereoB_Image_COR'),
    "Toggle Corona Graphs"
)

openspace.bindKey("Shift+c",
    "openspace.registerScreenSpaceRenderable({Type = 'ScreenSpaceImage', TexturePath = openspace.absPath('X:/CCMC/fieldlinesdata/colortables/kroyw_colorbar_enlil_speed.png') });",
    "Toggle screen space color table"
)

-- TODO(mnoven): SDO is not changing to hmi magnetogram properly, bit alt+d works if before
openspace.bindKey("Ctrl+g",
    helper.renderable.toggle('SolarImagery_Stereo_O_StereoA_Trail') ..
    helper.renderable.toggle('SolarImagery_Stereo_O_StereoB_Trail') ..
    helper.renderable.toggle('SolarImagery_Soho_Marker') ..
    helper.renderable.toggle('SolarImagery_Soho_Trail') ..
    "openspace.setPropertyValue('SolarImagery_SDO_Image_AIA.renderable.currentActiveInstrumentProperty', 'HMI_HMI_magnetogram');" ..
    "openspace.time.setTime('2012 JUL 12 19:10:15.000')" ..
    -- TURN ON ENLIL!!!!!!!!!
    helper.renderable.toggle('SolarImagery_Soho_Image_C3') ..
    "openspace.setPropertyValue('FL_PFSS.renderable.enabled', true);",
    "Turn off stereo a, b, soho trails, turn off soho marker and switch to hmi continuum and turn on PFSS, set to PFSS Time"
)

--"openspace.setPropertyValue('SolarImagery_SDO_Image_AIA.renderable.currentActiveInstrumentProperty', 'AIA_AIA_193');",

-- Make hot KEY
openspace.bindKey("Shift+p",
    --"openspace.time.setTime('2012 JUL 12 19:10:15.000')" ..
    helper.renderable.toggle('FL_PFSS'),
    "Toggle PFSS"
)

openspace.bindKey("Ctrl+p",
    "openspace.time.setDeltaTime(0.001)" ..
    helper.property.invert('FL_PFSS.renderable.showParticles'),
    "Toggle particles"
)

openspace.bindKey("Ctrl+e",
    helper.renderable.toggle('SolarImagery_EarthMarker_Marker'),
    --helper.renderable.toggle('SolarImagery_SDO_Marker'),
    "Toggle Earth Marker"
)

openspace.bindKey("Shift+e",
    helper.renderable.toggle('FL_ENLIL_slice_eqPlane_011AU_1') ..
    helper.renderable.toggle('FL_ENLIL_slice_eqPlane_011AU_2') ..
    helper.renderable.toggle('FL_ENLIL_slice_lat4_011AU_1') ..
    helper.renderable.toggle('FL_ENLIL_slice_lat4_011AU_2') ..
    helper.renderable.toggle('FL_ENLIL_earth') ..
    helper.renderable.toggle('FL_ENLIL_stereoa'),
    "Toggle Enlil"
)

openspace.bindKey("Ctrl+1",
    "openspace.setPropertyValue('Sun_Projection.renderable.loopId', 0);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', true);",
    "Activate Loop 1"
)

openspace.bindKey("Ctrl+2",
    "openspace.setPropertyValue('Sun_Projection.renderable.loopId', 1);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', true);",
    "Activate Loop 2"
)

openspace.bindKey("Ctrl+3",
    "openspace.setPropertyValue('Sun_Projection.renderable.loopId', 2);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', true);",
    "Activate Loop 3"
)

openspace.bindKey("Ctrl+4",
    "openspace.setPropertyValue('Sun_Projection.renderable.loopId', 3);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', true);",
    "Activate Loop 4"
)

openspace.bindKey("Ctrl+5",
    "openspace.setPropertyValue('Sun_Projection.renderable.loopId', 4);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', true);",
    "Activate Loop 5"
)

openspace.bindKey("Ctrl+6",
    "openspace.setPropertyValue('Sun_Projection.renderable.loopId', 5);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', true);",
    "Activate Loop 6"
)
openspace.bindKey("Ctrl+7",
    "openspace.setPropertyValue('Sun_Projection.renderable.loopId', 6);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', true);",
    "Activate Loop 7"
)

openspace.bindKey("Ctrl+8",
    "openspace.setPropertyValue('Sun_Projection.renderable.loopId', 7);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', true);",
    "Activate Loop 8"
)

openspace.bindKey("Ctrl+9",
    "openspace.setPropertyValue('Sun_Projection.renderable.loopId', 8);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);" ..
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', true);",
    "Activate Loop 9"
)

openspace.bindKey("Shift+g",
    "openspace.setPropertyValue('Sun_Projection.renderable.activateLooping', false);",
    "Stop Looping"
)

-- Toggle Additive Blending for ALL field lines
openspace.bindKey("Shift+a",
    helper.property.invert('FL_PFSS.renderable.additiveBlending') ..
    helper.property.invert('FL_BATSRUS_J12_OpenClosed.renderable.additiveBlending') ..
    helper.property.invert('FL_BATSRUS_J12_FlowLines.renderable.additiveBlending') ..
    helper.property.invert('FL_BATSRUS_Artificial_SingleStepFlowLines.renderable.additiveBlending') ..
    helper.property.invert('FL_BATSRUS_Artificial_SingleStepOpenClosed.renderable.additiveBlending') ..
    helper.property.invert('FL_ENLIL_slice_eqPlane_011AU_1.renderable.additiveBlending') ..
    helper.property.invert('FL_ENLIL_slice_eqPlane_011AU_2.renderable.additiveBlending') ..
    helper.property.invert('FL_ENLIL_slice_lat4_011AU_1.renderable.additiveBlending') ..
    helper.property.invert('FL_ENLIL_slice_lat4_011AU_2.renderable.additiveBlending') ..
    helper.property.invert('FL_ENLIL_earth.renderable.additiveBlending') ..
    helper.property.invert('FL_ENLIL_stereoa.renderable.additiveBlending'),
    "Toggle Additive Blending"
)
