#include "SGMVector.h"
#include "SGMMathematics.h"
#include "SGMInterval.h"
#include "SGMResult.h"
#include "SGMEnums.h"
#include "SGMBoxTree.h"
#include "SGMSegment.h"

#include "EntityClasses.h"
#include "Topology.h"
#include "Faceter.h"
#include "Graph.h"
#include "Surface.h"
#include "Curve.h"
#include "Primitive.h"

#include <list>
#include <cmath>
#include <algorithm>

namespace SGMInternal
{

edge *FindEdge(SGM::Result &rResult,
               face  const *pFace,
               entity      *pEntA,
               entity      *pEntB)
    {
    if(pEntA->GetType()==SGM::FaceType || pEntB->GetType()==SGM::FaceType)
        {
        return nullptr;
        }
    if(pEntA->GetType()==SGM::EdgeType && pEntB->GetType()==SGM::EdgeType && pEntA!=pEntB)
        {
        return nullptr;
        }
    if(pEntA->GetType()==SGM::EdgeType && pEntB->GetType()==SGM::VertexType)
        {
        edge *pEdge=(edge *)pEntA;
        vertex *pVertex=(vertex *)pEntB;
        if(pEdge->GetStart()!=pVertex && pEdge->GetEnd()!=pVertex)
            {
            return nullptr;
            }
        }
    if(pEntB->GetType()==SGM::EdgeType && pEntA->GetType()==SGM::VertexType)
        {
        edge *pEdge=(edge *)pEntB;
        vertex *pVertex=(vertex *)pEntA;
        if(pEdge->GetStart()!=pVertex && pEdge->GetEnd()!=pVertex)
            {
            return nullptr;
            }
        }
    if(pEntA->GetType()==SGM::EdgeType)
        {
        return (edge *)pEntA;
        }
    if(pEntB->GetType()==SGM::EdgeType)
        {
        return (edge *)pEntB;
        }
    vertex *pVertex1=(vertex *)pEntA;
    vertex *pVertex2=(vertex *)pEntB;
    std::vector<edge *> aEdges;
    size_t nEdges=FindCommonEdgesFromVertices(rResult,pVertex1,pVertex2,aEdges,pFace);
    if(nEdges)
        {
        return aEdges[0];
        }
    return nullptr;
    }

void SubdivideFacets(SGM::Result               &rResult,
                     face                const *pFace,
                     std::vector<SGM::Point3D> &aPoints3D,
                     std::vector<SGM::Point2D> &aPoints2D,
                     std::vector<unsigned int> &aTriangles,
                     std::vector<entity *>     &aEntities)
    {
    // First find all the edge mid points.

    size_t nTriangles=aTriangles.size();
    std::vector<unsigned int> aAdjacencies;
    SGM::FindAdjacences2D(aTriangles,aAdjacencies);
    surface const *pSurface=pFace->GetSurface();
    std::map<std::pair<unsigned int,unsigned int>,unsigned int> mMidPoints;
    std::vector<SGM::Point3D> aNewPoints3D;
    std::vector<SGM::Point2D> aNewPoints2D;
    std::vector<entity *> aNewEntities;
    size_t Index1;
    size_t nCount=aPoints2D.size();
    for(Index1=0;Index1<nTriangles;Index1+=3)
        {
        unsigned int a=aTriangles[Index1];
        unsigned int b=aTriangles[Index1+1];
        unsigned int c=aTriangles[Index1+2];
        entity *pEntA=aEntities[a];
        entity *pEntB=aEntities[b];
        entity *pEntC=aEntities[c];
        SGM::Point3D const &A=aPoints3D[a];
        SGM::Point3D const &B=aPoints3D[b];
        SGM::Point3D const &C=aPoints3D[c];
        SGM::Point2D const &Auv=aPoints2D[a];
        SGM::Point2D const &Buv=aPoints2D[b];
        SGM::Point2D const &Cuv=aPoints2D[c];

        if(aAdjacencies[Index1]==std::numeric_limits<unsigned int>::max() || a<b)
            {
            SGM::Point3D AB;
            SGM::Point2D ABuv;
            edge *pEdge0=aAdjacencies[Index1]==std::numeric_limits<unsigned int>::max() ? FindEdge(rResult,pFace,pEntA,pEntB) : nullptr;
            if(pEdge0)
                {
                curve const *pCurve=pEdge0->GetCurve();
                SGM::Point3D MidPos=SGM::MidPoint(A,B);
                pCurve->Inverse(MidPos,&AB);
                ABuv=pSurface->Inverse(AB);
                }
            else
                {
                ABuv=SGM::MidPoint(Auv,Buv);
                pSurface->Evaluate(ABuv,&AB);
                }

            mMidPoints[std::pair<unsigned int,unsigned int>(a<b ? a : b,a<b ? b : a)]=(unsigned int)nCount;
            ++nCount;
            aNewPoints2D.push_back(ABuv);
            aNewPoints3D.push_back(AB);
            aNewEntities.push_back(pEdge0 ? pEdge0 : (entity *)pFace);
            }

        if(aAdjacencies[Index1+1]==std::numeric_limits<unsigned int>::max() || b<c)
            {
            SGM::Point3D BC;
            SGM::Point2D BCuv;
            edge *pEdge1=aAdjacencies[Index1+1]==std::numeric_limits<unsigned int>::max() ? FindEdge(rResult,pFace,pEntB,pEntC) : nullptr;
            if(pEdge1)
                {
                curve const *pCurve=pEdge1->GetCurve();
                SGM::Point3D MidPos=SGM::MidPoint(B,C);
                pCurve->Inverse(MidPos,&BC);
                BCuv=pSurface->Inverse(BC);
                }
            else
                {
                BCuv=SGM::MidPoint(Buv,Cuv);
                pSurface->Evaluate(BCuv,&BC);
                }

            mMidPoints[std::pair<unsigned int,unsigned int>(b<c ? b : c,b<c ? c : b)]=(unsigned int)nCount;
            ++nCount;
            aNewPoints2D.push_back(BCuv);
            aNewPoints3D.push_back(BC);
            aNewEntities.push_back(pEdge1 ? pEdge1 : (entity *)pFace);
            }

        if(aAdjacencies[Index1+2]==std::numeric_limits<unsigned int>::max() || c<a)
            {
            SGM::Point3D CA;
            SGM::Point2D CAuv;
            edge *pEdge2=aAdjacencies[Index1+2]==std::numeric_limits<unsigned int>::max() ? FindEdge(rResult,pFace,pEntC,pEntA) : nullptr;
            if(pEdge2)
                {
                curve const *pCurve=pEdge2->GetCurve();
                SGM::Point3D MidPos=SGM::MidPoint(C,A);
                pCurve->Inverse(MidPos,&CA);
                CAuv=pSurface->Inverse(CA);
                }
            else
                {
                CAuv=SGM::MidPoint(Cuv,Auv);
                pSurface->Evaluate(CAuv,&CA);
                }

            mMidPoints[std::pair<unsigned int,unsigned int>(c<a ? c : a,c<a ? a : c)]=(unsigned int)nCount;
            ++nCount;
            aNewPoints2D.push_back(CAuv);
            aNewPoints3D.push_back(CA);
            aNewEntities.push_back(pEdge2 ? pEdge2 : (entity *)pFace);
            }
        }

    // Add the new points to the output.

    size_t nNewPoints2D=aNewPoints2D.size();
    for(Index1=0;Index1<nNewPoints2D;++Index1)
        {
        aPoints2D.push_back(aNewPoints2D[Index1]);
        aPoints3D.push_back(aNewPoints3D[Index1]);
        aEntities.push_back(aNewEntities[Index1]);
        }

    // Split the triangles.

    for(Index1=0;Index1<nTriangles;Index1+=3)
        {
        unsigned int a=aTriangles[Index1];
        unsigned int b=aTriangles[Index1+1];
        unsigned int c=aTriangles[Index1+2];

        unsigned int ab=mMidPoints[a<b ? std::pair<unsigned int,unsigned int>(a,b) : std::pair<unsigned int,unsigned int>(b,a)];
        unsigned int bc=mMidPoints[b<c ? std::pair<unsigned int,unsigned int>(b,c) : std::pair<unsigned int,unsigned int>(c,b)];
        unsigned int ca=mMidPoints[c<a ? std::pair<unsigned int,unsigned int>(c,a) : std::pair<unsigned int,unsigned int>(a,c)];

        aTriangles[Index1]=a;
        aTriangles[Index1+1]=ab;
        aTriangles[Index1+2]=ca;

        aTriangles.push_back(ab);
        aTriangles.push_back(b);
        aTriangles.push_back(bc);

        aTriangles.push_back(ca);
        aTriangles.push_back(ab);
        aTriangles.push_back(bc);

        aTriangles.push_back(ca);
        aTriangles.push_back(bc);
        aTriangles.push_back(c);
        }
    }

class Node
    {
    public:

        Node() {m_bMark=false;}

        size_t               m_nNext;
        size_t               m_nPrevious;
        SGM::Point3D         m_Pos;
        SGM::Point2D         m_uv;
        double               m_t;
        SGMInternal::entity *m_Entity;
        bool                 m_bMark;
    };

void FixBackPointers(unsigned int                     nTri,
                     std::vector<unsigned int> const &aTriangles,
                     std::vector<unsigned int>       &aAdjacencies)
    {
    unsigned int nT0=aAdjacencies[nTri];
    unsigned int nT1=aAdjacencies[nTri+1];
    unsigned int nT2=aAdjacencies[nTri+2];
    unsigned int a=aTriangles[nTri];
    unsigned int b=aTriangles[nTri+1];
    unsigned int c=aTriangles[nTri+2];
    
    if(nT0!=std::numeric_limits<unsigned int>::max())
        {
        unsigned int a0=aTriangles[nT0];
        unsigned int b0=aTriangles[nT0+1];
        if(a0!=a && a0!=b)
            {
            aAdjacencies[nT0+1]=nTri;
            }
        else if(b0!=a && b0!=b)
            {
            aAdjacencies[nT0+2]=nTri;
            }
        else
            {
            aAdjacencies[nT0]=nTri;
            }
        }

    if(nT1!=std::numeric_limits<unsigned int>::max())
        {
        unsigned int a1=aTriangles[nT1];
        unsigned int b1=aTriangles[nT1+1];
        if(a1!=c && a1!=b)
            {
            aAdjacencies[nT1+1]=nTri;
            }
        else if(b1!=c && b1!=b)
            {
            aAdjacencies[nT1+2]=nTri;
            }
        else
            {
            aAdjacencies[nT1]=nTri;
            }
        }

    if(nT2!=std::numeric_limits<unsigned int>::max())
        {
        unsigned int a2=aTriangles[nT2];
        unsigned int b2=aTriangles[nT2+1];
        if(a2!=a && a2!=c)
            {
            aAdjacencies[nT2+1]=nTri;
            }
        else if(b2!=a && b2!=c)
            {
            aAdjacencies[nT2+2]=nTri;
            }
        else
            {
            aAdjacencies[nT2]=nTri;
            }
        }
    }

bool TestTriangle(SGM::Point2D const &A,
                  SGM::Point2D const &B,
                  SGM::Point2D const &C)
    {
    SGM::Vector2D Vec1=B-A;
    SGM::Vector2D Vec2=C-A;
    double dTest=Vec1.m_u*Vec2.m_v-Vec1.m_v*Vec2.m_u;
    return 0<dTest;
    }

bool FlipTriangles(std::vector<SGM::Point2D> const &aPoints,
                   std::vector<unsigned int>       &aTriangles,
                   std::vector<unsigned int>       &aAdjacencies,
                   unsigned int                     nTri,
                   unsigned int                     nEdge)
    {
    unsigned int a=aTriangles[nTri];
    unsigned int b=aTriangles[nTri+1];
    unsigned int c=aTriangles[nTri+2];
    unsigned int nT=aAdjacencies[nTri+nEdge];
    if(nT==std::numeric_limits<unsigned int>::max())
        {
        return false;
        }
    unsigned int d=aTriangles[nT];
    unsigned int e=aTriangles[nT+1];
    unsigned int f=aTriangles[nT+2];
    unsigned int g,nTA,nTB;
    if(d!=a && d!=b && d!=c)
        {
        g=d;
        nTA=aAdjacencies[nT+2];
        nTB=aAdjacencies[nT];
        }
    else if(e!=a && e!=b && e!=c)
        {
        g=e;
        nTA=aAdjacencies[nT];
        nTB=aAdjacencies[nT+1];
        }
    else
        {
        g=f;
        nTA=aAdjacencies[nT+1];
        nTB=aAdjacencies[nT+2];
        }
    SGM::Point2D const &A=aPoints[a];
    SGM::Point2D const &B=aPoints[b];
    SGM::Point2D const &C=aPoints[c];
    SGM::Point2D const &G=aPoints[g];
    double dDet;
    SGM::InCircumcircle(A,B,C,G,dDet);
    bool bFlip=false;
    double dTol=SGM_MIN_TOL;
    if(dTol<dDet)
        {
        bFlip=true;
        }
    else if(fabs(dDet)<=dTol)
        {
        if(nEdge==0)
            {
            // CG<AB
            if(G.Distance(C)+dTol<A.Distance(B) && TestTriangle(G,C,A) && TestTriangle(G,B,C))
                {
                bFlip=true;
                }
            }
        else if(nEdge==1)
            {
            // AG<BC
            if(G.Distance(A)+dTol<C.Distance(B) && TestTriangle(G,A,B) && TestTriangle(G,C,A))
                {
                bFlip=true;
                }
            }
        else
            {
            // BG<CA
            if(G.Distance(B)+dTol<A.Distance(C) && TestTriangle(G,A,B) && TestTriangle(G,B,C))
                {
                bFlip=true;
                }
            }
        }
    if(bFlip)
        {
        unsigned int nT0=aAdjacencies[nTri];
        unsigned int nT1=aAdjacencies[nTri+1];
        unsigned int nT2=aAdjacencies[nTri+2];
        if(nEdge==0)
            {
            aTriangles[nTri]=g;
            aTriangles[nTri+1]=c;
            aTriangles[nTri+2]=a;
            aTriangles[nT]=g;
            aTriangles[nT+1]=b;
            aTriangles[nT+2]=c;

            aAdjacencies[nTri]=nT;
            aAdjacencies[nTri+1]=nT2;
            aAdjacencies[nTri+2]=nTA;
            aAdjacencies[nT]=nTB;
            aAdjacencies[nT+1]=nT1;
            aAdjacencies[nT+2]=nTri;
            }
        else if(nEdge==1)
            {
            aTriangles[nTri]=g;
            aTriangles[nTri+1]=a;
            aTriangles[nTri+2]=b;
            aTriangles[nT]=g;
            aTriangles[nT+1]=c;
            aTriangles[nT+2]=a;

            aAdjacencies[nTri]=nT;
            aAdjacencies[nTri+1]=nT0;
            aAdjacencies[nTri+2]=nTA;
            aAdjacencies[nT]=nTB;
            aAdjacencies[nT+1]=nT2;
            aAdjacencies[nT+2]=nTri;
            }
        else
            {
            aTriangles[nTri]=g;
            aTriangles[nTri+1]=a;
            aTriangles[nTri+2]=b;
            aTriangles[nT]=g;
            aTriangles[nT+1]=b;
            aTriangles[nT+2]=c;

            aAdjacencies[nTri]=nTB;
            aAdjacencies[nTri+1]=nT0;
            aAdjacencies[nTri+2]=nT;
            aAdjacencies[nT]=nTri;
            aAdjacencies[nT+1]=nT1;
            aAdjacencies[nT+2]=nTA;
            }
        FixBackPointers(nT,aTriangles,aAdjacencies);
        FixBackPointers(nTri,aTriangles,aAdjacencies);
        return true;
        }
    return false;
    }

void FindScales(surface                   const *pSurface,
                std::vector<SGM::Point2D> const &aPoints,
                std::vector<SGM::Vector2D>      &aScales)
    {
    size_t nPoints=aPoints.size();
    size_t Index1;
    for(Index1=0;Index1<nPoints;++Index1)
        {
        SGM::Point2D const &uv=aPoints[Index1];
        SGM::UnitVector3D Vec1,Vec2;
        double k1,k2;
        pSurface->PrincipleCurvature(uv,Vec1,Vec2,k1,k2);
        double r1=fabs(k1)<0.001 ? 1000.0 : 1.0/fabs(k1);
        double r2=fabs(k2)<0.001 ? 1000.0 : 1.0/fabs(k2);
        SGM::Vector3D DU,DV;
        pSurface->Evaluate(uv,nullptr,&DU,&DV);
        SGM::UnitVector3D UnitU=DU,UnitV=DV;
        double dDotU=Vec1%UnitU;
        double dDotV=Vec1%UnitV;
        SGM::Vector2D UVec(1,0),VVec(0,1);
        SGM::Vector2D PVec=(dDotU*UVec+dDotV*VVec)*(r2/r1);
        aScales.push_back(PVec);
        }
    }

void DelaunayFlips(std::vector<SGM::Point2D> const &aPoints,
                   std::vector<unsigned int>       &aTriangles,
                   std::vector<unsigned int>       &aAdjacencies)
    {
    size_t nTriangles=aTriangles.size();
    bool bFlipped=true;
    unsigned int Index1;
    while(bFlipped)
        {
        bFlipped=false;
        for(Index1=0;Index1<(unsigned int)nTriangles;Index1+=3)
            {
            if(FlipTriangles(aPoints,aTriangles,aAdjacencies,Index1,0))
                {
                bFlipped=true;
                }
            if(FlipTriangles(aPoints,aTriangles,aAdjacencies,Index1,1))
                {
                bFlipped=true;
                }
            if(FlipTriangles(aPoints,aTriangles,aAdjacencies,Index1,2))
                {
                bFlipped=true;
                }
            }
        }
    }

class FacetNode
    {
    public:

    FacetNode(double              dParam,
              SGM::Point3D const &Pos):m_dParam(dParam),m_Pos(Pos) {}

    double       m_dParam;
    SGM::Point3D m_Pos;
    SGM::Point2D m_uv;
    };

class FacetNodeNormal
    {
    public:

    FacetNodeNormal(double              dParam,
                    SGM::Point3D const &Pos):m_dParam(dParam),m_Pos(Pos) {m_bSingular=false;}

    double            m_dParam;
    SGM::Point3D      m_Pos;
    SGM::UnitVector3D m_Norm;
    bool              m_bSingular;
    };

void FacetCurve(curve               const *pCurve,
                SGM::Interval1D     const &Domain,
                FacetOptions        const &Options,
                std::vector<SGM::Point3D> &aPoints3D,
                std::vector<double>       &aParams)
    {
    SGM::EntityType nCurveType=pCurve->GetCurveType();
    switch(nCurveType)
        {
        case SGM::LineType:
            {
            SGM::Point3D Start,End;
            line const *pLine=(line const *)pCurve;
            pLine->Evaluate(Domain.m_dMin,&Start);
            pLine->Evaluate(Domain.m_dMax,&End);
            aPoints3D.reserve(2);
            aPoints3D.push_back(Start);
            aPoints3D.push_back(End);
            aParams.push_back(Domain.m_dMin);
            aParams.push_back(Domain.m_dMax);
            break;
            }
        case SGM::CircleType:
            {
            circle const *pCircle=(circle const *)pCurve;
            double dRadius=pCircle->GetRadius();
            double dDomainLength=Domain.Length();
            double dAngle=Options.m_dEdgeAngleTol;
            if(Options.m_dMaxLength)
                {
                double dArcLength=dDomainLength*dRadius;
                double dAngleFraction=dArcLength/Options.m_dMaxLength;
                dAngle=std::min(dAngle,dDomainLength*dAngleFraction);
                }
            if(Options.m_dCordHight)
                {
                // CordAngle = 2*acos(1-CordHight/Radius)
                dAngle=std::min(dAngle,2.0*acos(1.0-Options.m_dCordHight/dRadius));
                }
            size_t nPoints=std::min((size_t)(1.0000001+dDomainLength/dAngle),Options.m_nMaxFacets);
            if(nPoints<3)
                {
                nPoints=3;
                }
            aPoints3D.reserve(nPoints);
            aParams.reserve(nPoints);
            size_t Index1;
            for(Index1=0;Index1<nPoints;++Index1)
                {
                double dFraction=(Index1/(nPoints-1.0));
                double dt=Domain.MidPoint(dFraction);
                aParams.push_back(dt);
                SGM::Point3D Pos;
                pCircle->Evaluate(dt,&Pos);
                aPoints3D.push_back(Pos);
                }
            break;
            }
        case SGM::TorusKnotCurveType:
            {
            TorusKnot const *pTorusKnot=(TorusKnot const *)pCurve;
            double dRadius=pTorusKnot->m_dMinorRadius;
            size_t nA=pTorusKnot->m_nA;
            size_t nB=pTorusKnot->m_nB;
            double dDomainLength=Domain.Length()*nA*nB;
            double dAngle=Options.m_dEdgeAngleTol;
            if(Options.m_dMaxLength)
                {
                double dArcLength=dDomainLength*dRadius;
                double dAngleFraction=dArcLength/Options.m_dMaxLength;
                dAngle=std::min(dAngle,dDomainLength*dAngleFraction);
                }
            if(Options.m_dCordHight)
                {
                // CordAngle = 2*acos(1-CordHight/Radius)
                dAngle=std::min(dAngle,2.0*acos(1.0-Options.m_dCordHight/dRadius));
                }
            size_t nPoints=std::min((size_t)(1.0000001+dDomainLength/dAngle),Options.m_nMaxFacets);
            if(nPoints<3)
                {
                nPoints=3;
                }
            aPoints3D.reserve(nPoints);
            aParams.reserve(nPoints);
            size_t Index1;
            for(Index1=0;Index1<nPoints;++Index1)
                {
                double dFraction=(Index1/(nPoints-1.0));
                double dt=Domain.MidPoint(dFraction);
                aParams.push_back(dt);
                SGM::Point3D Pos;
                pTorusKnot->Evaluate(dt,&Pos);
                aPoints3D.push_back(Pos);
                }
            break;
            }
        default:
            {
            std::list<FacetNode> lNodes;
            SGM::Point3D Pos0,Pos1;
            double d0=Domain.m_dMin;
            double d1=Domain.m_dMax;
            double dMinLength=Domain.Length()*SGM_FIT;
            pCurve->Evaluate(d0,&Pos0);
            pCurve->Evaluate(d1,&Pos1);
            lNodes.emplace_back(d0,Pos0);
            if(pCurve->GetClosed())
                {
                SGM::Point3D Pos2;
                double d2=(d0+d1)*0.5;
                pCurve->Evaluate(d2,&Pos2);
                lNodes.emplace_back(d2,Pos2);
                }
            lNodes.emplace_back(d1,Pos1);
            double dAngle=SGM_PI-Options.m_dEdgeAngleTol;
            double dCos=cos(dAngle);
            bool bRefine=true;
            while(bRefine)
                {
                bRefine=false;
                std::list<FacetNode>::iterator LastIter=lNodes.begin();
                std::list<FacetNode>::iterator iter=lNodes.begin();
                ++iter;
                while(iter!=lNodes.end())
                    {
                    d0=LastIter->m_dParam;
                    d1=iter->m_dParam;

                    // Check to see if the segment from d0 to d1 needs more facets.

                    double da=d0*0.65433+d1*0.34567;
                    double db=d0*0.5+d1*0.5;
                    double dc=d0*0.34567+d1*0.65433;

                    Pos0=LastIter->m_Pos;
                    Pos1=iter->m_Pos;
                    SGM::Point3D PosA,PosB,PosC;
                    pCurve->Evaluate(da,&PosA);
                    pCurve->Evaluate(db,&PosB);
                    pCurve->Evaluate(dc,&PosC);

                    double a1=SGM::UnitVector3D(Pos0-PosA)%SGM::UnitVector3D(Pos1-PosA);
                    double a2=SGM::UnitVector3D(Pos0-PosB)%SGM::UnitVector3D(Pos1-PosB);
                    double a3=SGM::UnitVector3D(Pos0-PosC)%SGM::UnitVector3D(Pos1-PosC);
                    if(dMinLength<(dc-da) && (dCos<a1 || dCos<a2 || dCos<a3))
                        {
                        bRefine=true;
                        lNodes.insert(iter,FacetNode(db,PosB));
                        iter=LastIter;
                        }
                    else
                        {
                        ++LastIter;
                        }
                    ++iter;
                    }
                }
            size_t nNodes=lNodes.size();
            aPoints3D.reserve(nNodes);
            aParams.reserve(nNodes);
            std::list<FacetNode>::iterator iter2=lNodes.begin();
            while(iter2!=lNodes.end())
                {
                aPoints3D.push_back(iter2->m_Pos);
                aParams.push_back(iter2->m_dParam);
                ++iter2;
                }
            }
        }
    }

void FindCrossingPoint(curve  const *pCurve1, // Seam
                       curve  const *pCurve2, 
                       SGM::Point3D &Pos,     // Start point returned as answer.
                       double       &t)       // pCurve2 params, also returned as answer.
    {
    SGM::Point3D CPos;
    pCurve1->Inverse(Pos,&CPos);
    double dDist=Pos.Distance(CPos);
    Pos=CPos;
    size_t nCount=0;
    while(SGM_ZERO<dDist && nCount<100)
        {
        t=pCurve2->Inverse(Pos,&CPos,&t);
        pCurve1->Inverse(CPos,&Pos);
        dDist=Pos.Distance(CPos);
        ++nCount;
        }
    }

bool SplitAtSeams(SGM::Result                     &rResult,
                  surface                   const *pSurface,
                  edge                      const *pEdge,
                  curve                     const *pCurve,
                  std::vector<SGM::Point3D> const &aPoints3D,
                  std::vector<double>       const &aParams,
                  std::vector<double>             &aCrosses)
    {
    bool bFound=false;
    if(pSurface->ClosedInU() || pSurface->ClosedInV())
        {
        aCrosses.push_back(aParams.front());
        aCrosses.push_back(aParams.back());
        std::vector<Node> aNodes;
        unsigned int nParams=(unsigned int)aParams.size();
        unsigned int Index1;
        for(Index1=0;Index1<nParams;++Index1)
            {
            Node PosNode;
            PosNode.m_t=aParams[Index1];
            PosNode.m_Pos=aPoints3D[Index1];
            PosNode.m_uv=pSurface->Inverse(PosNode.m_Pos);
            if(Index1==nParams-1)
                {
                PosNode.m_nNext=0;
                }
            else
                {
                PosNode.m_nNext=Index1+1;
                }
            aNodes.push_back(PosNode);
            }
        if(pSurface->ClosedInU())
            {
            SGM::Interval1D const &Domain=pSurface->GetDomain().m_UDomain;
            double dGap=Domain.Length()*0.5;
            curve *pSeam=pSurface->UParamLine(rResult,Domain.m_dMin);
            for(Index1=0;Index1<nParams;++Index1)
                {
                Node const &Node0=aNodes[Index1];
                Node const &Node1=aNodes[Node0.m_nNext];
                double dDist=fabs(Node0.m_uv.m_u-Node1.m_uv.m_u);
                if(dGap<dDist)
                    {
                    SGM::Point3D Pos=Node0.m_Pos;
                    double t=Node0.m_t;
                    FindCrossingPoint(pSeam,pCurve,Pos,t);
                    if(SGM_MIN_TOL<fabs(t-Node0.m_t) && SGM_MIN_TOL<fabs(t-Node1.m_t))
                        {
                        if(pEdge->GetDomain().InInterval(t,SGM_MIN_TOL))
                            {
                            aCrosses.push_back(t);
                            bFound=true;
                            }
                        }
                    }
                }
            rResult.GetThing()->DeleteEntity(pSeam);
            }
        if(pSurface->ClosedInV())
            {
            SGM::Interval1D const &Domain=pSurface->GetDomain().m_VDomain;
            double dGap=Domain.Length()*0.5;
            curve *pSeam=pSurface->VParamLine(rResult,Domain.m_dMin);
            for(Index1=0;Index1<nParams;++Index1)
                {
                Node const &Node0=aNodes[Index1];
                Node const &Node1=aNodes[Node0.m_nNext];
                double dDist=fabs(Node0.m_uv.m_v-Node1.m_uv.m_v);
                if(dGap<dDist)
                    {
                    SGM::Point3D Pos=Node0.m_Pos;
                    double t=Node0.m_t;
                    FindCrossingPoint(pSeam,pCurve,Pos,t);
                    if(SGM_MIN_TOL<fabs(t-Node0.m_t) && SGM_MIN_TOL<fabs(t-Node1.m_t))
                        {
                        if(pEdge->GetDomain().InInterval(t,SGM_MIN_TOL))
                            {
                            aCrosses.push_back(t);
                            bFound=true;
                            }
                        }
                    }
                }
            rResult.GetThing()->DeleteEntity(pSeam);
            }
        }
    return bFound;
    }

void SplitFacet(curve                          const *pCurve,
                surface                        const *pSurface,
                std::list<FacetNodeNormal>::iterator &NodeA,
                std::list<FacetNodeNormal>::iterator &NodeB,
                std::list<FacetNodeNormal>           &lNodes)
    {
    double dParamA=NodeA->m_dParam;
    double dParamB=NodeB->m_dParam;
    double dParamC=(dParamA+dParamB)*0.5;
    SGM::Point3D Pos;
    pCurve->Evaluate(dParamC,&Pos);
    SGM::Point2D uv=pSurface->Inverse(Pos);
    FacetNodeNormal NodeC(dParamC,Pos);
    pSurface->Evaluate(uv,nullptr,nullptr,nullptr,&NodeC.m_Norm);
    lNodes.insert(NodeB,NodeC);
    NodeB=NodeA;
    ++NodeB;
    }

void SplitWithSurfaceNormals(FacetOptions        const &Options,
                             surface             const *pSurface,
                             curve               const *pCurve,
                             std::vector<SGM::Point3D> &aPoints3D,
                             std::vector<double>       &aParams)
    {
    std::list<FacetNodeNormal> lNodes;
    size_t nPoints=aPoints3D.size();
    size_t Index1;
    for(Index1=0;Index1<nPoints;++Index1)
        {
        SGM::Point3D const &Pos=aPoints3D[Index1];
        SGM::Point2D uv=pSurface->Inverse(Pos);
        SGM::UnitVector3D Norm;
        bool bSingular=false;
        if(Index1==0 && pSurface->IsSingularity(uv,SGM_MIN_TOL))
            {
            bSingular=true;
            double dParamA=aParams[0];
            double dParamB=aParams[1];
            double dParam=dParamA*(1.0-SGM_FIT)+dParamB*SGM_FIT;
            SGM::Point3D ClosePos;
            pCurve->Evaluate(dParam,&ClosePos);
            SGM::Point2D CloseUV=pSurface->Inverse(ClosePos);
            pSurface->Evaluate(CloseUV,nullptr,nullptr,nullptr,&Norm);
            }
        else if(Index1==nPoints-1 && pSurface->IsSingularity(uv,SGM_MIN_TOL))
            {
            bSingular=true;
            double dParamA=aParams[nPoints-1];
            double dParamB=aParams[nPoints-2];
            double dParam=dParamA*(1.0-SGM_FIT)+dParamB*SGM_FIT;
            SGM::Point3D ClosePos;
            pCurve->Evaluate(dParam,&ClosePos);
            SGM::Point2D CloseUV=pSurface->Inverse(ClosePos);
            pSurface->Evaluate(CloseUV,nullptr,nullptr,nullptr,&Norm);
            }
        else
            {
            pSurface->Evaluate(uv,nullptr,nullptr,nullptr,&Norm);
            }
        FacetNodeNormal Node(aParams[Index1],Pos);
        if(bSingular)
            {
            Node.m_bSingular=true;
            }
        Node.m_Norm=Norm;
        lNodes.push_back(Node);
        }
    bool bSplit=false;
    double dDotTol=std::cos(Options.m_dEdgeAngleTol);
    std::list<FacetNodeNormal>::iterator iter=lNodes.begin();
    std::list<FacetNodeNormal>::iterator LastIter=iter;
    ++iter;
    while(iter!=lNodes.end())
        {
        if(iter->m_bSingular==false && LastIter->m_bSingular==false && iter->m_Norm%LastIter->m_Norm<dDotTol)
            {
            SplitFacet(pCurve,pSurface,LastIter,iter,lNodes);
            bSplit=true;
            }
        else
            {
            ++LastIter;
            ++iter;
            }
        }
    if(bSplit)
        {
        aPoints3D.clear();
        aParams.clear();
        std::list<FacetNodeNormal>::iterator iterNodeSplit=lNodes.begin();
        while(iterNodeSplit!=lNodes.end())
            {
            aPoints3D.push_back(iterNodeSplit->m_Pos);
            aParams.push_back(iterNodeSplit->m_dParam);
            ++iterNodeSplit;
            }
        }
    }

void FacetEdge(SGM::Result               &rResult,
               edge                const *pEdge,
               FacetOptions        const &Options,
               std::vector<SGM::Point3D> &aPoints3D,
               std::vector<double>       &aParams)
    {
    curve const *pCurve=pEdge->GetCurve();
    SGM::Interval1D const &Domain=pEdge->GetDomain();
    FacetCurve(pCurve,Domain,Options,aPoints3D,aParams);
    
    // Find where the facets cross the seams of their surfaces.

    std::vector<double> aCrosses;
    std::set<surface *,EntityCompare> sSurfaces;
    FindSurfaces(rResult,pEdge,sSurfaces);
    std::set<surface *,EntityCompare>::iterator iter=sSurfaces.begin();
    bool bFound=false;
    while(iter!=sSurfaces.end())
        {
        if(SplitAtSeams(rResult,*iter,pEdge,pCurve,aPoints3D,aParams,aCrosses))
            {
            bFound=true;
            }
        ++iter;
        }

    // Split facets at the seams of their surfaces.

    if(bFound)
        {
        std::vector<double> aEnds;
        std::sort(aCrosses.begin(),aCrosses.end());
        size_t nCrosses=aCrosses.size();
        size_t Index1,Index2;
        aEnds.push_back(aCrosses.front());
        for(Index1=1;Index1<nCrosses;++Index1)
            {
            if(SGM_MIN_TOL<aCrosses[Index1]-aCrosses[Index1-1])
                {
                aEnds.push_back(aCrosses[Index1]);
                }
            }
        size_t nEnds=aEnds.size();
        aEnds[nEnds-1]=aCrosses.back();
        aPoints3D.clear();
        aParams.clear();
        for(Index1=1;Index1<nEnds;++Index1)
            {
            SGM::Interval1D PartDomain(aEnds[Index1-1],aEnds[Index1]);
            std::vector<SGM::Point3D> aTempPos;
            std::vector<double> aTempParams;
            FacetCurve(pCurve,PartDomain,Options,aTempPos,aTempParams);
            size_t nTempPos=aTempPos.size();
            size_t nStart = Index1==1 ? 0 : 1;
            for(Index2=nStart;Index2<nTempPos;++Index2)
                {
                aPoints3D.push_back(aTempPos[Index2]);
                aParams.push_back(aTempParams[Index2]);
                }
            }
        }

    // Subdivide facets by surface normals.

    iter=sSurfaces.begin();
    while(iter!=sSurfaces.end())
        {
        SplitWithSurfaceNormals(Options,*iter,pCurve,aPoints3D,aParams);
        ++iter;
        }
    }

size_t FindUCrosses(SGM::Interval1D           const &UDomain,
                    std::vector<SGM::Point2D> const &aPoints2D,
                    std::vector<size_t>       const &aPolygon,
                    std::vector<double>             &aSplitValues,
                    std::vector<size_t>             &aSplitIndex)
    {
    double dMaxGap=UDomain.Length()*0.5;
    double dMinU=UDomain.m_dMin;
    double dMaxU=UDomain.m_dMax;
    size_t nPoints=aPolygon.size();
    size_t Index1;
    for(Index1=0;Index1<nPoints;++Index1)
        {
        SGM::Point2D const &UV0=aPoints2D[aPolygon[Index1]];
        SGM::Point2D const &UV1=aPoints2D[aPolygon[(Index1+1)%nPoints]];
        double dUGap=fabs(UV0.m_u-UV1.m_u);
        if(dMaxGap<dUGap)
            {
            double d0=std::min(UV0.m_u-dMinU,dMaxU-UV0.m_u);
            double d1=std::min(UV1.m_u-dMinU,dMaxU-UV1.m_u);
            if(d0<d1)
                {
                aSplitValues.push_back(UV0.m_v);
                }
            else
                {
                aSplitValues.push_back(UV1.m_v);
                }
            aSplitIndex.push_back(Index1);
            }
        }
    return aSplitValues.size();
    }

class SplitData
    {
    public:

        SplitData() {}

        SplitData(double dParam,size_t nPolygon,size_t nSpan):
            m_dParam(dParam),m_nPolygon(nPolygon),m_nSpan(nSpan) {}

        bool operator<(SplitData const &Other) const {return m_dParam<Other.m_dParam;}

        double m_dParam;
        size_t m_nPolygon;
        size_t m_nSpan;
    };

class MergeData
    {
    public:

        SplitData m_Split1;
        SplitData m_Split2;
    };

GraphEdge FindGraphEdge(SGM::Interval1D           const &Domain,
                        std::vector<size_t>       const &aSplitMap,
                        size_t                           nPoints,
                        std::vector<SGM::Point2D> const &aPoints2D,
                        size_t                           nPos0,
                        size_t                           nPos1,
                        size_t                          &nCount)
    {
    GraphEdge GE(nPos0,nPos1,nCount);
    ++nCount;
    if(nPos0<nPoints && nPos1<nPoints)
        {
        }
    else if(nPos0<nPoints)
        {
        double u0=aPoints2D[nPos0].m_u;
        double u1A=aPoints2D[nPos1].m_u;
        double u1B=aPoints2D[aSplitMap[nPos1-nPoints]].m_u;
        if(u0<Domain.MidPoint())
            {
            GE.m_nEnd=u1A<u1B ? nPos1 : aSplitMap[nPos1-nPoints];
            }
        else
            {
            GE.m_nEnd=u1A<u1B ? aSplitMap[nPos1-nPoints] : nPos1;
            }
        }
    else if(nPos1<nPoints)
        {
        double u1=aPoints2D[nPos1].m_u;
        double u0A=aPoints2D[nPos0].m_u;
        double u0B=aPoints2D[aSplitMap[nPos0-nPoints]].m_u;
        if(u1<Domain.MidPoint())
            {
            GE.m_nStart=u0A<u0B ? nPos0 : aSplitMap[nPos0-nPoints];
            }
        else
            {
            GE.m_nStart=u0A<u0B ? aSplitMap[nPos0-nPoints] : nPos0;
            }
        }
    else
        {
        throw;
        }
    return GE;
    }

void FindLowGraphEdge(SGM::Interval1D           const &Domain,
                      std::vector<size_t>       const &aSplitMap,
                      size_t                           nPoints,
                      std::vector<SGM::Point2D> const &aPoints2D,
                      size_t                           nPos0A,
                      size_t                           nPos1A,
                      size_t                          &nCount,
                      std::set<GraphEdge>             &sGraphEdges)
    {
    size_t nLow0,nLow1,nHigh0,nHigh1;
    if(aPoints2D[nPos0A].m_u<Domain.MidPoint())
        {
        nLow0=nPos0A;
        nHigh0=aSplitMap[nPos0A-nPoints];
        }
    else
        {
        nLow0=aSplitMap[nPos0A-nPoints];
        nHigh0=nPos0A;
        }
    if(aPoints2D[nPos1A].m_u<Domain.MidPoint())
        {
        nLow1=nPos1A;
        nHigh1=aSplitMap[nPos1A-nPoints];
        }
    else
        {
        nLow1=aSplitMap[nPos1A-nPoints];
        nHigh1=nPos1A;
        }
    size_t nA,nB,nC,nD;
    if(aPoints2D[nLow0].m_v<aPoints2D[nLow1].m_v)
        {
        nA=nLow1;
        nB=nLow0;
        }
    else
        {
        nA=nLow0;
        nB=nLow1;
        }
    if(aPoints2D[nHigh0].m_v<aPoints2D[nHigh1].m_v)
        {
        nC=nHigh0;
        nD=nHigh1;
        }
    else
        {
        nC=nHigh1;
        nD=nHigh0;
        }

    sGraphEdges.insert(GraphEdge(nA,nB,nCount));
    ++nCount;
    sGraphEdges.insert(GraphEdge(nC,nD,nCount));
    ++nCount;
    }

void MergePolygons(face                              const *pFace,
                   std::vector<MergeData>            const &aMerge,
                   std::vector<SGM::Point2D>               &aPoints2D,
                   std::vector<SGM::Point3D>               &aPoints3D,
                   std::vector<entity *>                   &aEntities,
                   std::vector<std::vector<size_t> >       &aaPolygons)
    {
    SGM::Interval1D const &UDomain=pFace->GetSurface()->GetDomain().m_UDomain;
    double dMidDomain=UDomain.MidPoint();

    // Add the span points.

    size_t nPoints=aPoints2D.size();
    std::vector<size_t> aSplitMap;
    size_t nMerge=aMerge.size();
    size_t nSplits=nMerge*2;
    aSplitMap.reserve(nSplits);
    size_t Index1;
    std::vector<SplitData> aSplits;
    aSplits.reserve(nSplits);
    std::vector<size_t> aSplitVertices;
    aSplitVertices.reserve(nSplits);
    std::set<size_t> sUsedPolygons;
    for(Index1=0;Index1<nMerge;++Index1)
        {
        aSplits.push_back(aMerge[Index1].m_Split1);
        aSplits.push_back(aMerge[Index1].m_Split2);
        sUsedPolygons.insert(aMerge[Index1].m_Split1.m_nPolygon);
        sUsedPolygons.insert(aMerge[Index1].m_Split2.m_nPolygon);
        }
    for(Index1=0;Index1<nSplits;++Index1)
        {
        size_t nPoly=aSplits[Index1].m_nPolygon;
        size_t nSpan=aSplits[Index1].m_nSpan;
        std::vector<size_t> &aPoly=aaPolygons[nPoly];
        size_t nPolyPoints=aPoly.size();
        size_t nPos0=aPoly[nSpan];
        size_t nPos1=aPoly[(nSpan+1)%nPolyPoints];
        double u0=aPoints2D[nPos0].m_u;
        double u1=aPoints2D[nPos1].m_u;
        double v0=aPoints2D[nPos0].m_v;
        double v1=aPoints2D[nPos1].m_v;
        SGM::Point3D const &Pos0=aPoints3D[nPos0];
        SGM::Point3D const &Pos1=aPoints3D[nPos1];
        entity *pEnt0=aEntities[nPos0];
        entity *pEnt1=aEntities[nPos1];
        if(UDomain.OnBoundary(u0,SGM_MIN_TOL))
            {
            if(dMidDomain<u0)
                {
                u0=UDomain.m_dMin;
                }
            else
                {
                u0=UDomain.m_dMax;
                }
            aSplitMap.push_back(nPos0);
            nPos0=aPoints2D.size();
            aSplitVertices.push_back(nPos0);
            aPoly[nSpan]=nPos0;
            aPoints2D.emplace_back(u0,v0);
            aPoints3D.push_back(Pos0);
            aEntities.push_back(pEnt0);
            }
        else if(UDomain.OnBoundary(u1,SGM_MIN_TOL))
            {
            if(dMidDomain<u1)
                {
                u1=UDomain.m_dMin;
                }
            else
                {
                u1=UDomain.m_dMax;
                }
            aSplitMap.push_back(nPos1);
            nPos1=aPoints2D.size();
            aSplitVertices.push_back(nPos1);
            aPoly[(nSpan+1)%nPolyPoints]=nPos1;
            aPoints2D.emplace_back(u1,v1);
            aPoints3D.push_back(Pos1);
            aEntities.push_back(pEnt1);
            }
        else
            {
            throw;
            }
        }

    // Create a segment graph.

    size_t nCount=0;
    std::set<GraphEdge> sFacetEdges;
    std::set<size_t> sFacetPoints;
    std::set<size_t>::iterator PolyIter=sUsedPolygons.begin();
    while(PolyIter!=sUsedPolygons.end())
        {
        std::vector<size_t> const &aPoly=aaPolygons[*PolyIter];
        size_t nPoly=aPoly.size();
        for(Index1=0;Index1<nPoly;++Index1)
            {
            size_t nPos0=aPoly[Index1];
            size_t nPos1=aPoly[(Index1+1)%nPoly];
            sFacetPoints.insert(nPos0);
            sFacetEdges.insert(FindGraphEdge(UDomain,aSplitMap,nPoints,aPoints2D,nPos0,nPos1,nCount));
            }
        ++PolyIter;
        }
    for(Index1=0;Index1<nSplits;Index1+=2)
        {
        FindLowGraphEdge(UDomain,aSplitMap,nPoints,aPoints2D,aSplitVertices[Index1],aSplitVertices[Index1+1],nCount,sFacetEdges);
        }
    Graph graph(sFacetPoints,sFacetEdges);
    std::vector<Graph> aComps;
    size_t nComps=graph.FindComponents(aComps);

    // Build the new polygons.

    std::vector<std::vector<size_t> > aaNewPolygons;
    for(Index1=0;Index1<nComps;++Index1)
        {
        Graph const &comp=aComps[Index1];
        if(comp.IsCycle())
            {
            std::vector<size_t> aPolygon;
            comp.OrderVertices(aPolygon);
            aaNewPolygons.push_back(aPolygon);
            }
        else
            {
            throw;
            }
        }
    aaPolygons=aaNewPolygons;
    }

void CreateWholeSurfaceLoop(SGM::Result                       &rResult,
                            face                        const *pFace,
                            FacetOptions                const &Options,
                            std::vector<SGM::Point2D>         &aPoints2D,
                            std::vector<SGM::Point3D>         &aPoints3D,
                            std::vector<entity *>             &aEntities,
                            std::vector<std::vector<size_t> > &aaPolygons,
                            std::vector<size_t>               &aTriangles)
    {
    surface const *pSurface=pFace->GetSurface();
    if(pSurface->ClosedInU())
        {
        if(pSurface->ClosedInV())
            {
            // The torus case.

            SGM::Interval2D const &Domain=pSurface->GetDomain();
            double dMinU=Domain.m_UDomain.m_dMin;
            double dMaxU=Domain.m_UDomain.m_dMax;
            double dMinV=Domain.m_VDomain.m_dMin;
            double dMaxV=Domain.m_VDomain.m_dMax;

            curve *pUSeam=pSurface->UParamLine(rResult,dMinU);
            std::vector<SGM::Point3D> aUTemp3D;
            std::vector<double> aUParams;
            FacetCurve(pUSeam,Domain.m_UDomain,Options,aUTemp3D,aUParams);
            rResult.GetThing()->DeleteEntity(pUSeam);
            
            curve *pVSeam=pSurface->VParamLine(rResult,dMinV);
            std::vector<SGM::Point3D> aVTemp3D;
            std::vector<double> aVParams;
            FacetCurve(pVSeam,Domain.m_VDomain,Options,aVTemp3D,aVParams);
            rResult.GetThing()->DeleteEntity(pVSeam);

            size_t nUTemp3D=aUTemp3D.size();
            size_t nVTemp3D=aVTemp3D.size();
            size_t Index1;
            for(Index1=0;Index1<nVTemp3D-1;++Index1)
                {
                SGM::Point3D const &Pos=aVTemp3D[Index1];
                SGM::Point2D uv=pSurface->Inverse(Pos);
                uv.m_v=dMinV;
                if(Index1==0)
                    {
                    uv.m_u=dMinU;
                    }
                aPoints2D.push_back(uv);
                aPoints3D.push_back(Pos);
                }
            for(Index1=0;Index1<nUTemp3D-1;++Index1)
                {
                SGM::Point3D const &Pos=aUTemp3D[Index1];
                SGM::Point2D uv=pSurface->Inverse(Pos);
                uv.m_u=dMaxU;
                if(Index1==0)
                    {
                    uv.m_v=dMinV;
                    }
                aPoints2D.push_back(uv);
                aPoints3D.push_back(Pos);
                }
            for(Index1=nUTemp3D-1;0<Index1;--Index1)
                {
                SGM::Point3D const &Pos=aVTemp3D[Index1];
                SGM::Point2D uv=pSurface->Inverse(Pos);
                uv.m_v=dMaxV;
                if(Index1==nUTemp3D-1)
                    {
                    uv.m_u=dMaxU;
                    }
                aPoints2D.push_back(uv);
                aPoints3D.push_back(Pos);
                }
            for(Index1=nVTemp3D-1;0<Index1;--Index1)
                {
                SGM::Point3D const &Pos=aUTemp3D[Index1];
                SGM::Point2D uv=pSurface->Inverse(Pos);
                uv.m_u=dMinU;
                if(Index1==nVTemp3D-1)
                    {
                    uv.m_v=dMaxV;
                    }
                aPoints2D.push_back(uv);
                aPoints3D.push_back(Pos);
                }
            size_t nPoints=aPoints3D.size();
            aEntities.assign(nPoints,(entity *)pFace);
            std::vector<size_t> aPolygon;
            aPolygon.reserve(nPoints);
            for(Index1=0;Index1<nPoints;++Index1)
                {
                aPolygon.push_back(Index1);
                }
            aaPolygons.push_back(aPolygon);
            }
        else if(pSurface->SingularLowV() && pSurface->SingularHighV())
            {
            // The sphere case

            SGM::Interval2D const &Domain=pSurface->GetDomain();
            double dMinU=Domain.m_UDomain.m_dMin;
            double dMaxU=Domain.m_UDomain.m_dMax;
            double dMinV=Domain.m_VDomain.m_dMin;
            double dMaxV=Domain.m_VDomain.m_dMax;

            // Find the seam points.

            curve *pSeam=pSurface->UParamLine(rResult,dMinU);
            std::vector<SGM::Point3D> aTemp3D,aSeamPoints3D;
            std::vector<double> aParams;
            FacetCurve(pSeam,Domain.m_VDomain,Options,aTemp3D,aParams);
            
            // Trim off the two singularities.

            size_t nTemp3D=aTemp3D.size();
            aSeamPoints3D.reserve(nTemp3D-2);
            size_t Index1;
            for(Index1=1;Index1<nTemp3D-1;++Index1)
                {
                aSeamPoints3D.push_back(aTemp3D[Index1]);
                }
            size_t nSeamPoints3D=aSeamPoints3D.size();
            SGM::Point3D LowPos=aSeamPoints3D.front();
            SGM::Point3D HighPos=aSeamPoints3D.back();
            dMinV=pSurface->Inverse(LowPos).m_v;
            dMaxV=pSurface->Inverse(HighPos).m_v;

            std::vector<double> aSeamVs;
            aSeamVs.reserve(nSeamPoints3D);
            for(Index1=0;Index1<nSeamPoints3D;++Index1)
                {
                aSeamVs.push_back(pSeam->Inverse(aSeamPoints3D[Index1]));
                }
            rResult.GetThing()->DeleteEntity(pSeam);
            std::vector<size_t> aPolygon;
            aPolygon.reserve(nSeamPoints3D*2-2);
            aPoints2D.reserve(nSeamPoints3D*2-2);
            aPoints3D.reserve(nSeamPoints3D*2-2);
            aEntities.assign(nSeamPoints3D*2-2,(entity *)pFace);
            size_t nCount=0;

            // South pole.

            size_t nSouthPoleStart=aPoints3D.size();
            size_t nPolePoints=7;
            for(Index1=0;Index1<nPolePoints;++Index1)
                {
                double dFraction=Index1/(nPolePoints-1.0);
                double dU=Domain.m_UDomain.MidPoint(dFraction);
                SGM::Point3D Pos;
                SGM::Point2D uv(dU,dMinV);
                pSurface->Evaluate(uv,&Pos);
                aPoints3D.push_back(Pos);
                aPoints2D.push_back(uv);
                aPolygon.push_back(nCount);
                ++nCount;
                }
            
            // High U seam

            for(Index1=1;Index1<nSeamPoints3D-1;++Index1)
                {
                aPoints3D.push_back(aSeamPoints3D[Index1]);
                aPoints2D.emplace_back(dMaxU,aSeamVs[Index1]);
                aPolygon.push_back(nCount);
                ++nCount;
                }

            // North pole.

            size_t nNorthPoleStart=aPoints3D.size();
            for(Index1=0;Index1<nPolePoints;++Index1)
                {
                double dFraction=(nPolePoints-1-Index1)/(nPolePoints-1.0);
                double dU=Domain.m_UDomain.MidPoint(dFraction);
                SGM::Point3D Pos;
                SGM::Point2D uv(dU,dMaxV);
                pSurface->Evaluate(uv,&Pos);
                aPoints3D.push_back(Pos);
                aPoints2D.push_back(uv);
                aPolygon.push_back(nCount);
                ++nCount;
                }

            // Low U seam

            for(Index1=nSeamPoints3D-2;0<Index1;--Index1)
                {
                aPoints3D.push_back(aSeamPoints3D[Index1]);
                aPoints2D.emplace_back(dMinU,aSeamVs[Index1]);
                aPolygon.push_back(nCount);
                ++nCount;
                }

            aaPolygons.push_back(aPolygon);
            
            // Add in the polar triangles.

            SGM::Point3D NorthPole=aTemp3D.back();
            SGM::Point3D SouthPole=aTemp3D.front();
            SGM::Point2D NorthUV(Domain.m_UDomain.MidPoint(),Domain.m_VDomain.m_dMax);
            SGM::Point2D SouthUV(Domain.m_UDomain.MidPoint(),Domain.m_VDomain.m_dMin);
            size_t nNorth=aPoints3D.size();
            aPoints3D.push_back(NorthPole);
            aPoints2D.push_back(NorthUV);
            size_t nSouth=aPoints3D.size();
            aPoints3D.push_back(SouthPole);
            aPoints2D.push_back(SouthUV);
            for(Index1=nNorthPoleStart+1;Index1<nNorthPoleStart+nPolePoints;++Index1)
                {
                aTriangles.push_back(nNorth);
                aTriangles.push_back(Index1);
                aTriangles.push_back(Index1-1);
                }
            for(Index1=nSouthPoleStart+1;Index1<nSouthPoleStart+nPolePoints;++Index1)
                {
                aTriangles.push_back(nSouth);
                aTriangles.push_back(Index1);
                aTriangles.push_back(Index1-1);
                }
            }
        }
    }

size_t AddNode(std::vector<Node>  &aNodes,
               face         const *pFace,
               double              dU,
               double              dV)
    {
    surface const *pSurface=pFace->GetSurface();
    SGM::Point2D uv(dU,dV);
    SGM::Point3D Pos;
    pSurface->Evaluate(uv,&Pos);
    Node NewNode;
    NewNode.m_Entity=(entity *)pFace;
    NewNode.m_Pos=Pos;
    NewNode.m_uv=uv;
    size_t nAnswer=aNodes.size();
    aNodes.push_back(NewNode);
    return nAnswer;
    }

void Refine(face        const *pFace,
            double             dCosRefine,
            std::vector<Node> &aNodes,
            size_t             nNodeA,
            size_t             nNodeB)
    {
    surface const *pSurface=pFace->GetSurface();
    SGM::Point2D uvA=aNodes[nNodeA].m_uv;
    SGM::Point2D uvB=aNodes[nNodeB].m_uv;
    SGM::UnitVector3D NormA,NormB;
    pSurface->Evaluate(uvA,nullptr,nullptr,nullptr,&NormA);
    pSurface->Evaluate(uvB,nullptr,nullptr,nullptr,&NormB);
    SGM::Point2D uv=SGM::MidPoint(uvA,uvB);
    SGM::Point3D Pos;
    SGM::UnitVector3D Norm;
    pSurface->Evaluate(uv,&Pos,nullptr,nullptr,&Norm);
    if(NormA%NormB<dCosRefine || Norm%NormA<dCosRefine || Norm%NormB<dCosRefine)
        {
        Node MidNode;
        MidNode.m_Entity=(entity *)pFace;
        MidNode.m_Pos=Pos;
        MidNode.m_uv=uv;
        size_t nNodeC=aNodes.size();
        aNodes[nNodeA].m_nNext=nNodeC;
        aNodes[nNodeB].m_nPrevious=nNodeC;
        MidNode.m_nNext=nNodeB;
        MidNode.m_nPrevious=nNodeA;
        aNodes.push_back(MidNode);
        Refine(pFace,dCosRefine,aNodes,nNodeA,nNodeC);
        Refine(pFace,dCosRefine,aNodes,nNodeC,nNodeB);
        }
    else if(pSurface->IsSingularity(uv,SGM_MIN_TOL))
        {
        SGM::Interval2D const &Domain=pSurface->GetDomain();
        size_t nCuts=0;
        if(SGM::NearEqual(uv.m_u,Domain.m_UDomain.m_dMin,SGM_MIN_TOL,false))
            {
            nCuts=(size_t)(3.999999*fabs(uvA.m_v-uvB.m_v)/Domain.m_VDomain.Length());
            }
        else if(SGM::NearEqual(uv.m_u,Domain.m_UDomain.m_dMax,SGM_MIN_TOL,false))
            {
            nCuts=(size_t)(3.999999*fabs(uvA.m_v-uvB.m_v)/Domain.m_VDomain.Length());
            }
        else if(SGM::NearEqual(uv.m_v,Domain.m_VDomain.m_dMin,SGM_MIN_TOL,false))
            {
            nCuts=(size_t)(3.999999*fabs(uvA.m_u-uvB.m_u)/Domain.m_UDomain.Length());
            }
        else if(SGM::NearEqual(uv.m_v,Domain.m_VDomain.m_dMax,SGM_MIN_TOL,false))
            {
            nCuts=(size_t)(3.999999*fabs(uvA.m_u-uvB.m_u)/Domain.m_UDomain.Length());
            }

        if(nCuts)
            {
            size_t Index1;
            for(Index1=0;Index1<nCuts;++Index1)
                {
                double dFraction=(Index1+1.0)/(nCuts+1.0);
                uv=SGM::MidPoint(uvA,uvB,dFraction);
                pSurface->Evaluate(uv,&Pos,nullptr,nullptr,&Norm);
                Node MidNode;
                MidNode.m_Entity=(entity *)pFace;
                MidNode.m_Pos=Pos;
                MidNode.m_uv=uv;
                size_t nNodeC=aNodes.size();
                aNodes[nNodeA].m_nNext=nNodeC;
                aNodes[nNodeB].m_nPrevious=nNodeC;
                MidNode.m_nNext=nNodeB;
                MidNode.m_nPrevious=nNodeA;
                aNodes.push_back(MidNode);
                nNodeA=nNodeC;
                }
            }
        }
    }

void FindSeamCrossings(SGM::Result        &,//rResult,
                       face         const *pFace,
                       FacetOptions const &Options,
                       std::vector<Node>  &aNodes)
    {
   surface const *pSurface=pFace->GetSurface();
    std::vector<size_t> aCrossesU,aCrossesV;
    size_t Index1;
    if(pSurface->ClosedInU())
        {
        SGM::Interval1D const &Domain=pSurface->GetDomain().m_UDomain;
        double dGap=Domain.Length()*0.5;
        size_t nNodes=aNodes.size();
        for(Index1=0;Index1<nNodes;++Index1)
            {
            Node &NodeA=aNodes[Index1];
            Node &NodeB=aNodes[NodeA.m_nNext];
            if(dGap<fabs(NodeA.m_uv.m_u-NodeB.m_uv.m_u))
                {
                aCrossesU.push_back(Index1);
                }
            }
        }
    if(pSurface->ClosedInV())
        {
        SGM::Interval1D const &Domain=pSurface->GetDomain().m_VDomain;
        double dGap=Domain.Length()*0.5;
        size_t nNodes=aNodes.size();
        for(Index1=0;Index1<nNodes;++Index1)
            {
            Node &NodeA=aNodes[Index1];
            Node &NodeB=aNodes[NodeA.m_nNext];
            if(dGap<fabs(NodeA.m_uv.m_v-NodeB.m_uv.m_v))
                {
                aCrossesV.push_back(Index1);
                }
            }
        }

    // Add duplicate nodes on the other side of the seam for all
    // the crossing nodes and sever the links between them.
    // If a node points to itself, then it is an end node.

    SGM::Interval2D const &FullDomain=pSurface->GetDomain();
    std::vector<std::pair<double,size_t> > aUInLow,aUOutLow,aUInHigh,aUOutHigh;
    size_t nCrossesU=aCrossesU.size();
    if(nCrossesU)
        {
        SGM::Interval1D const &Domain=FullDomain.m_UDomain;
        for(Index1=0;Index1<nCrossesU;++Index1)
            {
            size_t nA=aCrossesU[Index1];
            Node NodeA=aNodes[nA];
            size_t nB=NodeA.m_nNext;
            Node NodeB=aNodes[nB];
            if(SGM::NearEqual(NodeA.m_uv.m_u,Domain.m_dMin,SGM_MIN_TOL,false))
                {
                if(NodeA.m_nNext==NodeA.m_nPrevious)
                    {
                    if(NodeA.m_uv.m_v<FullDomain.m_VDomain.MidPoint())
                        {
                        aUInLow.emplace_back(NodeA.m_uv.m_v,nA);
                        }
                    else
                        {
                        aUOutLow.emplace_back(NodeA.m_uv.m_v,nA);
                        }
                    }
                else if(SGM::NearEqual(NodeB.m_uv.m_u,Domain.m_dMax,SGM_MIN_TOL,false))
                    {
                    aUOutLow.emplace_back(NodeA.m_uv.m_v,nA);
                    aUInHigh.emplace_back(NodeA.m_uv.m_v,nB);
                    }
                else
                    {
                    Node NodeC=NodeA;
                    size_t nC=aNodes.size();
                    NodeC.m_uv.m_u=Domain.m_dMax;
                    NodeC.m_nNext=nB;
                    NodeC.m_nPrevious=nA;
                    aNodes.push_back(NodeC);
                    aNodes[nB].m_nPrevious=nC;
                    aNodes[nA].m_nNext=nC;
                    aUOutLow.emplace_back(NodeA.m_uv.m_v,nA);
                    aUInHigh.emplace_back(NodeA.m_uv.m_v,nC);
                    }
                }
            else if(SGM::NearEqual(NodeA.m_uv.m_u,Domain.m_dMax,SGM_MIN_TOL,false))
                {
                if(NodeA.m_nNext==NodeA.m_nPrevious)
                    {
                    if(NodeA.m_uv.m_v<FullDomain.m_VDomain.MidPoint())
                        {
                        aUOutHigh.emplace_back(NodeA.m_uv.m_v,nA);
                        }
                    else
                        {
                        aUInHigh.emplace_back(NodeA.m_uv.m_v,nA);
                        }
                    }
                else if(SGM::NearEqual(NodeB.m_uv.m_u,Domain.m_dMin,SGM_MIN_TOL,false))
                    {
                    aUOutHigh.emplace_back(NodeA.m_uv.m_v,nA);
                    aUInLow.emplace_back(NodeA.m_uv.m_v,nB);
                    }
                else
                    {
                    Node NodeC=NodeA;
                    size_t nC=aNodes.size();
                    NodeC.m_uv.m_u=Domain.m_dMin;
                    NodeC.m_nNext=nB;
                    NodeC.m_nPrevious=nA;
                    aNodes.push_back(NodeC);
                    aNodes[nB].m_nPrevious=nC;
                    aNodes[nA].m_nNext=nC;
                    aUOutHigh.emplace_back(NodeA.m_uv.m_v,nA);
                    aUInLow.emplace_back(NodeA.m_uv.m_v,nC);
                    }
                }
            else if(SGM::NearEqual(NodeB.m_uv.m_u,Domain.m_dMin,SGM_MIN_TOL,false))
                {
                if(NodeB.m_nNext==NodeB.m_nPrevious)
                    {
                    if(NodeB.m_uv.m_v<FullDomain.m_VDomain.MidPoint())
                        {
                        aUOutLow.emplace_back(NodeB.m_uv.m_v,nB);
                        }
                    else
                        {
                        aUInLow.emplace_back(NodeB.m_uv.m_v,nB);
                        }
                    }
                else if(SGM::NearEqual(NodeA.m_uv.m_u,Domain.m_dMax,SGM_MIN_TOL,false))
                    {
                    aUInLow.emplace_back(NodeA.m_uv.m_v,nB);
                    aUOutHigh.emplace_back(NodeA.m_uv.m_v,nA);
                    }
                else
                    {
                    Node NodeC=NodeB;
                    size_t nC=aNodes.size();
                    NodeC.m_uv.m_u=Domain.m_dMax;
                    NodeC.m_nNext=nB;
                    NodeC.m_nPrevious=nA;
                    aNodes.push_back(NodeC);
                    aNodes[nB].m_nPrevious=nC;
                    aNodes[nA].m_nNext=nC;
                    aUInLow.emplace_back(NodeB.m_uv.m_v,nB);
                    aUOutHigh.emplace_back(NodeB.m_uv.m_v,nC);
                    }
                }
            else if(SGM::NearEqual(NodeB.m_uv.m_u,Domain.m_dMax,SGM_MIN_TOL,false))
                {
                if(NodeB.m_nNext==NodeB.m_nPrevious)
                    {
                    if(NodeB.m_uv.m_v<FullDomain.m_VDomain.MidPoint())
                        {
                        aUOutHigh.emplace_back(NodeB.m_uv.m_v,nB);
                        }
                    else
                        {
                        aUInHigh.emplace_back(NodeB.m_uv.m_v,nB);
                        }
                    }
                else if(SGM::NearEqual(NodeA.m_uv.m_u,Domain.m_dMin,SGM_MIN_TOL,false))
                    {
                    aUInHigh.emplace_back(NodeB.m_uv.m_v,nB);
                    aUOutLow.emplace_back(NodeB.m_uv.m_v,nA);
                    }
                else
                    {
                    Node NodeC=NodeB;
                    size_t nC=aNodes.size();
                    NodeC.m_uv.m_u=Domain.m_dMin;
                    NodeC.m_nNext=nB;
                    NodeC.m_nPrevious=nA;
                    aNodes.push_back(NodeC);
                    aNodes[nB].m_nPrevious=nC;
                    aNodes[nA].m_nNext=nC;
                    aUInHigh.emplace_back(NodeB.m_uv.m_v,nB);
                    aUOutLow.emplace_back(NodeB.m_uv.m_v,nC);
                    }
                }
            }
        }

    size_t nCrossesV=aCrossesV.size();
    std::vector<std::pair<double,size_t> > aVInLow,aVOutLow,aVInHigh,aVOutHigh;
    if(nCrossesV)
        {
        SGM::Interval1D const &Domain=pSurface->GetDomain().m_VDomain;
        for(Index1=0;Index1<nCrossesV;++Index1)
            {
            size_t nA=aCrossesV[Index1];
            Node &NodeA=aNodes[nA];
            size_t nB=NodeA.m_nNext;
            Node &NodeB=aNodes[nB];
            if(SGM::NearEqual(NodeA.m_uv.m_v,Domain.m_dMin,SGM_MIN_TOL,false))
                {
                if(NodeA.m_nNext==NodeA.m_nPrevious)
                    {
                    if(NodeA.m_uv.m_u<FullDomain.m_UDomain.MidPoint())
                        {
                        aVInLow.emplace_back(NodeA.m_uv.m_u,nA);
                        }
                    else
                        {
                        aVOutLow.emplace_back(NodeA.m_uv.m_u,nA);
                        }
                    }
                else if(SGM::NearEqual(NodeB.m_uv.m_v,Domain.m_dMax,SGM_MIN_TOL,false))
                    {
                    aVOutLow.emplace_back(NodeA.m_uv.m_u,nA);
                    aVInHigh.emplace_back(NodeA.m_uv.m_u,nB);
                    }
                else
                    {
                    Node NodeC=NodeA;
                    size_t nC=aNodes.size();
                    NodeC.m_uv.m_v=Domain.m_dMax;
                    NodeC.m_nNext=nB;
                    NodeC.m_nPrevious=nA;
                    aNodes.push_back(NodeC);
                    aNodes[nB].m_nPrevious=nC;
                    aNodes[nA].m_nNext=nC;
                    aVOutLow.emplace_back(NodeA.m_uv.m_u,nA);
                    aVInHigh.emplace_back(NodeA.m_uv.m_u,nC);
                    }
                }
            else if(SGM::NearEqual(NodeA.m_uv.m_v,Domain.m_dMax,SGM_MIN_TOL,false))
                {
                if(NodeA.m_nNext==NodeA.m_nPrevious)
                    {
                    if(NodeA.m_uv.m_u<FullDomain.m_VDomain.MidPoint())
                        {
                        aVOutHigh.emplace_back(NodeA.m_uv.m_u,nA);
                        }
                    else
                        {
                        aVInHigh.emplace_back(NodeA.m_uv.m_u,nA);
                        }
                    }
                else if(SGM::NearEqual(NodeB.m_uv.m_v,Domain.m_dMin,SGM_MIN_TOL,false))
                    {
                    aVOutHigh.emplace_back(NodeA.m_uv.m_u,nA);
                    aVInLow.emplace_back(NodeA.m_uv.m_u,nB);
                    }
                else
                    {
                    Node NodeC=NodeA;
                    size_t nC=aNodes.size();
                    NodeC.m_uv.m_v=Domain.m_dMin;
                    NodeC.m_nNext=nB;
                    NodeC.m_nPrevious=nA;
                    aNodes.push_back(NodeC);
                    aNodes[nB].m_nPrevious=nC;
                    aNodes[nA].m_nNext=nC;
                    aVOutHigh.emplace_back(NodeA.m_uv.m_u,nA);
                    aVInLow.emplace_back(NodeA.m_uv.m_u,nC);
                    }
                }
            else if(SGM::NearEqual(NodeB.m_uv.m_v,Domain.m_dMin,SGM_MIN_TOL,false))
                {
                if(NodeB.m_nNext==NodeB.m_nPrevious)
                    {
                    if(NodeB.m_uv.m_u<FullDomain.m_VDomain.MidPoint())
                        {
                        aVOutLow.emplace_back(NodeB.m_uv.m_u,nB);
                        }
                    else
                        {
                        aVInLow.emplace_back(NodeB.m_uv.m_u,nB);
                        }
                    }
                else if(SGM::NearEqual(NodeA.m_uv.m_v,Domain.m_dMax,SGM_MIN_TOL,false))
                    {
                    aVInLow.emplace_back(NodeA.m_uv.m_u,nB);
                    aVOutHigh.emplace_back(NodeA.m_uv.m_u,nA);
                    }
                else
                    {
                    Node NodeC=NodeB;
                    size_t nC=aNodes.size();
                    NodeC.m_uv.m_v=Domain.m_dMax;
                    NodeC.m_nNext=nB;
                    NodeC.m_nPrevious=nA;
                    aNodes.push_back(NodeC);
                    aNodes[nB].m_nPrevious=nC;
                    aNodes[nA].m_nNext=nC;
                    aVInLow.emplace_back(NodeB.m_uv.m_u,nB);
                    aVOutHigh.emplace_back(NodeB.m_uv.m_u,nC);
                    }
                }
            else if(SGM::NearEqual(NodeB.m_uv.m_v,Domain.m_dMax,SGM_MIN_TOL,false))
                {
                if(NodeB.m_nNext==NodeB.m_nPrevious)
                    {
                    if(NodeB.m_uv.m_u<FullDomain.m_VDomain.MidPoint())
                        {
                        aVOutHigh.emplace_back(NodeB.m_uv.m_u,nB);
                        }
                    else
                        {
                        aVInHigh.emplace_back(NodeB.m_uv.m_u,nB);
                        }
                    }
                else if(SGM::NearEqual(NodeA.m_uv.m_v,Domain.m_dMin,SGM_MIN_TOL,false))
                    {
                    aVInHigh.emplace_back(NodeB.m_uv.m_u,nB);
                    aVOutLow.emplace_back(NodeB.m_uv.m_u,nA);
                    }
                else
                    {
                    Node NodeC=NodeB;
                    size_t nC=aNodes.size();
                    NodeC.m_uv.m_v=Domain.m_dMin;
                    NodeC.m_nNext=nB;
                    NodeC.m_nPrevious=nA;
                    aNodes.push_back(NodeC);
                    aNodes[nB].m_nPrevious=nC;
                    aNodes[nA].m_nNext=nC;
                    aVInHigh.emplace_back(NodeB.m_uv.m_u,nB);
                    aVOutLow.emplace_back(NodeB.m_uv.m_u,nC);
                    }
                }
            }
        }

    // Link up end nodes.

    if(nCrossesU || nCrossesV)
        {
        // Hook up the outs to the ins.

        std::sort(aUOutLow.begin(),aUOutLow.end());
        std::sort(aVOutLow.begin(),aVOutLow.end());
        std::sort(aUOutHigh.begin(),aUOutHigh.end());
        std::sort(aVOutHigh.begin(),aVOutHigh.end());
        std::reverse(aVOutHigh.begin(),aVOutHigh.end());
        std::reverse(aUOutLow.begin(),aUOutLow.end());

        std::sort(aUInLow.begin(),aUInLow.end());
        std::sort(aVInLow.begin(),aVInLow.end());
        std::sort(aUInHigh.begin(),aUInHigh.end());
        std::sort(aVInHigh.begin(),aVInHigh.end());
        std::reverse(aVInHigh.begin(),aVInHigh.end());
        std::reverse(aUInLow.begin(),aUInLow.end());

        size_t nUOutLow=aUOutLow.size();
        size_t nVOutLow=aVOutLow.size();
        size_t nUOutHigh=aUOutHigh.size();
        size_t nVOutHigh=aVOutHigh.size();

        SGM::Interval1D const &UDomain=pSurface->GetDomain().m_UDomain;
        SGM::Interval1D const &VDomain=pSurface->GetDomain().m_VDomain;
        double dCosRefineAngle=cos(Options.m_dEdgeAngleTol);

        for(Index1=0;Index1<nUOutLow;++Index1)
            {
            double v1=aUOutLow[Index1].first;
            double v2=aUInLow[Index1].first;
            size_t nNodeA=aUOutLow[Index1].second;
            size_t nNodeB=aUInLow[Index1].second;
            if(v2<v1)
                {
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeB);
                    }
                }
            else if(Index1<nUOutLow-1)
                {
                nNodeB=aUInLow[Index1+1].second;
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeB);
                    }
                }
            else // Must go around the corner.
                {
                // Through (min u,min v)
                nNodeB=aVInLow[0].second;
                size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMin,VDomain.m_dMin);
                aNodes[nNodeA].m_nNext=nNodeC;
                aNodes[nNodeB].m_nPrevious=nNodeC;
                aNodes[nNodeC].m_nNext=nNodeB;
                aNodes[nNodeC].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeC);
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeC,nNodeB);
                    }
                }
            }
        for(Index1=0;Index1<nVOutLow;++Index1)
            {
            double u1=aVOutLow[Index1].first;
            double u2=aVInLow[Index1].first;
            size_t nNodeA=aVOutLow[Index1].second;
            size_t nNodeB=aVInLow[Index1].second;
            if(u1<u2)
                {
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeB);
                    }
                }
            else if(Index1<nVOutLow-1)
                {
                nNodeB=aVInLow[Index1+1].second;
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeB);
                    }
                }
            else // Must go around the corner.
                {
                // Through (max u,min v)
                nNodeB=aUInHigh[0].second;
                size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMax,VDomain.m_dMin);
                aNodes[nNodeA].m_nNext=nNodeC;
                aNodes[nNodeB].m_nPrevious=nNodeC;
                aNodes[nNodeC].m_nNext=nNodeB;
                aNodes[nNodeC].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeC);
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeC,nNodeB);
                    }
                }
            }
        for(Index1=0;Index1<nUOutHigh;++Index1)
            {
            double v1=aUOutHigh[Index1].first;
            double v2=aUInHigh[Index1].first;
            size_t nNodeA=aUOutHigh[Index1].second;
            size_t nNodeB=aUInHigh[Index1].second;
            if(v1<v2)
                {
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeB);
                    }
                }
            else if(Index1<nUOutHigh-1)
                {
                nNodeB=aUInHigh[Index1+1].second;
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeB);
                    }
                }
            else // Must go around the corner.
                {
                // Through (max u,max v)
                nNodeB=aVInHigh[0].second;
                size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMax,VDomain.m_dMax);
                aNodes[nNodeA].m_nNext=nNodeC;
                aNodes[nNodeB].m_nPrevious=nNodeC;
                aNodes[nNodeC].m_nNext=nNodeB;
                aNodes[nNodeC].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeC);
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeC,nNodeB);
                    }
                }
            }
        for(Index1=0;Index1<nVOutHigh;++Index1)
            {
            double u1=aVOutHigh[Index1].first;
            double u2=aVInHigh[Index1].first;
            size_t nNodeA=aVOutHigh[Index1].second;
            size_t nNodeB=aVInHigh[Index1].second;
            if(u2<u1)
                {
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeB);
                    }
                }
            else if(Index1<nVOutHigh-1)
                {
                nNodeB=aVInHigh[Index1+1].second;
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeB);
                    }
                }
            else // Must go around the corner.
                {
                // Through (min u,max v)
                nNodeB=aUInLow[0].second;
                size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMin,VDomain.m_dMax);
                aNodes[nNodeA].m_nNext=nNodeC;
                aNodes[nNodeB].m_nPrevious=nNodeC;
                aNodes[nNodeC].m_nNext=nNodeB;
                aNodes[nNodeC].m_nPrevious=nNodeA;
                if(Options.m_bParametric==false)
                    {
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeA,nNodeC);
                    Refine(pFace,dCosRefineAngle,aNodes,nNodeC,nNodeB);
                    }
                }
            }
        }
    }

void FindPolygon(std::vector<Node>         &aNodes,
                 unsigned int               nStart,
                 std::vector<unsigned int> &aPolygon)
    {
    aPolygon.push_back(nStart);
    size_t nWhere=aNodes[nStart].m_nNext;
    aNodes[nStart].m_bMark=true;
    while(nWhere!=nStart)
        {
        aPolygon.push_back((unsigned int)nWhere);
        aNodes[nWhere].m_bMark=true;
        nWhere=aNodes[nWhere].m_nNext;
        }
    }

void FindPolygons(std::vector<Node>                       &aNodes,
                  std::vector<SGM::Point2D>               &aPoints2D,
                  std::vector<SGM::Point3D>               &aPoints3D,
                  std::vector<entity *>                   &aEntities,
                  std::vector<std::vector<unsigned int> > &aaPolygons)
    {
    unsigned int nNodes=(unsigned int)aNodes.size();
    unsigned int Index1;
    aPoints2D.reserve(nNodes);
    aPoints3D.reserve(nNodes);
    aEntities.reserve(nNodes);
    for(Index1=0;Index1<nNodes;++Index1)
        {
        aPoints2D.push_back(aNodes[Index1].m_uv);
        aPoints3D.push_back(aNodes[Index1].m_Pos);
        aEntities.push_back(aNodes[Index1].m_Entity);
        }
    for(Index1=0;Index1<nNodes;++Index1)
        {
        if(aNodes[Index1].m_bMark==false)
            {
            std::vector<unsigned int> aPolygon;
            FindPolygon(aNodes,Index1,aPolygon);
            aaPolygons.push_back(aPolygon);
            if(Index1==1)
                {
                break;
                }
            }
        }
    }

void FindOuterLoop(SGM::Result        &,//rResult,
                   face         const *pFace,
                   FacetOptions const &Options,
                   std::vector<Node>  &aNodes)
    {
    surface const *pSurface=pFace->GetSurface();
    size_t Index1;
    SGM::Interval2D const &Domain=pSurface->GetDomain();
    std::vector<SGM::Point2D> aCorners;
    aCorners.push_back(Domain.LowerLeft());
    aCorners.push_back(Domain.LowerRight());
    aCorners.push_back(Domain.UpperRight());
    aCorners.push_back(Domain.UpperLeft());
    if(pFace->GetFlipped())
        {
        std::reverse(aCorners.begin(),aCorners.end());
        }
    double dCosRefine=cos(Options.m_dEdgeAngleTol);
    for(Index1=0;Index1<4;++Index1)
        {
        SGM::Point3D Pos;
        pSurface->Evaluate(aCorners[Index1],&Pos);
        Node CornerNode;
        CornerNode.m_Entity=(entity *)pFace;
        CornerNode.m_uv=aCorners[Index1];
        CornerNode.m_Pos=Pos;
        CornerNode.m_nNext= Index1==3 ? 0 : Index1+1;
        CornerNode.m_nPrevious = Index1==0 ? 3 : Index1-1;
        aNodes.push_back(CornerNode);
        }

    if(Options.m_bParametric==false)
        {
        Refine(pFace,dCosRefine,aNodes,0,1);
        Refine(pFace,dCosRefine,aNodes,1,2);
        Refine(pFace,dCosRefine,aNodes,2,3);
        Refine(pFace,dCosRefine,aNodes,3,0);
        }
    }

bool PointOnNodes(SGM::Point3D      const &Pos,
                  std::vector<Node> const &aNodes,
                  size_t                  &nNode)
    {
    nNode=0;
    size_t nNodes=aNodes.size();
    size_t Index1;
    for(Index1=0;Index1<nNodes;++Index1)
        {
        if(aNodes[Index1].m_Pos.DistanceSquared(Pos)<SGM_ZERO)
            {
            nNode=Index1;
            return true;
            }
        }
    return false;
    }

void FindSingularitiesValues(SGM::Result               &rResult,
                             SGM::Point3D        const &Pos,
                             face                const *pFace,
                             bool                       bUValues,
                             std::map<entity *,double> &mValues)
    {
    std::vector<std::vector<edge *> > aaLoops;
    std::vector<std::vector<SGM::EdgeSideType> > aaEdgeSideTypes;
    size_t nLoops=pFace->FindLoops(rResult,aaLoops,aaEdgeSideTypes);
    size_t Index1,Index2;
    for(Index1=0;Index1<nLoops;++Index1)
        {
        std::vector<edge *> const &aLoop=aaLoops[Index1];
        std::vector<SGM::EdgeSideType> const &aSides=aaEdgeSideTypes[Index1];
        size_t nLoop=aLoop.size();
        for(Index2=0;Index2<nLoop;++Index2)
            {
            edge *pEdge=aLoop[Index2];
            SGM::EdgeSideType nSide=aSides[Index2];
            SGM::Interval1D const &Domain=pEdge->GetDomain();
            double dParam=std::numeric_limits<double>::max();
            if(SGM::NearEqual(pEdge->GetStart()->GetPoint(),Pos,SGM_MIN_TOL))
                {
                dParam=Domain.MidPoint(SGM_FIT);
                }
            else if(SGM::NearEqual(pEdge->GetEnd()->GetPoint(),Pos,SGM_MIN_TOL))
                {
                dParam=Domain.MidPoint(1.0-SGM_FIT);
                }
            if(dParam<std::numeric_limits<double>::max())
                {
                SGM::Point3D CPos;
                pEdge->GetCurve()->Evaluate(dParam,&CPos);
                SGM::Point2D uv=pFace->EvaluateParamSpace(pEdge,nSide,CPos);
                if(bUValues)
                    {
                    mValues[pEdge]=uv.m_v;
                    }
                else
                    {
                    mValues[pEdge]=uv.m_u;
                    }
                }
            }
        }
    }

void AddNodesAtSingularites(SGM::Result        &rResult,
                            face         const *pFace,
                            FacetOptions const &Options,
                            std::vector<Node>  &aNodes)
    {
    surface const *pSurface=pFace->GetSurface();
    SGM::Interval2D const &Domain=pSurface->GetDomain();
    double dCosRefine=cos(Options.m_dEdgeAngleTol);
    size_t nNode;
    if(pSurface->SingularHighU())
        {
        SGM::Point2D uvA(Domain.m_UDomain.m_dMax,Domain.m_VDomain.m_dMin);
        SGM::Point2D uvB(Domain.m_UDomain.m_dMax,Domain.m_VDomain.m_dMax);
        SGM::Point3D Pos;
        pSurface->Evaluate(uvA,&Pos);
        bool bPointOnNodes=PointOnNodes(Pos,aNodes,nNode);
        if( bPointOnNodes==false &&
            pFace->PointInFace(rResult,uvA)==true)
            {
            Node NodeA,NodeB;
            size_t nNodeA=aNodes.size();
            size_t nNodeB=nNodeA+1;
            NodeA.m_Entity=(entity *)pFace;
            NodeA.m_nNext=nNodeB;
            NodeA.m_nPrevious=nNodeB;
            NodeA.m_uv=uvA;
            NodeA.m_Pos=Pos;
            NodeB.m_Entity=(entity *)pFace;
            NodeB.m_nNext=nNodeA;
            NodeB.m_nPrevious=nNodeA;
            NodeB.m_uv=uvB;
            NodeB.m_Pos=Pos;
            aNodes.push_back(NodeA);
            aNodes.push_back(NodeB);
            Refine(pFace,dCosRefine,aNodes,nNodeA,nNodeB);
            }
        else if(bPointOnNodes)
            {
            int a=0;
            a*=1;
            }
        }
    if(pSurface->SingularHighV())
        {
        SGM::Point2D uvA(Domain.m_UDomain.m_dMax,Domain.m_VDomain.m_dMax);
        SGM::Point2D uvB(Domain.m_UDomain.m_dMin,Domain.m_VDomain.m_dMax);
        SGM::Point3D Pos;
        pSurface->Evaluate(uvA,&Pos);
        bool bPointOnNodes=PointOnNodes(Pos,aNodes,nNode);
        if( bPointOnNodes==false &&
            pFace->PointInFace(rResult,uvA)==true)
            {
            Node NodeA,NodeB;
            size_t nNodeA=aNodes.size();
            size_t nNodeB=nNodeA+1;
            NodeA.m_Entity=(entity *)pFace;
            NodeA.m_nNext=nNodeB;
            NodeA.m_nPrevious=nNodeB;
            NodeA.m_uv=uvA;
            NodeA.m_Pos=Pos;
            NodeB.m_Entity=(entity *)pFace;
            NodeB.m_nNext=nNodeA;
            NodeB.m_nPrevious=nNodeA;
            NodeB.m_uv=uvB;
            NodeB.m_Pos=Pos;
            aNodes.push_back(NodeA);
            aNodes.push_back(NodeB);
            Refine(pFace,dCosRefine,aNodes,nNodeA,nNodeB);
            }
        else if(bPointOnNodes)
            {
            std::map<entity *,double> mValues;
            FindSingularitiesValues(rResult,Pos,pFace,false,mValues);

            size_t nNodeB=aNodes.size();
            Node NodeB;
            NodeB.m_Entity=aNodes[aNodes[nNode].m_nNext].m_Entity;
            NodeB.m_nNext=aNodes[nNode].m_nNext;
            NodeB.m_nPrevious=nNode;
            NodeB.m_uv.m_v=aNodes[nNode].m_uv.m_v;
            NodeB.m_uv.m_u=mValues[NodeB.m_Entity];
            NodeB.m_Pos=Pos;
            aNodes.push_back(NodeB);

            aNodes[nNode].m_nNext=nNodeB;
            aNodes[nNode].m_uv.m_u=mValues[aNodes[nNode].m_Entity];

            Refine(pFace,dCosRefine,aNodes,nNode,nNodeB);
            }
        }
    if(pSurface->SingularLowU())
        {
        SGM::Point2D uvA(Domain.m_UDomain.m_dMin,Domain.m_VDomain.m_dMax);
        SGM::Point2D uvB(Domain.m_UDomain.m_dMin,Domain.m_VDomain.m_dMin);
        SGM::Point3D Pos;
        pSurface->Evaluate(uvA,&Pos);
        bool bPointOnNodes=PointOnNodes(Pos,aNodes,nNode);
        if( bPointOnNodes==false &&
            pFace->PointInFace(rResult,uvA)==true)
            {
            Node NodeA,NodeB;
            size_t nNodeA=aNodes.size();
            size_t nNodeB=nNodeA+1;
            NodeA.m_Entity=(entity *)pFace;
            NodeA.m_nNext=nNodeB;
            NodeA.m_nPrevious=nNodeB;
            NodeA.m_uv=uvA;
            NodeA.m_Pos=Pos;
            NodeB.m_Entity=(entity *)pFace;
            NodeB.m_nNext=nNodeA;
            NodeB.m_nPrevious=nNodeA;
            NodeB.m_uv=uvB;
            NodeB.m_Pos=Pos;
            aNodes.push_back(NodeA);
            aNodes.push_back(NodeB);
            Refine(pFace,dCosRefine,aNodes,nNodeA,nNodeB);
            }
        else if(bPointOnNodes)
            {
            int a=0;
            a*=1;
            }
        }
    if(pSurface->SingularLowV())
        {
        SGM::Point2D uvA(Domain.m_UDomain.m_dMin,Domain.m_VDomain.m_dMin);
        SGM::Point2D uvB(Domain.m_UDomain.m_dMax,Domain.m_VDomain.m_dMin);
        SGM::Point3D Pos;
        pSurface->Evaluate(uvA,&Pos);
        bool bPointOnNodes=PointOnNodes(Pos,aNodes,nNode);
        if( bPointOnNodes==false &&
            pFace->PointInFace(rResult,uvA)==true)
            {
            Node NodeA,NodeB;
            size_t nNodeA=aNodes.size();
            size_t nNodeB=nNodeA+1;
            NodeA.m_Entity=(entity *)pFace;
            NodeA.m_nNext=nNodeB;
            NodeA.m_nPrevious=nNodeB;
            NodeA.m_uv=uvA;
            NodeA.m_Pos=Pos;
            NodeB.m_Entity=(entity *)pFace;
            NodeB.m_nNext=nNodeA;
            NodeB.m_nPrevious=nNodeA;
            NodeB.m_uv=uvB;
            NodeB.m_Pos=Pos;
            aNodes.push_back(NodeA);
            aNodes.push_back(NodeB);
            Refine(pFace,dCosRefine,aNodes,nNodeA,nNodeB);
            }
        else if(bPointOnNodes)
            {
            int a=0;
            a*=1;
            }
        }
    }

size_t FacetFaceLoops(SGM::Result                             &rResult,
                      face                              const *pFace,
                      FacetOptions                      const &Options,
                      std::vector<SGM::Point2D>               &aPoints2D,
                      std::vector<SGM::Point3D>               &aPoints3D,
                      std::vector<entity *>                   &aEntities,
                      std::vector<std::vector<unsigned int> > &aaPolygons)
    {
    // Find all the needed face information.

    std::vector<std::vector<edge *> > aaLoops;
    std::vector<std::vector<SGM::EdgeSideType> > aaEdgeSideTypes;
    size_t nLoops=pFace->FindLoops(rResult,aaLoops,aaEdgeSideTypes);

    bool bFlip=pFace->GetFlipped();
    std::set<vertex *,EntityCompare> sVertices;
    FindVertices(rResult,pFace,sVertices);

    // Facet each loop.

    std::vector<Node> aNodes;
    size_t Index1,Index2,Index3;
    for(Index1=0;Index1<nLoops;++Index1)
        {
        std::vector<edge *> const &aLoop=aaLoops[Index1];
        std::vector<SGM::EdgeSideType> const &aSides=aaEdgeSideTypes[Index1];
        size_t nLoop=aLoop.size();
        size_t nLoopStartNode=aNodes.size();
        for(Index2=0;Index2<nLoop;++Index2)
            {
            size_t nStartNode=aNodes.size();
            edge *pEdge=aLoop[Index2];
            std::vector<SGM::Point3D> aFacets=pEdge->GetFacets(rResult);
            std::vector<double> aParams=pEdge->GetParams(rResult);
            SGM::EdgeSideType nSide=aSides[Index2];
            if(nSide==SGM::EdgeSideType::FaceOnRightType)
                {
                std::reverse(aFacets.begin(),aFacets.end());
                std::reverse(aParams.begin(),aParams.end());
                }
            if(bFlip)
                {
                std::reverse(aFacets.begin(),aFacets.end());
                std::reverse(aParams.begin(),aParams.end());
                }
            size_t nFacets=aFacets.size();
            for(Index3=1;Index3<nFacets;++Index3)
                {
                Node PointNode;
                PointNode.m_Entity=pEdge;
                PointNode.m_Pos=aFacets[Index3];
                PointNode.m_t=aParams[Index3];
                aNodes.push_back(PointNode);
                }
            size_t nNodes=aNodes.size();
            for(Index3=nStartNode;Index3<nNodes;++Index3)
                {
                if(Index3)
                    {
                    aNodes[Index3].m_nPrevious=Index3-1;
                    }
                aNodes[Index3].m_nNext=Index3+1;
                SGM::Point2D uv=pFace->EvaluateParamSpace(pEdge,nSide,aNodes[Index3].m_Pos);
                aNodes[Index3].m_uv=uv;
                }
            }
        
        // Make sure that last node is pointing to the start.

        aNodes.back().m_nNext=nLoopStartNode;
        aNodes[nLoopStartNode].m_nPrevious=aNodes.size()-1;
        }

    if(nLoops==0)
        {
        FindOuterLoop(rResult,pFace,Options,aNodes);
        }
    else
        {
        AddNodesAtSingularites(rResult,pFace,Options,aNodes);
        FindSeamCrossings(rResult,pFace,Options,aNodes);
        }
     
    FindPolygons(aNodes,aPoints2D,aPoints3D,aEntities,aaPolygons);

    return aaPolygons.size();
    }

class EdgeValue
    {
    public:

        EdgeValue(double dValue,unsigned int nTriangle,unsigned int nEdge):
            m_dDot(dValue),m_nTriangle(nTriangle),m_nEdge(nEdge) {}

        bool operator<(EdgeValue const &Other) const 
            {
            if(m_dDot<Other.m_dDot)
                {
                return true;
                }
            else if(m_dDot==Other.m_dDot)
                {
                if(m_nTriangle<Other.m_nTriangle)
                    {
                    return true;
                    }
                else if(m_nTriangle==Other.m_nTriangle)
                    {
                    if(m_nEdge<Other.m_nEdge)
                        {
                        return true;
                        }
                    }
                }
            return false;
            }

        double       m_dDot;
        unsigned int m_nTriangle;
        unsigned int m_nEdge;
    };

void FixBackPointer(unsigned int               nA,
                    unsigned int               nStart,
                    unsigned int               nT,
                    std::vector<unsigned int> &aTriangles,
                    std::vector<unsigned int> &aAdjacencies)
    {
    if(nA!=std::numeric_limits<unsigned int>::max())
        {
        if(nStart==aTriangles[nA])
            {
            aAdjacencies[nA]=nT;
            }
        else if(nStart==aTriangles[nA+1])
            {
            aAdjacencies[nA+1]=nT;
            }
        else
            {
            aAdjacencies[nA+2]=nT;
            }
        }
    }

double FindNormalDiff(bool                                  bFirst,
                      surface                        const *pSurface,
                      std::vector<SGM::UnitVector3D> const &aNormals,
                      std::vector<SGM::Point2D>      const &aPoints2D,
                      size_t                                nPos0,
                      size_t                                nPos1)
    {
    SGM::UnitVector3D const &Norm0=aNormals[nPos0];
    SGM::UnitVector3D const &Norm1=aNormals[nPos1];
    if(bFirst==true)
        {
        SGM::Point2D const &uv0=aPoints2D[nPos0];
        SGM::Point2D const &uv1=aPoints2D[nPos1];
        SGM::Point2D uv=SGM::MidPoint(uv0,uv1);
        SGM::UnitVector3D MidNorm;
        pSurface->Evaluate(uv,nullptr,nullptr,nullptr,&MidNorm);
        double dVal0=Norm0%Norm1;
        double dVal1=Norm0%MidNorm;
        double dVal2=Norm1%MidNorm;
        return std::min(dVal0,std::min(dVal1,dVal2));
        }
    return Norm0%Norm1;
    }

void FixEdgeData(surface                                         const *pSurface,
                 std::vector<SGM::Point2D>                       const &aPoints2D,
                 unsigned int                                           nTri1,
                 unsigned int                                           nTri2,
                 std::vector<SGM::UnitVector3D>                  const &aNormals,
                 std::vector<unsigned int>                       const &aTriangles,
                 std::vector<unsigned int>                       const &aAdjacencies,
                 std::set<EdgeValue>                                   &sEdgeData,
                 std::map<std::pair<unsigned int,unsigned int>,double> &mEdgeData)
    {
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nTri1,0)],nTri1,0));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nTri1,1)],nTri1,1));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nTri1,2)],nTri1,2));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nTri2,0)],nTri2,0));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nTri2,1)],nTri2,1));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nTri2,2)],nTri2,2));

    unsigned int A2=aTriangles[nTri1];
    unsigned int B2=aTriangles[nTri1+1];
    unsigned int C2=aTriangles[nTri1+2];
    unsigned int D2=aTriangles[nTri2];
    unsigned int E2=aTriangles[nTri2+1];
    unsigned int F2=aTriangles[nTri2+2];

    if(aTriangles[nTri1]<aTriangles[nTri1+1] && aAdjacencies[nTri1]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,A2,B2);
        sEdgeData.insert(EdgeValue(dVal,nTri1,0));
        mEdgeData[std::pair<unsigned int,unsigned int>(nTri1,0)]=dVal;
        }
    if(aTriangles[nTri1+1]<aTriangles[nTri1+2] && aAdjacencies[nTri1+1]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,B2,C2);
        sEdgeData.insert(EdgeValue(dVal,nTri1,1));
        mEdgeData[std::pair<unsigned int,unsigned int>(nTri1,1)]=dVal;
        }
    if(aTriangles[nTri1+2]<aTriangles[nTri1] && aAdjacencies[nTri1+2]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,C2,A2);
        sEdgeData.insert(EdgeValue(dVal,nTri1,2));
        mEdgeData[std::pair<unsigned int,unsigned int>(nTri1,2)]=dVal;
        }
    if(aTriangles[nTri2]<aTriangles[nTri2+1] && aAdjacencies[nTri2]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,D2,E2);
        sEdgeData.insert(EdgeValue(dVal,nTri2,0));
        mEdgeData[std::pair<unsigned int,unsigned int>(nTri2,0)]=dVal;
        }
    if(aTriangles[nTri2+1]<aTriangles[nTri2+2] && aAdjacencies[nTri2+1]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,E2,F2);
        sEdgeData.insert(EdgeValue(dVal,nTri2,1));
        mEdgeData[std::pair<unsigned int,unsigned int>(nTri2,1)]=dVal;
        }
    if(aTriangles[nTri2+2]<aTriangles[nTri2] && aAdjacencies[nTri2+2]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,F2,D2);
        sEdgeData.insert(EdgeValue(dVal,nTri2,2));
        mEdgeData[std::pair<unsigned int,unsigned int>(nTri2,2)]=dVal;
        }
    }

void IncrementalDelaunay(surface                                         const *pSurface,
                         std::set<unsigned int>                                &sSearchTris,
                         std::vector<SGM::Point2D>                       const &aPoints2D,
                         std::vector<SGM::Point2D>                       const &aScaled2D,
                         std::vector<SGM::UnitVector3D>                  const &aNormals,
                         std::vector<unsigned int>                             &aTriangles,
                         std::vector<unsigned int>                             &aAdjacencies,
                         std::set<EdgeValue>                                   &sEdgeData,
                         std::map<std::pair<unsigned int,unsigned int>,double> &mEdgeData)
    {
    while(sSearchTris.empty()==false)
        {
        unsigned int nT=*(sSearchTris.begin());
        unsigned int nT0=aAdjacencies[nT];
        unsigned int nT1=aAdjacencies[nT+1];
        unsigned int nT2=aAdjacencies[nT+2];
        sSearchTris.erase(nT);
        if(nT0!=std::numeric_limits<unsigned int>::max())
            {
            if(FlipTriangles(aScaled2D,aTriangles,aAdjacencies,nT,0))
                {
                sSearchTris.insert(nT);
                sSearchTris.insert(nT0);
                nT1=aAdjacencies[nT+1];
                nT2=aAdjacencies[nT+2];
                FixEdgeData(pSurface,aPoints2D,nT,nT0,aNormals,aTriangles,aAdjacencies,sEdgeData,mEdgeData);
                }
            }
        if(nT1!=std::numeric_limits<unsigned int>::max())
            {
            if(FlipTriangles(aScaled2D,aTriangles,aAdjacencies,nT,1))
                {
                sSearchTris.insert(nT);
                sSearchTris.insert(nT1);
                nT2=aAdjacencies[nT+2];
                FixEdgeData(pSurface,aPoints2D,nT,nT1,aNormals,aTriangles,aAdjacencies,sEdgeData,mEdgeData);
                }
            }
        if(nT2!=std::numeric_limits<unsigned int>::max())
            {
            if(FlipTriangles(aScaled2D,aTriangles,aAdjacencies,nT,2))
                {
                sSearchTris.insert(nT);
                sSearchTris.insert(nT2);
                FixEdgeData(pSurface,aPoints2D,nT,nT2,aNormals,aTriangles,aAdjacencies,sEdgeData,mEdgeData);
                }
            }
        }
    } 

void SplitEdge(EdgeValue                                       const &EV,
               face                                            const *pFace,
               std::vector<SGM::Point2D>                             &aPoints2D,
               std::vector<SGM::Point2D>                             &aScaled2D,
               double                                                 dScale,
               std::vector<SGM::Point3D>                             &aPoints3D,
               std::vector<SGM::UnitVector3D>                        &aNormals,
               std::vector<unsigned int>                             &aTriangles,
               std::vector<unsigned int>                             &aAdjacencies,
               std::set<EdgeValue>                                   &sEdgeData,
               std::map<std::pair<unsigned int,unsigned int>,double> &mEdgeData)
    {
    // Add the new point

    surface const *pSurface=pFace->GetSurface();
    unsigned int nT0=EV.m_nTriangle;
    unsigned int nEdge=EV.m_nEdge;
    unsigned int a=aTriangles[nT0+nEdge];
    unsigned int b=aTriangles[nT0+(nEdge+1)%3];
    unsigned int c=aTriangles[nT0+(nEdge+2)%3];
    SGM::Point2D const &uv0=aPoints2D[a];
    SGM::Point2D const &uv1=aPoints2D[b];
    SGM::Point2D Miduv=SGM::MidPoint(uv0,uv1);
    SGM::Point3D Pos;
    SGM::UnitVector3D Norm;
    pSurface->Evaluate(Miduv,&Pos,nullptr,nullptr,&Norm);
    unsigned int m=(unsigned int)aPoints2D.size();
    aPoints2D.push_back(Miduv);
    SGM::Point2D ScaledMidUV(Miduv.m_u,Miduv.m_v*dScale);
    aScaled2D.push_back(ScaledMidUV);
    aPoints3D.push_back(Pos);
    aNormals.push_back(Norm);

    // Find the adjacent triangle information
    
    unsigned int nT1=aAdjacencies[nT0+nEdge];
    unsigned int n0=aTriangles[nT1];
    unsigned int n1=aTriangles[nT1+1];
    unsigned int n2=aTriangles[nT1+2];
    unsigned int d,nA2,nA3;
    if(n0!=a && n0!=b)
        {
        d=n0;
        nA2=aAdjacencies[nT1+2];
        nA3=aAdjacencies[nT1];
        }
    else if(n1!=a && n1!=b)
        {
        d=n1;
        nA2=aAdjacencies[nT1];
        nA3=aAdjacencies[nT1+1];
        }
    else
        {
        d=n2;
        nA2=aAdjacencies[nT1+1];
        nA3=aAdjacencies[nT1+2];
        }

    // Add the new triangles.

    aTriangles[nT0]=a;
    aTriangles[nT0+1]=m;
    aTriangles[nT0+2]=c;

    aTriangles[nT1]=m;
    aTriangles[nT1+1]=b;
    aTriangles[nT1+2]=c;

    unsigned int nT2=(unsigned int)aTriangles.size();
    aTriangles.push_back(m);
    aTriangles.push_back(a);
    aTriangles.push_back(d);

    unsigned int nT3=(unsigned int)aTriangles.size();
    aTriangles.push_back(b);
    aTriangles.push_back(m);
    aTriangles.push_back(d);

    // Fix adjacencies

    unsigned int nA0=aAdjacencies[nT0+(nEdge+1)%3];
    unsigned int nA1=aAdjacencies[nT0+(nEdge+2)%3];

    aAdjacencies[nT0]=nT2;
    aAdjacencies[nT0+1]=nT1;
    aAdjacencies[nT0+2]=nA1;

    aAdjacencies[nT1]=nT3;
    aAdjacencies[nT1+1]=nA0;
    aAdjacencies[nT1+2]=nT0;

    aAdjacencies.push_back(nT0);
    aAdjacencies.push_back(nA2);
    aAdjacencies.push_back(nT3);

    aAdjacencies.push_back(nT1);
    aAdjacencies.push_back(nT2);
    aAdjacencies.push_back(nA3);

    FixBackPointer(nA0,c,nT1,aTriangles,aAdjacencies);
    FixBackPointer(nA1,a,nT0,aTriangles,aAdjacencies);
    FixBackPointer(nA2,d,nT2,aTriangles,aAdjacencies);
    FixBackPointer(nA3,b,nT3,aTriangles,aAdjacencies);

    // Fix edge data.

    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nT0,nEdge)],nT0,nEdge));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nT0,(nEdge+1)%3)],nT0,(nEdge+1)%3));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nT0,(nEdge+2)%3)],nT0,(nEdge+2)%3));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nT1,0)],nT1,0));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nT1,1)],nT1,1));
    sEdgeData.erase(EdgeValue(mEdgeData[std::pair<unsigned int,unsigned int>(nT1,2)],nT1,2));

    if(aAdjacencies[nT0+0]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,a,m);
        sEdgeData.insert(EdgeValue(dVal,nT0,0));
        mEdgeData[std::pair<unsigned int,unsigned int>(nT0,0)]=dVal;
        }
    if(aAdjacencies[nT3+0]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,b,m);
        sEdgeData.insert(EdgeValue(dVal,nT3,0));
        mEdgeData[std::pair<unsigned int,unsigned int>(nT3,0)]=dVal;
        }
    if(aAdjacencies[nT1+2]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,c,m);
        sEdgeData.insert(EdgeValue(dVal,nT1,2));
        mEdgeData[std::pair<unsigned int,unsigned int>(nT1,2)]=dVal;
        }
    if(aAdjacencies[nT2+2]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,d,m);
        sEdgeData.insert(EdgeValue(dVal,nT2,2));
        mEdgeData[std::pair<unsigned int,unsigned int>(nT2,2)]=dVal;
        }

    if(c<a && aAdjacencies[nT0+2]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,a,c);
        sEdgeData.insert(EdgeValue(dVal,nT0,2));
        mEdgeData[std::pair<unsigned int,unsigned int>(nT0,2)]=dVal;
        }
    if(b<c && aAdjacencies[nT1+1]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,b,c);
        sEdgeData.insert(EdgeValue(dVal,nT1,1));
        mEdgeData[std::pair<unsigned int,unsigned int>(nT1,1)]=dVal;
        }
    if(a<d  && aAdjacencies[nT2+1]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,a,d);
        sEdgeData.insert(EdgeValue(dVal,nT2,1));
        mEdgeData[std::pair<unsigned int,unsigned int>(nT2,1)]=dVal;
        }
    if(d<b && aAdjacencies[nT3+2]!=std::numeric_limits<unsigned int>::max())
        {
        double dVal=FindNormalDiff(false,pSurface,aNormals,aPoints2D,d,b);;
        sEdgeData.insert(EdgeValue(dVal,nT3,2));
        mEdgeData[std::pair<unsigned int,unsigned int>(nT3,2)]=dVal;
        }

    std::set<unsigned int> sSearchTris;
    sSearchTris.insert(nT0);
    sSearchTris.insert(nT1);
    sSearchTris.insert(nT2);
    sSearchTris.insert(nT3);
    IncrementalDelaunay(pSurface,sSearchTris,aPoints2D,aScaled2D,aNormals,aTriangles,aAdjacencies,sEdgeData,mEdgeData);
    }            

void RefineTriangles(double                          dScale,
                     face                     const *pFace,
                     FacetOptions             const &Options,
                     std::vector<SGM::Point2D>      &aPoints2D,
                     std::vector<SGM::Point2D>      &aScaled,
                     std::vector<SGM::Point3D>      &aPoints3D,
                     std::vector<SGM::UnitVector3D> &aNormals,
                     std::vector<unsigned int>      &aTriangles,
                     std::vector<unsigned int>      &aAdjacencies)
    {
    std::set<EdgeValue> sEdgeData;
    std::map<std::pair<unsigned int,unsigned int>,double> mEdgeData;
    size_t nTriangles=aTriangles.size();
    surface const *pSurface=pFace->GetSurface();
    size_t Index1;
    for(Index1=0;Index1<nTriangles;Index1+=3)
        {
        unsigned int a=aTriangles[Index1];
        unsigned int b=aTriangles[Index1+1];
        unsigned int c=aTriangles[Index1+2];
        if(a<b && aAdjacencies[Index1]!=std::numeric_limits<unsigned int>::max())
            {
            double dVal=FindNormalDiff(true,pSurface,aNormals,aPoints2D,a,b);
            sEdgeData.insert(EdgeValue(dVal,(unsigned int)Index1,0));
            mEdgeData[std::pair<unsigned int,unsigned int>((unsigned int)Index1,0)]=dVal;
            }
        if(b<c && aAdjacencies[Index1+1]!=std::numeric_limits<unsigned int>::max())
            {
            double dVal=FindNormalDiff(true,pSurface,aNormals,aPoints2D,c,b);
            sEdgeData.insert(EdgeValue(dVal,(unsigned int)Index1,1));
            mEdgeData[std::pair<unsigned int,unsigned int>((unsigned int)Index1,1)]=dVal;
            }
        if(c<a && aAdjacencies[Index1+2]!=std::numeric_limits<unsigned int>::max())
            {
            double dVal=FindNormalDiff(true,pSurface,aNormals,aPoints2D,a,c);
            sEdgeData.insert(EdgeValue(dVal,(unsigned int)Index1,2));
            mEdgeData[std::pair<unsigned int,unsigned int>((unsigned int)Index1,2)]=dVal;
            }
        }
    double dDotTol=std::cos(Options.m_dFaceAngleTol);
    bool bSplit=true;
    size_t nCount=0;
    while(bSplit && !sEdgeData.empty())
        {
        EdgeValue EV=*(sEdgeData.begin());
        if(EV.m_dDot<dDotTol)
            {
            SplitEdge(EV,pFace,aPoints2D,aScaled,dScale,aPoints3D,aNormals,aTriangles,aAdjacencies,sEdgeData,mEdgeData);
            bSplit=true;
            }
        else
            {
            bSplit=false;
            }
        ++nCount;
        }
    }

void FindNormals(SGM::Result                    &,//rResult,
                 face                     const *pFace,
                 std::vector<SGM::Point2D>      &aPoints2D,
                 std::vector<SGM::UnitVector3D> &aNormals)
    {
    size_t nPoints2D=aPoints2D.size();
    surface const *pSurface=pFace->GetSurface();
    size_t Index1;
    for(Index1=0;Index1<nPoints2D;++Index1)
        {
        SGM::Point2D const &uv=aPoints2D[Index1];
        SGM::UnitVector3D Norm;
        pSurface->Evaluate(uv,nullptr,nullptr,nullptr,&Norm);
        aNormals.push_back(Norm);
        }
    }

double ScaledUVs(face                      const *pFace,
                 std::vector<SGM::Point2D> const &aPoints2D,
                 std::vector<SGM::Point2D>       &aScaled)
    {
    surface const *pSurface=pFace->GetSurface();
    SGM::Vector3D DU,DV;
    SGM::Point2D uv=pSurface->GetDomain().MidPoint();
    pSurface->Evaluate(uv,nullptr,&DU,&DV);
    
    //double dUk=pSurface->DirectionalCurvature(uv,DU);
    //double dVk=pSurface->DirectionalCurvature(uv,DV);
    //double dScale=fabs(dUk/dVk)*0.25);

    double dScale=DV.Magnitude()/DU.Magnitude();
    size_t nPoints=aPoints2D.size();
    aScaled.reserve(nPoints);
    size_t Index1;
    for(Index1=0;Index1<nPoints;++Index1)
        {
        SGM::Point2D uvToScale=aPoints2D[Index1];
        uvToScale.m_v*=dScale;
        aScaled.push_back(uvToScale);
        }
    return dScale;
    }

void AddVertices(std::vector<SGM::Point3D> const &aPoints3D,
                 std::vector<entity *>           &aEntities)
    {
    size_t nEntities=aEntities.size();
    size_t Index1;
    for(Index1=0;Index1<nEntities;++Index1)
        {
        entity *pEnt=aEntities[Index1];
        SGM::Point3D const &Pos=aPoints3D[Index1];
        if(pEnt->GetType()==SGM::EdgeType)
            {
            edge *pEdge=(edge *)pEnt;
            if(vertex *pStart=pEdge->GetStart())
                {
                if(Pos.DistanceSquared(pStart->GetPoint())<SGM_ZERO_SQUARED)
                    {
                    aEntities[Index1]=pStart;
                    }
                }
            if(vertex *pEnd=pEdge->GetEnd())
                {
                if(Pos.DistanceSquared(pEnd->GetPoint())<SGM_ZERO_SQUARED)
                    {
                    aEntities[Index1]=pEnd;
                    }
                }
            }
        }
    }

bool PointOnSegment(SGM::Point2D const &A,
                    SGM::Point2D const &B,
                    SGM::Point2D const &D,
                    double              dTolerance)
    {
    SGM::UnitVector2D Axis=B-A;
    SGM::Point2D ClosePos=A+Axis*((D-A)%Axis);
    return ClosePos.DistanceSquared(D)<dTolerance*dTolerance;
    }

bool OnTwoEdges(unsigned int                                      a1,
                unsigned int                                      b1,
                unsigned int                                      c1,
                SGM::Point2D                               const &A,
                SGM::Point2D                               const &B,
                SGM::Point2D                               const &C,
                size_t                                            nStart,
                std::vector<SGM::BoxTree::BoundedItemType> const &aHits,
                std::vector<SGM::Point2D>                  const &aPoints,
                std::vector<unsigned int>                  const &aTriangles,
                SGM::Point2D                               const &D,
                double                                            dTolerance,
                size_t                                           &nOther,
                size_t                                           &nEdge1,
                size_t                                           &nEdge2)
    {
    unsigned int a=a1,b=b1;
    if(PointOnSegment(A,B,D,dTolerance))
        {
        nEdge1=0;
        }
    else if(PointOnSegment(B,C,D,dTolerance))
        {
        nEdge1=1;
        a=b1;
        b=c1;
        }
    else if(PointOnSegment(C,A,D,dTolerance))
        {
        nEdge1=2;
        a=c1;
        b=a1;
        }
    else
        {
        return false;
        }
    size_t Index1;
    size_t nHits=aHits.size();
    nEdge2=3;
    for(Index1=nStart+1;Index1<nHits;++Index1)
        {
        nOther=*((size_t *)aHits[Index1].first);
        unsigned int a2=aTriangles[nOther]; 
        unsigned int b2=aTriangles[nOther+1]; 
        unsigned int c2=aTriangles[nOther+2]; 
        if((a2==b && b2==a) || (b2==b && c2==a) || (c2==b && a2==a))
            {
            SGM::Point2D const &A2=aPoints[a2];
            SGM::Point2D const &B2=aPoints[b2];
            SGM::Point2D const &C2=aPoints[c2];
            if(PointOnSegment(A2,B2,D,dTolerance))
                {
                nEdge2=0;
                }
            else if(PointOnSegment(B2,C2,D,dTolerance))
                {
                nEdge2=1;
                }
            else if(PointOnSegment(C2,A2,D,dTolerance))
                {
                nEdge2=2;
                }
            if(nEdge2!=3)
                {
                return true;
                }
            }
        }
    return false;
    }

SGM::Interval3D TriangleBox(std::vector<SGM::Point2D> &aPoints,
                            std::vector<unsigned int> &aTriangles,
                            size_t                     nTri)
    {
    unsigned int a=aTriangles[nTri];
    unsigned int b=aTriangles[nTri+1];
    unsigned int c=aTriangles[nTri+2];
    SGM::Point2D const &A=aPoints[a];
    SGM::Point2D const &B=aPoints[b];
    SGM::Point2D const &C=aPoints[c];
    SGM::Point3D Pos0(A.m_u,A.m_v,0.0);
    SGM::Point3D Pos1(B.m_u,B.m_v,0.0);
    SGM::Point3D Pos2(C.m_u,C.m_v,0.0);
    std::vector<SGM::Point3D> aVertices;
    aVertices.reserve(3);
    aVertices.push_back(Pos0);
    aVertices.push_back(Pos1);
    aVertices.push_back(Pos2);
    return SGM::Interval3D(aVertices);
    }

void SplitEdgeUpdateTree(SGM::Point2D        const &D,
                         std::vector<SGM::Point2D> &aPoints,
                         std::vector<unsigned int> &aTriangles,
                         size_t                     nHitTri,
                         size_t                     nEdge1,
                         size_t                     nOther,
                         size_t                     nEdge2,
                         std::vector<size_t> const &aTris,
                         SGM::BoxTree              &Tree)
    {
    unsigned int a1=aTriangles[nHitTri];
    unsigned int b1=aTriangles[nHitTri+1];
    unsigned int c1=aTriangles[nHitTri+2];
    unsigned int a2=aTriangles[nOther];
    unsigned int b2=aTriangles[nOther+1];
    unsigned int c2=aTriangles[nOther+2];

    unsigned int d=(unsigned int)aPoints.size();
    aPoints.push_back(D);

    unsigned int A1,B1,C1,A2,B2,C2;
    if(nEdge1==0)
        {
        A1=a1;
        B1=b1;
        C1=c1;
        }
    else if(nEdge1==1)
        {
        A1=b1;
        B1=c1;
        C1=a1;
        }
    else
        {
        A1=c1;
        B1=a1;
        C1=b1;
        }
    if(nEdge2==0)
        {
        A2=a2;
        B2=b2;
        C2=c2;
        }
    else if(nEdge2==1)
        {
        A2=b2;
        B2=c2;
        C2=a2;
        }
    else
        {
        A2=c2;
        B2=a2;
        C2=b2;
        }

    // Make triangles (A1,d,C1) (C1,d,B1) (C2,d,B2) (A2,d,C2)

    aTriangles[nHitTri]=A1;
    aTriangles[nHitTri+1]=d;
    aTriangles[nHitTri+2]=C1;

    aTriangles[nOther]=C1;
    aTriangles[nOther+1]=d;
    aTriangles[nOther+2]=B1;

    size_t nNew1=aTriangles.size();

    aTriangles.push_back(C2);
    aTriangles.push_back(d);
    aTriangles.push_back(B2);

    size_t nNew2=aTriangles.size();

    aTriangles.push_back(A2);
    aTriangles.push_back(d);
    aTriangles.push_back(C2);

    SGM::Interval3D Tri1Box=TriangleBox(aPoints,aTriangles,nHitTri);
    SGM::Interval3D Tri2Box=TriangleBox(aPoints,aTriangles,nOther);
    SGM::Interval3D New1Box=TriangleBox(aPoints,aTriangles,nNew1);
    SGM::Interval3D New2Box=TriangleBox(aPoints,aTriangles,nNew2);

    // Update the tree.
    // Take nHitTri and nOther out of the tree.
    // Put nHitTree, nOther, nNew1, nNew2 into the tree.

    void const *pTri1=&aTris[nHitTri/3];
    void const *pTri2=&aTris[nOther/3];
    void const *pNew1=&aTris[nNew1/3];
    void const *pNew2=&aTris[nNew2/3];

    Tree.Erase(pTri1);
    Tree.Erase(pTri2);
    Tree.Insert(pTri1,Tri1Box);
    Tree.Insert(pTri2,Tri2Box);
    Tree.Insert(pNew1,New1Box);
    Tree.Insert(pNew2,New2Box);
    }

void SplitTriangleUpdateTree(SGM::Point2D        const &D,
                             std::vector<SGM::Point2D> &aPoints,
                             std::vector<unsigned int> &aTriangles,
                             size_t                     nHitTri,
                             std::vector<size_t> const &aTris,
                             SGM::BoxTree              &Tree)
    {
    
    unsigned int a=aTriangles[nHitTri];
    unsigned int b=aTriangles[nHitTri+1];
    unsigned int c=aTriangles[nHitTri+2];

    unsigned int d=(unsigned int)aPoints.size();
    aPoints.push_back(D);

    // Make triangles (a,b,d), (b,c,d), (c,a,d)

    aTriangles[nHitTri]=a;
    aTriangles[nHitTri+1]=b;
    aTriangles[nHitTri+2]=d;

    size_t nNew1=aTriangles.size();

    aTriangles.push_back(b);
    aTriangles.push_back(c);
    aTriangles.push_back(d);

    size_t nNew2=aTriangles.size();

    aTriangles.push_back(c);
    aTriangles.push_back(a);
    aTriangles.push_back(d);

    SGM::Interval3D Tri1Box=TriangleBox(aPoints,aTriangles,nHitTri);
    SGM::Interval3D New1Box=TriangleBox(aPoints,aTriangles,nNew1);
    SGM::Interval3D New2Box=TriangleBox(aPoints,aTriangles,nNew2);

    // Update the tree.
    // Take nHitTri out of the tree.
    // Put nHitTree, nNew1, nNew2 into the tree.

    void const *pTri1=&aTris[nHitTri/3];
    void const *pNew1=&aTris[nNew1/3];
    void const *pNew2=&aTris[nNew2/3];

    Tree.Erase(pTri1);
    Tree.Insert(pTri1,Tri1Box);
    Tree.Insert(pNew1,New1Box);
    Tree.Insert(pNew2,New2Box);
    }

bool NearBoundary(unsigned int                                           a,
                  unsigned int                                           b,
                  unsigned int                                           c,
                  SGM::Point2D                                    const &A,
                  SGM::Point2D                                    const &B,
                  SGM::Point2D                                    const &C,
                  SGM::Point2D                                    const &D,
                  std::set<std::pair<unsigned int,unsigned int> > const &sBoundaryEdges,
                  double                                                 dBoundaryTolerance)
    {
    if(sBoundaryEdges.find(std::pair<unsigned int,unsigned int>(a,b))!=sBoundaryEdges.end())
        {
        if(SGM::Segment2D(A,B).Distance(D)<dBoundaryTolerance)
            {
            return true;
            }
        }
    if(sBoundaryEdges.find(std::pair<unsigned int,unsigned int>(b,c))!=sBoundaryEdges.end())
        {
        if(SGM::Segment2D(B,C).Distance(D)<dBoundaryTolerance)
            {
            return true;
            }
        }
    if(sBoundaryEdges.find(std::pair<unsigned int,unsigned int>(c,a))!=sBoundaryEdges.end())
        {
        if(SGM::Segment2D(C,A).Distance(D)<dBoundaryTolerance)
            {
            return true;
            }
        }
    return false;
    }

void InsertPoints(SGM::Result                     &,//rResult,
                  face                      const *pFace,
                  std::vector<SGM::Point2D>       &aInsertPoints,
                  double                           dBoundaryTolerance,
                  std::vector<SGM::Point2D>       &aPoints2D,
                  std::vector<SGM::Point3D>       &aPoints3D,
                  std::vector<unsigned int>       &aTriangles,
                  bool                             bEdge)
    {
    // Find the boundary edges.

    std::vector<SGM::Point2D> aEdgePoints;
    std::set<std::pair<unsigned int,unsigned int> > sBoundaryEdges;
    SGM::FindBoundaryEdges(aTriangles,sBoundaryEdges);
    dBoundaryTolerance*=0.5;

    // Create a tree of the facets.

    size_t nInsertPoints=aInsertPoints.size();
    std::vector<size_t> aTris;
    size_t nTriangles=aTriangles.size();
    size_t nMaxTris=nTriangles+nInsertPoints*6;
    aTris.reserve(nMaxTris/3);
    size_t Index1,Index2;
    for(Index1=0;Index1<nMaxTris;Index1+=3)
        {
        aTris.push_back(Index1);
        }
    std::vector<SGM::Point3D> aVertices;
    aVertices.reserve(3);
    SGM::BoxTree Tree;
    for(Index1=0;Index1<nTriangles;Index1+=3)
        {
        unsigned int a=aTriangles[Index1]; 
        unsigned int b=aTriangles[Index1+1]; 
        unsigned int c=aTriangles[Index1+2]; 
        SGM::Point2D const &A=aPoints2D[a];
        SGM::Point2D const &B=aPoints2D[b];
        SGM::Point2D const &C=aPoints2D[c];
        aVertices.push_back(SGM::Point3D(A.m_u,A.m_v,0.0));
        aVertices.push_back(SGM::Point3D(B.m_u,B.m_v,0.0));
        aVertices.push_back(SGM::Point3D(C.m_u,C.m_v,0.0));
        SGM::Interval3D Box(aVertices);
        aVertices.clear();
        Tree.Insert(&aTris[Index1/3],Box);
        }

    // For each point find the triangles with intersecting boxes.

    surface const *pSurface=pFace->GetSurface();
    for(Index1=0;Index1<nInsertPoints;++Index1)
        {
        SGM::Point2D const &D=aInsertPoints[Index1];
        SGM::Point3D Pos3D(D.m_u,D.m_v,0.0);
        std::vector<SGM::BoxTree::BoundedItemType> aHits=Tree.FindIntersectsPoint(Pos3D,SGM_MIN_TOL);
        size_t nHits=aHits.size();
        for(Index2=0;Index2<nHits;++Index2)
            {
            size_t nHitTri=*((size_t *)aHits[Index2].first);
            unsigned int a=aTriangles[nHitTri]; 
            unsigned int b=aTriangles[nHitTri+1]; 
            unsigned int c=aTriangles[nHitTri+2]; 
            SGM::Point2D const &A=aPoints2D[a];
            SGM::Point2D const &B=aPoints2D[b];
            SGM::Point2D const &C=aPoints2D[c];
            if(SGM::InTriangle(A,B,C,D) && !NearBoundary(a,b,c,A,B,C,D,sBoundaryEdges,dBoundaryTolerance))
                {
                // There are three cases the point is a vertex, on an edge, or inside the triangle.

                size_t nOther,nEdge1,nEdge2;

                if(SGM::NearEqual(A,D,SGM_FIT) || SGM::NearEqual(B,D,SGM_FIT) || SGM::NearEqual(B,D,SGM_FIT))
                    {
                    // Point already there.
                    break;
                    }
                else if(OnTwoEdges(a,b,c,A,B,C,Index2,aHits,aPoints2D,aTriangles,D,SGM_MIN_TOL,nOther,nEdge1,nEdge2))
                    {
                    // Split nHitTri at nEdge1 and nOther at nEdge2.
                    if(bEdge)
                        {
                        SplitEdgeUpdateTree(D,aPoints2D,aTriangles,nHitTri,nEdge1,nOther,nEdge2,aTris,Tree);
                        SGM::Point3D Pos;
                        pSurface->Evaluate(D,&Pos);
                        aPoints3D.push_back(Pos);
                        }
                    else
                        {
                        aEdgePoints.push_back(D);
                        }
                    break;
                    }
                else
                    {
                    // Split nHitTri.
                    SplitTriangleUpdateTree(D,aPoints2D,aTriangles,nHitTri,aTris,Tree);
                    SGM::Point3D Pos;
                    pSurface->Evaluate(D,&Pos);
                    aPoints3D.push_back(Pos);
                    break;
                    }
                }
            }
        }
    aInsertPoints=aEdgePoints;
    }

void OrderPoints(std::vector<SGM::Point2D> &aPoints,
                 SGM::Point2D        const &Pos)
    {
    size_t nPoints=aPoints.size();
    std::vector<std::pair<double,size_t> > aPairs;
    aPairs.reserve(nPoints);
    size_t Index1;
    for(Index1=0;Index1<nPoints;++Index1)
        {
        double dDist=Pos.DistanceSquared(aPoints[Index1]);
        aPairs.push_back(std::pair<double,size_t>(dDist,Index1));
        }
    std::sort(aPairs.begin(),aPairs.end());
    std::vector<SGM::Point2D> aTemp;
    aTemp.reserve(nPoints);
    for(Index1=0;Index1<nPoints;++Index1)
        {
        aTemp.push_back(aPoints[aPairs[Index1].second]);
        }
    aPoints=aTemp;
    }

bool AddGrid(SGM::Result                    &rResult,
             face                     const *pFace,
             FacetOptions             const &Options,
             std::vector<SGM::Point2D>      &aPoints2D,
             std::vector<SGM::Point3D>      &aPoints3D,
             std::vector<unsigned int>      &aTriangles,
             std::vector<unsigned int>      &aAdjacencies)
    {
    surface const *pSurface=pFace->GetSurface();
    if(pSurface->GetSurfaceType()==SGM::TorusType)
        {
        SGM::Interval2D Box(aPoints2D);
        size_t nU=(size_t)(Box.m_UDomain.Length()/Options.m_dEdgeAngleTol+SGM_MIN_TOL);
        size_t nV=(size_t)(Box.m_VDomain.Length()/Options.m_dEdgeAngleTol+SGM_MIN_TOL);
        double dBoundaryTolerance=std::min(Box.m_UDomain.Length()/nU,Box.m_VDomain.Length()/nV);
        std::vector<SGM::Point2D> aInsertPoints;
        aInsertPoints.reserve(nU*nV);
        size_t Index1,Index2;
        for(Index1=0;Index1<nU;++Index1)
            {
            double u=Box.m_UDomain.MidPoint((Index1+1)/(nU+1.0));
            for(Index2=0;Index2<nV;++Index2)
                {
                double v=Box.m_VDomain.MidPoint((Index2+1)/(nV+1.0));
                SGM::Point2D uv(u,v);
                aInsertPoints.push_back(uv);
                }
            }
        OrderPoints(aInsertPoints,Box.MidPoint());
        InsertPoints(rResult,pFace,aInsertPoints,dBoundaryTolerance,aPoints2D,aPoints3D,aTriangles,false);
        aAdjacencies.clear();
        SGM::FindAdjacences2D(aTriangles,aAdjacencies);
        DelaunayFlips(aPoints2D,aTriangles,aAdjacencies);
        InsertPoints(rResult,pFace,aInsertPoints,dBoundaryTolerance,aPoints2D,aPoints3D,aTriangles,true);
        aAdjacencies.clear();
        SGM::FindAdjacences2D(aTriangles,aAdjacencies);
        return true;
        }
    return false;
    }

#if 0
void CheckTriangles(std::vector<SGM::Point2D> &aPoints,
                    std::vector<unsigned int> &aTriangles)
    {
    size_t nTriangles=aTriangles.size();
    size_t Index1;
    for(Index1=0;Index1<nTriangles;Index1+=3)
        {
        unsigned int a=aTriangles[Index1];
        unsigned int b=aTriangles[Index1+1];
        unsigned int c=aTriangles[Index1+2];
        SGM::Point2D const &A=aPoints[a];
        SGM::Point2D const &B=aPoints[b];
        SGM::Point2D const &C=aPoints[c];
        SGM::Vector2D Vec1=B-A;
        SGM::Vector2D Vec2=C-A;
        double dTest=Vec1.m_u*Vec2.m_v-Vec1.m_v*Vec2.m_u;
        if(dTest<0)
            {
            int w=0;
            w*=1;
            }
        }
    }
#endif
    
void FacetFace(SGM::Result                    &rResult,
               face                     const *pFace,
               FacetOptions             const &Options,
               std::vector<SGM::Point2D>      &aPoints2D,
               std::vector<SGM::Point3D>      &aPoints3D,
               std::vector<SGM::UnitVector3D> &aNormals,
               std::vector<unsigned int>      &aTriangles,
               std::vector<entity *>          &aEntities)
    {
    std::vector<std::vector<unsigned int> > aaPolygons;
    std::vector<unsigned int> aAdjacencies;
    FacetFaceLoops(rResult,pFace,Options,aPoints2D,aPoints3D,aEntities,aaPolygons);
    SGM::TriangulatePolygon(rResult,aPoints2D,aaPolygons,aTriangles,aAdjacencies);
    size_t nOldSize=aPoints2D.size();
    if(AddGrid(rResult,pFace,Options,aPoints2D,aPoints3D,aTriangles,aAdjacencies))
        {
        FindNormals(rResult,pFace,aPoints2D,aNormals);
        size_t nNewSize=aPoints2D.size();
        size_t Index1;
        for(Index1=nOldSize;Index1<nNewSize;++Index1)
            {
            aEntities.push_back((entity *)pFace);
            }
        AddVertices(aPoints3D,aEntities);
        DelaunayFlips(aPoints2D,aTriangles,aAdjacencies);
        }
    else if(Options.m_bParametric==false)
        {
        FindNormals(rResult,pFace,aPoints2D,aNormals);
        std::vector<SGM::Point2D> aScaled;
        double dScale=ScaledUVs(pFace,aPoints2D,aScaled);
        RefineTriangles(dScale,pFace,Options,aPoints2D,aScaled,aPoints3D,aNormals,aTriangles,aAdjacencies);
        size_t nNewSize=aPoints2D.size();
        size_t Index1;
        for(Index1=nOldSize;Index1<nNewSize;++Index1)
            {
            aEntities.push_back((entity *)pFace);
            }
        AddVertices(aPoints3D,aEntities);
        DelaunayFlips(aScaled,aTriangles,aAdjacencies);
        }
    }

} // End of SGMInternal namespace