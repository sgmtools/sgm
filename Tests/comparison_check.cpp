#include <limits>
#include <gtest/gtest.h>

#include "SGMEntityFunctions.h"
#include "SGMPrimitives.h"
#include "SGMTopology.h"
#include "SGMGeometry.h"
#include "SGMInterrogate.h"
#include "SGMIntersector.h"
#include "SGMTranslators.h"

#include "test_utility.h"

void SetupFaces(SGM::Result &rResult,
	SGM::Face &Face1,
	SGM::Face &Translated,
	SGM::Face &Different,
    SGM::Face &Scaled,
    SGM::Face &Mirrored)
{
	SGM::Point3D Origin1(0, 0, 1);
	SGM::Point3D Origin2(0, 0, -1);
	SGM::Point3D Origin3(0, 0, 2);
    SGM::Point3D Origin4(1, 1, -3);
    SGM::Point3D Origin5(0, 0, 1.5);
	SGM::UnitVector3D Normal(0, 0, 1);
	std::vector<SGM::EdgeSideType> aTypes = { SGM::FaceOnLeftType, SGM::FaceOnLeftType, SGM::FaceOnLeftType, SGM::FaceOnLeftType };

	std::vector<SGM::Point3D> aPoints1 = { {1, 1, 1},  {2, 1, 1},  {2.1, 1.1, 1},  {1.1, 1.2, 1} };
	std::vector<SGM::Edge> aEdges1(aPoints1.size());
	for (size_t iPoint = 0; iPoint < aPoints1.size(); ++iPoint)
		aEdges1[iPoint] = SGM::CreateLinearEdge(rResult, aPoints1[iPoint], aPoints1[(iPoint + 1) % 4]);
	SGM::Surface Plane1 = SGM::CreatePlane(rResult, Origin1, Normal);
	SGM::Body Body1 = SGM::CreateSheetBody(rResult, Plane1, aEdges1, aTypes);

	std::vector<SGM::Point3D> aPoints2 = { {1, 1, -1}, {2, 1, -1}, {2.1, 1.1, -1}, {1.1, 1.2, -1} };
	std::vector<SGM::Edge> aEdges2(aPoints2.size());
	for (size_t iPoint = 0; iPoint < aPoints2.size(); ++iPoint)
		aEdges2[iPoint] = SGM::CreateLinearEdge(rResult, aPoints2[iPoint], aPoints2[(iPoint + 1) % 4]);
	SGM::Surface Plane2 = SGM::CreatePlane(rResult, Origin2, Normal);
	SGM::Body Body2 = SGM::CreateSheetBody(rResult, Plane2, aEdges2, aTypes);

	std::vector<SGM::Point3D> aPoints3 = { {1, 1, 2},  {2, 1, 2},  {2, 1.1, 2},    {1, 1.1, 2} };
	std::vector<SGM::Edge> aEdges3(aPoints3.size());
	for (size_t iPoint = 0; iPoint < aPoints3.size(); ++iPoint)
		aEdges3[iPoint] = SGM::CreateLinearEdge(rResult, aPoints3[iPoint], aPoints3[(iPoint + 1) % 4]);
	SGM::Surface Plane3 = SGM::CreatePlane(rResult, Origin3, Normal);
	SGM::Body Body3 = SGM::CreateSheetBody(rResult, Plane3, aEdges3, aTypes);

    std::vector<SGM::Point3D> aPoints4 = { {3, 3, -3},  {6, 3, -3},  {6.3, 3.3, -3},    {3.3, 3.6, -3} };
    std::vector<SGM::Edge> aEdges4(aPoints4.size());
    for (size_t iPoint = 0; iPoint < aPoints4.size(); ++iPoint)
        aEdges4[iPoint] = SGM::CreateLinearEdge(rResult, aPoints4[iPoint], aPoints4[(iPoint + 1) % 4]);
    SGM::Surface Plane4 = SGM::CreatePlane(rResult, Origin4, Normal);
    SGM::Body Body4 = SGM::CreateSheetBody(rResult, Plane4, aEdges4, aTypes);

    std::vector<SGM::Point3D> aPoints5 = { {1, 1, 1},  {0, 1, 1},  {-0.1, 1.1, 1},  {0.9, 1.2, 1} };
    std::vector<SGM::Edge> aEdges5(aPoints5.size());
    for (size_t iPoint = 0; iPoint < aPoints5.size(); ++iPoint)
        aEdges5[iPoint] = SGM::CreateLinearEdge(rResult, aPoints5[iPoint], aPoints5[(iPoint + 1) % 4]);
    SGM::Surface Plane5 = SGM::CreatePlane(rResult, Origin5, Normal);
    aTypes = { SGM::FaceOnRightType, SGM::FaceOnRightType, SGM::FaceOnRightType, SGM::FaceOnRightType };
    SGM::Body Body5 = SGM::CreateSheetBody(rResult, Plane5, aEdges5, aTypes);

    std::set<SGM::Face> sFaces;
	SGM::FindFaces(rResult, Body1, sFaces);
	Face1 = (*(sFaces.begin()));

	sFaces.clear();
	SGM::FindFaces(rResult, Body2, sFaces);
	Translated = (*(sFaces.begin()));

	sFaces.clear();
	SGM::FindFaces(rResult, Body3, sFaces);
	Different = (*(sFaces.begin()));

    sFaces.clear();
    SGM::FindFaces(rResult, Body4, sFaces);
    Scaled = (*(sFaces.begin()));

    sFaces.clear();
    SGM::FindFaces(rResult, Body5, sFaces);
    Mirrored = (*(sFaces.begin()));

}

//void SetupTwoBlocks(SGM::Result &rResult,
//                    SGM::Body &Block1,
//                    SGM::Face &Face1,
//                    SGM::Body &Block2,
//                    SGM::Face &Face2)
//{
//    SGM::Point3D Point1(1, 1, 1);
//    SGM::Point3D Point2(2, 1.1, 1.2);
//    SGM::Point3D Point3(3, 1.2, 1.4);
//    Block1 = SGM::CreateBlock(rResult, Point1, Point2);
//    Block2 = SGM::CreateBlock(rResult, Point2, Point3);
//
//    std::set<SGM::Face> sFaces;
//    SGM::FindFaces(rResult, Block1, sFaces);
//    Face1= (*(sFaces.begin()));
//
//    sFaces.clear();
//    SGM::FindFaces(rResult, Block2, sFaces);
//    Face2= (*(sFaces.begin()));
//}

TEST(comparison_check, face_comparison)
{
	SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
	SGM::Result rResult(pThing);

    SGM::Face Face1, Translated, Different, Scaled, Mirrored;
    SetupFaces(rResult, Face1, Translated, Different, Scaled, Mirrored);

    bool bCheckScale = true;
	std::vector<SGM::Face> aSimilar;
	SGM::FindSimilarFaces(rResult, Face1, aSimilar, bCheckScale);
    EXPECT_EQ(aSimilar.size(), 2U);

    aSimilar.clear();
    bCheckScale = false;
    SGM::FindSimilarFaces(rResult, Face1, aSimilar, bCheckScale);
    EXPECT_EQ(aSimilar.size(), 3U);

	SGMTesting::ReleaseTestThing(pThing);
}

TEST(comparison_check, compare_faces_bounded_by_circles)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    // create a sphere surface
    SGM::Surface SphereID = SGM::CreateSphereSurface(rResult, SGM::Point3D(0, 0, 0), 2.0);
    SGM::Surface PlaneID = SGM::CreatePlane(rResult, SGM::Point3D(1, 0, 0), SGM::UnitVector3D(1, 0, 0));
    std::vector<SGM::Curve> aCurves;
    SGM::IntersectSurfaces(rResult, SphereID, PlaneID, aCurves);

    EXPECT_EQ(aCurves.size(), 1);
    if (aCurves.size() == 1)
    {
        SGM::Edge EdgeID = SGM::CreateEdge(rResult, aCurves[0]);
        std::vector<SGM::Edge> aEdges;
        aEdges.emplace_back(EdgeID);
        std::vector<SGM::EdgeSideType> aTypes;
        aTypes.emplace_back(SGM::FaceOnLeftType);
        SGM::Body LargeSphereSheet = SGM::CreateSheetBody(rResult, SphereID, aEdges, aTypes);

        aTypes.clear();
        aTypes.emplace_back(SGM::FaceOnRightType);
        aEdges.clear();
        EdgeID = SGM::CreateEdge(rResult, aCurves[0]);
        aEdges.emplace_back(EdgeID);
        SGM::CreateSheetBody(rResult, SphereID, aEdges, aTypes);

        aTypes.clear();
        aTypes.emplace_back(SGM::FaceOnLeftType);
        aEdges.clear();
        EdgeID = SGM::CreateEdge(rResult, aCurves[0]);
        aEdges.emplace_back(EdgeID);
        SGM::CreateSheetBody(rResult, PlaneID, aEdges, aTypes);

        std::set<SGM::Face> sFaces;
        SGM::FindFaces(rResult, LargeSphereSheet, sFaces);
        EXPECT_EQ(sFaces.size(), 1);
        SGM::Face Face1 = (*(sFaces.begin()));

        bool bCheckScale = true;
        std::vector<SGM::Face> aSimilar;
        SGM::FindSimilarFaces(rResult, Face1, aSimilar, bCheckScale);
        EXPECT_EQ(aSimilar.size(), 0U);
    }

    SGMTesting::ReleaseTestThing(pThing);
}

TEST(comparison_check, compare_spheres)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    SGM::Body SphereID = SGM::CreateSphere(rResult, SGM::Point3D(0,0,0), 1);
    SGM::CreateSphere(rResult, SGM::Point3D(5,-1,1), 1);
    SGM::CreateSphere(rResult, SGM::Point3D(7,3,3), 2.5);

    std::set<SGM::Face> sFaces;
    SGM::FindFaces(rResult, SphereID, sFaces);
    ASSERT_EQ(sFaces.size(), 1);

    std::vector<SGM::Face> aSimilar;
    bool bCheckScale = true;
    SGM::FindSimilarFaces(rResult, *sFaces.begin(), aSimilar, bCheckScale);
    EXPECT_EQ(aSimilar.size(), 1);

    bCheckScale = false;
    aSimilar.clear();
    SGM::FindSimilarFaces(rResult, *sFaces.begin(), aSimilar, bCheckScale);
    EXPECT_EQ(aSimilar.size(), 2);

    SGMTesting::ReleaseTestThing(pThing);
}

TEST(comparison_check, compare_torii)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    SGM::Body TorusID = SGM::CreateTorus(rResult, SGM::Point3D(0,0,0), SGM::UnitVector3D(0,0,1), 0.5, 4.0);
    SGM::CreateTorus(rResult, SGM::Point3D(9,0,-5), SGM::UnitVector3D(0,1,1), 0.5, 4.0);
    SGM::CreateTorus(rResult, SGM::Point3D(0,0,0), SGM::UnitVector3D(0,0,1), 0.25, 2.0); //uniform scaling of major and minor radii

    std::set<SGM::Face> sFaces;
    SGM::FindFaces(rResult, TorusID, sFaces);
    ASSERT_EQ(sFaces.size(), 1);

    std::vector<SGM::Face> aSimilar;
    bool bCheckScale = true;
    SGM::FindSimilarFaces(rResult, *sFaces.begin(), aSimilar, bCheckScale);
    EXPECT_EQ(aSimilar.size(), 1);

    bCheckScale = false;
    aSimilar.clear();
    SGM::FindSimilarFaces(rResult, *(sFaces.begin()), aSimilar, bCheckScale);
    EXPECT_EQ(aSimilar.size(), 2);

    SGMTesting::ReleaseTestThing(pThing);
}

TEST(DISABLED_comparison_check, compare_left_and_right)
{
    SGMInternal::thing *pThing = SGMTesting::AcquireTestThing();
    SGM::Result rResult(pThing);

    std::vector<SGM::Entity> entities;
    std::vector<std::string> log;
    SGM::TranslatorOptions const options;

    std::string right_file("right.stp");
    std::string left_file("left.stp");
    std::string file_path = get_models_file_path(right_file);
    SGM::ReadFile(rResult, file_path, entities, log, options);
    auto resultType = rResult.GetResult();
    ASSERT_EQ(resultType, SGM::ResultTypeOK);

    file_path = get_models_file_path(left_file);
    SGM::ReadFile(rResult, file_path, entities, log, options);
    resultType = rResult.GetResult();
    ASSERT_EQ(resultType, SGM::ResultTypeOK);

    //std::set<SGM::Body> sBodies;
    //SGM::FindBodies(rResult, SGM::Thing().m_ID, sBodies);
    //bool bCheckScale = true;
    //bool bCheckHanded = true;
    //std::vector<SGM::Body> aSimilar;
    //SGM::FindSimilarBodies(rResult, *(sBodies.begin()), aSimilar, bCheckScale, bCheckHanded);
    //EXPECT_EQ(aSimilar.size(), 0);

    //bCheckScale = true; 
    //bCheckHanded = false;
    //aSimilar.clear();
    //SGM::FindSimilarBodies(rResult, *(sBodies.begin()), aSimilar, bCheckScale, bCheckHanded);
    //EXPECT_EQ(aSimilar.size(), 1);

    SGMTesting::ReleaseTestThing(pThing);
}
