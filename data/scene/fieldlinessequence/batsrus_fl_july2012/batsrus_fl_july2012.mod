-- local batsrusDataRoot = '${OPENSPACE_DATA}/scene/fieldlinessequence/july2012/batsrus_fl_july2012/';
-- local batsrusTemperatureColorTable = batsrusDataRoot..'colortables/batsrus_temperature.txt';
-- local batsrusDensityColorTable = batsrusDataRoot..'colortables/batsrus_density.txt';
-- local batsrusCurrentColorTable = batsrusDataRoot..'colortables/batsrus_current.txt';
-- local batsrusVelocityColorTable = batsrusDataRoot..'colortables/batsrus_transition.txt';
-- local batsrusTopologyColorTable = batsrusDataRoot..'colortables/batsrus_topology.txt';
local asherBatsrus = 'X:/CCMC/fieldlinesdata/batsrus/asher_static_seeds/downsampled4/';

local batsrusColorTables = 'X:/CCMC/fieldlinesdata/colortables/';

local fr = 'fullres/';
local d2 = 'downsampled2/';
local d4 = 'downsampled4/';

local lutzJulyEvent = 'X:/CCMC/fieldlinesdata/batsrus/lutz_july_event/';
local lutzArtificial = 'X:/CCMC/fieldlinesdata/batsrus/lutz_model_run/';

local lutzJulyBatsrusMagnetic = lutzJulyEvent..d4..'/magnetic_fieldlines/';
local lutzJulyBatsrusFlow = lutzJulyEvent..d4..'/velocity_flowlines/';

local lutzArtificialBatsMagFullRes = lutzArtificial..fr..'magnetic_fieldlines/';
local lutzArtificialBatsrusMagnetic = lutzArtificial..d4..'magnetic_fieldlines/';
local lutzArtificialBatsrusFlow = lutzArtificial..d4..'velocity_flowlines/';

local batsrusTemperatureColorTable = batsrusColorTables..'batsrus_temperature.txt';
local batsrusDensityColorTable = batsrusColorTables..'batsrus_density.txt';
local batsrusCurrentColorTable = batsrusColorTables..'batsrus_current.txt';
local batsrusVelocityColorTable = batsrusColorTables..'batsrus_velocity.txt';
local batsrusTopologyColorTable = batsrusColorTables..'batsrus_topology.txt';

return {
    {
        Name = "FL_BATSRUS_Asher_RandomStaticSeeds",
        Parent = "GSMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = asherBatsrus,
            -- StartStateOffset = 1080,
            -- StateStepSize = 100,
            -- MaxNumStates = 12,
            LoadBinariesAtRuntime = true,
            ColorTablePaths = {
                batsrusTemperatureColorTable,
                batsrusDensityColorTable,
                batsrusCurrentColorTable,
                batsrusVelocityColorTable,
                batsrusTopologyColorTable,
            },
            ColorTableMinMax = {
                {0, 1.3},
                {0, 60},
                {-0.015, 0.015},
                {150, 900},
                {0, 3},
            },
            ExtraMinMaxLimits = {
                {-1, 10},
                {-1, 150},
                {-0.12, 0.12},
                {0, 5000},
                {0, 3},
            },
            DefaultColor = {0.7,0.4,0,0.6},
            RadialDomainLimits = {0, 350},
            CartesianDomainLimits = {{-400, 60},{-80,80},{-32,32},},
        },
    },

----------------------LUTZ's JULY TRACES-------------------------
    {
        Name = "FL_BATSRUS_OpenClosed",
        Parent = "GSMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = lutzJulyBatsrusMagnetic..'open-closed/',
            -- StartStateOffset = 1908,
            -- StateStepSize = 100,
            -- MaxNumStates = 12,
            LoadBinariesAtRuntime = true,
            ColorTablePaths = {
                batsrusTemperatureColorTable,
                batsrusDensityColorTable,
                batsrusCurrentColorTable,
                batsrusVelocityColorTable,
                batsrusTopologyColorTable,
            },
            ColorTableMinMax = {
                {0, 100000000},
                {0, 60},
                {-0.015, 0.015},
                {150, 900},
                {0, 3},
            },
            ExtraMinMaxLimits = {
                {0, 1000000000},
                {-1, 150},
                {-0.2, 0.2},
                {0, 6000},
                {0, 3},
            },
            DefaultColor = {0.7,0.4,0,0.6},
            RadialDomainLimits = {0, 350},
            CartesianDomainLimits = {{-400, 60},{-80,80},{-32,32},},
        },
    },
    --------------------- VELOCITY FLOWLINES ------------------------
    {
        Name = "FL_BATSRUS_FlowLines",
        Parent = "GSMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = lutzJulyBatsrusFlow..'upstream/',
            -- StartStateOffset = 650,
            -- StateStepSize = 100,
            -- MaxNumStates = 12,
            LoadBinariesAtRuntime = true,
            ColorTablePaths = {
                batsrusTemperatureColorTable,
                batsrusDensityColorTable,
                batsrusCurrentColorTable,
                batsrusVelocityColorTable,
                batsrusTopologyColorTable,
            },
            ColorTableMinMax = {
                {0, 100000000},
                {0, 60},
                {-0.015, 0.015},
                {150, 900},
                {0, 3},
            },
            ExtraMinMaxLimits = {
                {0, 1000000000},
                {-1, 150},
                {-0.2, 0.2},
                {0, 6000},
                {0, 3},
            },
            DefaultColor = {0.7,0.4,0,0.6},
            RadialDomainLimits = {0, 350},
            CartesianDomainLimits = {{-400, 60},{-80,80},{-32,32},},
        },
    },



-------------------- ARTIFICIAL RUN -------------------------
    {
        Name = "FL_BATSRUS_Artificial_OpenClosed",
        Parent = "GSMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = lutzArtificialBatsrusMagnetic..'open-closed/',
            -- StartStateOffset = 1080,
            -- StateStepSize = 100,
            -- MaxNumStates = 12,
            LoadBinariesAtRuntime = true,
            ColorTablePaths = {
                batsrusTemperatureColorTable,
                batsrusDensityColorTable,
                batsrusCurrentColorTable,
                batsrusVelocityColorTable,
                batsrusTopologyColorTable,
            },
            ColorTableMinMax = {
                {0, 100000000},
                {0, 60},
                {-0.015, 0.015},
                {150, 550},
                {0, 3},
            },
            ExtraMinMaxLimits = {
                {0, 1000000000},
                {-1, 150},
                {-0.2, 0.2},
                {0, 6000},
                {0, 3},
            },
            DefaultColor = {0.7,0.4,0,0.6},
            RadialDomainLimits = {0, 350},
            CartesianDomainLimits = {{-400, 60},{-80,80},{-32,32},},
        },
    },
    {
        Name = "FL_BATSRUS_Artificial_ReconnectionDayside",
        Parent = "GSMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = lutzArtificialBatsMagFullRes..'user_selected/',
            -- StartStateOffset = 1080,
            -- StateStepSize = 100,
            -- MaxNumStates = 12,
            LoadBinariesAtRuntime = true,
            ColorTablePaths = {
                batsrusTemperatureColorTable,
                batsrusDensityColorTable,
                batsrusCurrentColorTable,
                batsrusVelocityColorTable,
                batsrusTopologyColorTable,
            },
            ColorTableMinMax = {
                {0, 100000000},
                {0, 60},
                {-0.015, 0.015},
                {150, 550},
                {0, 3},
            },
            ExtraMinMaxLimits = {
                {0, 1000000000},
                {-1, 150},
                {-0.2, 0.2},
                {0, 6000},
                {0, 3},
            },
            DefaultColor = {0.7,0.4,0,0.6},
            RadialDomainLimits = {0, 350},
            CartesianDomainLimits = {{-400, 60},{-80,80},{-32,32},},
        },
    },
    {
        Name = "FL_BATSRUS_Artificial_throughout",
        Parent = "GSMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = lutzArtificialBatsrusMagnetic..'throughout/',
            -- StartStateOffset = 1080,
            -- StateStepSize = 100,
            -- MaxNumStates = 12,
            LoadBinariesAtRuntime = true,
            ColorTablePaths = {
                batsrusTemperatureColorTable,
                batsrusDensityColorTable,
                batsrusCurrentColorTable,
                batsrusVelocityColorTable,
                batsrusTopologyColorTable,
            },
            ColorTableMinMax = {
                {0, 100000000},
                {0, 60},
                {-0.015, 0.015},
                {150, 550},
                {0, 3},
            },
            ExtraMinMaxLimits = {
                {0, 1000000000},
                {-1, 150},
                {-0.2, 0.2},
                {0, 6000},
                {0, 3},
            },
            DefaultColor = {0.7,0.4,0,0.6},
            RadialDomainLimits = {0, 350},
            CartesianDomainLimits = {{-400, 60},{-80,80},{-32,32},},
        },
    },
    --------------- VELOCITY FLOWLINES --------------------
    {
        Name = "FL_BATSRUS_Artificial_FlowLines",
        Parent = "GSMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = lutzArtificialBatsrusFlow..'upstream/',
            -- StartStateOffset = 1080,
            -- StateStepSize = 100,
            -- MaxNumStates = 12,
            LoadBinariesAtRuntime = true,
            ColorTablePaths = {
                batsrusTemperatureColorTable,
                batsrusDensityColorTable,
                batsrusCurrentColorTable,
                batsrusVelocityColorTable,
                batsrusTopologyColorTable,
            },
            ColorTableMinMax = {
                {0, 100000000},
                {0, 4.5},
                {-0.015, 0.015},
                {150, 550},
                {0, 3},
            },
            ExtraMinMaxLimits = {
                {0, 1000000000},
                {-1, 150},
                {-0.2, 0.2},
                {0, 6000},
                {0, 3},
            },
            DefaultColor = {0.7,0.4,0,0.6},
            RadialDomainLimits = {0, 350},
            CartesianDomainLimits = {{-400, 60},{-80,80},{-32,32},},
        },
    },




----------------------------------------SINGLE TIMESTEP-------------------------------------------------
    {
        Name = "FL_BATSRUS_Artificial_SingleStepFlowLines",
        Parent = "GSMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = lutzArtificialBatsrusFlow..'single-timestep/',
            -- LoadBinariesAtRuntime = true,
            ColorTablePaths = {
                batsrusTemperatureColorTable,
                batsrusDensityColorTable,
                batsrusCurrentColorTable,
                batsrusVelocityColorTable,
                batsrusTopologyColorTable,
            },
            ColorTableMinMax = {
                {0, 100000000},
                {0, 4.5},
                {-0.015, 0.015},
                {150, 550},
                {0, 3},
            },
            ExtraMinMaxLimits = {
                {0, 1000000000},
                {-1, 150},
                {-0.2, 0.2},
                {0, 6000},
                {0, 3},
            },
            DefaultColor = {0.7,0.4,0,0.6},
            RadialDomainLimits = {0, 350},
            CartesianDomainLimits = {{-400, 60},{-80,80},{-32,32},},
        },
    },
    {
        Name = "FL_BATSRUS_Artificial_SingleStepOpenClosed",
        Parent = "GSMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = lutzArtificialBatsrusMagnetic..'single-timestep/',
            -- LoadBinariesAtRuntime = true,
            ColorTablePaths = {
                batsrusTemperatureColorTable,
                batsrusDensityColorTable,
                batsrusCurrentColorTable,
                batsrusVelocityColorTable,
                batsrusTopologyColorTable,
            },
            ColorTableMinMax = {
                {0, 100000000},
                {0, 4.5},
                {-0.015, 0.015},
                {150, 550},
                {0, 3},
            },
            ExtraMinMaxLimits = {
                {0, 1000000000},
                {-1, 150},
                {-0.2, 0.2},
                {0, 6000},
                {0, 3},
            },
            DefaultColor = {0.7,0.4,0,0.6},
            RadialDomainLimits = {0, 350},
            CartesianDomainLimits = {{-400, 60},{-80,80},{-32,32},},
        },
    },

--------------------------------------------------------------------------------------------------------

}
