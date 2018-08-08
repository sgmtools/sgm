#ifndef SGM_INTERSECTOR_H
#define SGM_INTERSECTOR_H

#include <vector>

#include "sgm_export.h"

namespace SGM
    {
    class Result;
    class Point3D;
    class UnitVector3D;
    class Segment3D;
    class Entity;
    class Curve;
    class Edge;
    class Face;
    class Surface;

    SGM_EXPORT size_t RayFire(SGM::Result                        &rResult,
                              SGM::Point3D                 const &Origin,
                              SGM::UnitVector3D            const &Axis,
                              SGM::Entity                  const &EntityID,
                              std::vector<SGM::Point3D>          &aPoints,
                              std::vector<SGM::IntersectionType> &aTypes,
                              double                              dTolerance=SGM_ZERO,
                              bool                                bUseWholeLine=false);

    SGM_EXPORT size_t IntersectCurves(SGM::Result &rResult,
                                      SGM::Curve const &CurveID1,
                                      SGM::Curve const &CurveID2,
                                      std::vector<SGM::Point3D> &aPoints,
                                      std::vector<SGM::IntersectionType> &aTypes,
                                      SGM::Edge const *pEdge1 = nullptr,
                                      SGM::Edge const *pEdge2 = nullptr,
                                      double dTolerance = SGM_ZERO);

    SGM_EXPORT size_t IntersectCurveAndSurface(SGM::Result                        &rResult,
                                               SGM::Curve                   const &CurveID,
                                               SGM::Surface                 const &SurfaceID,
                                               std::vector<SGM::Point3D>          &aPoints,
                                               std::vector<SGM::IntersectionType> &aTypes,
                                               SGM::Edge                    const *pEdge = nullptr,
                                               SGM::Face                    const *pFace = nullptr,
                                               double                              dTolerance = SGM_ZERO);

    SGM_EXPORT size_t IntersectSurfaces(SGM::Result             &rResult,
                                        SGM::Surface      const &SurfaceID1,
                                        SGM::Surface      const &SurfaceID2,
                                        std::vector<SGM::Curve> &aCurves,
                                        SGM::Face         const *pFace1 = nullptr,
                                        SGM::Face         const *pFace2 = nullptr,
                                        double                   dTolerance = SGM_ZERO);

    SGM_EXPORT size_t IntersectSegment(SGM::Result               &rResult,
                                       SGM::Segment3D      const &Segment,
                                       SGM::Entity         const &EntityID,
                                       std::vector<SGM::Point3D> &aPoints,
                                       double                     dTolerance = SGM_ZERO);

    SGM_EXPORT size_t IntersectRectangleWithEdges(SGM::Result               &rResult,
                                                  SGM::Interval3D     const &rRectangle,
                                                  SGM::Entity         const &EntityID,
                                                  std::vector<SGM::Point3D> &aPoints,
                                                  std::vector<SGM::Entity>  &aEntity);

    SGM_EXPORT size_t IntersectCurveAndPlane(SGM::Result                        &rResult,
                                             SGM::Curve                   const &CurveID,
                                             SGM::Point3D                 const &PlaneOrigin,
                                             SGM::UnitVector3D            const &PlaneNorm,
                                             std::vector<SGM::Point3D>          &aPoints,
                                             std::vector<SGM::IntersectionType> &aTypes,
                                             double                              dTolerance = SGM_ZERO);

} // End of SGM namespace

#endif // SGM_INTERSECTOR_H