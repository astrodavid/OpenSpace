-- local enlilDataRoot = '${OPENSPACE_DATA}/scene/fieldlinessequence/enlil_fl_july2012/';
-- local enlilVelocityColorTable = enlilDataRoot..'colortables/enlil_velocity.txt';
-- local enlilDensityColorTable = enlilDataRoot..'colortables/enlil_density.txt';
local enlilDataRoot = 'X:/CCMC/fieldlinesdata/enlil/downsampled40/';
local enlilColorTables = 'X:/CCMC/fieldlinesdata/colortables/';
local enlilVelocityColorTable = enlilColorTables..'kroyw.txt';
local enlilDensityColorTable = enlilColorTables..'enlil_density.txt';

return {
    {
        Name = "FL_ENLIL_slice_eqPlane_011AU_1",
        Parent = "HNMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = enlilDataRoot..'011AU_eq_plane_1/',
            -- StartStateOffset = 309,
            -- MaxNumStates = 1,
            ColorTablePaths = {
                enlilDensityColorTable,
                enlilVelocityColorTable,
            },
            DefaultColor = {0.4,0.15,0.4,0.60},
            ColorTableMinMax = {{0, 1000000},{100, 2000},},
            ExtraMinMaxLimits = {{-1, 10000000},{0, 5000},},
            RadialDomainLimits = {0, 5},
            CartesianDomainLimits = {{-2.5, 2.5},{-2.5,2.5},{-2.5,2.5},},
            LoadBinariesAtRuntime = true,
        },
        -- Tag = {"fieldline_sequence", "fieldline_enlil"},
    },

    {
        Name = "FL_ENLIL_slice_eqPlane_011AU_2",
        Parent = "HNMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = enlilDataRoot..'011AU_eq_plane_2/',
            -- StartStateOffset = 309,
            -- MaxNumStates = 1,
            ColorTablePaths = {
                enlilDensityColorTable,
                enlilVelocityColorTable,
            },
            DefaultColor = {0.4,0.15,0.4,0.60},
            ColorTableMinMax = {{0, 1000000},{100, 2000},},
            ExtraMinMaxLimits = {{-1, 10000000},{0, 5000},},
            RadialDomainLimits = {0, 5},
            CartesianDomainLimits = {{-2.5, 2.5},{-2.5,2.5},{-2.5,2.5},},
            LoadBinariesAtRuntime = true,
        },
        -- Tag = {"fieldline_sequence", "fieldline_enlil"},
    },

    {
        Name = "FL_ENLIL_slice_lat4_011AU_1",
        Parent = "HNMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = enlilDataRoot..'011AU_lat4_1/',
            -- StartStateOffset = 309,
            -- MaxNumStates = 1,
            ColorTablePaths = {
                enlilDensityColorTable,
                enlilVelocityColorTable,
            },
            DefaultColor = {0.4,0.15,0.4,0.60},
            ColorTableMinMax = {{0, 1000000},{100, 2000},},
            ExtraMinMaxLimits = {{-1, 10000000},{0, 5000},},
            RadialDomainLimits = {0, 5},
            CartesianDomainLimits = {{-2.5, 2.5},{-2.5,2.5},{-2.5,2.5},},
            LoadBinariesAtRuntime = true,
        },
        -- Tag = {"fieldline_sequence", "fieldline_enlil"},
    },
    {
        Name = "FL_ENLIL_slice_lat4_011AU_2",
        Parent = "HNMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = enlilDataRoot..'011AU_lat4_2/',
            -- StartStateOffset = 309,
            -- MaxNumStates = 1,
            ColorTablePaths = {
                enlilDensityColorTable,
                enlilVelocityColorTable,
            },
            DefaultColor = {0.4,0.15,0.4,0.60},
            ColorTableMinMax = {{0, 1000000},{100, 2000},},
            ExtraMinMaxLimits = {{-1, 10000000},{0, 5000},},
            RadialDomainLimits = {0, 5},
            CartesianDomainLimits = {{-2.5, 2.5},{-2.5,2.5},{-2.5,2.5},},
            LoadBinariesAtRuntime = true,
        },
        -- Tag = {"fieldline_sequence", "fieldline_enlil"},
    },
    {
        Name = "FL_ENLIL_earth",
        Parent = "HNMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = enlilDataRoot..'earth/',
            -- StartStateOffset = 309,
            -- MaxNumStates = 1,
            ColorTablePaths = {
                enlilDensityColorTable,
                enlilVelocityColorTable,
            },
            DefaultColor = {1,1,1,0.60},
            ColorTableMinMax = {{0, 1000000},{100, 2000},},
            ExtraMinMaxLimits = {{-1, 10000000},{0, 5000},},
            RadialDomainLimits = {0, 5},
            CartesianDomainLimits = {{-2.5, 2.5},{-2.5,2.5},{-2.5,2.5},},
            LoadBinariesAtRuntime = true,
        },
        -- Tag = {"fieldline_sequence", "fieldline_enlil"},
    },
    {
        Name = "FL_ENLIL_stereoa",
        Parent = "HNMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = enlilDataRoot..'stereoa/',
            -- StartStateOffset = 309,
            -- MaxNumStates = 1,
            ColorTablePaths = {
                enlilDensityColorTable,
                enlilVelocityColorTable,
            },
            DefaultColor = {1,1,1,0.60},
            ColorTableMinMax = {{0, 1000000},{100, 2000},},
            ExtraMinMaxLimits = {{-1, 10000000},{0, 5000},},
            RadialDomainLimits = {0, 5},
            CartesianDomainLimits = {{-2.5, 2.5},{-2.5,2.5},{-2.5,2.5},},
            LoadBinariesAtRuntime = true,
        },
        -- Tag = {"fieldline_sequence", "fieldline_enlil"},
    },
}
