return {
    {
        Name = "HNMReferenceFrame",
        Parent = "SolarSystemBarycenter",
        Transform = {
            Rotation = {
                Type = "SpiceRotation",
                SourceFrame = "HEEQ180",
                DestinationFrame = "GALACTIC",
                Kernels = "${OPENSPACE_DATA}/scene/fieldlinessequence/customkernels/HNM-enlil_ref_frame.tf",
            },
        },
    },
}
