return {
    {
        Name = "Rhea",
        Parent = "SaturnBarycenter",
        Renderable = {
            Type = "RenderableGlobe",
            Radii = 765000,
            SegmentsPerPatch = 64,
            Layers = {
                ColorLayers = {
                    {
                        Name = "Rhea Texture",
                        FilePath = "textures/rhea.jpg",
                        Enabled = true
                    }
                }
            }
        },
        Transform = {
            Translation = {
                Type = "SpiceTranslation",
                Target = "RHEA",
                Observer = "SATURN BARYCENTER",
                Kernels = "${OPENSPACE_DATA}/spice/sat375.bsp"
            },
            Rotation = {
                Type = "SpiceRotation",
                SourceFrame = "IAU_ENCELADUS",
                DestinationFrame = "GALACTIC"
            }
        },
        GuiPath = "/Solar System/Planets/Saturn/Moons"
    },
    {
        Name = "RheaTrail",
        Parent = "SaturnBarycenter",
        Renderable = {
            Type = "RenderableTrailOrbit",
            Translation = {
                Type = "SpiceTranslation",
                Target = "RHEA",
                Observer = "SATURN BARYCENTER",
            },
            Color = { 0.5, 0.3, 0.3 },
            Period = 108 / 24,
            Resolution = 1000
        },
        GuiPath = "/Solar System/Planets/Saturn/Moons"
    }
}
