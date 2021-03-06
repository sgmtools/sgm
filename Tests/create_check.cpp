#include <limits>
#include <string>
#include <gtest/gtest.h>
#include <FacetToBRep.h>
#include <SGMDisplay.h>

#include "SGMGeometry.h"
#include "SGMEntityFunctions.h"
#include "SGMTransform.h"
#include "SGMAttribute.h"
#include "SGMInterval.h"
#include "SGMTopology.h"
#include "SGMPrimitives.h"
#include "SGMComplex.h"

#include "test_utility.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"
#endif

TEST(create_check, voxels)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    std::vector<SGM::Point3D> aPoints1={{0,0,0},{1,1,1},{1,0,1}};
    SGM::CreateVoxels(rResult,aPoints1,1,true);
    std::vector<SGM::Point3D> aPoints2={{1,0,0},{0,1,1},{0,0,1}};
    SGM::CreateVoxels(rResult,aPoints2,1,false);

    SGMTesting::ReleaseTestThing(pThing);
} 

TEST(create_check, output_segments_to_sgm)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    std::vector<SGM::Point3D> aPoints;
    aPoints.push_back(SGM::Point3D(0,0,0));
    aPoints.push_back(SGM::Point3D(10,0,0));
    aPoints.push_back(SGM::Point3D(10,10,0));
    aPoints.push_back(SGM::Point3D(0,10,0));
    std::vector<unsigned> aSegements,aTriangles;
    aSegements.push_back(0);
    aSegements.push_back(1);
    aSegements.push_back(1);
    aSegements.push_back(2);
    aSegements.push_back(2);
    aSegements.push_back(3);
    aSegements.push_back(3);
    aSegements.push_back(0);

    SGM::Complex ComplexID=SGM::CreateComplex(rResult,aPoints,aSegements,aTriangles);

    SGM::SaveSGM(rResult,"Segments.sgm",ComplexID,SGM::TranslatorOptions());

    SGMTesting::ReleaseTestThing(pThing);
} 

TEST(create_check, offset_surface)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    std::vector<std::vector<SGM::Point3D> > aaPoints;
    size_t Index1,Index2;
    double dStep=SGM_PI/8.0;
    for(Index1=0;Index1<49;++Index1)
        {
        double x=Index1*dStep;
        std::vector<SGM::Point3D> aPoints;
        for(Index2=0;Index2<49;++Index2)
            {
            double y=Index2*dStep;
            double z=sin(x)*cos(y);
            SGM::Point3D Pos(x,y,z);
            aPoints.push_back(Pos);
            }
        aaPoints.push_back(aPoints);
        }
    SGM::Surface SurfaceID=SGM::CreateNUBSurface(rResult,aaPoints);
    SGM::Surface OffsetID=SGM::CreateOffsetSurface(rResult,SurfaceID,0.1);

    std::vector<SGM::Point3D> aTemp;
    for(Index1=0;Index1<49;++Index1)
        {
        double u=Index1/48.0;
        for(Index2=0;Index2<49;++Index2)
            {
            double v=Index2/48.0;
            SGM::Point3D Pos;
            SGM::EvaluateSurface(rResult,OffsetID,SGM::Point2D(u,v),&Pos);
            aTemp.push_back(Pos);
            }
       }
    SGM::CreatePoints(rResult,aTemp);
    SGM::TestSurface(rResult,OffsetID,SGM::Point2D(0.1,0.2));

    SGMTesting::ReleaseTestThing(pThing);
} 

TEST(create_check, cover_wire_body)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    std::set<SGM::Edge> sEdges;
    sEdges.insert(SGM::CreateLinearEdge(rResult,SGM::Point3D(0,0,0),SGM::Point3D(10,0,0)));
    sEdges.insert(SGM::CreateLinearEdge(rResult,SGM::Point3D(0,10,0),SGM::Point3D(10,10,0)));
    sEdges.insert(SGM::CreateLinearEdge(rResult,SGM::Point3D(0,0,0),SGM::Point3D(0,10,0)));
    sEdges.insert(SGM::CreateLinearEdge(rResult,SGM::Point3D(10,0,0),SGM::Point3D(10,10,0)));
    SGM::Body WireID=SGM::CreateWireBody(rResult,sEdges);
    SGM::CopyEntity(rResult,WireID);
    SGM::CoverPlanarWire(rResult,WireID);

    std::set<SGM::Edge> sOutEdges;
    SGM::FindWireEdges(rResult,WireID,sOutEdges);
    EXPECT_EQ(sOutEdges.size(),4U);

    SGMTesting::ReleaseTestThing(pThing);
} 

TEST(create_check, create_cylinder_from_entities)
    {
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    // Create a cylinder about the Z-axis with bottom at (0,0,0), 
    // top at (0,0,2) and radius 1.0, by making all the topology and geometry.

    // Create the topology.

    SGM::Body BodyID=SGM::CreateBody(rResult);
    SGM::Volume VolumeID=SGM::CreateVolume(rResult);
    SGM::Face TopFaceID=SGM::CreateFace(rResult);
    SGM::Face SideFaceID=SGM::CreateFace(rResult);
    SGM::Face BottomFaceID=SGM::CreateFace(rResult);
    SGM::Edge TopEdge=SGM::CreateEdge(rResult);
    SGM::Edge BottomEdge=SGM::CreateEdge(rResult);

    // Ask for points entities of a face with no surface
    SGM::FindPointEntities(rResult,SideFaceID);
    EXPECT_NE(rResult.GetResult(), SGM::ResultTypeOK);

    // Create the curves.

    SGM::Point3D TopCenter(0,0,2),BottomCenter(0,0,0);
    SGM::UnitVector3D Axis(0,0,1);
    double dRadius=1.0;
    SGM::Curve TopCurve=SGM::CreateCircle(rResult,TopCenter,Axis,dRadius);
    SGM::Curve BottomCurve=SGM::CreateCircle(rResult,BottomCenter,Axis,dRadius);

    // Create the surfaces.

    SGM::Surface TopSurface=SGM::CreatePlane(rResult,TopCenter,
        TopCenter+SGM::Vector3D(1,0,0),TopCenter+SGM::Vector3D(0,1,0));
    SGM::Surface BottomSurface=SGM::CreatePlane(rResult,BottomCenter,
        BottomCenter+SGM::Vector3D(1,0,0),BottomCenter+SGM::Vector3D(0,1,0));
    SGM::Surface SideSurface=SGM::CreateCylinderSurface(rResult,BottomCenter,TopCenter,dRadius);

    // Hook things up.

    SGM::AddVolumeToBody(rResult,VolumeID,BodyID);
    SGM::AddFaceToVolume(rResult,TopFaceID,VolumeID);
    SGM::AddFaceToVolume(rResult,SideFaceID,VolumeID);
    SGM::AddFaceToVolume(rResult,BottomFaceID,VolumeID);
    SGM::AddEdgeToFace(rResult,TopEdge,SGM::EdgeSideType::FaceOnLeftType,TopFaceID);
    SGM::AddEdgeToFace(rResult,BottomEdge,SGM::EdgeSideType::FaceOnRightType,BottomFaceID);
    SGM::AddEdgeToFace(rResult,TopEdge,SGM::EdgeSideType::FaceOnRightType,SideFaceID);
    SGM::AddEdgeToFace(rResult,BottomEdge,SGM::EdgeSideType::FaceOnLeftType,SideFaceID);
    SGM::SetCurveOfEdge(rResult,TopCurve,TopEdge);
    SGM::SetCurveOfEdge(rResult,BottomCurve,BottomEdge);
    SGM::SetSurfaceOfFace(rResult,TopSurface,TopFaceID);
    SGM::SetSurfaceOfFace(rResult,BottomSurface,BottomFaceID);
    SGM::SetSurfaceOfFace(rResult,SideSurface,SideFaceID);
    SGM::SetFlippedOfFace(rResult,true,BottomFaceID);

    // Ask for points entities of the side face
    SGM::FindPointEntities(rResult,SideFaceID);
    SGM::GetFacePoints2D(rResult,SideFaceID);

    SGMTesting::ReleaseTestThing(pThing);
    }

TEST(create_check, create_parabola)
    {
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    double dTolerance = SGM_MIN_TOL;
    std::vector<SGM::Point3D> aPoints {{0.0, 0.0, 0.0},
                                       {1.0, 2.0, 0.0},
                                       {-1.0, 2.0, 0.0},
                                       {2.0, 8.0, 0.0},
                                       {-3.0, 18.0, 0.0}};
    aPoints.reserve(5);

    // y=ax^2 parabola
    // a=2

    SGM::Curve CurveID=SGM::FindConic(rResult,aPoints,dTolerance);
    EXPECT_TRUE(SGM::TestCurve(rResult,CurveID,0.1));

    SGM::Curve CopyID=SGM::Curve(SGM::CopyEntity(rResult,CurveID).m_ID);
    SGM::Transform3D Trans(SGM::Vector3D(1,1,1));
    SGM::TransformEntity(rResult,Trans,CopyID);
    EXPECT_TRUE(SGM::TestCurve(rResult,CopyID,0.1));
    SGM::DeleteEntity(rResult,CopyID);
    EXPECT_EQ(rResult.GetResult(), SGM::ResultTypeOK);

    SGM::SaveSGM(rResult,"CoverageTest.sgm",SGM::Thing(),SGM::TranslatorOptions());
    
    SGM::DeleteEntity(rResult,CurveID);
    EXPECT_EQ(rResult.GetResult(), SGM::ResultTypeOK);
    SGMTesting::ReleaseTestThing(pThing);
    }

TEST(create_check, create_hyperbola)
    {
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    double dTolerance = SGM_MIN_TOL;
    std::vector<SGM::Point3D> aPoints;
    aPoints.reserve(5);

    // x^2/a^2-y^2/b^2=1 hyperbola
    // a=2 b=3

    aPoints.clear();

    aPoints.emplace_back(2.0, 0.0, 0.0);
    aPoints.emplace_back(3.0, 3.3541019662496845446137605030969, 0.0);
    aPoints.emplace_back(6.0, 8.4852813742385702928101323452582, 0.0);
    aPoints.emplace_back(6.0, -8.4852813742385702928101323452582, 0.0);
    aPoints.emplace_back(3.0, -3.3541019662496845446137605030969, 0.0);

    SGM::Curve CurveID=SGM::FindConic(rResult,aPoints,dTolerance);
    EXPECT_TRUE(SGM::TestCurve(rResult,CurveID,0.1));
    
    SGM::Curve CopyID=SGM::Curve(SGM::CopyEntity(rResult,CurveID).m_ID);
    SGM::Transform3D Trans(SGM::Vector3D(1,1,1));
    SGM::TransformEntity(rResult,Trans,CopyID);
    EXPECT_TRUE(SGM::TestCurve(rResult,CopyID,0.1));
    SGM::DeleteEntity(rResult,CopyID);
    EXPECT_EQ(rResult.GetResult(), SGM::ResultTypeOK);

    SGM::SaveSGM(rResult,"CoverageTest.sgm",SGM::Thing(),SGM::TranslatorOptions());

    SGM::DeleteEntity(rResult,CurveID);
    EXPECT_EQ(rResult.GetResult(), SGM::ResultTypeOK);
    SGMTesting::ReleaseTestThing(pThing);
    }


TEST(create_check, create_ellipse)
    {
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    double dTolerance = SGM_MIN_TOL;
    std::vector<SGM::Point3D> aPoints;
    aPoints.reserve(5);

    // x^2/a^2+y^2/b^2=1 ellipse
    // a=2 b=3

    aPoints.clear();

    aPoints.emplace_back(0.0, 3.0, 0.0);
    aPoints.emplace_back(2.0, 0.0, 0.0);
    aPoints.emplace_back(-2.0, 0.0, 0.0);
    aPoints.emplace_back(0.0, -3.0, 0.0);
    aPoints.emplace_back(1.0, 2.5980762113533159402911695122588, 0.0);

    SGM::Curve CurveID=SGM::FindConic(rResult,aPoints,dTolerance);
    EXPECT_TRUE(SGM::TestCurve(rResult,CurveID,0.1));
    
    SGM::Curve CopyID=SGM::Curve(SGM::CopyEntity(rResult,CurveID).m_ID);
    SGM::Transform3D Trans(SGM::Vector3D(1,1,1));
    SGM::TransformEntity(rResult,Trans,CopyID);
    EXPECT_TRUE(SGM::TestCurve(rResult,CopyID,0.1));
    SGM::DeleteEntity(rResult,CopyID);
    EXPECT_EQ(rResult.GetResult(), SGM::ResultTypeOK);

    SGM::SaveSGM(rResult,"CoverageTest.sgm",SGM::Thing(),SGM::TranslatorOptions());

    SGM::DeleteEntity(rResult,CurveID);
    EXPECT_EQ(rResult.GetResult(), SGM::ResultTypeOK);
    SGMTesting::ReleaseTestThing(pThing);
    }

TEST(create_check, create_torus_knot)
    {
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);
        
    SGM::Point3D Center(0,0,0);
    SGM::UnitVector3D XAxis(0,1,0),YAxis(1,0,0);
    size_t nA=2,nB=3;
    double dR=5.0,dr=2;

    SGM::Curve IDCurve = SGM::CreateTorusKnot(rResult, Center, XAxis, YAxis, dr, dR, nA, nB);
    EXPECT_TRUE(SGM::TestCurve(rResult,IDCurve,1.0));
    SGM::Edge Edge1 = SGM::CreateEdge(rResult, IDCurve);

    SGM::Curve IDCurve2 = SGM::Curve(SGM::CopyEntity(rResult, IDCurve).m_ID);
    SGM::Transform3D Trans;
    SGM::Point3D Origin(0, 0, 0);
    SGM::UnitVector3D Axis(0, 0, 1);
    double dAngle = 0.52359877559829887307710723054658; // 30 degrees.
    SGM::Rotate(Origin, Axis, dAngle, Trans);
    SGM::TransformEntity(rResult, Trans, IDCurve2);
    SGM::Edge Edge2 = SGM::CreateEdge(rResult, IDCurve2);

    SGM::Curve IDCurveCopy=SGM::Curve(SGM::CopyEntity(rResult,IDCurve).m_ID);
    EXPECT_TRUE(SGM::SameCurve(rResult,IDCurve,IDCurveCopy,SGM_MIN_TOL));
    EXPECT_FALSE(SGM::SameCurve(rResult,IDCurve,IDCurve2,SGM_MIN_TOL));

    SGM::UnitVector3D Normal = XAxis * YAxis;
    SGM::Surface SurfaceID = SGM::CreateTorusSurface(rResult, Center, Normal, dr, dR);
    std::vector<SGM::Edge> aEdges;
    aEdges.push_back(Edge1);
    aEdges.push_back(Edge2);
    std::vector<SGM::EdgeSideType> aTypes;
    aTypes.push_back(SGM::EdgeSideType::FaceOnLeftType);
    aTypes.push_back(SGM::EdgeSideType::FaceOnRightType);
    SGM::Body BodyID = SGM::CreateSheetBody(rResult, SurfaceID, aEdges, aTypes);

    SGM::CheckOptions Options;
    std::vector<std::string> aCheckStrings;
    SGM::CheckEntity(rResult,BodyID,Options,aCheckStrings);

    SGM::SaveSGM(rResult,"CoverageTest.sgm",SGM::Thing(),SGM::TranslatorOptions());

    SGMTesting::ReleaseTestThing(pThing);
    }
    
TEST(create_check, create_attributes)
    {
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    SGM::Attribute AttributeID1=SGM::CreateAttribute(rResult,"AttributeTestName");
    EXPECT_EQ(SGM::GetAttributeType(rResult,AttributeID1),SGM::EntityType::AttributeType);

    std::vector<int> aIntegers(3,1);
    SGM::Attribute AttributeID2=SGM::CreateIntegerAttribute(rResult,"IntegerAttributeTestName",aIntegers);
    EXPECT_EQ(SGM::GetAttributeType(rResult,AttributeID2),SGM::EntityType::IntegerAttributeType);
    std::vector<int> const &aVectorIntData = SGM::GetIntegerAttributeData(rResult,AttributeID2);
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ(aVectorIntData[i], aIntegers[i]);

    std::vector<double> aDoubles(3,1.0);
    SGM::Attribute AttributeID3=SGM::CreateDoubleAttribute(rResult,"DoubleAttributeTestName",aDoubles);
    EXPECT_EQ(SGM::GetAttributeType(rResult,AttributeID3),SGM::EntityType::DoubleAttributeType);

    std::vector<char> aChars(3,'c');
    SGM::Attribute AttributeID4=SGM::CreateCharAttribute(rResult,"CharAttributeTestName",aChars);
    EXPECT_EQ(SGM::GetAttributeType(rResult,AttributeID4),SGM::EntityType::CharAttributeType);

    SGM::Attribute AttributeID5=SGM::CreateStringAttribute(rResult,"StringAttributeTestName","string_test");
    EXPECT_EQ(SGM::GetAttributeType(rResult,AttributeID5),SGM::EntityType::StringAttributeType);
    std::string testname = SGM::GetStringAttributeData(rResult,AttributeID5);
    EXPECT_EQ(testname, "string_test");

    SGM::Body BodyID=SGM::CreateBlock(rResult,SGM::Point3D(0,0,0),SGM::Point3D(10,10,10));
    SGM::AddAttribute(rResult,BodyID,AttributeID1);
    SGM::AddAttribute(rResult,BodyID,AttributeID2);
    SGM::AddAttribute(rResult,BodyID,AttributeID3);
    SGM::AddAttribute(rResult,BodyID,AttributeID4);
    SGM::AddAttribute(rResult,BodyID,AttributeID5);

    std::set<SGM::Attribute> sAttributes;
    SGM::GetAttributes(rResult,BodyID,sAttributes);
    EXPECT_EQ(sAttributes.size(),5U);

    SGM::SaveSGM(rResult,"CoverageTest.sgm",SGM::Thing(),SGM::TranslatorOptions());

    SGMTesting::ReleaseTestThing(pThing);
    }

TEST(create_check, miscellaneous_entity)
    {
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    // cloning a thing is not currently allowed
    EXPECT_THROW(pThing->Clone(rResult), std::logic_error);

    // other thing member functions
    EXPECT_TRUE(pThing->IsTopLevel());
    
    SGMTesting::ReleaseTestThing(pThing);
    }

TEST(create_check, visitors_get_box)
    {
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    EXPECT_TRUE(SGM::RunInternalTest(rResult,3));

    SGMTesting::ReleaseTestThing(pThing);
    }

TEST(create_check, create_offset)
    {
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    EXPECT_TRUE(SGM::RunInternalTest(rResult,5));

    SGMTesting::ReleaseTestThing(pThing);
    }

TEST(create_check, edge_copy)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    SGM::Point3D Origin(0,0,0);
    SGM::UnitVector3D Axis(1,0,0);
    SGM::Curve LineID = SGM::CreateLine(rResult, Origin, Axis);

    SGM::Interval1D Interval1(0,1);
    SGM::Interval1D Interval2(2,3);
    SGM::Edge EdgeID1 = SGM::CreateEdge(rResult, LineID, &Interval1);
    /* SGM::Edge EdgeID2 = */ SGM::CreateEdge(rResult, LineID, &Interval2);

    /* SGM::Entity Edge1Copy = */ SGM::CopyEntity(rResult, EdgeID1);

    EXPECT_TRUE(SGMTesting::CheckEntityAndPrintLog(rResult, SGM::Thing()));

    SGMTesting::ReleaseTestThing(pThing);
}

TEST(create_check, enum_type_names)
    {
    EXPECT_ANY_THROW(SGM::EntityTypeName((SGM::EntityType)-1));

    EXPECT_STREQ("assembly",SGM::EntityTypeName(SGM::AssemblyType));
    EXPECT_STREQ("reference",SGM::EntityTypeName(SGM::ReferenceType));
    EXPECT_STREQ("complex",SGM::EntityTypeName(SGM::ComplexType));
    EXPECT_STREQ("body",SGM::EntityTypeName(SGM::BodyType));
    EXPECT_STREQ("volume",SGM::EntityTypeName(SGM::VolumeType));
    EXPECT_STREQ("face",SGM::EntityTypeName(SGM::FaceType));
    EXPECT_STREQ("edge",SGM::EntityTypeName(SGM::EdgeType));
    EXPECT_STREQ("vertex",SGM::EntityTypeName(SGM::VertexType));
    EXPECT_STREQ("curve",SGM::EntityTypeName(SGM::CurveType));
    EXPECT_STREQ("line",SGM::EntityTypeName(SGM::LineType));
    EXPECT_STREQ("circle",SGM::EntityTypeName(SGM::CircleType));
    EXPECT_STREQ("ellipse",SGM::EntityTypeName(SGM::EllipseType));
    EXPECT_STREQ("parabola",SGM::EntityTypeName(SGM::ParabolaType));
    EXPECT_STREQ("hyperbola",SGM::EntityTypeName(SGM::HyperbolaType));
    EXPECT_STREQ("NUBcurve",SGM::EntityTypeName(SGM::NUBCurveType));
    EXPECT_STREQ("NURBcurve",SGM::EntityTypeName(SGM::NURBCurveType));
    EXPECT_STREQ("PointCurve",SGM::EntityTypeName(SGM::PointCurveType));
    EXPECT_STREQ("helix",SGM::EntityTypeName(SGM::HelixCurveType));
    EXPECT_STREQ("hermite",SGM::EntityTypeName(SGM::HermiteCurveType));
    EXPECT_STREQ("torus",SGM::EntityTypeName(SGM::TorusKnotCurveType));
    EXPECT_STREQ("surface",SGM::EntityTypeName(SGM::SurfaceType));
    EXPECT_STREQ("plane",SGM::EntityTypeName(SGM::PlaneType));
    EXPECT_STREQ("cylinder",SGM::EntityTypeName(SGM::CylinderType));
    EXPECT_STREQ("cone",SGM::EntityTypeName(SGM::ConeType));
    EXPECT_STREQ("sphere",SGM::EntityTypeName(SGM::SphereType));
    EXPECT_STREQ("torus",SGM::EntityTypeName(SGM::TorusType));
    EXPECT_STREQ("NUBsurface",SGM::EntityTypeName(SGM::NUBSurfaceType));
    EXPECT_STREQ("NURBsurface",SGM::EntityTypeName(SGM::NURBSurfaceType));
    EXPECT_STREQ("revolve",SGM::EntityTypeName(SGM::RevolveType));
    EXPECT_STREQ("extrude",SGM::EntityTypeName(SGM::ExtrudeType));
    EXPECT_STREQ("offset",SGM::EntityTypeName(SGM::OffsetType));
    EXPECT_STREQ("attribute",SGM::EntityTypeName(SGM::AttributeType));
    EXPECT_STREQ("StringAttribute",SGM::EntityTypeName(SGM::StringAttributeType));
    EXPECT_STREQ("IntegerAttribute",SGM::EntityTypeName(SGM::IntegerAttributeType));
    EXPECT_STREQ("DoubleAttribute",SGM::EntityTypeName(SGM::DoubleAttributeType));
    EXPECT_STREQ("CharAttribute",SGM::EntityTypeName(SGM::CharAttributeType));
    }

TEST(create_check, cone_sheet_body)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    SGM::Surface ConeID = SGM::CreateConeSurface(rResult, SGM::Point3D(5,5,0), SGM::UnitVector3D(0,0,1), 2.0, SGM_PI/6.0);
    SGM::Interval2D Domain = SGM::GetDomainOfSurface(rResult, ConeID);
    Domain.m_VDomain.m_dMin = -Domain.m_VDomain.m_dMax;
    SGM::CreateSheetBody(rResult, ConeID, Domain);

    EXPECT_TRUE(SGMTesting::CheckEntityAndPrintLog(rResult, SGM::Thing()));

    SGMTesting::ReleaseTestThing(pThing);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
