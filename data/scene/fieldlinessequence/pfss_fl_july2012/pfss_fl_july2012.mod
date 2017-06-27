local pfssDataRoot = 'X:/CCMC/fieldlinesdata/pfss/leilas_solar_soft/';
local pfssColorTables = 'X:/CCMC/fieldlinesdata/colortables/';
local pfssTransitionColorTable = pfssColorTables ..'pfss_transition.txt';
local pfssTopologyColorTable = pfssColorTables ..'pfss_topology.txt';
local pfssBsignColorTable = pfssColorTables .. 'pfss_bsign.txt';

return {
    {
        Name = "FL_PFSS",
        Parent = "HNMReferenceFrame",
        Renderable = {
            Type = "RenderableFieldlinesSequence",
            TracingMethod = "PreTracedBinary",
            SourceFolder = pfssDataRoot,
            -- StartStateOffset = 515,
            -- StateStepSize = 100,
            -- MaxNumStates = 55,
            ColorTablePaths = {
                --pfssTransitionColorTable,
                pfssTopologyColorTable,
                pfssBsignColorTable,
            },
            ColorTableMinMax = {
                --{0, 1},
                {0, 2},
                {-1, 1},
            },
            ExtraMinMaxLimits = {
                --{-1, 1},
                {0, 2},
                {-1, 1},
            },
            DefaultColor = {0.3861,1.0,0.92174,0.18644},
            RadialDomainLimits = {0, 50},
            CartesianDomainLimits = {{-50, 50},{-50,50},{-50,50},},
            -- LoadBinariesAtRuntime = true,
        },
    },
}
