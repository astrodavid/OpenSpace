return {
    -- Barycenter module
    {
        Name = "VenusBarycenter",
        Parent = "SolarSystemBarycenter",
        Transform = {
            Translation = {
                Type = "SpiceTranslation",
                Target = "VENUS BARYCENTER",
                Observer = "SUN",
                Kernels = "${OPENSPACE_DATA}/spice/de430_1850-2150.bsp"
            }
        },
        GuiPath = "/Solar System/Planets/Venus"
    },
    -- RenderableGlobe module
    {   
        Name = "Venus",
        Parent = "VenusBarycenter",
        Transform = {
            Rotation = {
                Type = "SpiceRotation",
                SourceFrame = "IAU_VENUS",
                DestinationFrame = "GALACTIC"
            },
            Translation = {
                Type = "SpiceTranslation",
                Target = "VENUS",
                Observer = "VENUS BARYCENTER",
                Kernels = "${OPENSPACE_DATA}/spice/de430_1850-2150.bsp"
            }
        },
        Renderable = {
            Type = "RenderableGlobe",
            Radii = { 6051900, 6051900, 6051800 },
            SegmentsPerPatch = 64,
            Layers = {
                ColorLayers = {
                    {
                        Name = "Venus Texture",
                        FilePath = "textures/venus.jpg",
                        Enabled = true
                    }
                }
            }
        },
        Tag = { "planet_solarSystem", "planet_terrestrial" },
        GuiPath = "/Solar System/Planets/Venus"
    },
    -- Trail module
    {   
        Name = "VenusTrail",
        Parent = "SolarSystemBarycenter",
        Renderable = {
            Type = "RenderableTrailOrbit",
            Translation = {
                Type = "SpiceTranslation",
                Target = "VENUS BARYCENTER",
                Observer = "SUN"
            },
            Color = { 1.0, 0.5, 0.2 },
            Period = 224.695,
            Resolution = 1000
        },
        Tag = { "planetTrail_solarSystem", "planetTrail_terrestrial" },
        GuiPath = "/Solar System/Planets/Venus"
    }
}
