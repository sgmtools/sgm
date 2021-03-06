#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "Curve.h"

#include "SGMVector.h"
#include "EntityClasses.h"

namespace SGMInternal
{
body *CreateBlock(SGM::Result        &rResult,
                  SGM::Point3D const &Point1,
                  SGM::Point3D const &Point2);

body *CreateSphere(SGM::Result        &rResult,
                   SGM::Point3D const &Center,
                   double              dRadius);

body *CreateCylinder(SGM::Result        &rResult,
                     SGM::Point3D const &BottomCenter,
                     SGM::Point3D const &TopCenter,
                     double              dRadius,
                     bool                bSheetBody=false);

body *CreateCone(SGM::Result        &rResult,
                 SGM::Point3D const &BottomCenter,
                 SGM::Point3D const &TopCenter,
                 double              dBottomRadius,
                 double              dTopRadius,
                 bool                bSheetBody=false);

body *CreateTorus(SGM::Result             &rResult,
                  SGM::Point3D      const &Center,
                  SGM::UnitVector3D const &Axis,
                  double                   dMinorRadius,
                  double                   dMajorRadius,
                  bool                     bApple);

body *CreateRevolve(SGM::Result             &rResult,
                    SGM::Point3D      const &Origin,
                    SGM::UnitVector3D const &Axis,
                    curve                   *pCurve);

surface *CreateRevolveSurface(SGM::Result             &rResult,
                              SGM::Point3D      const &Origin,
                              SGM::UnitVector3D const &Axis,
                              curve                   *pCurve);

surface *CreateExtrudeSurface(SGM::Result             &rResult,
                              SGM::UnitVector3D const &Axis,
                              curve                   *pCurve);

edge *CreateEdge(SGM::Result           &rResult,
                 curve                 *pCurve,
                 SGM::Interval1D const *pDomain);

// Returns a linear edge from the start point to the end points.

edge *CreateEdge(SGM::Result        &rResult,
                 SGM::Point3D const &StartPos,
                 SGM::Point3D const &EndPos);

// returns an edge on a curve with the specified start and end vertex
edge *CreateEdge(SGM::Result &rResult,
                 curve       *pCurve,
                 vertex      *pStart,
                 vertex      *pEnd);

body *CreateWireBody(SGM::Result            &rResult,
                     std::set<edge *> const &sEdges);

body *CreatePointBody(SGM::Result                  &rResult,
                      std::set<SGM::Point3D> const &aPoints);

NUBcurve *CreateNUBCurve(SGM::Result                     &rResult,
                         std::vector<SGM::Point3D> const &aPoints,
                         std::vector<double>       const *pParams=nullptr);

NUBcurve *CreateNUBCurveWithEndVectors(SGM::Result                     &rResult,
                                       std::vector<SGM::Point3D> const &aPoints,
                                       SGM::Vector3D             const &StartVec,
                                       SGM::Vector3D             const &EndVec,
                                       std::vector<double>       const *pParams);

body *CreateSheetBody(SGM::Result                    &rResult,
                      surface                        *pSurface,
                      std::vector<edge *>            &aEdges,
                      std::vector<SGM::EdgeSideType> &aTypes);

body *CreateSheetBody(SGM::Result           &rResult,
                      surface               *pSurface,
                      SGM::Interval2D const &Domain);

face *CreateFaceFromSurface(SGM::Result                    &rResult,
                            surface                        *pSurface,
                            std::vector<edge *>            &aEdges,
                            std::vector<SGM::EdgeSideType> &aTypes,
                            SGM::Interval2D          const *pDomain=nullptr);

body *CoverPlanarWire(SGM::Result &rResult,
                      body        *pPlanarWire);

body *CreateDisk(SGM::Result             &rResult,
                 SGM::Point3D      const &Center,
                 SGM::UnitVector3D const &Normal,
                 double                   dRadius);

complex *CreateComplex(SGM::Result                     &rResult,
                       std::vector<SGM::Point3D> const &aPoints,
                       std::vector<unsigned int> const &aSegments,
                       std::vector<unsigned int> const &aTriangles);

// pEntity should be a bod, volume, or face.

complex *CreateComplex(SGM::Result  &rResult,
                       entity const *pEntity);
    }
#endif // PRIMITIVE_H
