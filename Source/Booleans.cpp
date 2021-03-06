#include "SGMVector.h"
#include "SGMTransform.h"
#include "SGMModify.h"
#include "SGMGraph.h"
#include "SGMTopology.h"
#include "SGMPolygon.h"
#include "SGMTriangle.h"

#include "EntityClasses.h"
#include "EntityFunctions.h"
#include "Curve.h"
#include "Surface.h"
#include "Intersectors.h"
#include "Modify.h"
#include "Primitive.h"
#include "Topology.h"
#include "Faceter.h"
#include "Query.h"

#include <limits>
#include <algorithm>
#include <map>

namespace SGMInternal
{

void ReduceToVolumes(SGM::Result                      &rResult,
                     body                             *pBody,
                     std::set<volume *,EntityCompare> &sVolumes)
    {
    FindVolumes(rResult,pBody,sVolumes);
    pBody->SeverRelations(rResult);
    rResult.GetThing()->DeleteEntity(pBody);
    }

vertex *ImprintPointOnEdge(SGM::Result        &rResult,
                           SGM::Point3D const &Pos,
                           edge               *pEdge,
                           edge               **pOutNewEdge)
    {
    // Create the new vertex.

    std::set<face *,EntityCompare> sFaces=pEdge->GetFaces();
    double t=pEdge->GetCurve()->Inverse(Pos);
    auto pAnswer=new vertex(rResult, Pos);
    auto pEnd=pEdge->GetEnd();

    // Split open edges at the new vertex and 
    // closed edge with a vertex at the new vertex

    if(pEnd==nullptr)
        {
        pEdge->SetStart(rResult,pAnswer);
        pEdge->SetEnd(rResult,pAnswer);

        SGM::Interval1D Domain(t,pEdge->GetCurve()->GetDomain().Length()+t);
        pEdge->SetDomain(rResult,Domain);
        }
    else
        {
        auto pNewEdge=new edge(rResult);
        if(pOutNewEdge)
            {
            *pOutNewEdge=pNewEdge;
            }
        pNewEdge->SetCurve(rResult,pEdge->GetCurve());
    
        SGM::Interval1D Domain=pEdge->GetDomain();
        pNewEdge->SetEnd(rResult,pEnd);
        pNewEdge->SetStart(rResult,pAnswer);
        pEdge->SetEnd(rResult,pAnswer);
        if(pEdge->GetStart()==pEnd)
            {
            pEdge->GetStart()->AddEdge(pEdge);
            }

        SGM::Interval1D Domain1(Domain.m_dMin,t),Domain2(t,Domain.m_dMax);
        pEdge->SetDomain(rResult,Domain1);
        pNewEdge->SetDomain(rResult,Domain2);

        // Add the pNewEdge to the pface.

        for(face *pFace : sFaces)
            {
            SGM::EdgeSideType nType=pFace->GetSideType(pEdge);
            pFace->AddEdge(rResult,pNewEdge,nType);
            }
        }

    return pAnswer;
    }

vertex *ImprintPoint(SGM::Result        &rResult,
                     SGM::Point3D const &Pos,
                     topology           *pTopology)
    {
    vertex *pAnswer=nullptr;
    switch(pTopology->GetType())
        {
        case SGM::EdgeType:
            {
            auto pEdge=(edge *)pTopology;
            pAnswer=ImprintPointOnEdge(rResult,Pos,pEdge);
            break;
            }
        case SGM::BodyType:
            {
            auto pBody=(body *)pTopology;
            std::set<edge *,EntityCompare> sEdges;
            FindEdges(rResult,pBody,sEdges,false);
            bool bFound=false;
            for(auto pEdge : sEdges)
                {
                if(pEdge->PointInEdge(Pos,SGM_MIN_TOL))
                    {
                    pAnswer=ImprintPointOnEdge(rResult,Pos,pEdge);
                    bFound=true;
                    break;
                    }
                }
            if(!bFound)
                {
                std::set<face *,EntityCompare> sFaces;
                FindFaces(rResult,pBody,sFaces,false);
                for(auto pFace : sFaces)
                    {
                    SGM::Point3D CPos;
                    SGM::Point2D uv=pFace->GetSurface()->Inverse(Pos,&CPos);
                    if(SGM::NearEqual(Pos,CPos,SGM_MIN_TOL) && pFace->PointInFace(rResult,uv))
                        {
                        curve *pCurve=new PointCurve(rResult,Pos);
                        edge *pEdge=CreateEdge(rResult,pCurve,&pCurve->GetDomain());
                        std::set<curve *,EntityCompare> sDeleteCurves;
                        ImprintTrimmedEdgeOnFace(rResult,pEdge,pFace,sDeleteCurves);
                        break;
                        }
                    }
                }
            break;
            }
        default:
            {
            throw; // Add more code.
            }
        }
    return pAnswer;
    }

bool PreImprintVertices(SGM::Result &rResult,
                        face  const *pFace0,
                        face  const *pFace1)
    {
    bool bAnswer=false;
    std::set<vertex *,EntityCompare> sVertices;
    FindVertices(rResult,pFace0,sVertices);
    for(vertex *pVertex : sVertices)
        {
        SGM::Point3D const &Pos=pVertex->GetPoint();
        std::set<edge *,EntityCompare> sEdges=pFace1->GetEdges();
        for(edge *pEdge : sEdges)
            {
            if(pEdge->PointInEdge(Pos,SGM_MIN_TOL))
                {
                if( SGM::NearEqual(Pos,pEdge->GetStart()->GetPoint(),SGM_MIN_TOL)==false && 
                    SGM::NearEqual(Pos,pEdge->GetEnd()->GetPoint(),SGM_MIN_TOL)==false)
                    {
                    vertex *pTempVertex=ImprintPointOnEdge(rResult,Pos,pEdge);
                    MergeVertices(rResult,pVertex,pTempVertex);
                    bAnswer=true;
                    break;
                    }
                }
            }
        }
    return bAnswer;
    }

bool TrimCurveWithFaces(SGM::Result               &rResult,
                        curve                     *pCurve,
                        face                const *pFace0,
                        face                const *pFace1,
                        std::vector<edge *>       &aEdges,
                        double                     dTolerance,
                        SGM::Interval1D     const *pLimitDomain)
    {
    bool bAnswer=false;
    if(pCurve==nullptr)
        {
        if(PreImprintVertices(rResult,pFace0,pFace1))
            {
            bAnswer=true;
            }
        if(PreImprintVertices(rResult,pFace1,pFace0))
            {
            bAnswer=true;
            }
        std::set<edge *,EntityCompare> const &sEdges0=pFace0->GetEdges();
        for(auto pEdge : sEdges0)
            {
            if(TrimCurveWithFaces(rResult,pEdge->GetCurve(),pFace1,nullptr,aEdges,dTolerance,&pEdge->GetDomain()))
                {
                bAnswer=true;
                }
            }
        std::set<edge *,EntityCompare> const &sEdges1=pFace1->GetEdges();
        for(auto pEdge : sEdges1)
            {
            if(TrimCurveWithFaces(rResult,pEdge->GetCurve(),pFace0,nullptr,aEdges,dTolerance,&pEdge->GetDomain()))
                {
                bAnswer=true;
                }
            }
        }
    else
        {
        std::vector<SGM::Point3D> aHits;
        std::set<edge *,EntityCompare> const &sEdges0=pFace0->GetEdges();
        size_t Index1;
        for(edge *pEdge : sEdges0)
            {
            std::vector<SGM::Point3D> aIntersectionPoints;
            std::vector<SGM::IntersectionType> aTypes;
            size_t nHits=IntersectCurves(rResult,pCurve,pEdge->GetCurve(),aIntersectionPoints,aTypes,dTolerance);
            for(Index1=0;Index1<nHits;++Index1)
                {
                if(aTypes[Index1]!=SGM::CoincidentType)
                    {
                    aHits.push_back(aIntersectionPoints[Index1]);
                    }
                }
            }
        if(pFace1)
            {
            std::set<edge *,EntityCompare> const &sEdges1=pFace1->GetEdges();
            for(edge *pEdge : sEdges1)
                {
                std::vector<SGM::Point3D> aIntersectionPoints;
                std::vector<SGM::IntersectionType> aTypes;
                size_t nHits=IntersectCurves(rResult,pCurve,pEdge->GetCurve(),aIntersectionPoints,aTypes,dTolerance);
                for(Index1=0;Index1<nHits;++Index1)
                    {
                    if(aTypes[Index1]!=SGM::CoincidentType)
                        {
                        aHits.push_back(aIntersectionPoints[Index1]);
                        }
                    }
                }
            }

        // Add the end points of the curve and remove duplicates.

        SGM::Interval3D Box=pFace0->GetBox(rResult,false);
        if(pFace1)
            {
            Box&=pFace1->GetBox(rResult,false);
            }
        SGM::Interval1D const &Domain=pCurve->GetDomain();
        SGM::Point3D StartPos,EndPos;
        pCurve->Evaluate(Domain.m_dMin,&StartPos);
        pCurve->Evaluate(Domain.m_dMax,&EndPos);
        aHits.push_back(StartPos);
        aHits.push_back(EndPos);
        if(Box.IsEmpty())
            {
            SGM::RemoveDuplicates3D(aHits,dTolerance);
            }
        else
            {
            SGM::RemoveDuplicates3D(aHits,dTolerance,&Box);
            }

        // Find the parameters for each intersection.

        std::vector<std::pair<double,SGM::Point3D> > aHitParams;
        size_t nHits=aHits.size();
        aHitParams.reserve(nHits);
        for(Index1=0;Index1<nHits;++Index1)
            {
            SGM::Point3D const &Pos=aHits[Index1];
            double dParam=pCurve->Inverse(Pos);
            aHitParams.emplace_back(dParam,Pos);
            if(pCurve->GetClosed() && pCurve->GetDomain().OnBoundary(dParam,SGM_MIN_TOL))
                {
                if(dParam<pCurve->GetDomain().MidPoint())
                    {
                    aHitParams.emplace_back(dParam+pCurve->GetDomain().Length(),Pos);
                    }
                else
                    {
                    aHitParams.emplace_back(dParam-pCurve->GetDomain().Length(),Pos);
                    }
                }
            }
        std::sort(aHitParams.begin(),aHitParams.end());

        // Check each segment to see if it is inside or outside of the face(s).

        surface const *pSurface0=pFace0->GetSurface();
        surface const *pSurface1=pFace1 ? pFace1->GetSurface() : nullptr;
        std::vector<bool> aIsolated;
        size_t nHitParams=aHitParams.size();
        aIsolated.assign(nHitParams,true);
        for(Index1=1;Index1<nHitParams;++Index1)
            {
            double dParam0=aHitParams[Index1-1].first;
            double dParam1=aHitParams[Index1].first;
            double dMidParam=(dParam0+dParam1)*0.5;
            SGM::Point3D Pos;
            pCurve->Evaluate(dMidParam,&Pos);
            SGM::Point2D uv0=pSurface0->Inverse(Pos);
            if(pFace0->PointInFace(rResult,uv0))
                {
                if(pFace1==nullptr)
                    {
                    SGM::Interval1D ParamDomain(dParam0,dParam1);
                    if(pLimitDomain)
                        {
                        ParamDomain&=*pLimitDomain;
                        }
                    if(ParamDomain.Length()<SGM_MIN_TOL)
                        {
                        // Could add isolated point here?
                        }
                    else
                        {
                        aEdges.push_back(CreateEdge(rResult,pCurve,&ParamDomain));
                        aIsolated[Index1-1]=false;
                        aIsolated[Index1]=false;
                        }
                    }
                else
                    {
                    SGM::Point2D uv1=pSurface1->Inverse(Pos);
                    if(pFace1->PointInFace(rResult,uv1))
                        {
                        SGM::Interval1D ParamDomain(dParam0,dParam1);
                        if(pLimitDomain)
                            {
                            ParamDomain&=*pLimitDomain;
                            }
                        aEdges.push_back(CreateEdge(rResult,pCurve,&ParamDomain));
                        aIsolated[Index1-1]=false;
                        aIsolated[Index1]=false;
                        }
                    }
                }
            }

        // Check for closed curves inside the face.

        if(aHits.size()==1 && pCurve->GetClosed())
            {
            aIsolated[0]=false;
            SGM::Point3D Pos=aHitParams[0].second;
            bool bInFaces=true;
            SGM::Point2D uv0=pSurface0->Inverse(Pos);
            if(pFace0->PointInFace(rResult,uv0)==false)
                {
                bInFaces=false;
                }
            if(bInFaces && pFace1)
                {
                SGM::Point2D uv1=pSurface1->Inverse(Pos);
                if(pFace1->PointInFace(rResult,uv1)==false)
                    {
                    bInFaces=false;
                    }
                }
            if(bInFaces)
                {
                aEdges.push_back(CreateEdge(rResult,pCurve,pLimitDomain));
                }
            }

        // Check for isolated points.

        for(Index1=0;Index1<nHitParams;++Index1)
            {
            if(aIsolated[Index1])
                {
                SGM::Point3D Pos=aHitParams[Index1].second;
                if(pLimitDomain)
                    {
                    double dParam=pCurve->Inverse(Pos);
                    if(pLimitDomain->InInterior(dParam,dTolerance)==false)
                        {
                        continue;
                        }
                    }
                SGM::Point2D uv0=pSurface0->Inverse(Pos);
                if(pFace0->PointInFace(rResult,uv0))
                    {
                    if(pFace1==nullptr)
                        {
                        PointCurve *pPointCurve=new PointCurve(rResult,Pos);
                        aEdges.push_back(CreateEdge(rResult,pPointCurve,&pPointCurve->GetDomain()));
                        }
                    else
                        {
                        SGM::Point2D uv1=pSurface1->Inverse(Pos);
                        if(pFace1->PointInFace(rResult,uv1))
                            {
                            PointCurve *pPointCurve=new PointCurve(rResult,Pos);
                            aEdges.push_back(CreateEdge(rResult,pPointCurve,&pPointCurve->GetDomain()));
                            }
                        }
                    }
                }
            }
        }

    return bAnswer;
    }

void MergeVertices(SGM::Result &rResult,
                  vertex      *pKeepVertex,
                  vertex      *pDeleteVertex)
    {
    if(pKeepVertex==pDeleteVertex)
        {
        return;
        }
    std::set<edge *,EntityCompare> sEdges=pDeleteVertex->GetEdges();
    for(edge *pEdge : sEdges)
        {
        if(pEdge->GetStart()==pDeleteVertex)
            {
            pEdge->SetStart(rResult,pKeepVertex);
            }
        if(pEdge->GetEnd()==pDeleteVertex)
            {
            pEdge->SetEnd(rResult,pKeepVertex);
            }
        }
    rResult.GetThing()->DeleteEntity(pDeleteVertex);
    }

void MergeVertexSet(SGM::Result                       &rResult,
                    std::set<vertex *, EntityCompare> &sVertices)
{
    // Note that this only works on single volume wires.
    // Also note that this is a n^2 algorithum which could be n*ln(n).

    std::set<vertex *> sDeleted;
    for(vertex *pVertex:sVertices)
        {
        for(vertex *pTest:sVertices)
            {
            if( pVertex!=pTest && 
                sDeleted.find(pTest)==sDeleted.end() &&
                sDeleted.find(pVertex)==sDeleted.end() &&
                SGM::NearEqual(pVertex->GetPoint(),pTest->GetPoint(),SGM_MIN_TOL))
                {
                MergeVertices(rResult,pVertex,pTest);
                sDeleted.insert(pTest);
                }
            }
        }
}


void ImprintPeninsula(SGM::Result &rResult,
                      edge        *pEdge,
                      face        *pFace,
                      entity      *pStartEntity,
                      entity      *pEndEntity)
    {
     if(pStartEntity)
        {
        if(pStartEntity->GetType()==SGM::EdgeType)
            {
            pStartEntity=ImprintPointOnEdge(rResult,pEdge->GetStart()->GetPoint(),(edge *)pStartEntity);
            }
        MergeVertices(rResult,(vertex *)pStartEntity,pEdge->GetStart());
        }
    else 
        {
        if(pEndEntity->GetType()==SGM::EdgeType)
            {
            pEndEntity=ImprintPointOnEdge(rResult,pEdge->GetEnd()->GetPoint(),(edge *)pEndEntity);
            }
        MergeVertices(rResult,(vertex *)pEndEntity,pEdge->GetEnd());
        }
    pFace->AddEdge(rResult,pEdge,SGM::EdgeSideType::FaceOnBothSidesType);
    }


void ImprintBridge(SGM::Result &rResult,
                   edge        *pEdge,
                   face        *pFace,
                   entity      *pStartEntity,
                   entity      *pEndEntity)
    {
    if(pStartEntity->GetType()==SGM::EdgeType)
        {
        pStartEntity=ImprintPointOnEdge(rResult,pEdge->GetStart()->GetPoint(),(edge *)pStartEntity);
        }
    MergeVertices(rResult,(vertex *)pStartEntity,pEdge->GetStart());

    if(pEndEntity->GetType()==SGM::EdgeType)
        {
        pEndEntity=ImprintPointOnEdge(rResult,pEdge->GetEnd()->GetPoint(),(edge *)pEndEntity);
        }
    MergeVertices(rResult,(vertex *)pEndEntity,pEdge->GetEnd());

    pFace->AddEdge(rResult,pEdge,SGM::EdgeSideType::FaceOnBothSidesType);
    }

size_t FindLoop(entity                                  *pStartEntity,
                std::vector<std::vector<edge *> > const &aaLoops)
    {
    vertex *pVertex=pStartEntity->GetType()==SGM::VertexType ? (vertex *)pStartEntity : nullptr;
    edge *pEdge=pStartEntity->GetType()==SGM::EdgeType ? (edge *)pStartEntity : nullptr;
    size_t nAnswer=0;
    size_t nnLoops=aaLoops.size();
    size_t Index1,Index2;
    for(Index1=0;Index1<nnLoops;++Index1)
        {
        std::vector<edge *> const &aLoops=aaLoops[Index1];
        size_t nLoops=aLoops.size();
        for(Index2=0;Index2<nLoops;++Index2)
            {
            edge *pTestEdge=aLoops[Index2];
            if(pEdge && pEdge==pTestEdge)
                {
                return Index1;
                }
            if(pVertex && (pTestEdge->GetStart()==pVertex || pTestEdge->GetEnd()==pVertex))
                {
                return Index1;
                }
            }
        }
    return nAnswer;
    }

bool ShouldEdgeFlip(std::vector<edge *>            const &aLoop,
                    std::vector<SGM::EdgeSideType> const &aSides,
                    edge                           const *pNewEdge)
    {
    vertex *pVertex=pNewEdge->GetEnd();
    size_t nLoop=aLoop.size();
    size_t Index1;
    for(Index1=0;Index1<nLoop;++Index1)
        {
        edge *pEdge=aLoop[Index1];
        vertex *pTestVertex=aSides[Index1]==SGM::FaceOnRightType ? pEdge->GetStart() : pEdge->GetEnd();
        if(pVertex==pTestVertex)
            {
            return true;
            }
        }
    return false;
    }

void SplitLoop(SGM::Result                          &rResult,
               std::vector<edge *>            const &aLoop,
               std::vector<SGM::EdgeSideType> const &aSides,
               entity                               *pStartEntity,
               entity                               *pEndEntity,
               edge                                 *pNewEdge,
               face                                 *pFace,
               std::vector<edge *>                  &aLoopA,
               std::vector<edge *>                  &aLoopB,
               std::vector<SGM::EdgeSideType>       &aSidesA,
               std::vector<SGM::EdgeSideType>       &aSidesB)
    {
    // Run around until pStartEntity is found.

    size_t nLoop=aLoop.size();
    size_t Index1,nStart=0,nEnd=0;
    for(Index1=0;Index1<nLoop;++Index1)
        {
        edge *pEdge=aLoop[Index1];
        vertex *pEnd=pEdge->GetEnd();
        if(aSides[Index1]==SGM::FaceOnRightType)
            {
            pEnd=pEdge->GetStart();
            }
        if(pEnd==pStartEntity)
            {
            nStart=Index1+1;
            break;
            }
        }

    // Keep going until pEndEntity is found.

    size_t nMaxEdge=nLoop*2;
    for(Index1=nStart;Index1<nMaxEdge;++Index1)
        {
        edge *pEdge=aLoop[Index1%nLoop];
        vertex *pEnd=pEdge->GetEnd();
        if(aSides[Index1%nLoop]==SGM::FaceOnRightType)
            {
            pEnd=pEdge->GetStart();
            }
        if(pEnd==pEndEntity)
            {
            nEnd=Index1+1;
            break;
            }
        }

    // Set up the two loops.

    for(Index1=nStart;Index1<nEnd;++Index1)
        {
        edge *pEdge=aLoop[Index1%nLoop];
        SGM::EdgeSideType nType=aSides[Index1%nLoop];
        aLoopA.push_back(pEdge);
        aSidesA.push_back(nType);
        }
    size_t nStart2=nEnd;
    size_t nEnd2=nStart+nLoop;
    for(Index1=nStart2;Index1<nEnd2;++Index1)
        {
        edge *pEdge=aLoop[Index1%nLoop];
        SGM::EdgeSideType nType=aSides[Index1%nLoop];
        aLoopB.push_back(pEdge);
        aSidesB.push_back(nType);
        }

    // Add the new edge to the two loops.

    bool bFlipEdge=ShouldEdgeFlip(aLoopA,aSidesA,pNewEdge);
    aLoopA.push_back(pNewEdge);
    aLoopB.push_back(pNewEdge);
    if(bFlipEdge)
        {
        aSidesA.push_back(SGM::FaceOnRightType);
        aSidesB.push_back(SGM::FaceOnLeftType);
        }
    else
        {
        aSidesA.push_back(SGM::FaceOnLeftType);
        aSidesB.push_back(SGM::FaceOnRightType);
        }

    // Check to see if everything needs flipping.

    surface *pSurface=pFace->GetSurface();
    //std::vector<SGM::Point3D> aPolygonB3D;
    std::vector<SGM::Point2D> aPolygonB2D;
    size_t nLoopB=aLoopB.size();
    for(Index1=0;Index1<nLoopB;++Index1)
        {
        edge *pEdge=aLoopB[Index1];
        std::vector<SGM::Point3D> aPoints3D=pEdge->GetFacets(rResult);
        if(aSidesB[Index1]==SGM::FaceOnRightType)
            {
            std::reverse(aPoints3D.begin(),aPoints3D.end());
            }
        for(auto Pos : aPoints3D)
            {
            //aPolygonB3D.push_back(Pos);
            aPolygonB2D.push_back(pSurface->Inverse(Pos));
            }
        }

    //int nUWinds,nVWinds;
    //FindWindingNumbers(pSurface,aPolygonB3D,nUWinds,nVWinds);
    double dAreaB=SGM::PolygonArea(aPolygonB2D);
    if(dAreaB<0)// && nUWinds==0 && nVWinds==0)
        {
        size_t nSidesA=aSidesA.size();
        for(Index1=0;Index1<nSidesA;++Index1)
            {
            if(aSidesA[Index1]==SGM::FaceOnLeftType)
                {
                aSidesA[Index1]=SGM::FaceOnRightType;
                }
            else if(aSidesA[Index1]==SGM::FaceOnRightType)
                {
                aSidesA[Index1]=SGM::FaceOnLeftType;
                }
            }
        size_t nSidesB=aSidesB.size();
        for(Index1=0;Index1<nSidesB;++Index1)
            {
            if(aSidesB[Index1]==SGM::FaceOnLeftType)
                {
                aSidesB[Index1]=SGM::FaceOnRightType;
                }
            else if(aSidesB[Index1]==SGM::FaceOnRightType)
                {
                aSidesB[Index1]=SGM::FaceOnLeftType;
                }
            }
        }
    }

face *ImprintSplitter(SGM::Result &rResult,
                      edge        *pEdge,
                      face        *pFace,
                      entity      *pStartEntity,
                      entity      *pEndEntity)
    {
    // Imprint the end points.

    if(pStartEntity->GetType()==SGM::EdgeType)
        {
        pStartEntity=ImprintPointOnEdge(rResult,pEdge->GetStart()->GetPoint(),(edge *)pStartEntity);
        }
    if(pEndEntity->GetType()==SGM::EdgeType)
        {
        pEndEntity=ImprintPointOnEdge(rResult,pEdge->GetEnd()->GetPoint(),(edge *)pEndEntity);
        }
    MergeVertices(rResult,(vertex *)pStartEntity,pEdge->GetStart());
    MergeVertices(rResult,(vertex *)pEndEntity,pEdge->GetEnd());

    // Split the loop into two loops.

    std::vector<std::vector<edge *> > aaLoops;
    std::vector<std::vector<SGM::EdgeSideType> > aaEdgeSideTypes;
    size_t nLoops=pFace->FindLoops(rResult,aaLoops,aaEdgeSideTypes);

    size_t nSplitLoop=FindLoop(pStartEntity,aaLoops);
    std::vector<edge *> aLoopA,aLoopB;
    std::vector<SGM::EdgeSideType> aEdgeSideTypesA,aEdgeSideTypesB;
    SplitLoop(rResult,aaLoops[nSplitLoop],aaEdgeSideTypes[nSplitLoop],pStartEntity,
              pEndEntity,pEdge,pFace,aLoopA,aLoopB,aEdgeSideTypesA,aEdgeSideTypesB);

    // Take all the aLoopB edges out of pFace.

    size_t Index1,Index2;
    size_t nLoopB=aLoopB.size();
    for(Index1=0;Index1<nLoopB;++Index1)
        {
        edge *pLoopEdge=aLoopB[Index1];
        pFace->RemoveEdge(rResult,pLoopEdge);
        }

    // Make a new face for the second part of the split loop.

    auto pNewFace=new face(rResult);
    pNewFace->SetSurface(rResult,pFace->GetSurface());
    pNewFace->SetSides(pFace->GetSides());
    pFace->GetVolume()->AddFace(rResult,pNewFace);
    size_t nLoopA=aLoopA.size();
    for(Index1=0;Index1<nLoopA;++Index1)
        {
        edge *pLoopEdge=aLoopA[Index1];
        SGM::EdgeSideType nType=aEdgeSideTypesA[Index1];
        pFace->AddEdge(rResult,pLoopEdge,nType);
        }
    for(Index1=0;Index1<nLoopB;++Index1)
        {
        edge *pLoopEdge=aLoopB[Index1];
        SGM::EdgeSideType nType=aEdgeSideTypesB[Index1];
        pNewFace->AddEdge(rResult,pLoopEdge,nType);
        }
        
    // Move the other loops to the correct face.

    for(Index1=0;Index1<nLoops;++Index1)
        {
        if(Index1!=nSplitLoop)
            {
            edge *pEdge1=aaLoops[Index1].front();
            SGM::Point3D TestPos=pEdge1->FindMidPoint();
            SGM::Point2D uv=pNewFace->GetSurface()->Inverse(TestPos);
            if(pNewFace->PointInFace(rResult,uv))
                {
                std::vector<edge *> const &aLoop=aaLoops[Index1];
                std::vector<SGM::EdgeSideType> const &aType=aaEdgeSideTypes[Index1];
                size_t nLoop=aLoop.size();
                for(Index2=0;Index2<nLoop;++Index2)
                    {
                    edge *pLoopEdge=aLoop[Index2];
                    pFace->RemoveEdge(rResult,pLoopEdge);
                    pNewFace->AddEdge(rResult,pLoopEdge,aType[Index2]);
                    }
                }
            }
        }

    if(rResult.GetLog())
        {
        rResult.AddLog(SGM::Entity(pFace->GetID()),SGM::Entity(pNewFace->GetID()),SGM::LogSplit);
        }
    return pNewFace;
    }

void ImprintLowersGenus(SGM::Result &rResult,
                        edge        *pEdge,
                        face        *pFace)
    {
    pFace->AddEdge(rResult,pEdge,SGM::FaceOnBothSidesType);
    }

std::vector<SGM::Point3D> FacetLoop(SGM::Result                          &rResult,
                                    std::vector<edge *>            const &aLoop,
                                    std::vector<SGM::EdgeSideType> const &aFlips)
    {
    std::vector<SGM::Point3D> aFacets;
    size_t nLoop=aLoop.size();
    size_t Index1;
    for(Index1=0;Index1<nLoop;++Index1)
        {
        edge *pEdge=aLoop[Index1];
        std::vector<SGM::Point3D> aEdgeFacets=pEdge->GetFacets(rResult);
        if(aFlips[Index1]==SGM::FaceOnRightType)
            {
            std::reverse(aEdgeFacets.begin(),aEdgeFacets.end());
            }
        aFacets.insert(aFacets.end(),aEdgeFacets.begin(),aEdgeFacets.end());
        }
    return aFacets;
    }

face *ImprintAtoll(SGM::Result &rResult,
                   edge        *pEdge,
                   face        *pFace)
    {

    // Test to see if pEdge is clockwise in the face.
    // Create a new face and add pEdge to both pFace and pNewFace.

    auto *pNewFace=new face(rResult);
    pNewFace->SetSurface(rResult,pFace->GetSurface());
    pNewFace->SetSides(pFace->GetSides());
    pNewFace->AddEdge(rResult,pEdge,SGM::FaceOnLeftType);
    pFace->GetVolume()->AddFace(rResult,pNewFace);

    int nUWinds,nVWinds;
    FindWindingNumbers(pFace->GetSurface(),pEdge->GetFacets(rResult),nUWinds,nVWinds);

    if(nUWinds==1)
        {
        std::vector<std::vector<edge *> > aaLoops;
        std::vector<std::vector<SGM::EdgeSideType> > aaFlipped;
        pFace->FindLoops(rResult,aaLoops,aaFlipped);
        size_t nLoops=aaLoops.size();
        size_t Index1;
        std::vector<size_t> aInsideLoops;
        for(Index1=0;Index1<nLoops;++Index1)
            {
            int nLoopUWinds,nLoopVWinds;
            FindWindingNumbers(pFace->GetSurface(),
                               FacetLoop(rResult,aaLoops[Index1],aaFlipped[Index1]),
                               nLoopUWinds,nLoopVWinds);
            if(nLoopUWinds==-1)
                {
                for(auto pLoopEdge : aaLoops[Index1])
                    {
                    SGM::EdgeSideType nType=pFace->GetSideType(pLoopEdge);
                    pFace->RemoveEdge(rResult,pLoopEdge);
                    pNewFace->AddEdge(rResult,pLoopEdge,nType);
                    }
                }
            else if(nLoopUWinds==0)
                {
                aInsideLoops.push_back(Index1);
                }
            }
        pFace->AddEdge(rResult,pEdge,SGM::FaceOnRightType);
        if(aInsideLoops.size())
            {
            throw; // Add code to move interior loops to the correct face.
            }
        }
    else
        {
        std::vector<std::vector<unsigned int> > aaPolygons;
        std::vector<SGM::Point2D> aPoints2D;
        std::vector<SGM::Point3D> aPoints3D;
        std::vector<entity *> aEntities;
        FacetFaceLoops(rResult,pNewFace,aPoints2D,aPoints3D,aaPolygons,pEdge);
        double dArea=0.0;
        for(std::vector<unsigned int> const &aPolygon : aaPolygons)
            {
            dArea+=SGM::PolygonArea(PointsFromPolygon2D(aPoints2D,aPolygon));
            }
        if(dArea<0)
            {
            pNewFace->RemoveEdge(rResult,pEdge);
            pNewFace->AddEdge(rResult,pEdge,SGM::FaceOnRightType);
            }
        pFace->AddEdge(rResult,pEdge,pNewFace->GetSideType(pEdge)==SGM::FaceOnRightType ? SGM::FaceOnLeftType : SGM::FaceOnRightType);
        }
    
    if(rResult.GetLog())
        {
        rResult.AddLog(SGM::Entity(pFace->GetID()),SGM::Entity(pNewFace->GetID()),SGM::LogSplit);
        }
    return pNewFace;
    }

bool LowersGenus(SGM::Result &rResult,
                 edge        *pEdge,
                 face        *pFace)
    {
    surface *pSurface=pFace->GetSurface();
    if(pSurface->ClosedInU() && pSurface->ClosedInV())
        {
        std::vector<SGM::Point3D> const &aPoints=pEdge->GetFacets(rResult);
        int nUWinds,nVWinds;
        FindWindingNumbers(pSurface,aPoints,nUWinds,nVWinds);
        if(nUWinds || nVWinds)
            {
            return true;
            }
        }
    return false;
    }

bool AreEdgesCoincident(edge const *pImprintEdge,
                        edge const *pFaceEdge)
    {
    curve const *pImprintCurve=pImprintEdge->GetCurve();
    curve const *pFaceCurve=pFaceEdge->GetCurve();
    if(pImprintCurve->IsSame(pFaceCurve,SGM_MIN_TOL))
        {
        // Test to see if the mid point of pImprintEdge is on pFaceEdge.
        SGM::Point3D Pos=pImprintEdge->FindMidPoint();
        if(pFaceEdge->PointInEdge(Pos,SGM_MIN_TOL))
            {
            return true;
            }
        }
    else if(pImprintEdge->GetDomain().Length()<SGM_MIN_TOL &&
            pFaceEdge->PointInEdge(pImprintEdge->FindStartPoint(),SGM_MIN_TOL))
        {
        return true;
        }
    return false;
    }

edge *CutEdgeToEdge(SGM::Result &rResult,
                    edge        *pBigEdge,
                    edge        *pSmallEdge)
    {
    bool bImprintStartPos=false;
    if(pSmallEdge->GetStart())
        {
        bImprintStartPos=true;
        SGM::Point3D StartPos=pSmallEdge->GetStart()->GetPoint();
        if(pBigEdge->GetStart() && SGM::NearEqual(pBigEdge->GetStart()->GetPoint(),StartPos,SGM_MIN_TOL))
            {
            bImprintStartPos=false;
            }
        else if(pBigEdge->GetEnd() && SGM::NearEqual(pBigEdge->GetEnd()->GetPoint(),StartPos,SGM_MIN_TOL))
            {
            bImprintStartPos=false;
            }
        }
    bool bImprintEndPos=false;
    if(pSmallEdge->GetEnd() && pSmallEdge->GetStart()!=pSmallEdge->GetEnd())
        {
        bImprintEndPos=true;
        SGM::Point3D EndPos=pSmallEdge->GetEnd()->GetPoint();
        if(pBigEdge->GetStart() && SGM::NearEqual(pBigEdge->GetStart()->GetPoint(),EndPos,SGM_MIN_TOL))
            {
            bImprintEndPos=false;
            }
        else if(pBigEdge->GetEnd() && SGM::NearEqual(pBigEdge->GetEnd()->GetPoint(),EndPos,SGM_MIN_TOL))
            {
            bImprintEndPos=false;
            }
        }
    std::set<edge *> sAllEdges;
    sAllEdges.insert(pBigEdge);
    if(bImprintStartPos)
        {
        SGM::Point3D StartPos=pSmallEdge->GetStart()->GetPoint();
        vertex *pNewVertex1=ImprintPointOnEdge(rResult,StartPos,pBigEdge);
        edge *pOnEdge=nullptr;
        for(auto pEdge : pNewVertex1->GetEdges())
            {
            if(pEdge->PointInEdge(StartPos,SGM_MIN_TOL))
                {
                pOnEdge=pEdge;
                }
            sAllEdges.insert(pEdge);
            }
        if(bImprintEndPos)
            {
            SGM::Point3D EndPos=pSmallEdge->GetEnd()->GetPoint();
            vertex *pNewVertex2=ImprintPointOnEdge(rResult,EndPos,pOnEdge);
            for(auto pEdge : pNewVertex2->GetEdges())
                {
                sAllEdges.insert(pEdge);
                }
            }
        }
    else if(bImprintEndPos)
        {
        SGM::Point3D EndPos=pSmallEdge->GetEnd()->GetPoint();
        vertex *pNewVertex1=ImprintPointOnEdge(rResult,EndPos,pBigEdge);
        for(auto pEdge : pNewVertex1->GetEdges())
            {
            sAllEdges.insert(pEdge);
            }
        }
    edge *pAnswer=nullptr;
    SGM::Point3D TestPos=pSmallEdge->FindMidPoint();
    for(auto pEdge : sAllEdges)
        {
        if(pEdge->PointInEdge(TestPos,SGM_MIN_TOL))
            {
            pAnswer=pEdge;
            break;
            }
        }

    return pAnswer;
    }

edge *AreCoincident(SGM::Result &rResult,
                    edge        *pEdge,
                    face        *pFace)
    {
    for(auto pTestEdge : pFace->GetEdges())
        {
        if(AreEdgesCoincident(pEdge,pTestEdge))
            {
            return CutEdgeToEdge(rResult,pTestEdge,pEdge);
            }
        }
    return nullptr;
    }

void MergeEdges(SGM::Result                     &rResult,
                edge                            *pKeepEdge,
                edge                            *pDeleteEdge,
                std::set<curve *,EntityCompare> &sDeleteCurves)
    {
    if(pKeepEdge->GetDomain().Length()<SGM_MIN_TOL)
        {
        SGM::Point3D Pos=pKeepEdge->GetStart()->GetPoint();
        if(pDeleteEdge->GetStart() && SGM::NearEqual(Pos,pDeleteEdge->GetStart()->GetPoint(),SGM_MIN_TOL))
            {
            MergeVertices(rResult,pKeepEdge->GetStart(),pDeleteEdge->GetStart());
            }
        else if(pDeleteEdge->GetEnd() && SGM::NearEqual(Pos,pDeleteEdge->GetEnd()->GetPoint(),SGM_MIN_TOL))
            {
            MergeVertices(rResult,pKeepEdge->GetStart(),pDeleteEdge->GetEnd());
            }
        else
            {
            vertex *pTempVertex=ImprintPointOnEdge(rResult,Pos,pDeleteEdge);
            MergeVertices(rResult,pKeepEdge->GetStart(),pTempVertex);
            }
        return;
        }
    if(pKeepEdge==pDeleteEdge)
        {
        return;
        }
    bool bFlip=false;
    if( pKeepEdge->GetStart() && pKeepEdge->GetEnd() && 
        pDeleteEdge->GetStart() && pDeleteEdge->GetEnd())
        {
        vertex *pStartKeep=pKeepEdge->GetStart();
        vertex *pEndKeep=pKeepEdge->GetEnd();
        vertex *pStartDelete=pDeleteEdge->GetStart();
        vertex *pEndDelete=pDeleteEdge->GetEnd();
      
        if( pStartKeep->GetPoint().DistanceSquared(pStartDelete->GetPoint())<SGM_ZERO &&
            pEndKeep->GetPoint().DistanceSquared(pEndDelete->GetPoint())<SGM_ZERO)
            {
            // Complete match.

            MergeVertices(rResult,pStartKeep,pStartDelete);
            if(pStartDelete!=pEndDelete)
                {
                MergeVertices(rResult,pEndKeep,pEndDelete);
                }
            }
        else if( pEndKeep->GetPoint().DistanceSquared(pStartDelete->GetPoint())<SGM_ZERO &&
                 pStartKeep->GetPoint().DistanceSquared(pEndDelete->GetPoint())<SGM_ZERO)
            {
            // Flipped complete match.

            bFlip=true;
            MergeVertices(rResult,pStartKeep,pEndDelete);
            if(pStartDelete!=pEndDelete)
                {
                MergeVertices(rResult,pEndKeep,pStartDelete);
                }
            }
        else 
            {
            throw; // The edges should already been made to match.
            }
        }
    else
        {
        double t1=pKeepEdge->GetDomain().MidPoint();
        SGM::Vector3D Vec1,Vec2;
        SGM::Point3D Pos;
        pKeepEdge->GetCurve()->Evaluate(t1,&Pos,&Vec1);
        double t2=pDeleteEdge->GetCurve()->Inverse(Pos);
        pDeleteEdge->GetCurve()->Evaluate(t2,nullptr,&Vec2);
        if(Vec1%Vec2<0)
            {
            bFlip=true;
            }
        }
    std::set<face *,EntityCompare> sFaces=pDeleteEdge->GetFaces();
    for(face *pFace : sFaces)
        {
        SGM::EdgeSideType nType=pFace->GetSideType(pDeleteEdge);
        pFace->RemoveEdge(rResult,pDeleteEdge);
        if(bFlip)
            {
            if(nType==SGM::EdgeSideType::FaceOnLeftType)
                {
                nType=SGM::EdgeSideType::FaceOnRightType;
                }
            else if(nType==SGM::EdgeSideType::FaceOnRightType)
                {
                nType=SGM::EdgeSideType::FaceOnLeftType;
                }
            pFace->AddEdge(rResult,pKeepEdge,nType);
            }
        else
            {
            pFace->AddEdge(rResult,pKeepEdge,nType);
            }
        }
    curve *pCurve=pDeleteEdge->GetCurve();
    pDeleteEdge->SeverRelations(rResult);
    pCurve->RemoveEdge(pDeleteEdge);
    rResult.GetThing()->DeleteEntity(pDeleteEdge);
    if(pCurve->GetEdges().empty())
        {
        sDeleteCurves.insert(pCurve);
        }
    }

entity *FindSubEntity(vertex const *pVertex,
                      face   const *pFace)
    {
    if(pVertex==nullptr)
        {
        return nullptr;
        }
    SGM::Point3D Pos=pVertex->GetPoint();
    std::set<edge *,EntityCompare> const &sEdges=pFace->GetEdges();
    entity *pAnswer=nullptr;
    for(auto pEdge : sEdges)
        {
        if(pEdge->GetStart() && pEdge->GetStart()->GetPoint().Distance(Pos)<SGM_MIN_TOL)
            {
            return pEdge->GetStart();
            }
        if(pEdge->GetEnd() && pEdge->GetEnd()->GetPoint().Distance(Pos)<SGM_MIN_TOL)
            {
            return pEdge->GetEnd();
            }
        double t=pEdge->GetCurve()->Inverse(Pos);
        pEdge->SnapToDomain(t,SGM_MIN_TOL);
        if(pEdge->GetDomain().InInterval(t,SGM_MIN_TOL))
            {
            SGM::Point3D CPos;
            pEdge->GetCurve()->Evaluate(t,&CPos);
            if(CPos.Distance(Pos)<SGM_MIN_TOL)
                {
                return pEdge;
                }
            }
        }
    return pAnswer;
    }

bool IsIsland(SGM::Result &rResult,
              edge        *pEdge,
              face        *pFace)
    {
    SGM::Point3D Pos=pEdge->FindMidPoint();
    SGM::Point2D uv=pFace->GetSurface()->Inverse(Pos);
    return pFace->PointInFace(rResult,uv);
    }

void ImprintPointCurveOnEdge(SGM::Result &rResult,
                             edge        *pPointCurveEdge,
                             entity      *pEntity)
    {
    vertex *pVertex1=pPointCurveEdge->GetStart();
    if(pEntity->GetType()==SGM::VertexType)
        {
        vertex *pVertex2=(vertex *)pEntity;
        MergeVertices(rResult,pVertex1,pVertex2);
        }
    else
        {
        edge *pEdge=(edge *)pEntity;
        SGM::Point3D Pos=pVertex1->GetPoint();
        vertex *pVertex2=ImprintPointOnEdge(rResult,Pos,pEdge);
        MergeVertices(rResult,pVertex1,pVertex2);
        }
    }

std::vector<face *> ImprintTrimmedEdgeOnFace(SGM::Result                     &rResult,
                                             edge                            *pEdge,
                                             face                            *pFace,
                                             std::set<curve *,EntityCompare> &sDeleteCurves)
    {
    std::vector<face *> aFaces;
    aFaces.push_back(pFace);

    entity *pStartEntity=FindSubEntity(pEdge->GetStart(),pFace);
    entity *pEndEntity=FindSubEntity(pEdge->GetEnd(),pFace);
    
    // Figure out which of the following seven cases the edge is on the face.
    //
    // Peninsula (Starts on a loop and ends in the interior of the face.)
    // Splitter (Starts and ends on the same loop and is contractible in the face.)
    // Bridge (Starts and ends on different loops of the face.)
    // Island (Starts and ends in the interior of the face and is either closed or degenerate.)
    // Atoll (Is closed, non-degenerate and starts in the interior of the face.)
    // Non-Contractible (The edge is not contractible in the face.)
    // Coincident (The edge is coincident with an existing edge of the face.)

    if(edge *pConincentEdge=AreCoincident(rResult,pEdge,pFace))
        {
        MergeEdges(rResult,pEdge,pConincentEdge,sDeleteCurves);
        }
    else if(pStartEntity && pEndEntity)
        {
        // Bridge, Splitter

        std::vector<std::vector<edge *> > aaLoops;
        std::vector<std::vector<SGM::EdgeSideType> > aaEdgeSideTypes;
        pFace->FindLoops(rResult,aaLoops,aaEdgeSideTypes);
        size_t nStartLoop=FindLoop(pStartEntity,aaLoops);
        size_t nEndLoop=FindLoop(pEndEntity,aaLoops);
        if(nStartLoop==nEndLoop)
            {
            if(pEdge->GetCurve()->GetCurveType()==SGM::PointCurveType)
                {
                // Coincident point on edge

                ImprintPointCurveOnEdge(rResult,pEdge,pStartEntity);
                }
            else
                {
                // Splitter

                aFaces.push_back(ImprintSplitter(rResult,pEdge,pFace,pStartEntity,pEndEntity));
                }
            }
        else
            {
            // Bridge

            ImprintBridge(rResult,pEdge,pFace,pStartEntity,pEndEntity);
            }
        }
    else if(pStartEntity || pEndEntity)
        {
        // Peninsula
        
        ImprintPeninsula(rResult,pEdge,pFace,pStartEntity,pEndEntity);
        }
    else if(LowersGenus(rResult,pEdge,pFace))
        {
        // Lowers Genus

        ImprintLowersGenus(rResult,pEdge,pFace);
        }
    else if(pEdge->IsClosed() && !pEdge->IsDegenerate())
        {
        // Atoll

        aFaces.push_back(ImprintAtoll(rResult,pEdge,pFace));
        }
    else if(IsIsland(rResult,pEdge,pFace))
        {
        // Island

        pFace->AddEdge(rResult,pEdge,SGM::EdgeSideType::FaceOnBothSidesType);
        }

    return aFaces;
    }

void FindStartAndEndEnts(edge                const *pEdge,
                         std::map<double,entity *> &mHitMap1,
                         std::map<double,entity *> &mHitMap2,
                         entity                    *&pStartEnt,
                         entity                    *&pEndEnt)
    {
    pStartEnt=nullptr;
    pEndEnt=nullptr;
    SGM::Interval1D const &Domain=pEdge->GetDomain();
    double dStart=Domain.m_dMin;
    double dEnd=Domain.m_dMax;
    double dMinDist=std::numeric_limits<double>::max();
    for(auto iter : mHitMap1)
        {
        double dDist=fabs(dStart-iter.first);
        if(dDist<dMinDist)
            {
            dMinDist=dDist;
            pStartEnt=iter.second;
            }
        }
    for(auto iter : mHitMap2)
        {
        double dDist=fabs(dStart-iter.first);
        if(dDist<dMinDist)
            {
            pStartEnt=nullptr;
            break;
            }
        }
    dMinDist=std::numeric_limits<double>::max();
    for(auto iter : mHitMap1)
        {
        double dDist=fabs(dEnd-iter.first);
        if(dDist<dMinDist)
            {
            dMinDist=dDist;
            pEndEnt=iter.second;
            }
        }
    for(auto iter : mHitMap2)
        {
        double dDist=fabs(dEnd-iter.first);
        if(dDist<dMinDist)
            {
            pEndEnt=nullptr;
            break;
            }
        }
    }

bool ImprintFaces(SGM::Result                                                                &rResult,
                  face                                                                       *pFace1,
                  face                                                                       *pFace2,
                  std::vector<std::pair<face *,face *> >                                     &aSplits,
                  std::map<std::pair<surface const *,surface const *>,std::vector<curve *> > &mIntersections,
                  std::set<curve *,EntityCompare>                                            &sDeleteCurves,
                  SGM::Interval1D                                                      const *pLimitDomain,
                  std::set<std::pair<surface const *,surface const *> >                      *pConincidentSurfaces)
    {
    // Check for conicident vertices.

    std::set<vertex *,EntityCompare> sVertices1;
    FindVertices(rResult,pFace1,sVertices1);
    for(vertex *pVertex1 : sVertices1)
        {
        std::set<vertex *,EntityCompare> sVertices2;
        FindVertices(rResult,pFace2,sVertices2);
        for(vertex *pVertex2 : sVertices2)
            {
            if(pVertex1!=pVertex2 && SGM::NearEqual(pVertex1->GetPoint(),pVertex2->GetPoint(),SGM_MIN_TOL))
                {
                MergeVertices(rResult,pVertex1,pVertex2);
                }
            }
        }

    // Find the new edges.

    bool bAnswer=false;
    double dTolerance=SGM_MIN_TOL;
    std::vector<curve *> aCurves;
    surface const *pSurface1=pFace1->GetSurface();
    surface const *pSurface2=pFace2 ? pFace2->GetSurface() : nullptr;
    if(pSurface2 && pSurface2->GetID()<pSurface1->GetID())
        {
        std::swap(pSurface1,pSurface2);
        }
    if(mIntersections.find({pSurface1,pSurface2})==mIntersections.end())
        {
        if(IntersectSurfaces(rResult,pSurface1,pSurface2,aCurves,dTolerance))
            {
            aCurves.push_back(nullptr);
            if(pConincidentSurfaces)
                {
                pConincidentSurfaces->insert({pSurface1,pSurface2});
                }
            }
        mIntersections[{pSurface1,pSurface2}]=aCurves;
        }
    else
        {
        aCurves=mIntersections[{pSurface1,pSurface2}];
        }
    for(auto pCurve : aCurves)
        {
        std::vector<edge *> aEdges;
        if(TrimCurveWithFaces(rResult,pCurve,pFace1,pFace2,aEdges,SGM_MIN_TOL,pLimitDomain))
            {
            bAnswer=true;
            }
        for(edge *pEdge : aEdges)
            {
            std::vector<face *> aFaces1=ImprintTrimmedEdgeOnFace(rResult,pEdge,pFace1,sDeleteCurves);
            for(face *pFace : aFaces1)
                {
                bAnswer=true;
                if(pFace!=pFace1)
                    {
                    aSplits.push_back({pFace1,pFace});
                    }
                }
            if(pFace2)
                {
                std::vector<face *> aFaces2=ImprintTrimmedEdgeOnFace(rResult,pEdge,pFace2,sDeleteCurves);
                for(face *pFace : aFaces2)
                    {
                    bAnswer=true;
                    if(pFace!=pFace2)
                        {
                        aSplits.push_back({pFace2,pFace});
                        }
                    }
                }
            if(pEdge->GetFaces().empty())
                {
                vertex *pVertex=pEdge->GetStart();
                if(pVertex->GetEdges().size()==1)
                    {
                    pVertex->SeverRelations(rResult);
                    rResult.GetThing()->DeleteEntity(pVertex);
                    }
                curve *pCurve1=pEdge->GetCurve();
                pEdge->SeverRelations(rResult);
                rResult.GetThing()->DeleteEntity(pEdge);
                if(pCurve1->GetEdges().empty())
                    {
                    rResult.GetThing()->DeleteEntity(pCurve1);
                    }
                }
            }
        }
    return bAnswer;
    }

std::vector<face *> ImprintEdgeOnFace(SGM::Result &rResult,
                                      edge        *pEdge,
                                      face        *pFace)
    {
    std::vector<std::pair<face *,face *> > aSplits;
    std::map<std::pair<surface const *,surface const *>,std::vector<curve *> > mIntersections;
    std::vector<curve *> aCurves;
    aCurves.push_back(pEdge->GetCurve());
    mIntersections[{pFace->GetSurface(),nullptr}]=aCurves;
    SGM::Interval1D Domain=pEdge->GetDomain();
    std::set<curve *,EntityCompare> sDeleteCurves;
    ImprintFaces(rResult,pFace,nullptr,aSplits,mIntersections,sDeleteCurves,&Domain,nullptr);
    std::set<face *> sFaces;
    for(auto FacePair : aSplits)
        {
        sFaces.insert(FacePair.first);
        sFaces.insert(FacePair.second);
        }
    std::vector<face *> aAnswer;
    aAnswer.reserve(sFaces.size());
    for(auto pFace1 : sFaces)
        {
        aAnswer.push_back(pFace1);
        }
    return aAnswer;
    }

void MergeAndMoveVolumes(SGM::Result            &rResult,
                         body                   *pKeepBody,
                         std::set<size_t> const &sVolumeIDs)
    {
    // First find the volume to keep.

    std::set<volume *,EntityCompare> sVolumes=pKeepBody->GetVolumes();
    volume *pKeepVolume=nullptr;
    for(size_t VolumeID : sVolumeIDs)
        {
        auto pVolume=(volume *)rResult.GetThing()->FindEntity(VolumeID);
        if(pKeepVolume==nullptr && sVolumes.find(pVolume)!=sVolumes.end())
            {
            pKeepVolume=pVolume;
            break;
            }
        }

    // Move all faces and edges of each non-keep volume over to the keep volume.
    
    for(size_t VolumeID : sVolumeIDs)
        {
        auto pVolume=(volume *)rResult.GetThing()->FindEntity(VolumeID);
        if(pVolume!=pKeepVolume)
            {
            if(pKeepVolume==nullptr)
                {
                pVolume->SetBody(pKeepBody);
                }
            else
                {
                std::set<face *,EntityCompare> sFaces=pVolume->GetFaces();
                for(face *pFace : sFaces)
                    {
                    pVolume->RemoveFace(rResult,pFace);
                    pKeepVolume->AddFace(rResult,pFace);
                    }
                std::set<edge *,EntityCompare> sEdges=pVolume->GetEdges();
                for(edge *pEdge : sEdges)
                    {
                    pVolume->RemoveEdge(rResult,pEdge);
                    pKeepVolume->AddEdge(rResult,pEdge);
                    }
                body *pBody=pVolume->GetBody();
                pBody->RemoveVolume(pVolume);
                rResult.GetThing()->DeleteEntity(pVolume);
                }
            }
        }
    }

void AddFacePairs(size_t                                  nOldSplits,
                  std::vector<std::pair<face *,face *> > &aSplits,
                  std::vector<std::pair<face *,face *> > &aFacePairs)
    {
    size_t Index1,Index2;
    size_t nSplits=aSplits.size();
    for(Index1=nOldSplits;Index1<nSplits;++Index1)
        {
        auto Split=aSplits[Index1];
        size_t nFacePairs=aFacePairs.size();
        for(Index2=0;Index2<nFacePairs;++Index2)
            {
            auto FacePair=aFacePairs[Index2];
            if(FacePair.first==Split.first)
                {
                aFacePairs.push_back({Split.second,FacePair.second});
                }
            if(FacePair.second==Split.first)
                {
                aFacePairs.push_back({Split.second,FacePair.first});
                }
            }
        }
    }

void ImprintBodies(SGM::Result                            &rResult,
                   body                                   *pKeepBody,
                   body                                   *pDeleteBody,
                   std::vector<std::pair<face *,face *> > &aSplits,
                   std::set<std::pair<surface const *,surface const *> > *sCoincidentSurfaces)
    {
    // Find the face pairs to consider.

    std::set<face *,EntityCompare> sFaces;
    FindFaces(rResult,pKeepBody,sFaces);
    std::set<volume *,EntityCompare> sVolumes;
    FindVolumes(rResult,pDeleteBody,sVolumes);
    std::vector<std::pair<face *,face *> > aFacePairs;
    for(face *pFace1 : sFaces)
        {
        for(volume *pVolume : sVolumes)
            {
            SGM::BoxTree const &Tree=pVolume->GetFaceTree(rResult);
            auto aHits=Tree.FindIntersectsBox(pFace1->GetBox(rResult));
            for(void const* pVoid : aHits)
                {
                face *pFace2=(face *)pVoid;
                aFacePairs.push_back({pFace1,pFace2});
                }
            }
        }

    // Imprint the faces with each other.

    std::set<curve *,EntityCompare> sDeleteCurves;
    std::set<std::pair<size_t,size_t> > sMergedVolumes;
    std::map<std::pair<surface const *,surface const *>,std::vector<curve *> > mIntersections;
    size_t Index1;
    for(Index1=0;Index1<aFacePairs.size();++Index1)
        {
        auto FacePair=aFacePairs[Index1];
        size_t nOldSplits=aSplits.size();
        if(ImprintFaces(rResult,FacePair.first,FacePair.second,aSplits,mIntersections,
                        sDeleteCurves,nullptr,sCoincidentSurfaces))
            {
            sMergedVolumes.insert({FacePair.first->GetVolume()->GetID(),FacePair.second->GetVolume()->GetID()});
            }
        AddFacePairs(nOldSplits,aSplits,aFacePairs);
        }

    // Delete unused intersection curves and check all edges of hit faces.

    for(auto InPair : mIntersections)
        {
        for(auto pCurve : InPair.second)
            {
            if(pCurve && pCurve->GetEdges().empty())
                {
                sDeleteCurves.insert(pCurve);
                }
            }
        }
    for(curve *pCurve : sDeleteCurves)
        {
        rResult.GetThing()->DeleteEntity(pCurve);
        }

    // Merge volumes and bodies.

    std::set<volume *,EntityCompare> const &sVolumes0=pKeepBody->GetVolumes();
    std::set<volume *,EntityCompare> const &sVolumes1=pDeleteBody->GetVolumes();
    std::set<size_t> sGraphVertices;
    std::set<SGM::GraphEdge> sGraphEdges;
    for(volume *pVolume : sVolumes0)
        {
        sGraphVertices.insert(pVolume->GetID());
        }
    for(volume *pVolume : sVolumes1)
        {
        sGraphVertices.insert(pVolume->GetID());
        }
    size_t nGraphEdgeCount=0;
    for(auto iter : sMergedVolumes)
        {
        sGraphEdges.insert(SGM::GraphEdge(iter.first,iter.second,nGraphEdgeCount));
        ++nGraphEdgeCount;
        }
    SGM::Graph graph(sGraphVertices,sGraphEdges);
    std::vector<SGM::Graph> aComps;
    graph.FindComponents(aComps);
    for(SGM::Graph const &comp : aComps)
        {
        std::set<size_t> const &sVerts=comp.GetVertices();
        MergeAndMoveVolumes(rResult,pKeepBody,sVerts);
        }
    rResult.GetThing()->DeleteEntity(pDeleteBody);

    // Orient the sheet body faces to be consistant with each other.

    OrientBody(rResult,pKeepBody);
    }

size_t RayFireFaceSet(SGM::Result                        &rResult,
                      SGM::Point3D                 const &Origin,
                      SGM::UnitVector3D            const &Axis,
                      SGM::BoxTree                 const &FaceTree,
                      std::vector<SGM::Point3D>          &aPoints,
                      std::vector<SGM::IntersectionType> &aTypes,
                      std::vector<entity *>              &aEntities,
                      double                              dTolerance,
                      bool                                bUseWholeLine)
    {
    // Find all ray hits for all faces.
    SGM::Ray3D Ray(Origin,Axis);
    std::vector<void const*> aHitFaces=FaceTree.FindIntersectsRay(Ray,dTolerance);

    std::vector<SGM::Point3D> aAllPoints;
    std::vector<SGM::IntersectionType> aAllTypes;
    std::vector<entity *> aAllEntities;
    for (void const* pVoid : aHitFaces)
        {
        face const * pFace = (face*)pVoid;
        std::vector<SGM::Point3D> aSubPoints;
        std::vector<SGM::IntersectionType> aSubTypes;
        std::vector<entity *> aEntity;
        size_t nHits=RayFireFace(rResult,Origin,Axis,pFace,aSubPoints,aSubTypes,aEntity,dTolerance,bUseWholeLine);
        for(size_t Index1=0;Index1<nHits;++Index1)
            {
            aAllPoints.push_back(aSubPoints[Index1]);
            aAllTypes.push_back(aSubTypes[Index1]);
            aAllEntities.push_back(aEntity[Index1]);
            }
        }

    size_t nAnswer=OrderAndRemoveDuplicates(Origin,Axis,dTolerance,bUseWholeLine,aAllPoints,aAllTypes,aAllEntities);
    aPoints=aAllPoints;
    aTypes=aAllTypes;
    aEntities=aAllEntities;
    return nAnswer;
    }

SGM::Point3D FindInteriorPoint(SGM::Result &rResult,
                               face  const *pFace)
    {
    std::vector<SGM::Point2D> const &aPoints=pFace->GetPoints2D(rResult);
    std::vector<unsigned> const &aTriangles=pFace->GetTriangles(rResult);
    unsigned a=aTriangles[0];
    unsigned b=aTriangles[1];
    unsigned c=aTriangles[2];
    SGM::Point2D const &A=aPoints[a];
    SGM::Point2D const &B=aPoints[b];
    SGM::Point2D const &C=aPoints[c];
    SGM::Point2D uv=SGM::CenterOfMass(A,B,C);
    SGM::Point3D Pos;
    pFace->GetSurface()->Evaluate(uv,&Pos);
    return Pos;
    }

bool FaceInFaces(SGM::Result        &rResult,
                 face         const *pFace,
                 SGM::BoxTree const &FaceTree)
    {
    SGM::Point3D Point=FindInteriorPoint(rResult,pFace);
    size_t nHits=0;
    bool bFound=true;
    size_t nCount=1;
    SGM::UnitVector3D Axis(0,0,1);
    while(bFound)
        {
        bFound=false;
        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;
        std::vector<entity *> aEntities;
        nHits=RayFireFaceSet(rResult,Point,Axis,FaceTree,aPoints,aTypes,aEntities,SGM_MIN_TOL,false);
        size_t Index1;
        for(Index1=0;Index1<nHits;++Index1)
            {
            if(aTypes[Index1]!=SGM::IntersectionType::PointType)
                {
                if(SGM::NearEqual(Point,aPoints[Index1],SGM_MIN_TOL))
                    {
                    return true;
                    }
                Axis=SGM::UnitVector3D (cos(nCount),sin(nCount),cos(nCount+17));
                bFound=true;
                break;
                }
            }
        ++nCount;
        }

    return nHits%2==1;
    }

void FixVolumes(SGM::Result &rResult,
                body        *pKeepBody)
    {
    std::set<volume *,EntityCompare> sVolumes=pKeepBody->GetVolumes();
    for(volume *pVolume : sVolumes)
        {
        std::set<SGM::Face> sFaces;
        std::set<face *,EntityCompare> const &sfaces=pVolume->GetFaces();
        for(auto pFace : sfaces)
            {
            sFaces.insert(SGM::Face(pFace->GetID()));
            }
        SGM::Graph graph(rResult,sFaces,false);
        std::vector<SGM::Graph> aComponents;
        size_t nComponents=graph.FindComponents(aComponents);
        size_t Index1;
        for(Index1=1;Index1<nComponents;++Index1)
            {
            volume *pNewVolume=new volume(rResult);
            SGM::Graph const &comp=aComponents[Index1];
            std::set<size_t> const &sVertices=comp.GetVertices();
            for(size_t FaceID : sVertices)
                {
                face *pFace=(face *)rResult.GetThing()->FindEntity(FaceID);
                pVolume->RemoveFace(rResult,pFace);
                pNewVolume->AddFace(rResult,pFace);
                }
            pKeepBody->AddVolume(pNewVolume);
            }
        }
    }

void ImprintBodies(SGM::Result &rResult,
                   body        *pKeepBody,
                   body        *pDeleteBody)
    {
    std::vector<std::pair<face *,face *> > aSplits;
    ImprintBodies(rResult,pKeepBody,pDeleteBody,aSplits,nullptr);
    }

void FindFaceTrees(SGM::Result         &rResult,
                   body                *pKeepBody,
                   body                *pDeleteBody,
                   std::set<SGM::Face> &sFaces1,
                   std::set<SGM::Face> &sFaces2,
                   SGM::BoxTree        &Tree1,
                   SGM::BoxTree        &Tree2)
    {
    SGM::FindFaces(rResult,SGM::Body(pKeepBody->GetID()),sFaces1);
    SGM::FindFaces(rResult,SGM::Body(pDeleteBody->GetID()),sFaces2);
    for(SGM::Face FaceID : sFaces1)
        {
        face *pFace1=(face *)rResult.GetThing()->FindEntity(FaceID.m_ID);
        if(pFace1->GetSides()==1)
            {
            Tree1.Insert(pFace1,pFace1->GetBox(rResult));
            }
        }
    for(SGM::Face FaceID : sFaces2)
        {
        face *pFace2=(face *)rResult.GetThing()->FindEntity(FaceID.m_ID);
        if(pFace2->GetSides()==1)
            {
            Tree2.Insert(pFace2,pFace2->GetBox(rResult));
            }
        }
    }

void UpdateFaceTrees(SGM::Result                            &rResult,
                     std::vector<std::pair<face *,face *> > &aSplits,
                     std::set<SGM::Face>                    &sFaces1,
                     std::set<SGM::Face>                    &sFaces2,
                     SGM::BoxTree                           &Tree1,
                     SGM::BoxTree                           &Tree2)
    {
    for(auto FacePair : aSplits)
        {
        face *pFace1=FacePair.first;
        face *pFace2=FacePair.second;
        if(sFaces1.find(SGM::Face(pFace1->GetID()))!=sFaces1.end())
            {
            sFaces1.insert(SGM::Face(pFace2->GetID()));
            if(pFace2->GetSides()==1)
                {
                Tree1.Insert(pFace2,pFace2->GetBox(rResult));
                }
            }
        else
            {
            sFaces2.insert(SGM::Face(pFace2->GetID()));
            if(pFace2->GetSides()==1)
                {
                Tree2.Insert(pFace2,pFace2->GetBox(rResult));
                }
            }
        }
    }

void DeleteFaces(SGM::Result         &rResult,
                 std::vector<face *> &aDelete)
    {
    std::set<surface *,EntityCompare> sSurfaces;
    std::set<edge *,EntityCompare> sEdges;
    for(face *pFace : aDelete)
        {
        sSurfaces.insert(pFace->GetSurface());
        sEdges.insert(pFace->GetEdges().begin(),pFace->GetEdges().end());
        }
    for(face *pFace : aDelete)
        {
        pFace->SeverRelations(rResult);
        rResult.GetThing()->DeleteEntity(pFace);
        }
    for(surface *pSurface : sSurfaces)
        {
        if(pSurface->GetFaces().empty())
            {
            rResult.GetThing()->DeleteEntity(pSurface);
            }
        }
    std::set<curve *,EntityCompare> sCurves;
    std::set<vertex *,EntityCompare> sVertices;
    for(edge *pEdge : sEdges)
        {
        if(pEdge->GetFaces().empty())
            {
            if(pEdge->GetStart())
                {
                sVertices.insert(pEdge->GetStart());
                sVertices.insert(pEdge->GetEnd());
                }
            curve *pCurve=pEdge->GetCurve();
            sCurves.insert(pCurve);
            pEdge->SeverRelations(rResult);
            rResult.GetThing()->DeleteEntity(pEdge);
            }
        }
    for(curve *pCurve : sCurves)
        {
        if(pCurve->GetEdges().empty())
            {
            rResult.GetThing()->DeleteEntity(pCurve);
            }
        }
    for(vertex *pVertex : sVertices)
        {
        if(pVertex->GetEdges().empty())
            {
            rResult.GetThing()->DeleteEntity(pVertex);
            }
        }
    }

void SubtractBodies(SGM::Result &rResult,
                    body        *pKeepBody,
                    body        *pDeleteBody)
    {
    // Find all the faces before imprinting.  Also build box trees
    // of the one sided faces.

    std::set<SGM::Face> sFaces1,sFaces2;
    SGM::BoxTree Tree1,Tree2;
    FindFaceTrees(rResult,pKeepBody,pDeleteBody,sFaces1,sFaces2,Tree1,Tree2);

    // Imprint the bodies and find all the faces
    // that came from each body.

    std::vector<std::pair<face *,face *> > aSplits;
    ImprintBodies(rResult,pKeepBody,pDeleteBody,aSplits,nullptr);
    UpdateFaceTrees(rResult,aSplits,sFaces1,sFaces2,Tree1,Tree2);
    
    // Delete faces from pKeepBody that are outside of pDeleteBody.
    
    std::vector<face *> aDelete,aFlip;
    for(SGM::Face FaceID : sFaces1)
        {
        face *pFace=(face *)rResult.GetThing()->FindEntity(FaceID.m_ID);
        if(FaceInFaces(rResult,pFace,Tree2))
            {
            aDelete.push_back(pFace);
            }
        }

    // Flip faces from pDeleteBody that are inside of pKeepBody.
    // Delete faces from pDeleteBody that are outside of pKeepBody.

    for(SGM::Face FaceID : sFaces2)
        {
        face *pFace=(face *)rResult.GetThing()->FindEntity(FaceID.m_ID);
        if(FaceInFaces(rResult,pFace,Tree1))
            {
            aFlip.push_back(pFace);
            }
        else
            {
            aDelete.push_back(pFace);
            }
        }

    // Delete the delete faces.

    DeleteFaces(rResult,aDelete);

    // Flip the filp faces.

    for(face *pFace : aFlip)
        {
        pFace->Negate();
        }

    FixVolumes(rResult,pKeepBody);
    }

bool AreCoincidentFaces(face *pFace0,face *pFace1,bool &bFlipped)
    {
    if(pFace0->GetEdges()==pFace1->GetEdges())
        {
        if(pFace0->GetEdges().empty())
            {
            return true;
            }
        else
            {
            edge *pEdge=*(pFace0->GetEdges().begin());
            SGM::Point3D Pos=pEdge->FindMidPoint();
            SGM::UnitVector3D Norm0=pFace0->FindNormal(Pos);
            SGM::UnitVector3D Norm1=pFace1->FindNormal(Pos);
            if(SGM::NearEqual(Norm0,Norm1,SGM_MIN_TOL))
                {
                bFlipped=false;
                return pFace0->GetSides()==pFace1->GetSides();
                }
            else
                {
                std::map<edge *,SGM::EdgeSideType>::const_iterator iter0=pFace0->GetEdgeSides().begin();
                std::map<edge *,SGM::EdgeSideType>::const_iterator iter1=pFace1->GetEdgeSides().begin();
                while(iter0!=pFace0->GetEdgeSides().end())
                    {
                    SGM::EdgeSideType nType0=iter0->second;
                    SGM::EdgeSideType nType1=iter1->second;
                    if( (nType0==SGM::FaceOnLeftType && nType1==SGM::FaceOnLeftType) ||
                        (nType0==SGM::FaceOnRightType && nType1==SGM::FaceOnRightType))
                        {
                        return false;
                        }
                    ++iter0;
                    ++iter1;
                    }
                bFlipped=true;
                return true;
                }
            }
        }
    return false;
    }

void LookForCoincidentFaces(std::set<std::pair<surface const *,surface const *> > &aCoincidentSurfaces,
                            std::vector<std::pair<face *,face *> >                &aCoincidentFaces,
                            std::vector<bool>                                     &aFlippedFace)
    {
    for(auto SurfPair : aCoincidentSurfaces)
        {
        surface const *pSurface0=SurfPair.first;
        surface const *pSurface1=SurfPair.second;
        std::set<face *,EntityCompare> const &sFaces0=pSurface0->GetFaces();
        std::set<face *,EntityCompare> const &sFaces1=pSurface1->GetFaces();
        for(face *pFace0 : sFaces0)
            {
            for(face *pFace1 : sFaces1)
                {
                bool bFlipped;
                if(AreCoincidentFaces(pFace0,pFace1,bFlipped))
                    {
                    aCoincidentFaces.push_back({pFace0,pFace1});
                    aFlippedFace.push_back(bFlipped);
                    }
                }
            }
        }
    }

void UniteBodies(SGM::Result &rResult,
                 body        *pKeepBody,
                 body        *pDeleteBody)
    {
    // Find all the faces before imprinting.  Also build box trees
    // of the one sided faces.

    std::set<SGM::Face> sFaces1,sFaces2;
    SGM::BoxTree Tree1,Tree2;
    FindFaceTrees(rResult,pKeepBody,pDeleteBody,sFaces1,sFaces2,Tree1,Tree2);

    // Imprint the bodies and find all the faces
    // that came from each body.

    std::vector<std::pair<face *,face *> > aSplits;
    std::set<std::pair<surface const *,surface const *> > aCoincidentSurfaces;
    ImprintBodies(rResult,pKeepBody,pDeleteBody,aSplits,&aCoincidentSurfaces);
    UpdateFaceTrees(rResult,aSplits,sFaces1,sFaces2,Tree1,Tree2);

    int a=0;
    if(a)
        {
        return;
        }
    
    // Delete faces from pKeepBody that are inside of pDeleteBody,
    // face from pDeleteBody that are inside pKeepBody.
    
    std::vector<face *> aDelete;
    std::vector<std::pair<face *,face *> > aCoincidentFaces;
    std::vector<bool> aFlippedFace;
    std::set<face *> sCoincident;
    LookForCoincidentFaces(aCoincidentSurfaces,aCoincidentFaces,aFlippedFace);
    size_t nCoincidentFaces=aCoincidentFaces.size();
    size_t Index1;
    for(Index1=0;Index1<nCoincidentFaces;++Index1)
        {
        auto FacePair=aCoincidentFaces[Index1];
        bool bFlip=aFlippedFace[Index1];
        face *pFace0=FacePair.first;
        face *pFace1=FacePair.second;
        sCoincident.insert(pFace0);
        sCoincident.insert(pFace1);
        if(pFace1->GetSides()==2)
            {
            aDelete.push_back(pFace1);
            }
        else if(bFlip)
            {
            aDelete.push_back(pFace0);
            aDelete.push_back(pFace1);
            }
        }

    for(SGM::Face FaceID : sFaces1)
        {
        face *pFace=(face *)rResult.GetThing()->FindEntity(FaceID.m_ID);
        if(FaceInFaces(rResult,pFace,Tree2) && sCoincident.find(pFace)==sCoincident.end())
            {
            aDelete.push_back(pFace);
            }
        }
    for(SGM::Face FaceID : sFaces2)
        {
        face *pFace=(face *)rResult.GetThing()->FindEntity(FaceID.m_ID);
        if(FaceInFaces(rResult,pFace,Tree1) && sCoincident.find(pFace)==sCoincident.end())
            {
            aDelete.push_back(pFace);
            }
        }

    // Delete the delete faces.

    DeleteFaces(rResult,aDelete);

    FixVolumes(rResult,pKeepBody);

    Merge(rResult,pKeepBody);
    }

void IntersectBodies(SGM::Result &rResult,
                     body        *pKeepBody,
                     body        *pDeleteBody)
    {
    // Find all the faces before imprinting.  Also build box trees
    // of the one sided faces.

    std::set<SGM::Face> sFaces1,sFaces2;
    SGM::BoxTree Tree1,Tree2;
    FindFaceTrees(rResult,pKeepBody,pDeleteBody,sFaces1,sFaces2,Tree1,Tree2);

    // Imprint the bodies and find all the faces
    // that came from each body.

    std::vector<std::pair<face *,face *> > aSplits;
    ImprintBodies(rResult,pKeepBody,pDeleteBody,aSplits,nullptr);
    UpdateFaceTrees(rResult,aSplits,sFaces1,sFaces2,Tree1,Tree2);

    // Delete faces from pKeepBody that are outside of pDeleteBody,
    // and faces from pDeleteBody that are outside of pKeepBody.
    
    std::vector<face *> aDelete;
    for(SGM::Face FaceID : sFaces1)
        {
        face *pFace=(face *)rResult.GetThing()->FindEntity(FaceID.m_ID);
        if(FaceInFaces(rResult,pFace,Tree2)==false)
            {
            aDelete.push_back(pFace);
            }
        }
    for(SGM::Face FaceID : sFaces2)
        {
        face *pFace=(face *)rResult.GetThing()->FindEntity(FaceID.m_ID);
        if(FaceInFaces(rResult,pFace,Tree1)==false)
            {
            aDelete.push_back(pFace);
            }
        }
    
    // Delete the delete faces.

    DeleteFaces(rResult,aDelete);

    FixVolumes(rResult,pKeepBody);
    }

void FindWindingNumbers(surface                   const *pSurface,
                        std::vector<SGM::Point3D> const &aPolygon3D,
                        int                             &nUWinds,
                        int                             &nVWinds)
    {
    // Find the uv values of the points on the surface.

    std::vector<SGM::Point2D> aPolygon2D;
    size_t nPolygon=aPolygon3D.size();
    aPolygon2D.reserve(nPolygon);
    size_t Index1;
    for(Index1=0;Index1<nPolygon;++Index1)
        {
        SGM::Point2D uv=pSurface->Inverse(aPolygon3D[Index1]);
        aPolygon2D.push_back(uv);
        }

    // Find the U winding number.

    nUWinds=0;
    if(pSurface->ClosedInU())
        {
        // Find an unused U Value.

        SGM::Interval1D UDomain=pSurface->GetDomain().m_UDomain;
        double dMid=UDomain.MidPoint();
        double dDist=UDomain.Length()/4.0;
        std::vector<double> aUs;
        aUs.reserve(nPolygon);
        for(auto uv : aPolygon2D)
            {
            aUs.push_back(uv.m_u);
            }
        std::sort(aUs.begin(),aUs.end());
        double dU=SGM_MAX;
        for(Index1=1;Index1<nPolygon;++Index1)
            {
            if(SGM_MIN_TOL<aUs[Index1]-aUs[Index1-1] && fabs(aUs[Index1]-dMid)<dDist)
                {
                double dTest=(aUs[Index1]+aUs[Index1-1])*0.5;
                if( fabs(dTest-dMid)<fabs(dU-dMid) )
                    {
                    dU=dTest;
                    }
                }
            }
    
        // Find the crossing of dU.

        for(Index1=1;Index1<=nPolygon;++Index1)
            {
            SGM::Point2D const &uv1=aPolygon2D[Index1%nPolygon];
            if(fabs(uv1.m_u-dMid)<dDist)
                {
                SGM::Point2D const &uv0=aPolygon2D[Index1-1];
                if(uv0.m_u<dU && dU<uv1.m_u)
                    {
                    ++nUWinds;
                    }
                else if(uv1.m_u<dU && dU<uv0.m_u)
                    {
                    --nUWinds;
                    }
                }
            }
        }

    // Find the V winding number.

    nVWinds=0;
    if(pSurface->ClosedInV())
        {
        // Find an unused V value.

        SGM::Interval1D VDomain=pSurface->GetDomain().m_VDomain;
        double dMid=VDomain.MidPoint();
        double dDist=VDomain.Length()/4.0;
        std::vector<double> aVs;
        aVs.reserve(nPolygon);
        for(auto uv : aPolygon2D)
            {
            aVs.push_back(uv.m_v);
            }
        std::sort(aVs.begin(),aVs.end());
        double dV=SGM_MAX;
        for(Index1=1;Index1<nPolygon;++Index1)
            {
            if(SGM_MIN_TOL<aVs[Index1]-aVs[Index1-1] && fabs(aVs[Index1]-dMid)<dDist)
                {
                double dTest=(aVs[Index1]+aVs[Index1-1])*0.5;
                if( fabs(dTest-dMid)<fabs(dV-dMid) )
                    {
                    dV=dTest;
                    }
                }
            }

        // Find the crossings of dV.

        for(Index1=1;Index1<=nPolygon;++Index1)
            {
            SGM::Point2D const &uv1=aPolygon2D[Index1%nPolygon];
            if(fabs(uv1.m_v-dMid)<dDist)
                {
                SGM::Point2D const &uv0=aPolygon2D[Index1-1];
                if(uv0.m_v<dV && dV<uv1.m_v)
                    {
                    ++nVWinds;
                    }
                else if(uv1.m_v<dV && dV<uv0.m_v)
                    {
                    --nVWinds;
                    }
                }
            }
        }
    }

bool ConsistentFaces(SGM::Result &rResult,
                     face  const *pFace1,
                     face  const *pFace2)
    {
    std::vector<edge *> aEdges;
    FindCommonEdgesFromFaces(rResult,pFace1,pFace2,aEdges);
    edge *pEdge=aEdges[0];
    std::set<face *,EntityCompare> const &sFaces=pEdge->GetFaces();
    if(sFaces.size()==2)
        {
        if(pFace1->GetSideType(pEdge)==pFace2->GetSideType(pEdge))
            {
            return false;
            }
        return true;
        }
    return false;
    }

void OrientFaces(SGM::Result                    &rResult,
                 std::set<face *,EntityCompare> &sfaces)
    {
    std::set<face *,EntityCompare> sUsed;
    face *pFace=*sfaces.begin();
    sfaces.erase(pFace);
    sUsed.insert(pFace);
    while(sfaces.size())
        {
        face *pTestFace=*sfaces.begin();
        std::set<face *,EntityCompare> sAdjacentFaces;
        FindAdjacentFaces(rResult,pTestFace,sAdjacentFaces);
        for(auto pAdjacentFace : sAdjacentFaces)
            {
            if(sUsed.find(pAdjacentFace)!=sUsed.end())
                {
                if(ConsistentFaces(rResult,pTestFace,pAdjacentFace)==false)
                    {
                    pFace->Negate();
                    }
                sUsed.insert(pTestFace);
                sfaces.erase(pTestFace);
                break;
                }
            }
        }
    }

void OrientBody(SGM::Result &rResult,
                body        *pBody)
    {
    std::set<face *,EntityCompare> sFaces;
    FindFaces(rResult,pBody,sFaces);
    if(!sFaces.empty())
        {
        std::set<SGM::Face> sDouble;
        for(auto pFace : sFaces)
            {
            if(pFace->GetSides()==2)
                {
                sDouble.insert(SGM::Face(pFace->GetID()));
                }
            }
        SGM::Graph graph(rResult,sDouble,true);
        std::vector<SGM::Graph> aComponents;
        graph.FindComponents(aComponents);
        for(auto comp : aComponents)
            {
            std::set<size_t> const &sVerts=comp.GetVertices();
            std::set<face *,EntityCompare> sfaces;
            for(auto FaceID : sVerts)
                {
                sfaces.insert((face *)rResult.GetThing()->FindEntity(FaceID));
                }
            OrientFaces(rResult,sfaces);
            }
        }
    }

} // End of SGMInternal namespace.

