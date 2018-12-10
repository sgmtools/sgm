#ifndef SGM_ENUMS_H
#define SGM_ENUMS_H

namespace SGM
    {
    enum EntityType
        {
        ThingType=0,

        // Topology

        AssemblyType,
        ReferenceType,
        ComplexType,
        BodyType,
        VolumeType,
        FaceType,
        EdgeType,
        VertexType,

        CurveType,

        LineType,
        CircleType,
        EllipseType,
        ParabolaType,
        HyperbolaType,
        NUBCurveType,
        NURBCurveType,
        PointCurveType,
        HelixCurveType,
        HermiteCurveType,
        TorusKnotCurveType,

        SurfaceType,

        PlaneType,
        CylinderType,
        ConeType,
        SphereType,
        TorusType,
        NUBSurfaceType,
        NURBSurfaceType, 
        RevolveType,    
        ExtrudeType,
        OffsetType,

        // Attributes
        
        AttributeType,
        StringAttributeType,
        IntegerAttributeType,
        DoubleAttributeType,
        CharAttributeType
        };

    enum TorusKindType
        {
        AppleType,
        LemonType,
        PinchedType,
        DonutType
        };

    enum IntersectionType
        {
        PointType,
        TangentType,
        CoincidentType,
        };

    enum EdgeSideType
        {
        FaceOnLeftType,
        FaceOnRightType,
        FaceOnBothSidesType,
        FaceOnUnknown,
        };

    enum EdgeSeamType
        {
        NotASeamType,
        LowerUSeamType,
        UpperUSeamType,
        LowerVSeamType,
        UpperVSeamType,
        };

    enum ResultType 
        {
        ResultTypeOK=0,             // Every thing is OK.
        ResultInterrupt,            // User requests the modeler to return.
        ResultTypeFileOpen,         // A file could not be found or openned.
        ResultTypeUnknownType,      // An unknown data type was found in a file.
        ResultTypeInsufficientData, // Not enough data was given to the function.
        ResultTypeUnknownCommand,   // An unknown script command was used.
        ResultTypeUnknownFileType,  // An unknown file type was sent to ReadFile.
        ResultTypeUnknownEntityID,  // No matching entity for given ID.
        ResultTypeInconsistentData, // Inside polygons are not inside outside polygons.
        ResultTypeDeleteWillCorruptModel, // Deleting an entity would corrupt the database
        };

    enum LogType
        {
        LogCreate,      // Indicates that the entity was created for an unknown reason.
        LogDelete,      // Indicates that the entity was deleted for an unknown reason.
        LogBottom,      // Bottom Face of a primitive.
        LogTop,         // Top Face of a primitive.
        LogMain,        // Main Face of a primitive.
        LogFront,       // Front Face of a block, i.e. -Y.
        LogBack,        // Back Face of a block, i.e. +Y.
        LogLeft,        // Left Face of a block, i.e. -X.
        LogRight        // Right Face of a block, i.e. +X.
        };

    enum FileType
        {
        SGMFileType,     // SGM internal file format.
        STLFileType,     // STL file.
        STEPFileType,    // STEP file either stp or step.
        UnknownFileType 
        };

    } // End of SGM namespace

#endif // SGM_ENUMS_H
