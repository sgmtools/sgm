#ifndef SGM_COMPLEX_H
#define SGM_COMPLEX_H

#include "SGMEntityClasses.h"
#include "SGMVector.h"
#include "SGMResult.h"

#include "sgm_export.h"

namespace SGM
    {
    // Creation functions

    SGM_EXPORT SGM::Complex CreatePoints(SGM::Result                     &rResult,
                                         std::vector<SGM::Point3D> const &aPoints);

    SGM_EXPORT SGM::Complex CreateSegments(SGM::Result                    &rResult,
                                           std::vector<SGM::Point3D> const &aPoints,
                                           std::vector<unsigned int> const &aSegments);

    SGM_EXPORT SGM::Complex CreateTriangles(SGM::Result                    &rResult,
                                            std::vector<SGM::Point3D> const &aPoints,
                                            std::vector<unsigned int> const &aTriangles,
                                            bool                             bMerge=false);

    SGM_EXPORT SGM::Complex CreateSlice(SGM::Result             &rResult,
                                        SGM::Complex      const &ComplexID,
                                        SGM::Point3D      const &Point,
                                        SGM::UnitVector3D const &Normal,
                                        bool                     bLocal);

    SGM_EXPORT SGM::Complex CreatePolygon(SGM::Result                     &rResult,
                                          std::vector<SGM::Point3D> const &aPoints,
                                          bool                             bFilled);

    SGM_EXPORT SGM::Complex CreateRectangle(SGM::Result        &rResult,
                                            SGM::Point2D const &Pos0,
                                            SGM::Point2D const &Pos1,
                                            bool                bFilled);

    // pEntity should be a body, volume, or face.

    SGM_EXPORT SGM::Complex CreateComplex(SGM::Result       &rResult,
                                          SGM::Entity const &EntityID);

    SGM_EXPORT SGM::Complex CreateVoxels(SGM::Result                     &rResult,
                                         std::vector<SGM::Point3D> const &aPoints,
                                         double                           dLength,
                                         bool                             bSolid);

    // Find functions

    SGM_EXPORT double FindComplexLength(SGM::Result        &rResult,
                                        SGM::Complex const &ComplexID);

    SGM_EXPORT double FindAverageEdgeLength(SGM::Result        &rResult,
                                            SGM::Complex const &ComplexID,
                                            double             *dMaxLength=nullptr);

    SGM_EXPORT double FindComplexArea(SGM::Result        &rResult,
                                      SGM::Complex const &ComplexID);

    SGM_EXPORT size_t FindGenus(SGM::Result        &rResult,
                                SGM::Complex const &ComplexID);
    
    SGM_EXPORT SGM::Complex FindBoundary(SGM::Result        &rResult,
                                         SGM::Complex const &ComplexID);

    SGM_EXPORT bool IsLinear(SGM::Result        &rResult,
                             SGM::Complex const &ComplexID);

    SGM_EXPORT bool IsCycle(SGM::Result        &rResult,
                            SGM::Complex const &ComplexID);

    SGM_EXPORT bool IsConnected(SGM::Result        &rResult,
                                SGM::Complex const &ComplexID);

    SGM_EXPORT bool IsPlanar(SGM::Result        &rResult,
                             SGM::Complex const &ComplexID,
                             SGM::Point3D       &Origin,
                             SGM::UnitVector3D  &Normal,
                             double              dTolerance);

    SGM_EXPORT bool IsOriented(SGM::Result        &rResult,
                               SGM::Complex const &ComplexID);

    SGM_EXPORT bool IsManifold(SGM::Result        &rResult,
                               SGM::Complex const &ComplexID);

    // Splitting functions

    SGM_EXPORT size_t SplitWithPlane(SGM::Result               &rResult,
                                     SGM::Complex        const &ComplexID,
                                     SGM::Point3D        const &Point,
                                     SGM::UnitVector3D   const &Normal,
                                     std::vector<SGM::Complex> &aComponents);

    // Split with slices will split the given two-dimensional complex
    // with the given one-dimensional planar complexes.

    SGM_EXPORT size_t SplitWithSlices(SGM::Result                     &rResult,
                                      SGM::Complex              const &ComplexID,
                                      std::vector<SGM::Complex> const &aSlices,
                                      std::vector<SGM::Complex>       &aComponents);

    // Split with complex will split the given two-dimensional complex
    // with the given one dimensional sub-complex.  That is to say that
    // the vertices and edges of SliceID must be vertices and edges of
    // ComplexID.

    SGM_EXPORT size_t SplitWithComplex(SGM::Result               &rResult,
                                       SGM::Complex        const &ComplexID,
                                       SGM::Complex        const &SliceID,
                                       std::vector<SGM::Complex> &aComponents);

    SGM_EXPORT size_t FindComponents(SGM::Result               &rResult,
                                     SGM::Complex const        &ComplexID,
                                     std::vector<SGM::Complex> &aComponents);

    SGM_EXPORT size_t FindPlanarParts(SGM::Result               &rResult,
                                      SGM::Complex const        &ComplexID,
                                      std::vector<SGM::Complex> &aPlanarParts,
                                      double                     dTolerance);

    // Other complex functions.

    SGM_EXPORT SGM::Complex CoverComplex(SGM::Result        &rResult,
                                         SGM::Complex const &ComplexID);

    SGM_EXPORT SGM::Complex MergePoints(SGM::Result        &rResult,
                                        SGM::Complex const &ComplexID,
                                        double              dTolerance);

    SGM_EXPORT SGM::Complex FindSharpEdges(SGM::Result        &rResult,
                                           SGM::Complex const &ComplexID,
                                           double              dAngle,
                                           bool                bIncludeBoundary=false);

    SGM_EXPORT SGM::Complex FindDegenerateTriangles(SGM::Result        &rResult,
                                                    SGM::Complex const &ComplexID);

    SGM_EXPORT std::vector<double> FindTriangleAreas(SGM::Result        &rResult,
                                                     SGM::Complex const &ComplexID);

    SGM_EXPORT size_t FindHoles(SGM::Result               &rResult,
                                SGM::Complex        const &ComplexID,
                                std::vector<SGM::Complex> &aHoles);

    SGM_EXPORT SGM::Complex MergeComplexes(SGM::Result                     &rResult,
                                           std::vector<SGM::Complex> const &aComplexIDs);

    SGM_EXPORT void ReduceToUsedPoints(SGM::Result  &rResult,
                                       SGM::Complex &ComplexID);

    SGM_EXPORT void SplitComplexAtPoints(SGM::Result                     &rResult,
                                         SGM::Complex                    &ComplexID,
                                         std::vector<SGM::Point3D> const &aPoints,
                                         double                           dTolerance=SGM_MIN_TOL);

    } // End of SGM namespace

#endif // SGM_COMPLEX_H