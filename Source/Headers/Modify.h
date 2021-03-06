#ifndef MODIFY_H
#define MODIFY_H

#include "SGMVector.h"

#include "EntityClasses.h"

#include <set>
#include <map>

namespace SGMInternal
{

void UniteBodies(SGM::Result &rResult,
                 body        *pKeepBody,
                 body        *pDeleteBody);

void SubtractBodies(SGM::Result &rResult,
                    body        *pKeepBody,
                    body        *pDeleteBody);

void IntersectBodies(SGM::Result &rResult,
                     body        *pKeepBody,
                     body        *pDeleteBody);

void ImprintBodies(SGM::Result &rResult,
                   body        *pKeepBody,
                   body        *pDeleteBody);

void ReduceToVolumes(SGM::Result                      &rResult,
                     body                             *pBody,
                     std::set<volume *,EntityCompare> &sVolumes);

// pFace1 may be a nullptr and mHitMap(n) returns the edges or vertices
// hit by paramters on the curve on face n.  The function returns true 
// if a vertex from one face is imprinted on the other.

bool TrimCurveWithFaces(SGM::Result               &rResult,
                        curve                     *pCurve,
                        face                const *pFace0,
                        face                const *pFace1, 
                        std::vector<edge *>       &aEdges,
                        double                     dTolerance,
                        SGM::Interval1D     const *pLimitDomain=nullptr); 

std::vector<face *> ImprintEdgeOnFace(SGM::Result &rResult,
                                      edge        *pEdge,
                                      face        *pFace);

// ImprintTrimmedEdgeOnFace assumes that pEdge has been trimmed to the face.

std::vector<face *> ImprintTrimmedEdgeOnFace(SGM::Result                     &rResult,
                                             edge                            *pEdge,
                                             face                            *pFace,
                                             std::set<curve *,EntityCompare> &sDeleteCurves);

vertex *ImprintPoint(SGM::Result        &rResult,
                     SGM::Point3D const &Pos,
                     topology           *pTopology);

// Imprints the given point on given edge and returns a vertex at the point.

vertex *ImprintPointOnEdge(SGM::Result        &rResult,
                           SGM::Point3D const &Pos,
                           edge               *pEdge,
                           edge               **pNewEdge=nullptr);

void MergeVertices(SGM::Result &rResult,
                   vertex      *pKeepVertex,
                   vertex      *pDeleteVertex);

void MergeVertexSet(SGM::Result &rResult,
                    std::set<vertex *, EntityCompare> &sVertices);

void MergeEdges(SGM::Result                     &rResult,
                edge                            *pKeepEdge,
                edge                            *pDeleteEdge,
                std::set<curve *,EntityCompare> &sDeleteCurves);

void FindWindingNumbers(surface                   const *pSurface,
                        std::vector<SGM::Point3D> const &aPolygon3D,
                        int                             &nUWinds,
                        int                             &nVWinds);

void OrientBody(SGM::Result &rResult,
                body        *pBody);

void FixVolumes(SGM::Result &rResult,
                body        *pKeepBody);

void MergeFaces(SGM::Result &rResult,
                face        *pFace1,
                face        *pFace2);
}

#endif