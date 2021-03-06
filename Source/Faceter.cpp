#include "SGMVector.h"
#include "SGMMathematics.h"
#include "SGMInterval.h"
#include "SGMPrimitives.h"
#include "SGMResult.h"
#include "SGMEnums.h"
#include "SGMBoxTree.h"
#include "SGMSegment.h"
#include "SGMEntityClasses.h"
#include "SGMGraph.h"
#include "SGMTriangle.h"
#include "SGMPolygon.h"

#include "EntityClasses.h"
#include "Topology.h"
#include "Faceter.h"
#include "Surface.h"
#include "Curve.h"
#include "Primitive.h"
#include "Query.h"

#include <list>
#include <cmath>
#include <algorithm>

namespace SGMInternal
{

void RefineTrianglesSub(std::vector<SGM::Point2D>               &aPoints2D,
                        std::vector<SGM::Point3D>               &aPoints3D,
                        std::vector<SGM::UnitVector3D>          &aNormals,
                        std::vector<unsigned>                   &aTriangles,
                        std::set<std::pair<unsigned,unsigned> > &sSplitEdges,
                        surface                           const *pSurface)
    {
    // Conduct full refining of the inside.

    std::map<std::pair<unsigned,unsigned>,unsigned> mEdgeMap;
    size_t nTriangles=aTriangles.size();
    size_t Index1;
    for(Index1=0;Index1<nTriangles;Index1+=3)
        {
        unsigned a=aTriangles[Index1];
        unsigned b=aTriangles[Index1+1];
        unsigned c=aTriangles[Index1+2];
        std::pair<unsigned,unsigned> ab=a<b ? std::pair<unsigned,unsigned>(a,b) : std::pair<unsigned,unsigned>(b,a);
        std::pair<unsigned,unsigned> bc=b<c ? std::pair<unsigned,unsigned>(b,c) : std::pair<unsigned,unsigned>(c,b);
        std::pair<unsigned,unsigned> ca=c<a ? std::pair<unsigned,unsigned>(c,a) : std::pair<unsigned,unsigned>(a,c);
        bool bSplitAB=false,bSplitBC=false,bSplitCA=false;
        unsigned nAB=0,nBC=0,nCA=0; 

        if(sSplitEdges.find(ab)!=sSplitEdges.end())
            {
            bSplitAB=true;
            if(mEdgeMap.find(ab)==mEdgeMap.end())
                {
                SGM::Point2D uv=SGM::MidPoint(aPoints2D[a],aPoints2D[b]);
                SGM::UnitVector3D Norm;
                SGM::Point3D Pos;
                pSurface->Evaluate(uv,&Pos,nullptr,nullptr,&Norm);
                nAB=(unsigned)aPoints2D.size();
                mEdgeMap[ab]=nAB;
                aPoints2D.push_back(uv);
                aPoints3D.push_back(Pos);
                aNormals.push_back(Norm);
               }
            else
                {
                nAB=mEdgeMap[ab];
                }
            }
        if(sSplitEdges.find(bc)!=sSplitEdges.end())
            {
            bSplitBC=true;
            if(mEdgeMap.find(bc)==mEdgeMap.end())
                {
                SGM::Point2D uv=SGM::MidPoint(aPoints2D[b],aPoints2D[c]);
                SGM::UnitVector3D Norm;
                SGM::Point3D Pos;
                pSurface->Evaluate(uv,&Pos,nullptr,nullptr,&Norm);
                nBC=(unsigned)aPoints2D.size();
                mEdgeMap[bc]=nBC;
                aPoints2D.push_back(uv);
                aPoints3D.push_back(Pos);
                aNormals.push_back(Norm);
                }
            else
                {
                nBC=mEdgeMap[bc];
                }
            }
        if(sSplitEdges.find(ca)!=sSplitEdges.end())
            {
            bSplitCA=true;
            if(mEdgeMap.find(ca)==mEdgeMap.end())
                {
                SGM::Point2D uv=SGM::MidPoint(aPoints2D[c],aPoints2D[a]);
                SGM::UnitVector3D Norm;
                SGM::Point3D Pos;
                pSurface->Evaluate(uv,&Pos,nullptr,nullptr,&Norm);
                nCA=(unsigned)aPoints2D.size();
                mEdgeMap[ca]=nCA;
                aPoints2D.push_back(uv);
                aPoints3D.push_back(Pos);
                aNormals.push_back(Norm);
                }
            else
                {
                nCA=mEdgeMap[ca];
                }
            }

        // There are seven cases ABC, AB, BC, CA, A, B, C.

        if(bSplitAB && bSplitBC && bSplitCA)
            {
            // Make four triangles. (A,AB,CA),(AB,BC,CA),(AB,B,BC),(BC,C,CA)
            aTriangles[Index1]=a;
            aTriangles[Index1+1]=nAB;
            aTriangles[Index1+2]=nCA;
            aTriangles.push_back(nAB);
            aTriangles.push_back(nBC);
            aTriangles.push_back(nCA);
            aTriangles.push_back(nAB);
            aTriangles.push_back(b);
            aTriangles.push_back(nBC);
            aTriangles.push_back(nBC);
            aTriangles.push_back(c);
            aTriangles.push_back(nCA);
            }
        else if(bSplitAB && bSplitBC)
            {
            // Make three triangles. (A,AB,BC),(AB,B,BC),(BC,C,A)
            aTriangles[Index1]=a;
            aTriangles[Index1+1]=nAB;
            aTriangles[Index1+2]=nBC;
            aTriangles.push_back(nAB);
            aTriangles.push_back(b);
            aTriangles.push_back(nBC);
            aTriangles.push_back(nBC);
            aTriangles.push_back(c);
            aTriangles.push_back(a);
            }
        else if(bSplitBC && bSplitCA)
            {
            // Make three triangles. (A,B,BC),(BC,CA,A),(BC,C,CA)
            aTriangles[Index1]=a;
            aTriangles[Index1+1]=b;
            aTriangles[Index1+2]=nBC;
            aTriangles.push_back(nBC);
            aTriangles.push_back(nCA);
            aTriangles.push_back(a);
            aTriangles.push_back(nBC);
            aTriangles.push_back(c);
            aTriangles.push_back(nCA);
            }
        else if(bSplitCA && bSplitAB)
            {
            // Make three triangles. (A,AB,CA),(AB,B,CA),(B,C,CA)
            aTriangles[Index1]=a;
            aTriangles[Index1+1]=nAB;
            aTriangles[Index1+2]=nCA;
            aTriangles.push_back(nAB);
            aTriangles.push_back(b);
            aTriangles.push_back(nCA);
            aTriangles.push_back(b);
            aTriangles.push_back(c);
            aTriangles.push_back(nCA);
            }
        else if(bSplitAB)
            {
            // Make two triangles. (A,AB,C),(AB,B,C)
            aTriangles[Index1]=a;
            aTriangles[Index1+1]=nAB;
            aTriangles[Index1+2]=c;
            aTriangles.push_back(nAB);
            aTriangles.push_back(b);
            aTriangles.push_back(c);
            }
        else if(bSplitBC)
            {
            // Make two triangles. (A,B,BC),(BC,C,A)
            aTriangles[Index1]=a;
            aTriangles[Index1+1]=b;
            aTriangles[Index1+2]=nBC;
            aTriangles.push_back(nBC);
            aTriangles.push_back(c);
            aTriangles.push_back(a);
            }
        else if(bSplitCA)
            {
            // Make two triangles. (A,B,CA),(B,C,CA)
            aTriangles[Index1]=a;
            aTriangles[Index1+1]=b;
            aTriangles[Index1+2]=nCA;
            aTriangles.push_back(b);
            aTriangles.push_back(c);
            aTriangles.push_back(nCA);
            }
        }
    }

void RefineTriangles(std::vector<SGM::Point2D>      &aPoints2D,
                     std::vector<SGM::Point3D>      &aPoints3D,
                     std::vector<SGM::UnitVector3D> &aNormals,
                     std::vector<unsigned>          &aTriangles,
                     surface                  const *pSurface)
    {
    // Find the boundary edges so that they are not refined.

    std::vector<unsigned int> aBoundary;
    std::set<unsigned int> sInterior;
    SGM::FindBoundary(aTriangles,aBoundary,sInterior);
    std::set<std::pair<unsigned,unsigned> > sBoundary;
    size_t Index1;
    size_t nBoundary=aBoundary.size();
    for(Index1=0;Index1<nBoundary;Index1+=2)
        {
        unsigned a=aBoundary[Index1];
        unsigned b=aBoundary[Index1+1];
        sBoundary.insert(a<b ? std::pair<unsigned,unsigned>(a,b) : std::pair<unsigned,unsigned>(b,a));
        }

    // Test to see if we need to refine.

    size_t nCount=0;
    size_t nMaxCount=5;
    size_t nMinCount=1;
    std::set<std::pair<unsigned,unsigned> > sSplitEdges,sAllInside;
    while(nCount<nMaxCount)
        {
        size_t nTriangles=aTriangles.size();
        for(Index1=0;Index1<nTriangles;Index1+=3)
            {
            unsigned a=aTriangles[Index1];
            unsigned b=aTriangles[Index1+1];
            unsigned c=aTriangles[Index1+2];
            std::pair<unsigned,unsigned> ab=a<b ? std::pair<unsigned,unsigned>(a,b) : std::pair<unsigned,unsigned>(b,a);
            std::pair<unsigned,unsigned> bc=b<c ? std::pair<unsigned,unsigned>(b,c) : std::pair<unsigned,unsigned>(c,b);
            std::pair<unsigned,unsigned> ca=c<a ? std::pair<unsigned,unsigned>(c,a) : std::pair<unsigned,unsigned>(a,c);
            if(CheckFacet(aPoints3D,aNormals,aTriangles,pSurface,Index1))
                {
                if(sBoundary.find(ab)==sBoundary.end())
                    {
                    sSplitEdges.insert(ab);
                    }
                if(sBoundary.find(bc)==sBoundary.end())
                    {
                    sSplitEdges.insert(bc);
                    }
                if(sBoundary.find(ca)==sBoundary.end())
                    {
                    sSplitEdges.insert(ca);
                    }
                }
            if(sBoundary.find(ab)==sBoundary.end())
                {
                sAllInside.insert(ab);
                }
            if(sBoundary.find(bc)==sBoundary.end())
                {
                sAllInside.insert(bc);
                }
            if(sBoundary.find(ca)==sBoundary.end())
                {
                sAllInside.insert(ca);
                }
            }
        if(sSplitEdges.empty()==false || nCount<nMinCount)
            {
            if(nCount<2 || 20<sSplitEdges.size())
                {
                RefineTrianglesSub(aPoints2D,aPoints3D,aNormals,aTriangles,sAllInside,pSurface);
                }
            else
                {
                RefineTrianglesSub(aPoints2D,aPoints3D,aNormals,aTriangles,sSplitEdges,pSurface);
                }
            std::vector<unsigned> aAdjacencies;
            SGM::FindAdjacencies2D(aTriangles,aAdjacencies);
            DelaunayFlips(aPoints2D,aTriangles,aAdjacencies);
            }
        else
            {
            break;
            }
        ++nCount;
        }
    }

// Like std::set_intersection, but return after finding the first intersection.

template <class COMPARE, class INPUT1, class INPUT2, class OUTPUT>
OUTPUT SetIntersectionFirst(INPUT1 pFirst1, INPUT1 pLast1,
                            INPUT2 pFirst2, INPUT2 pLast2,
                            OUTPUT pResult, COMPARE compareFunction)
    {
    while (pFirst1 != pLast1 && pFirst2 != pLast2)
        {
        if (compareFunction(*pFirst1, *pFirst2))
            ++pFirst1;
        else
            {
            if (!compareFunction(*pFirst2, *pFirst1))
                {
                *pResult = *pFirst1;
                // ++pFirst1;
                return ++pResult;
                }
            ++pFirst2;
            }
        }
    return pResult;
    }

edge *FindEdge(entity *pEntA,entity *pEntB)
    {
    bool isAEdge = pEntA->GetType()==SGM::EdgeType;
    bool isBEdge = pEntB->GetType()==SGM::EdgeType;
    if (isAEdge)
        {
        edge *pEdgeA = (edge *)pEntA;
        if (pEdgeA->GetStart()==pEntB || pEdgeA->GetEnd()==pEntB || (isBEdge && pEntA==pEntB))
            {
            return pEdgeA;
            }
        }
    if (isBEdge)
        {
        edge *pEdgeB = (edge *)pEntB;
        if (pEdgeB->GetStart()==pEntA || pEdgeB->GetEnd()==pEntA)
            {
            return pEdgeB;
            }
        }
    if( pEntA->GetType()==SGM::VertexType &&
        pEntB->GetType()==SGM::VertexType)
        {
        auto const &sEdgesA=((vertex*)pEntA)->GetEdges();
        auto const &sEdgesB=((vertex*)pEntB)->GetEdges();
        std::vector<edge*> aIntersection;
        SetIntersectionFirst(sEdgesA.begin(), sEdgesA.end(),
                             sEdgesB.begin(), sEdgesB.end(),
                             std::back_inserter(aIntersection),
                             EntityCompare());
        if (!aIntersection.empty())
            return (edge*)aIntersection[0];
        }
    return nullptr;
    }

void SubdivideTriangles(std::vector<unsigned int> &aTriangles, size_t nPoints)
    {
    size_t nTriangles = aTriangles.size();
    unsigned a,b,c,ab,bc,ca,nPointsCount;

    size_t nNewTriangles=nTriangles*4;
    if (nNewTriangles > aTriangles.capacity())
        {
        aTriangles.reserve(nNewTriangles);
        }

    nPointsCount = (unsigned)nPoints;
    for(size_t Index1=0;Index1<nTriangles;Index1+=3)
        {
        ab = nPointsCount++;
        bc = nPointsCount++;
        ca = nPointsCount++;

        a = aTriangles[Index1];
        b = aTriangles[Index1 + 1];
        c = aTriangles[Index1 + 2];

        aTriangles[Index1] = ab;
        aTriangles[Index1 + 1] = bc;
        aTriangles[Index1 + 2] = ca;

        aTriangles.push_back(a);
        aTriangles.push_back(ab);
        aTriangles.push_back(ca);
        aTriangles.push_back(b);
        aTriangles.push_back(bc);
        aTriangles.push_back(ab);
        aTriangles.push_back(c);
        aTriangles.push_back(ca);
        aTriangles.push_back(bc);
        }
    }

void SubdividePoints(face                      const *pFace,
                     size_t                           nOriginalTriangles,
                     std::vector<unsigned int> const &aTriangles,
                     std::vector<unsigned int> const &aAdjacencies,
                     std::vector<SGM::Point2D>       &aPoints2D,
                     std::vector<SGM::Point3D>       &aPoints3D,
                     std::vector<entity *>           &aEntities)
    {
    surface const *pSurface=pFace->GetSurface();

    size_t nPoints=aPoints3D.size();
    assert(nPoints == aPoints3D.size());

    size_t nNewPoints=nOriginalTriangles+2*nPoints-2;
    if (nNewPoints > aPoints3D.capacity())
        {
        aPoints3D.reserve(nNewPoints);
        aPoints2D.reserve(nNewPoints);
        aEntities.reserve(nNewPoints);
        }

    unsigned a,b,c,ab,bc,ca;
    unsigned nOffset = (unsigned)nOriginalTriangles;
    for(size_t Index1=0;Index1<nOriginalTriangles;Index1+=3,nOffset+=9)
        {
        a  = aTriangles[nOffset];
        ab = aTriangles[nOffset+1];
        ca = aTriangles[nOffset+2];
        b  = aTriangles[nOffset+3];
        bc = aTriangles[nOffset+4];
        c  = aTriangles[nOffset+6];

        SGM::Point2D const &A=aPoints2D[a];
        SGM::Point2D const &B=aPoints2D[b];
        SGM::Point2D const &C=aPoints2D[c];
        aPoints2D.emplace_back(0.5*(A.m_u+B.m_u),0.5*(A.m_v+B.m_v));
        aPoints2D.emplace_back(0.5*(B.m_u+C.m_u),0.5*(B.m_v+C.m_v));
        aPoints2D.emplace_back(0.5*(C.m_u+A.m_u),0.5*(C.m_v+A.m_v));

        SGM::Point3D AB3D,BC3D,CA3D;
        pSurface->Evaluate(aPoints2D[ab],&AB3D);
        pSurface->Evaluate(aPoints2D[bc],&BC3D);
        pSurface->Evaluate(aPoints2D[ca],&CA3D);

        size_t nCurrentEntities = aEntities.size();

        aPoints3D.push_back(AB3D);
        aPoints3D.push_back(BC3D);
        aPoints3D.push_back(CA3D);

        aEntities.push_back((entity *)pFace);
        aEntities.push_back((entity *)pFace);
        aEntities.push_back((entity *)pFace);

        SGM::Point3D CPos;

        if (aAdjacencies[Index1] == std::numeric_limits<unsigned>::max())
            {
            edge *pEdgeAB = FindEdge(aEntities[a],aEntities[b]);
            if (pEdgeAB)
                {
                pEdgeAB->GetCurve()->Inverse(AB3D, &CPos);
                aPoints2D[ab] = pSurface->Inverse(CPos, nullptr, &aPoints2D[ab]);
                aPoints3D[ab] = CPos;
                aEntities[nCurrentEntities] = pEdgeAB;
                }
            }

        if (aAdjacencies[Index1+1] == std::numeric_limits<unsigned>::max())
            {
            edge *pEdgeBC = FindEdge(aEntities[b],aEntities[c]);
            if(pEdgeBC)
                {
                pEdgeBC->GetCurve()->Inverse(BC3D,&CPos);
                aPoints2D[bc] = pSurface->Inverse(CPos,nullptr,&aPoints2D[bc]);
                aPoints3D[bc] = CPos;
                aEntities[nCurrentEntities+1] = pEdgeBC;
                }
            }

        if (aAdjacencies[Index1+2] == std::numeric_limits<unsigned>::max())
            {
            edge *pEdgeCA = FindEdge(aEntities[c],aEntities[a]);
            if(pEdgeCA)
                {
                pEdgeCA->GetCurve()->Inverse(CA3D,&CPos);
                aPoints2D[ca] = pSurface->Inverse(CPos,nullptr,&aPoints2D[ca]);
                aPoints3D[ca] = CPos;
                aEntities[nCurrentEntities+2] = pEdgeCA;
                }
            }
        }
    }

void SubdivideFacets(face                const *pFace,
                     std::vector<SGM::Point3D> &aPoints3D,
                     std::vector<SGM::Point2D> &aPoints2D,
                     std::vector<unsigned int> &aTriangles,
                     std::vector<entity *>     &aEntities)
    {

    size_t nPoints=aPoints3D.size();
    size_t nTriangles=aTriangles.size();

    std::vector<unsigned> aAdjacencies;
    SGM::FindAdjacencies2D(aTriangles, aAdjacencies);

    SubdivideTriangles(aTriangles, nPoints);

    SubdividePoints(pFace, nTriangles, aTriangles, aAdjacencies, aPoints2D, aPoints3D, aEntities);
    }

class Node
    {
    public:

        Node() {m_bMark=false,m_bImprint=true;}

        size_t               m_nNext;
        size_t               m_nPrevious;
        SGM::Point2D         m_uv;
        SGM::Point3D         m_Pos;
        double               m_t;
        SGMInternal::entity *m_Entity;
        bool                 m_bMark;
        bool                 m_bImprint;
    };

static void FixBackPointers(unsigned int                     nTri,
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

static bool TestTriangle(SGM::Point2D const &A,
                         SGM::Point2D const &B,
                         SGM::Point2D const &C)
    {
    SGM::Vector2D Vec1=B-A;
    SGM::Vector2D Vec2=C-A;
    double dTest=Vec1.m_u*Vec2.m_v-Vec1.m_v*Vec2.m_u;
    return 0<dTest;
    }

static bool FlipTriangles(std::vector<SGM::Point2D> const &aPoints2D,
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
    SGM::Point2D const &A=aPoints2D[a];
    SGM::Point2D const &B=aPoints2D[b];
    SGM::Point2D const &C=aPoints2D[c];
    SGM::Point2D const &G=aPoints2D[g];
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

void DelaunayFlips(std::vector<SGM::Point2D> const &aPoints2D,
                   std::vector<unsigned int>       &aTriangles,
                   std::vector<unsigned int>       &aAdjacencies)
    {
    size_t nTriangles=aTriangles.size();
    bool bFlipped=true;
    unsigned int Index1;
    size_t nMaxFlips=aTriangles.size()/3;
    size_t nCount=0;
    while(bFlipped)
        {
        ++nCount;
        if(nMaxFlips<nCount)
            {
            break;
            }
        bFlipped=false;
        for(Index1=0;Index1<(unsigned int)nTriangles;Index1+=3)
            {
            if(FlipTriangles(aPoints2D,aTriangles,aAdjacencies,Index1,0))
                {
                bFlipped=true;
                }
            if(FlipTriangles(aPoints2D,aTriangles,aAdjacencies,Index1,1))
                {
                bFlipped=true;
                }
            if(FlipTriangles(aPoints2D,aTriangles,aAdjacencies,Index1,2))
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
              SGM::Point3D const &Pos):m_dParam(dParam),m_Pos(Pos),m_uv() {}

    double       m_dParam;
    SGM::Point3D m_Pos;
    SGM::Point2D m_uv;
    };

class FacetNodeNormal
    {
    public:

    FacetNodeNormal(double              dParam,
                    SGM::Point3D const &Pos,
                    SGM::Point2D const &uv):
    m_dParam(dParam),m_Pos(Pos),m_uv(uv),m_Norm(),m_bSingular(false) {}

    double            m_dParam;
    SGM::Point3D      m_Pos;
    SGM::Point2D      m_uv;
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
            auto pLine=(line const *)pCurve;
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
            auto pCircle=(circle const *)pCurve;
            double dRadius=pCircle->GetRadius();
            double dDomainLength=Domain.Length();
            double dAngle=Options.m_dEdgeAngleTol;
            if(Options.m_dMaxLength)
                {
                double dArcLength=dDomainLength*dRadius;
                double dAngleFraction=dArcLength/Options.m_dMaxLength;
                dAngle=std::min(dAngle,dDomainLength*dAngleFraction);
                }
            if(Options.m_dCordHeight)
                {
                // CordAngle = 2*acos(1-CordHight/Radius)
                dAngle=std::min(dAngle,2.0*acos(1.0-Options.m_dCordHeight/dRadius));
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
            auto pTorusKnot=(TorusKnot const *)pCurve;
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
            if(Options.m_dCordHeight)
                {
                // CordAngle = 2*acos(1-CordHight/Radius)
                dAngle=std::min(dAngle,2.0*acos(1.0-Options.m_dCordHeight/dRadius));
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
            std::vector<double> aKnots=pCurve->SpecialFacetParams();
            for(double dt : aKnots)
                {
                if(Domain.InInterior(dt,SGM_FIT))
                    {
                    SGM::Point3D Pos2;
                    pCurve->Evaluate(dt,&Pos2);
                    lNodes.emplace_back(dt,Pos2);
                    }
                }
            lNodes.emplace_back(d1,Pos1);
            double dAngle=SGM_PI-Options.m_dEdgeAngleTol;
            if(nCurveType==SGM::ParabolaType)
                {
                dAngle=SGM_PI-0.03490658503988659153847381536977; // 2 degrees
                }
            double dCos=cos(dAngle);
            bool bRefine=true;
            while(bRefine)
                {
                bRefine=false;
                auto LastIter=lNodes.begin();
                auto iter=lNodes.begin();
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
            for (auto node : lNodes)
                {
                aPoints3D.push_back(node.m_Pos);
                aParams.push_back(node.m_dParam);
                }
            }
        }
    }

static void FindCrossingPoint(curve  const *pCurve1, // Seam
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

static bool SplitAtSeams(SGM::Result                     & ,
                         surface                   const *pSurface,
                         edge                      const *pEdge,
                         curve                     const *pCurve,
                         std::vector<SGM::Point3D> const &aPoints3D,
                         std::vector<double>       const &aParams,
                         std::vector<double>             &aCrosses,
                         std::vector<SGM::Point3D>       &aCrossPoints)
    {
    SGM::Result EmptyResult(nullptr);
    bool bFound=false;
    if(pSurface->ClosedInU() || pSurface->ClosedInV())
        {
        aCrosses.push_back(aParams.front());
        aCrosses.push_back(aParams.back());
        aCrossPoints.push_back(aPoints3D.front());
        aCrossPoints.push_back(aPoints3D.back());
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

            // make sure pSeam gets deleted because it is not owned by a Thing
            std::shared_ptr<curve> pSeam(pSurface->UParamLine(EmptyResult,Domain.m_dMin));

            for(Index1=0;Index1<nParams;++Index1)
                {
                Node const &Node0=aNodes[Index1];
                Node const &Node1=aNodes[Node0.m_nNext];
                double dDist=fabs(Node0.m_uv.m_u-Node1.m_uv.m_u);
                if(dGap<dDist)
                    {
                    SGM::Point3D Pos=Node0.m_Pos;
                    double t=Node0.m_t;
                    FindCrossingPoint(pSeam.get(),pCurve,Pos,t);
                    pEdge->SnapToDomain(t,SGM_MIN_TOL);
                    if(SGM_MIN_TOL<fabs(t-Node0.m_t) && SGM_MIN_TOL<fabs(t-Node1.m_t))
                        {
                        if(pEdge->GetDomain().InInterval(t,SGM_MIN_TOL))
                            {
                            aCrosses.push_back(t);
                            aCrossPoints.push_back(Pos);
                            bFound=true;
                            }
                        }
                    }
                }
            }
        if(pSurface->ClosedInV())
            {
            SGM::Interval1D const &Domain=pSurface->GetDomain().m_VDomain;
            double dGap=Domain.Length()*0.5;
            std::shared_ptr<curve> pSeam(pSurface->VParamLine(EmptyResult,Domain.m_dMin));
            for(Index1=0;Index1<nParams;++Index1)
                {
                Node const &Node0=aNodes[Index1];
                Node const &Node1=aNodes[Node0.m_nNext];
                double dDist=fabs(Node0.m_uv.m_v-Node1.m_uv.m_v);
                if(dGap<dDist)
                    {
                    SGM::Point3D Pos=Node0.m_Pos;
                    double t=Node0.m_t;
                    FindCrossingPoint(pSeam.get(),pCurve,Pos,t);
                    pEdge->SnapToDomain(t,SGM_MIN_TOL);
                    if(SGM_MIN_TOL<fabs(t-Node0.m_t) && SGM_MIN_TOL<fabs(t-Node1.m_t))
                        {
                        if(pEdge->GetDomain().InInterval(t,SGM_MIN_TOL))
                            {
                            aCrosses.push_back(t);
                            aCrossPoints.push_back(Pos);
                            bFound=true;
                            }
                        }
                    }
                }
            }
        }
    return bFound;
    }

static bool SplitFacet(curve                          const *pCurve,
                       surface                        const *pSurface,
                       std::list<FacetNodeNormal>::iterator &NodeA,
                       std::list<FacetNodeNormal>::iterator &NodeB,
                       std::list<FacetNodeNormal>           &lNodes)
    {
    int u_cont = pSurface->UContinuity();
    int v_cont = pSurface->VContinuity();
    double dParamA=NodeA->m_dParam;
    double dParamB=NodeB->m_dParam;

    if (u_cont < 1 || v_cont < 1)
        {
        if ((dParamB - dParamA) < SGM_FIT)
            {
            return false;
            }
        }
    double dParamC=(dParamA+dParamB)*0.5;
    SGM::Point3D Pos;
    pCurve->Evaluate(dParamC,&Pos);
    SGM::Point2D uv=pSurface->Inverse(Pos);
    FacetNodeNormal NodeC(dParamC,Pos,uv);
    pSurface->Evaluate(uv,nullptr,nullptr,nullptr,&NodeC.m_Norm);
    lNodes.insert(NodeB,NodeC);
    NodeB=NodeA;
    ++NodeB;
    return true;
    }

static void SplitWithSurfaceNormals(FacetOptions        const &Options,
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
            if(pSurface->SingularHighU() || pSurface->SingularHighV())
                {
                uv.m_v=CloseUV.m_v;
                }
            else
                {
                uv.m_u=CloseUV.m_u;
                }
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
            if(pSurface->SingularHighU() || pSurface->SingularHighV())
                {
                uv.m_v=CloseUV.m_v;
                }
            else
                {
                uv.m_u=CloseUV.m_u;
                }
            }
        else
            {
            SGM::Point3D TestPos;
            pSurface->Evaluate(uv,&TestPos,nullptr,nullptr,&Norm);
            }
        FacetNodeNormal Node(aParams[Index1],Pos,uv);
        if(bSingular)
            {
            Node.m_bSingular=true;
            }
        Node.m_Norm=Norm;
        lNodes.push_back(Node);
        }

    double dDotTol=std::cos(Options.m_dFaceAngleTol);
    auto iter=lNodes.begin();
    auto LastIter=iter;
    ++iter;
    size_t nCount=0;
    size_t nMaxSplit=1000;
    while(iter!=lNodes.end())
        {
        double dotProd = iter->m_Norm%LastIter->m_Norm;
        if(!iter->m_bSingular && !LastIter->m_bSingular && dotProd < dDotTol)
            {
            bool bSplitThisTime = SplitFacet(pCurve,pSurface,LastIter,iter,lNodes);
            if (!bSplitThisTime)
                {
                ++LastIter;
                ++iter;
                }
            else
                {
                ++nCount;
                if(nMaxSplit<nCount)
                    {
                    break;
                    }
                }
            }
        else
            {
            ++LastIter;
            ++iter;
            }
        }

    // Force all edges on NUB surfaces to have at least two faces.
    if(nCount==0 && nPoints==2)
        {
        if( pSurface->GetSurfaceType()==SGM::NUBSurfaceType || 
            pSurface->GetSurfaceType()==SGM::NURBSurfaceType)
            {
            auto Node=lNodes.begin();
            auto NodeA=Node;
            ++Node;
            auto NodeB=Node;
            double dParamA=NodeA->m_dParam;
            double dParamB=NodeB->m_dParam;
            double dParamC=(dParamA+dParamB)*0.5;
            SGM::Point3D Pos;
            pCurve->Evaluate(dParamC,&Pos);
            SGM::Point2D uv=pSurface->Inverse(Pos);
            FacetNodeNormal NodeC(dParamC,Pos,uv);
            lNodes.insert(NodeB,NodeC);
            NodeB=NodeA;
            }
        }
    
    aPoints3D.clear();
    aParams.clear();
    auto iterNodeSplit=lNodes.begin();
    while(iterNodeSplit!=lNodes.end())
        {
        aPoints3D.push_back(iterNodeSplit->m_Pos);
        aParams.push_back(iterNodeSplit->m_dParam);
        ++iterNodeSplit;
        }
    }

double MoreFacetChecks(edge const *pEdge)
    {
    double dAnswer=1.0;
    if( (pEdge->GetStart() && pEdge->GetStart()->CuspValue()<SGM_MIN_TOL) ||
        (pEdge->GetEnd() && pEdge->GetEnd()->CuspValue()<SGM_MIN_TOL))
        {
        return 0.5;
        }
    if(pEdge->GetCurve()->GetCurveType()==SGM::CircleType)
        {
        std::set<edge *,EntityCompare> sAllEdges;
        if(pEdge->GetStart())
            {
            std::set<edge *,EntityCompare> sStartEdges=pEdge->GetStart()->GetEdges();
            for(edge *pStartEdge : sStartEdges)
                {
                if(pStartEdge!=pEdge && pStartEdge->GetCurve()->GetCurveType()==SGM::CircleType)
                    {
                    sAllEdges.insert(pStartEdge);
                    }
                }
            }
        if(pEdge->GetEnd())
            {
            std::set<edge *,EntityCompare> sEndEdges=pEdge->GetEnd()->GetEdges();
            for(edge *pEndEdge : sEndEdges)
                {
                if(pEndEdge!=pEdge && pEndEdge->GetCurve()->GetCurveType()==SGM::CircleType)
                    {
                    sAllEdges.insert(pEndEdge);
                    }
                }
            }
        if(sAllEdges.empty()==false)
            {
            double dMinRadius=SGM_MAX;
            for(edge *pTestEdge : sAllEdges)
                {
                circle *pCircle=(circle *)(pTestEdge->GetCurve());
                if(pCircle->GetRadius()<dMinRadius)
                    {
                    dMinRadius=pCircle->GetRadius();
                    }
                }
            dAnswer=dMinRadius/((circle *)(pEdge->GetCurve()))->GetRadius();
            if(1<dAnswer)
                {
                dAnswer=1.0;
                }
            if(dAnswer<0.1)
                {
                dAnswer=0.1;
                }
            }
        }
    return dAnswer;
    }

void FacetEdge(SGM::Result               &rResult,
               edge                const *pEdge,
               FacetOptions        const &Options,
               std::vector<SGM::Point3D> &aPoints3D,
               std::vector<double>       &aParams)
    {
    curve const *pCurve=pEdge->GetCurve();
    SGM::Interval1D const &Domain=pEdge->GetDomain();
    double dFactor=MoreFacetChecks(pEdge);
    if(dFactor!=1.0)
        {
        FacetOptions TempOptions=Options;
        TempOptions.m_dEdgeAngleTol*=dFactor;
        FacetCurve(pCurve,Domain,TempOptions,aPoints3D,aParams);
        }
    else
        {
        FacetCurve(pCurve,Domain,Options,aPoints3D,aParams);
        if(pCurve->GetCurveType()==SGM::NUBCurveType)
            {
            NUBcurve *pNUB=(NUBcurve *)pCurve;
            if(aParams.size()<pNUB->GetControlPoints().size() && aParams.size()<10)
                {
                aPoints3D.clear();
                aParams.clear();
                FacetOptions TempOptions=Options;
                TempOptions.m_dEdgeAngleTol*=0.5;
                FacetCurve(pCurve,Domain,TempOptions,aPoints3D,aParams);
                }
            }
        }

    // Added so that this will be found at this time so that things will be thread safe after.

    pEdge->GetTolerance(); 
    if(pEdge->GetStart())
        {
        pEdge->GetStart()->GetTolerance();
        pEdge->GetEnd()->GetTolerance();
        }
    
    // Find where the facets cross the seams of their surfaces.

    std::vector<double> aCrosses;
    std::vector<SGM::Point3D> aCrossPoints;
    std::set<surface *,EntityCompare> sSurfaces;
    FindSurfaces(rResult,pEdge,sSurfaces);
    bool bFound=false;
    for (auto pSurface : sSurfaces)
        {
        if(SplitAtSeams(rResult,pSurface,pEdge,pCurve,aPoints3D,aParams,aCrosses,aCrossPoints))
            {
            bFound=true;
            }
        }

    // Split facets at the seams of their surfaces.

    if(bFound)
        {
        std::vector<double> aEnds;
        std::vector<SGM::Point3D> aEndPoints;
        size_t nCrosses=aCrosses.size();
        std::vector<SGM::Point4D> aParamAndPos;
        aParamAndPos.reserve(nCrosses);
        size_t Index1,Index2;
        for(Index1=0;Index1<nCrosses;++Index1)
            {
            SGM::Point3D const &Pos=aCrossPoints[Index1];
            double dParam=aCrosses[Index1];
            aParamAndPos.emplace_back(dParam,Pos.m_x,Pos.m_y,Pos.m_z);
            }
        std::sort(aParamAndPos.begin(),aParamAndPos.end());
        std::sort(aCrosses.begin(),aCrosses.end());
        aEnds.push_back(aCrosses.front());
        SGM::Point4D Pos4D=aParamAndPos.front();
        aEndPoints.emplace_back(Pos4D.m_y,Pos4D.m_z,Pos4D.m_w);
        for(Index1=1;Index1<nCrosses;++Index1)
            {
            if(SGM_MIN_TOL<aCrosses[Index1]-aCrosses[Index1-1])
                {
                SGM::Point4D Pos4DIndex=aParamAndPos[Index1];
                aEndPoints.emplace_back(Pos4DIndex.m_y,Pos4DIndex.m_z,Pos4DIndex.m_w);
                aEnds.push_back(aCrosses[Index1]);
                }
            }
        size_t nEnds=aEnds.size();
        SGM::Point4D Pos4DBack=aParamAndPos.back();
        aEndPoints[nEnds-1]=SGM::Point3D(Pos4DBack.m_y,Pos4DBack.m_z,Pos4DBack.m_w);
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
            aTempPos[0]=aEndPoints[Index1-1];
            aTempPos[nTempPos-1]=aEndPoints[Index1];
            size_t nStart = Index1==1 ? 0 : 1;
            for(Index2=nStart;Index2<nTempPos;++Index2)
                {
                aPoints3D.push_back(aTempPos[Index2]);
                aParams.push_back(aTempParams[Index2]);
                }
            }
        }

    // Subdivide facets by surface normals.

    for (auto pSurface : sSurfaces)
        {
        SplitWithSurfaceNormals(Options,pSurface,pCurve,aPoints3D,aParams);
        }

    // Force end points to be on vertices.

    if(pEdge->GetStart()!=pEdge->GetEnd())
        {
        aPoints3D[0]=pEdge->GetStart()->GetPoint();
        aPoints3D[aPoints3D.size()-1]=pEdge->GetEnd()->GetPoint();
        }
    }

class SplitData
    {
    public:

        SplitData() = default;

        SplitData(double dParam,size_t nPolygon,size_t nSpan):
            m_dParam(dParam),m_nPolygon(nPolygon),m_nSpan(nSpan) {}

        bool operator<(SplitData const &Other) const {return m_dParam<Other.m_dParam;}

        double m_dParam;
        size_t m_nPolygon;
        size_t m_nSpan;
    };

static size_t AddNode(std::vector<Node>  &aNodes,
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
    NewNode.m_bImprint=false;
    size_t nAnswer=aNodes.size();
    aNodes.push_back(NewNode);
    return nAnswer;
    }

static bool FindSeamCrossings(face        const *pFace,
                              std::vector<Node> &aNodes)
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

            double dDist0=fabs(NodeA.m_uv.m_u-Domain.m_dMin);
            double dDist1=fabs(NodeA.m_uv.m_u-Domain.m_dMax);
            double dDist2=fabs(NodeB.m_uv.m_u-Domain.m_dMin);
            double dDist3=fabs(NodeB.m_uv.m_u-Domain.m_dMax);

            if(dDist0<dDist1 && dDist0<dDist2 && dDist0<dDist3)
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
            else if(dDist1<dDist2 && dDist1<dDist3)
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
            else if(dDist2<dDist3)
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
            else 
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

        for(Index1=0;Index1<nUOutLow;++Index1)
            {
            if(aUInLow.size()<=Index1)
                {
                // Through aUOutLow (min u,min v) and (max u,min v) then to aUInHigh
                size_t nNodeA=aUOutLow[Index1].second;
                size_t nNodeB=aUInHigh[Index1].second;
                nNodeB=aUInHigh[0].second;
                size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMin,VDomain.m_dMin);
                size_t nNodeD=AddNode(aNodes,pFace,UDomain.m_dMax,VDomain.m_dMin);
                aNodes[nNodeA].m_nNext=nNodeC;
                aNodes[nNodeB].m_nPrevious=nNodeD;
                aNodes[nNodeC].m_nNext=nNodeD;
                aNodes[nNodeC].m_nPrevious=nNodeA;
                aNodes[nNodeD].m_nNext=nNodeB;
                aNodes[nNodeD].m_nPrevious=nNodeC;
                aNodes[nNodeA].m_bImprint=false;
                aNodes[nNodeB].m_bImprint=false;
                }
            else
                {
                double v1=aUOutLow[Index1].first;
                double v2=aUInLow[Index1].first;
                size_t nNodeA=aUOutLow[Index1].second;
                size_t nNodeB=aUInLow[Index1].second;
                if(v2<v1)
                    {
                    aNodes[nNodeA].m_nNext=nNodeB;
                    aNodes[nNodeB].m_nPrevious=nNodeA;
                    aNodes[nNodeA].m_bImprint=false;
                    aNodes[nNodeB].m_bImprint=false;
                    }
                else if(Index1<nUOutLow-1)
                    {
                    nNodeB=aUInLow[Index1+1].second;
                    aNodes[nNodeA].m_nNext=nNodeB;
                    aNodes[nNodeB].m_nPrevious=nNodeA;
                    aNodes[nNodeA].m_bImprint=false;
                    aNodes[nNodeB].m_bImprint=false;
                    }
                else // Must go around the corner.
                    {
                    if(aVInLow.empty())
                        {
                        // Through (min u,min v) and (max u,min v) then to aUInHigh[0]
                        if(aUInHigh.empty())
                            {
                            return false;
                            }
                        nNodeB=aUInHigh[0].second;
                        size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMin,VDomain.m_dMin);
                        size_t nNodeD=AddNode(aNodes,pFace,UDomain.m_dMax,VDomain.m_dMin);
                        aNodes[nNodeA].m_nNext=nNodeC;
                        aNodes[nNodeB].m_nPrevious=nNodeD;
                        aNodes[nNodeC].m_nNext=nNodeD;
                        aNodes[nNodeC].m_nPrevious=nNodeA;
                        aNodes[nNodeD].m_nNext=nNodeB;
                        aNodes[nNodeD].m_nPrevious=nNodeC;
                        aNodes[nNodeA].m_bImprint=false;
                        aNodes[nNodeB].m_bImprint=false;
                        }
                    else
                        {
                        // Through (min u,min v)
                        nNodeB=aVInLow[0].second;
                        size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMin,VDomain.m_dMin);
                        aNodes[nNodeA].m_nNext=nNodeC;
                        aNodes[nNodeB].m_nPrevious=nNodeC;
                        aNodes[nNodeC].m_nNext=nNodeB;
                        aNodes[nNodeC].m_nPrevious=nNodeA;
                        aNodes[nNodeA].m_bImprint=false;
                        aNodes[nNodeB].m_bImprint=false;
                        }
                    }
                }
            }

        for(Index1=0;Index1<nVOutLow;++Index1)
            {
            if(aVInLow.size()<=Index1)
                {
                return false;
                }
            double u1=aVOutLow[Index1].first;
            double u2=aVInLow[Index1].first;
            size_t nNodeA=aVOutLow[Index1].second;
            size_t nNodeB=aVInLow[Index1].second;
            if(u1<u2)
                {
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                aNodes[nNodeA].m_bImprint=false;
                aNodes[nNodeB].m_bImprint=false;
                }
            else if(Index1<nVOutLow-1)
                {
                nNodeB=aVInLow[Index1+1].second;
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                aNodes[nNodeA].m_bImprint=false;
                aNodes[nNodeB].m_bImprint=false;
                }
            else // Must go around the corner.
                {
                // Through (max u,min v)
                if(aUInHigh.empty())
                    {
                    return false;
                    }
                nNodeB=aUInHigh[0].second;
                size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMax,VDomain.m_dMin);
                aNodes[nNodeA].m_nNext=nNodeC;
                aNodes[nNodeB].m_nPrevious=nNodeC;
                aNodes[nNodeC].m_nNext=nNodeB;
                aNodes[nNodeC].m_nPrevious=nNodeA;
                aNodes[nNodeA].m_bImprint=false;
                aNodes[nNodeB].m_bImprint=false;
                }
            }

         for(Index1=0;Index1<nUOutHigh;++Index1)
            {
            if(aUInHigh.size()<=Index1)
                {
                // Through aUOutHigh (max u,max v) and (min u,max v) then to aUInLow
                size_t nNodeA=aUOutHigh[Index1].second;
                size_t nNodeB=aUInLow[Index1].second;
                nNodeB=aUInLow[0].second;
                size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMax,VDomain.m_dMax);
                size_t nNodeD=AddNode(aNodes,pFace,UDomain.m_dMin,VDomain.m_dMax);
                aNodes[nNodeA].m_nNext=nNodeC;
                aNodes[nNodeB].m_nPrevious=nNodeD;
                aNodes[nNodeC].m_nNext=nNodeD;
                aNodes[nNodeC].m_nPrevious=nNodeA;
                aNodes[nNodeD].m_nNext=nNodeB;
                aNodes[nNodeD].m_nPrevious=nNodeC;
                aNodes[nNodeA].m_bImprint=false;
                aNodes[nNodeB].m_bImprint=false;
                }
            else 
                {
                double v1=aUOutHigh[Index1].first;
                double v2=aUInHigh[Index1].first;
                size_t nNodeA=aUOutHigh[Index1].second;
                size_t nNodeB=aUInHigh[Index1].second;
                if(v1<v2)
                    {
                    aNodes[nNodeA].m_nNext=nNodeB;
                    aNodes[nNodeB].m_nPrevious=nNodeA;
                    aNodes[nNodeA].m_bImprint=false;
                    aNodes[nNodeB].m_bImprint=false;
                    }
                else if(Index1<nUOutHigh-1)
                    {
                    nNodeB=aUInHigh[Index1+1].second;
                    aNodes[nNodeA].m_nNext=nNodeB;
                    aNodes[nNodeB].m_nPrevious=nNodeA;
                    aNodes[nNodeA].m_bImprint=false;
                    aNodes[nNodeB].m_bImprint=false;
                    }
                else // Must go around the corner.
                    {
                    if(aVInHigh.empty())
                        {
                        // Through (max u,max v) and (min u,max v) then to aUInLow[0]
                        if(aUInLow.empty())
                            {
                            return false;
                            }
                        nNodeB=aUInLow[0].second;
                        size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMax,VDomain.m_dMax);
                        size_t nNodeD=AddNode(aNodes,pFace,UDomain.m_dMin,VDomain.m_dMax);
                        aNodes[nNodeA].m_nNext=nNodeC;
                        aNodes[nNodeB].m_nPrevious=nNodeD;
                        aNodes[nNodeC].m_nNext=nNodeD;
                        aNodes[nNodeC].m_nPrevious=nNodeA;
                        aNodes[nNodeD].m_nNext=nNodeB;
                        aNodes[nNodeD].m_nPrevious=nNodeC;
                        aNodes[nNodeA].m_bImprint=false;
                        aNodes[nNodeB].m_bImprint=false;
                        }
                    else
                        {
                        // Through (max u,max v)
                        nNodeB=aVInHigh[0].second;
                        size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMax,VDomain.m_dMax);
                        aNodes[nNodeA].m_nNext=nNodeC;
                        aNodes[nNodeB].m_nPrevious=nNodeC;
                        aNodes[nNodeC].m_nNext=nNodeB;
                        aNodes[nNodeC].m_nPrevious=nNodeA;
                        aNodes[nNodeA].m_bImprint=false;
                        aNodes[nNodeB].m_bImprint=false;
                        }
                    }
                }
            }

        for(Index1=0;Index1<nVOutHigh;++Index1)
            {
            if(aVInHigh.size()<=Index1)
                {
                return false;
                }
            double u1=aVOutHigh[Index1].first;
            double u2=aVInHigh[Index1].first;
            size_t nNodeA=aVOutHigh[Index1].second;
            size_t nNodeB=aVInHigh[Index1].second;
            if(u2<u1)
                {
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                aNodes[nNodeA].m_bImprint=false;
                aNodes[nNodeB].m_bImprint=false;
                }
            else if(Index1<nVOutHigh-1)
                {
                nNodeB=aVInHigh[Index1+1].second;
                aNodes[nNodeA].m_nNext=nNodeB;
                aNodes[nNodeB].m_nPrevious=nNodeA;
                aNodes[nNodeA].m_bImprint=false;
                aNodes[nNodeB].m_bImprint=false;
                }
            else // Must go around the corner.
                {
                // Through (min u,max v)
                if(aUInLow.empty())
                    {
                    return false;
                    }
                nNodeB=aUInLow[0].second;
                size_t nNodeC=AddNode(aNodes,pFace,UDomain.m_dMin,VDomain.m_dMax);
                aNodes[nNodeA].m_nNext=nNodeC;
                aNodes[nNodeB].m_nPrevious=nNodeC;
                aNodes[nNodeC].m_nNext=nNodeB;
                aNodes[nNodeC].m_nPrevious=nNodeA;
                aNodes[nNodeA].m_bImprint=false;
                aNodes[nNodeB].m_bImprint=false;
                }
            }
        }

    return true;
    }
    
static bool FindPolygon(std::vector<Node>         &aNodes,
                        unsigned int               nStart,
                        std::vector<unsigned int> &aPolygon)
    {
    aPolygon.push_back(nStart);
    size_t nWhere=aNodes[nStart].m_nNext;
    aNodes[nStart].m_bMark=true;
    size_t nCount=0;
    size_t nMaxCount=aNodes.size();
    while(nWhere!=nStart)
        {
        aPolygon.push_back((unsigned int)nWhere);
        aNodes[nWhere].m_bMark=true;
        nWhere=aNodes[nWhere].m_nNext;
        ++nCount;
        if(nMaxCount<nCount)
            {
            return false;
            }
        }
    return true;
    }

static bool FindPolygons(std::vector<Node>                       &aNodes,
                         std::vector<SGM::Point2D>               &aPoints2D,
                         std::vector<SGM::Point3D>               &aPoints3D,
                         std::vector<std::vector<unsigned int> > &aaPolygons,
                         std::vector<bool>                       *pImprintFlags)
    {
    unsigned int nNodes=(unsigned int)aNodes.size();
    unsigned int Index1;
    aPoints2D.reserve(nNodes);
    aPoints3D.reserve(nNodes);
    if(pImprintFlags)
        {
        pImprintFlags->reserve(nNodes);
        }
    for(Index1=0;Index1<nNodes;++Index1)
        {
        aPoints2D.push_back(aNodes[Index1].m_uv);
        aPoints3D.push_back(aNodes[Index1].m_Pos);
        if(pImprintFlags)
            {
            pImprintFlags->push_back(aNodes[Index1].m_bImprint);
            }
        }
    for(Index1=0;Index1<nNodes;++Index1)
        {
        if(!aNodes[Index1].m_bMark)
            {
            std::vector<unsigned int> aPolygon;
            if(!FindPolygon(aNodes,Index1,aPolygon))
                {
                return false;
                }
            aaPolygons.push_back(aPolygon);
            if(Index1==1)
                {
                break;
                }
            }
        }
    return true;
    }

bool FacetFaceLoops(SGM::Result                             &rResult,
                    face                              const *pFace,
                    std::vector<SGM::Point2D>               &aPoints2D,
                    std::vector<SGM::Point3D>               &aPoints3D,
                    std::vector<std::vector<unsigned int> > &aaPolygons,
                    edge                                    *pInputEdge,
                    std::vector<bool>                       *pImprintFlags)
    {
    // Find all the needed face information.

    std::vector<std::vector<edge *> > aaLoops;
    std::vector<std::vector<SGM::EdgeSideType> > aaEdgeSideTypes;
    size_t nLoops=1;
    size_t Index1,Index2,Index3;
    if(pInputEdge==nullptr)
        {
        nLoops=pFace->FindLoops(rResult,aaLoops,aaEdgeSideTypes);

        // Check the loops.
        std::set<edge *> sLoopEdges;
        for(auto aLoop : aaLoops)
            {
            for(auto pEdge : aLoop)
                {
                sLoopEdges.insert(pEdge);
                }
            }
        if(sLoopEdges.size()!=pFace->GetEdges().size())
            {
            return false;
            }
        }
    else
        {
        std::vector<edge *> aLoop;
        aLoop.push_back(pInputEdge);
        aaLoops.push_back(aLoop);
        std::vector<SGM::EdgeSideType> aSides;
        aSides.push_back(pFace->GetSideType(pInputEdge));
        aaEdgeSideTypes.push_back(aSides);
        }

    bool bFlip=pFace->GetFlipped();
    std::set<vertex *,EntityCompare> sVertices;
    FindVertices(rResult,pFace,sVertices);

    // Facet each loop.

    std::vector<Node> aNodes;
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
                SGM::Point2D uv=pFace->AdvancedInverse(pEdge,nSide,aNodes[Index3].m_Pos);
                aNodes[Index3].m_uv=uv;
                }
            }
        
        // Make sure that last node is pointing to the start.

        aNodes.back().m_nNext=nLoopStartNode;
        aNodes[nLoopStartNode].m_nPrevious=aNodes.size()-1;
        }

    if(nLoops)
        {
        if(!FindSeamCrossings(pFace,aNodes))
            {
            return false;
            }
        }
    
    return FindPolygons(aNodes,aPoints2D,aPoints3D,aaPolygons,pImprintFlags);
    }

static void FindNormals(face                     const *pFace,
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

static void FindNormalsAndPoints(face                     const *pFace,
                                 std::vector<SGM::Point2D>      &aPoints2D,
                                 std::vector<SGM::UnitVector3D> &aNormals,
                                 std::vector<SGM::Point3D>      &aPoints3D)
    {
    size_t nPoints2D=aPoints2D.size();
    surface const *pSurface=pFace->GetSurface();
    aNormals.reserve(nPoints2D);
    aPoints3D.reserve(nPoints2D);
    SGM::Interval2D const &Domain=pSurface->GetDomain();
    SGM::Interval1D const &DomainU=Domain.m_UDomain;
    SGM::Interval1D const &DomainV=Domain.m_VDomain;
    size_t Index1;
    for(Index1=0;Index1<nPoints2D;++Index1)
        {
        SGM::Point2D uv=aPoints2D[Index1];
        SGM::UnitVector3D Norm;
        SGM::Point3D Pos;
        if(uv.m_u<DomainU.m_dMin)
            {
            uv.m_u=DomainU.m_dMin;
            }
        if(DomainU.m_dMax<uv.m_u)
            {
            uv.m_u=DomainU.m_dMax;
            }
        if(uv.m_v<DomainV.m_dMin)
            {
            uv.m_v=DomainV.m_dMin;
            }
        if(DomainV.m_dMax<uv.m_v)
            {
            uv.m_v=DomainV.m_dMax;
            }
        pSurface->Evaluate(uv,&Pos,nullptr,nullptr,&Norm);
        if(aNormals.size()<=Index1)
            {
            aNormals.emplace_back(Norm);
            }
        if(aPoints3D.size()<=Index1)
            {
            aPoints3D.emplace_back(Pos);
            }
        }
    }

static double ScaledUVs(face                      const *pFace,
                        std::vector<SGM::Point2D> const &aPoints2D,
                        std::vector<SGM::Point2D>       &aScaled)
    {
    surface const *pSurface=pFace->GetSurface();
    SGM::Vector3D DU,DV;
    SGM::Interval2D Box=SGM::Interval2D(aPoints2D);
    SGM::Point2D uv=Box.MidPoint();
    pSurface->Evaluate(uv,nullptr,&DU,&DV);
    
    double dUk=std::max(SGM_FIT,fabs(pSurface->DirectionalCurvature(uv,DU)));
    double dVk=std::max(SGM_FIT,fabs(pSurface->DirectionalCurvature(uv,DV)));
    double dScale=dVk/dUk;

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

//static double ScaledUVs2(double                          dUGap,
//                         double                          dVGap,
//                         std::vector<SGM::Point2D> const &aPoints2D,
//                         std::vector<SGM::Point2D>       &aScaled)
//    {
//    size_t Index1;
//    double dScale=dUGap/dVGap;
//    size_t nPoints=aPoints2D.size();
//    aScaled.reserve(nPoints);
//    for(Index1=0;Index1<nPoints;++Index1)
//        {
//        SGM::Point2D uvToScale=aPoints2D[Index1];
//        uvToScale.m_v*=dScale;
//        aScaled.push_back(uvToScale);
//        }
//    return dScale;
//    }

static SGM::Interval3D TriangleBox(std::vector<SGM::Point2D> &aPoints,
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

void SplitEdgeUpdateTree(SGM::Point2D             const &D,
                                std::vector<SGM::Point2D>      &aPoints2D,
                                std::vector<unsigned int>      &aTriangles,
                                size_t                          nHitTri,
                                size_t                          nEdge1,
                                std::vector<size_t>      const &aTris,
                                SGM::BoxTree                   &Tree)
    {
    unsigned int a1=aTriangles[nHitTri];
    unsigned int b1=aTriangles[nHitTri+1];
    unsigned int c1=aTriangles[nHitTri+2];

    unsigned int d=(unsigned int)aPoints2D.size();
    aPoints2D.push_back(D);

    unsigned int A1,B1,C1;
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

    // Make triangles (A1,d,C1) (C1,d,B1) 

    aTriangles[nHitTri]=A1;
    aTriangles[nHitTri+1]=d;
    aTriangles[nHitTri+2]=C1;

    size_t nNew1=aTriangles.size();

    aTriangles.push_back(C1);
    aTriangles.push_back(d);
    aTriangles.push_back(B1);

    SGM::Interval3D Tri1Box=TriangleBox(aPoints2D,aTriangles,nHitTri);
    SGM::Interval3D New1Box=TriangleBox(aPoints2D,aTriangles,nNew1);

    // Update the tree.
    // Take nHitTri out of the tree.
    // Put nHitTree, nNew1 into the tree.

    void const *pTri1=&aTris[nHitTri/3];
    void const *pNew1=&aTris[nNew1/3];

    Tree.Erase(pTri1);
    Tree.Insert(pTri1,Tri1Box);
    Tree.Insert(pNew1,New1Box);
    }

void SplitTriangleUpdateTree(SGM::Point2D        const &D,
                                    std::vector<SGM::Point2D> &aPoints2D,
                                    std::vector<unsigned int> &aTriangles,
                                    size_t                     nHitTri,
                                    std::vector<size_t> const &aTris,
                                    SGM::BoxTree              &Tree)
    {
    
    unsigned int a=aTriangles[nHitTri];
    unsigned int b=aTriangles[nHitTri+1];
    unsigned int c=aTriangles[nHitTri+2];

    unsigned int d=(unsigned int)aPoints2D.size();
    aPoints2D.push_back(D);

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

    SGM::Interval3D Tri1Box=TriangleBox(aPoints2D,aTriangles,nHitTri);
    SGM::Interval3D New1Box=TriangleBox(aPoints2D,aTriangles,nNew1);
    SGM::Interval3D New2Box=TriangleBox(aPoints2D,aTriangles,nNew2);

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

static std::vector<bool> FindPolygonImprintFlags(std::vector<bool> const &aInputFlags,
                                                 std::vector<unsigned int> &aPolygons)
    {
    std::vector<bool> aFlags;
    size_t nPolygons=aPolygons.size();
    aFlags.reserve(nPolygons);
    size_t Index1;
    for(Index1=0;Index1<nPolygons;++Index1)
        {
        aFlags.push_back(aInputFlags[aPolygons[Index1]]);
        }
    return aFlags;
    }

//static bool AngleGrid(SGM::Result                                   &rResult,
//                      surface                                 const *pSurface,
//                      FacetOptions                            const &Options,
//                      std::vector<SGM::Point2D>               const &aPolygonPoints,
//                      std::vector<std::vector<unsigned int> >       &aaPolygons,
//                      std::vector<SGM::Point2D>                     &aPoints2D,
//                      std::vector<unsigned int>                     &aTriangles,
//                      std::vector<bool>                             *pImprintFlag)
//    {
//    // Create the base grid.
//
//    std::vector<double> aUValues,aVValues;
//    SGM::Interval2D Box(aPolygonPoints);
//    if(Box.IsEmpty())
//        {
//        Box=pSurface->GetDomain();
//        }
//    size_t nU=(size_t)(Box.m_UDomain.Length()/Options.m_dFaceAngleTol+SGM_MIN_TOL)*2;
//    size_t nV=(size_t)(Box.m_VDomain.Length()/Options.m_dFaceAngleTol+SGM_MIN_TOL)*2;
//    if(nU<3)
//        {
//        nU=3;
//        }
//    if(nV<3)
//        {
//        nV=3;
//        }
//    size_t Index1;
//    for(Index1=0;Index1<=nU;++Index1)
//        {
//        aUValues.push_back(Box.m_UDomain.MidPoint(((double)Index1)/nU));
//        }
//    for(Index1=0;Index1<=nV;++Index1)
//        {
//        aVValues.push_back(Box.m_VDomain.MidPoint(((double)Index1)/nV));
//        }
//    double dMinGrid=std::min(Box.m_UDomain.Length()/nU,Box.m_VDomain.Length()/nV);
//    SGM::CreateTrianglesFromGrid(aUValues,aVValues,aPoints2D,aTriangles);
//    if(aaPolygons.empty())
//        {
//        return true;
//        }
//
//    // Scale the base grid.
//
//    std::vector<SGM::Point2D> aScaled;
//    double dScale=ScaledUVs2(Box.m_UDomain.Length()/(nU-1.0),Box.m_VDomain.Length()/(nV-1.0),aPoints2D,aScaled);
//    std::vector<SGM::Point2D> aScaledPolygonPoints;
//    aScaledPolygonPoints.reserve(aPolygonPoints.size());
//    for(SGM::Point2D uv : aPolygonPoints)
//        {
//        uv.m_v*=dScale;
//        aScaledPolygonPoints.push_back(uv);
//        }
//
//    // Insert the polygons.
//
//    size_t nPolygons=aaPolygons.size();
//    for(Index1=0;Index1<nPolygons;++Index1)
//        {
//        std::vector<unsigned int> aPolygonIndices;
//        std::vector<bool> aFlags=FindPolygonImprintFlags(*pImprintFlag,aaPolygons[Index1]);
//        if(!SGM::InsertPolygon(rResult, SGM::PointsFromPolygon2D(aScaledPolygonPoints, aaPolygons[Index1]),
//                               aScaled, aTriangles, aPolygonIndices, nullptr, nullptr, nullptr, &aFlags))
//            {
//            return false;
//            }
//        aaPolygons[Index1]=aPolygonIndices;
//        }
//    RemoveOutsideTriangles(rResult,aaPolygons,aScaled,aTriangles,dMinGrid*0.25);
//
//    // Since points were removed and added reset aPoints2D from aScaled.
//
//    aPoints2D.clear();
//    aPoints2D.reserve(aScaled.size());
//    double dRScale=1.0/dScale;
//    for(SGM::Point2D uv : aScaled)
//        {
//        uv.m_v*=dRScale;
//        aPoints2D.push_back(uv);
//        }
//    return true;
//    }

void RemoveClosePoints(SGM::Result                               &rResult,
                       std::vector<SGM::Point2D>           const &aPolygonPoints,
                       std::vector<std::vector<unsigned> > const &aaPolygons,
                       std::vector<SGM::Point2D>                 &aPoints2D,
                       std::vector<unsigned>                     &aTriangles,
                       double                                     dMinDist,
                       std::vector<SGM::Point3D>                 *pPoints3D,
                       std::vector<SGM::UnitVector3D>            *pNormals)
    {
    size_t Index1,Index2;
    std::vector<unsigned> aNewTriangles=aTriangles;

    // Build a tree for the aaPolygons line segments, and test all
    // other points to see if they are within dMinDist to the tree.

    std::set<unsigned> sBoundary;
    std::vector<SGM::Segment2D> aSegments;
    for(std::vector<unsigned> const &aPolygon : aaPolygons)
        {
        size_t nPolygon=aPolygon.size();
        aSegments.reserve(aSegments.size() + nPolygon);
        for(Index2=0;Index2<nPolygon;++Index2)
            {
            SGM::Point2D const &Pos0=aPolygonPoints[aPolygon[Index2]];
            SGM::Point2D const &Pos1=aPolygonPoints[aPolygon[(Index2+1)%nPolygon]];
            aSegments.emplace_back(Pos0,Pos1);
            sBoundary.insert(aPolygon[Index2]);
            }
        }

    // Do not remove points from the boundary of the triangles.

    std::vector<unsigned> aBoundary;
    std::set<unsigned> sInterior;
    SGM::FindBoundary(aTriangles,aBoundary,sInterior);
    for(auto nBoundIndex : aBoundary)
        {
        sBoundary.insert(nBoundIndex);
        }

    SGM::BoxTree Tree;
    size_t nSegments=aSegments.size();
    for(Index1=0;Index1<nSegments;++Index1)
        {
        SGM::Segment2D const &Seg=aSegments[Index1];
        SGM::Point3D Pos0(Seg.m_Start.m_u,Seg.m_Start.m_v,0.0);
        SGM::Point3D Pos1(Seg.m_End.m_u,Seg.m_End.m_v,0.0);
        SGM::Interval3D Box(Pos0,Pos1);
        Tree.Insert(&(aSegments[Index1]),Box);
        }

    size_t nPoints2D=aPoints2D.size();
    for(Index1=0;Index1<nPoints2D;++Index1)
        {
        if(sBoundary.find((unsigned)Index1)==sBoundary.end())
            {
            SGM::Point2D const &Pos2D=aPoints2D[Index1];
            SGM::Point3D Pos3D(Pos2D.m_u,Pos2D.m_v,0.0);
            std::vector<const void*> aHits=Tree.FindIntersectsPoint(Pos3D,dMinDist);
            double dDist=std::numeric_limits<unsigned>::max();
            for(const void* pVoid : aHits)
                {
                auto pSeg=(SGM::Segment2D const *)(pVoid);
                double dTestDist=pSeg->Distance(Pos2D);
                if(dTestDist<dDist)
                    {
                    dDist=dTestDist;
                    }
                }
            if(dDist<dMinDist)
                {
                std::vector<unsigned> aRemovedOrChanged,aReplacedTriangles;
                SGM::RemovePointFromTriangles(rResult,(unsigned)Index1,aPoints2D,aNewTriangles,aRemovedOrChanged,aReplacedTriangles);
                }
            }
        }
    aTriangles=aNewTriangles;
    ReduceToUsedPoints(aPoints2D,aTriangles,pPoints3D,pNormals);
    }

void FindPointsToRemove(std::vector<SGM::Point2D>               const &aPolygonPoints,
                        std::vector<std::vector<unsigned int> > const &aaPolygons,
                        std::vector<bool>                       const &aImprintFlags,
                        std::vector<SGM::Point2D>               const &aPoints2D,
                        std::vector<double>                     const &aDistances,
                        surface                                 const *pSurface,
                        std::vector<unsigned>                   const &aTriangles,
                        std::vector<unsigned int>                     &aRemovePoints)
    {
    // Build a tree for the aaPolygons line segments.

    size_t Index1;
    std::vector<SGM::Segment2D> aSegments;
    for(std::vector<unsigned> const &aPolygon : aaPolygons)
        {
        size_t nPolygon=aPolygon.size();
        aSegments.reserve(aSegments.size() + nPolygon);
        for(Index1=0;Index1<nPolygon;++Index1)
            {
            SGM::Point2D const &Pos0=aPolygonPoints[aPolygon[Index1]];
            SGM::Point2D const &Pos1=aPolygonPoints[aPolygon[(Index1+1)%nPolygon]];
            bool bSkip=false;
            if(pSurface->ClosedInU())
                {
                double dMax=pSurface->GetDomain().m_UDomain.Length()*0.5;
                if(dMax<fabs(Pos0.m_u-Pos1.m_u))
                    {
                    bSkip=true;
                    }
                }
            if(pSurface->ClosedInV())
                {
                double dMax=pSurface->GetDomain().m_VDomain.Length()*0.5;
                if(dMax<fabs(Pos0.m_v-Pos1.m_v))
                    {
                    bSkip=true;
                    }
                }
            if( aImprintFlags[aPolygon[Index1]]==false &&
                aImprintFlags[aPolygon[(Index1+1)%nPolygon]]==false)
                {
                bSkip=true;
                }
            if(bSkip==false)
                {
                aSegments.emplace_back(Pos0,Pos1);
                }
            }
        }

    SGM::BoxTree Tree;
    size_t nSegments=aSegments.size();
    for(Index1=0;Index1<nSegments;++Index1)
        {
        SGM::Segment2D const &Seg=aSegments[Index1];
        SGM::Point3D Pos0(Seg.m_Start.m_u,Seg.m_Start.m_v,0.0);
        SGM::Point3D Pos1(Seg.m_End.m_u,Seg.m_End.m_v,0.0);
        SGM::Interval3D Box(Pos0,Pos1);
        Tree.Insert(&(aSegments[Index1]),Box);
        }

    std::vector<unsigned> aBoundary;
    std::set<unsigned> sInterior;
    SGM::FindBoundary(aTriangles,aBoundary,sInterior);

    size_t nPoints2D=aPoints2D.size();
    for(Index1=0;Index1<nPoints2D;++Index1)
        {
        if(sInterior.find((unsigned)Index1)!=sInterior.end())
            {
            SGM::Point2D const &Pos2D=aPoints2D[Index1];
            double dMinDist=aDistances[Index1];
            SGM::Point3D Pos3D(Pos2D.m_u,Pos2D.m_v,0.0);
            std::vector<void const *> aHits=Tree.FindIntersectsPoint(Pos3D,dMinDist);
            double dDist=std::numeric_limits<unsigned>::max();
            for(void const* pVoid : aHits)
                {
                auto pSeg=(SGM::Segment2D const *)(pVoid);
                double dTestDist=pSeg->Distance(Pos2D);
                if(dTestDist<dDist)
                    {
                    dDist=dTestDist;
                    }
                }
            if(dDist<dMinDist)
                {
                aRemovePoints.push_back((unsigned)Index1);
                }
            }
        }
    }

static void FindSpherePoints(surface                  const *pSurface,
                             std::vector<SGM::Point3D>      &aPoints3D,
                             std::vector<unsigned int>      &aTriangles,
                             std::vector<SGM::Point2D>      &aPoints2D,
                             std::vector<SGM::UnitVector3D> &aNormals)
    {
    // Find all the 2D points and normals.

    size_t nPoints=aPoints3D.size();
    aPoints2D.reserve(nPoints);
    aNormals.reserve(nPoints);
    std::set<size_t> sSeamPoints,sPolePoints;
    size_t Index1;
    for(Index1=0;Index1<nPoints;++Index1)
        {
        SGM::Point3D Pos=aPoints3D[Index1];
        SGM::Point2D uv=pSurface->Inverse(Pos);
        SGM::UnitVector3D Norm;
        pSurface->Evaluate(uv,nullptr,nullptr,nullptr,&Norm);
        aNormals.push_back(Norm);
        // Make all seam and pole points have a uv.m_u values of zero.
        if(uv.m_v-SGM_MIN_TOL<-SGM_HALF_PI)
            {
            uv.m_u=0.0;
            sPolePoints.insert(Index1);
            sSeamPoints.insert(Index1);
            }
        else if(SGM_HALF_PI<uv.m_v+SGM_MIN_TOL)
            {
            uv.m_u=0.0;
            sPolePoints.insert(Index1);
            sSeamPoints.insert(Index1);
            }
        else
            {
            if(uv.m_u<SGM_MIN_TOL)
                {
                sSeamPoints.insert(Index1);
                }
            else if(SGM_TWO_PI<uv.m_u+SGM_MIN_TOL)
                {
                uv.m_u=0.0;
                sSeamPoints.insert(Index1);
                }
            }
        aPoints2D.push_back(uv);
        }

    // Create a point on the high value of the seam for every
    // point on the low value.

    std::map<size_t,size_t> mSeamMap;
    for(auto nSeamIndex : sSeamPoints)
        {
        SGM::Point2D uv=aPoints2D[nSeamIndex];
        size_t nSize=aPoints2D.size();
        mSeamMap[nSeamIndex]=nSize;
        uv.m_u=SGM_TWO_PI;
        aPoints2D.push_back(uv);
        aNormals.push_back(aNormals[nSeamIndex]);
        aPoints3D.push_back(aPoints3D[nSeamIndex]);
        }

    // Fix triangles that are on the seam to point to the correct side.

    size_t nTriangles=aTriangles.size();
    for(Index1=0;Index1<nTriangles;Index1+=3)
        {
        unsigned int a=aTriangles[Index1];
        unsigned int b=aTriangles[Index1+1];
        unsigned int c=aTriangles[Index1+2];
        if( sSeamPoints.find(a)!=sSeamPoints.end() ||
            sSeamPoints.find(b)!=sSeamPoints.end() ||
            sSeamPoints.find(c)!=sSeamPoints.end())
            {
            SGM::Point3D const &A=aPoints3D[a];
            SGM::Point3D const &B=aPoints3D[b];
            SGM::Point3D const &C=aPoints3D[c];
            SGM::Point3D CM=SGM::CenterOfMass(A,B,C);
            SGM::Point2D GuessUV=pSurface->Inverse(CM);
            if( sPolePoints.find(a)!=sPolePoints.end() ||
                sPolePoints.find(b)!=sPolePoints.end() ||
                sPolePoints.find(c)!=sPolePoints.end())
                {
                if(GuessUV.m_u<SGM_PI)
                    {
                    GuessUV.m_u=0.0;
                    }
                else
                    {
                    GuessUV.m_u=SGM_TWO_PI;
                    }
                }
            SGM::Point2D Auv=pSurface->Inverse(A,nullptr,&GuessUV);
            SGM::Point2D Buv=pSurface->Inverse(B,nullptr,&GuessUV);
            SGM::Point2D Cuv=pSurface->Inverse(C,nullptr,&GuessUV);
            if(SGM_TWO_PI<Auv.m_u+SGM_MIN_TOL)
                {
                aTriangles[Index1]=(unsigned int)mSeamMap[a];
                }
            if(SGM_TWO_PI<Buv.m_u+SGM_MIN_TOL)
                {
                aTriangles[Index1+1]=(unsigned int)mSeamMap[b];
                }
            if(SGM_TWO_PI<Cuv.m_u+SGM_MIN_TOL)
                {
                aTriangles[Index1+2]=(unsigned int)mSeamMap[c];
                }
            }
        }

    // Add the two pole triangles.

    size_t nLowMid=0,nHighMid=0,nLowLeft=0,nLowRight=0,nHighLeft=0,nHighRight=0;
    double dMaxV=pSurface->GetDomain().m_VDomain.m_dMax;
    double dMinV=pSurface->GetDomain().m_VDomain.m_dMin;
    double dLowMid=dMaxV,dLowLeft=dMaxV,dLowRight=dMaxV;
    double dHighMid=dMinV,dHighLeft=dMinV,dHighRight=dMinV;
    nPoints=aPoints2D.size();
    for(Index1=0;Index1<nPoints;++Index1)
        {
        SGM::Point2D const &uv=aPoints2D[Index1];
        if(uv.m_u<SGM_MIN_TOL)
            {
            if(uv.m_v<dLowLeft)
                {
                dLowLeft=uv.m_v;
                nLowLeft=Index1;
                }
            if(dHighLeft<uv.m_v)
                {
                dHighLeft=uv.m_v;
                nHighLeft=Index1;
                }
            }
        else if(SGM_TWO_PI<uv.m_u+SGM_MIN_TOL)
            {
            if(uv.m_v<dLowRight)
                {
                dLowRight=uv.m_v;
                nLowRight=Index1;
                }
            if(dHighRight<uv.m_v)
                {
                dHighRight=uv.m_v;
                nHighRight=Index1;
                }
            }
        else if(SGM_PI-SGM_MIN_TOL<uv.m_u && uv.m_u<SGM_PI+SGM_MIN_TOL)
            {
            if(uv.m_v<dLowMid)
                {
                dLowMid=uv.m_v;
                nLowMid=Index1;
                }
            if(dHighMid<uv.m_v)
                {
                dHighMid=uv.m_v;
                nHighMid=Index1;
                }
            }
        }
    aTriangles.push_back((unsigned int)nLowRight);
    aTriangles.push_back((unsigned int)nLowMid);
    aTriangles.push_back((unsigned int)nLowLeft);
    aTriangles.push_back((unsigned int)nHighRight);
    aTriangles.push_back((unsigned int)nHighLeft);
    aTriangles.push_back((unsigned int)nHighMid);
    }

void FindTorusPoints(torus                    const *pSurface,
                     SGM::Interval2D          const &InBox,
                     FacetOptions             const &Options,
                     std::vector<SGM::Point3D>      &aPoints3D,
                     std::vector<unsigned int>      &aTriangles,
                     std::vector<SGM::Point2D>      &aPoints2D,
                     std::vector<SGM::UnitVector3D> &aNormals)
    {
    // Create the base grid.

    SGM::Interval2D Box=InBox;
    if(Box.IsEmpty())
        {
        Box=pSurface->GetDomain();
        }
    std::vector<double> aUValues,aVValues;
    size_t nU=(size_t)(Box.m_UDomain.Length()/Options.m_dFaceAngleTol+SGM_MIN_TOL)*2;
    size_t nV=(size_t)(Box.m_VDomain.Length()/Options.m_dFaceAngleTol+SGM_MIN_TOL)*2;
    if(nU<3)
        {
        nU=3;
        }
    if(nV<3)
        {
        nV=3;
        }
    if(10<nU)
        {
        nU=(size_t)(nU*1.5);
        }
    size_t Index1;
    for(Index1=0;Index1<=nU;++Index1)
        {
        aUValues.push_back(Box.m_UDomain.MidPoint(((double)Index1)/nU));
        }
    for(Index1=0;Index1<=nV;++Index1)
        {
        aVValues.push_back(Box.m_VDomain.MidPoint(((double)Index1)/nV));
        }
    SGM::CreateTrianglesFromGrid(aUValues,aVValues,aPoints2D,aTriangles);

    size_t nPoints2D=aPoints2D.size();
    aPoints3D.reserve(nPoints2D);
    aNormals.reserve(nPoints2D);
    for(Index1=0;Index1<nPoints2D;++Index1)
        {
        SGM::Point3D Pos;
        SGM::UnitVector3D Norm;
        SGM::Point2D const &uv=aPoints2D[Index1];
        pSurface->Evaluate(uv,&Pos,nullptr,nullptr,&Norm);
        aPoints3D.push_back(Pos);
        aNormals.push_back(Norm);
        }
    }

void DoubleValues(std::vector<double> &aValues)
    {
    std::vector<double> aTemp;
    size_t nValues=aValues.size();
    size_t Index1;
    for(Index1=1;Index1<nValues;++Index1)
        {
        aTemp.push_back(aValues[Index1-1]);
        aTemp.push_back((aValues[Index1-1]+aValues[Index1])*0.5);
        }
    aTemp.push_back(aValues.back());
    aValues=aTemp;
    }

void FindParamPoints(surface                  const *pSurface,
                     SGM::Interval2D          const &InBox,
                     FacetOptions             const &Options,
                     std::vector<SGM::Point3D>      &aPoints3D,
                     std::vector<unsigned int>      &aTriangles,
                     std::vector<SGM::Point2D>      &aPoints2D,
                     std::vector<SGM::UnitVector3D> &aNormals)
    {
    // Create the base grid.

    std::vector<double> aUValues,aVValues;
    SGM::Interval2D Box=InBox;
    if(Box.IsEmpty())
        {
        Box=pSurface->GetDomain();
        }
    double dMidU=Box.m_UDomain.MidPoint();
    double dMidV=Box.m_VDomain.MidPoint();
    SGM::Result EmptyResult(nullptr);
    std::shared_ptr<curve> pUParam(pSurface->UParamLine(EmptyResult,dMidU));
    std::shared_ptr<curve> pVParam(pSurface->VParamLine(EmptyResult,dMidV));
    FacetOptions TempOptions;
    TempOptions.m_dEdgeAngleTol=Options.m_dFaceAngleTol;
    if( pSurface->GetSurfaceType()==SGM::NUBSurfaceType ||
        pSurface->GetSurfaceType()==SGM::NURBSurfaceType)
        {
        TempOptions.m_dEdgeAngleTol=Options.m_dEdgeAngleTol;
        }
    std::vector<SGM::Point3D> aTempPoints3D;
    FacetCurve(pVParam.get(),Box.m_UDomain,TempOptions,aTempPoints3D,aUValues);
    if(pSurface->GetSurfaceType()==SGM::RevolveType)
        {
        TempOptions.m_dEdgeAngleTol=Options.m_dEdgeAngleTol;
        }
    aTempPoints3D.clear();
    FacetCurve(pUParam.get(),Box.m_VDomain,TempOptions,aTempPoints3D,aVValues);
    size_t Index1;

    // Expand U and V values so that they are not hit by bondary curves.
    if(!pSurface->ClosedInU())
        {
        double dLength=Box.m_UDomain.Length()/aUValues.size();
        aUValues[0]-=dLength;
        aUValues[aUValues.size()-1]+=dLength;
        }
    if(!pSurface->ClosedInV())
        {
        double dLength=Box.m_VDomain.Length()/aVValues.size();
        aVValues[0]-=dLength;
        aVValues[aVValues.size()-1]+=dLength;
        }

    if(aUValues.size()<5)
        {
        double dVal0=aUValues.front();
        double dVal1=aUValues.back();
        aUValues.clear();
        aUValues.push_back(dVal0);
        aUValues.push_back(dVal1*0.2+dVal0*0.8);
        aUValues.push_back(dVal1*0.4+dVal0*0.6);
        aUValues.push_back(dVal1*0.6+dVal0*0.4);
        aUValues.push_back(dVal1*0.8+dVal0*0.6);
        aUValues.push_back(dVal1);
        }

    if(aVValues.size()<5)
        {
        double dVal0=aVValues.front();
        double dVal1=aVValues.back();
        aVValues.clear();
        aVValues.push_back(dVal0);
        aVValues.push_back(dVal1*0.2+dVal0*0.8);
        aVValues.push_back(dVal1*0.4+dVal0*0.6);
        aVValues.push_back(dVal1*0.6+dVal0*0.4);
        aVValues.push_back(dVal1*0.8+dVal0*0.6);
        aVValues.push_back(dVal1);
        }

    //DoubleValues(aVValues);
    //DoubleValues(aUValues);

    SGM::CreateTrianglesFromGrid(aUValues,aVValues,aPoints2D,aTriangles);

    size_t nPoints2D=aPoints2D.size();
    aPoints3D.reserve(nPoints2D);
    aNormals.reserve(nPoints2D);
    for(Index1=0;Index1<nPoints2D;++Index1)
        {
        SGM::Point3D Pos;
        SGM::UnitVector3D Norm;
        SGM::Point2D const &uv=aPoints2D[Index1];
        pSurface->Evaluate(uv,&Pos,nullptr,nullptr,&Norm);
        aPoints3D.push_back(Pos);
        aNormals.push_back(Norm);
        }
    }

void FindDistances(std::vector<SGM::Point2D> const &aPoints2D,
                   std::vector<unsigned int> const &aTriangles,
                   std::vector<double>             &aDistances)
    {
    aDistances.assign(aPoints2D.size(),SGM_MAX);
    size_t nTriangles=aTriangles.size();
    size_t Index1;
    for(Index1=0;Index1<nTriangles;Index1+=3)
        {
        unsigned a=aTriangles[Index1];
        unsigned b=aTriangles[Index1+1];
        unsigned c=aTriangles[Index1+2];
        SGM::Point2D const &A=aPoints2D[a];
        SGM::Point2D const &B=aPoints2D[b];
        SGM::Point2D const &C=aPoints2D[c];
        double dDistAB=A.Distance(B);
        double dDistBC=B.Distance(C);
        double dDistCA=C.Distance(A);
        if(dDistAB<aDistances[b])
            {
            aDistances[b]=dDistAB;
            }
        if(dDistAB<aDistances[a])
            {
            aDistances[a]=dDistAB;
            }
        if(dDistBC<aDistances[b])
            {
            aDistances[b]=dDistBC;
            }
        if(dDistBC<aDistances[c])
            {
            aDistances[c]=dDistBC;
            }
        if(dDistCA<aDistances[a])
            {
            aDistances[a]=dDistCA;
            }
        if(dDistCA<aDistances[c])
            {
            aDistances[c]=dDistCA;
            }
        }
    }

//void RefineParametrically(surface             const *pTorus,
//                          std::vector<SGM::Point3D> &aPoints3D,
//                          std::vector<unsigned>     &aTriangles,
//                          size_t                     nRefine)
//    {
//    size_t nPoints=aPoints3D.size();
//    size_t nTriangles=aTriangles.size();
//    aTriangles.reserve(nTriangles*4);
//    aPoints3D.reserve(nTriangles+2*nPoints-2);
//    std::map<std::pair<unsigned,unsigned>,unsigned> aMap;
//    size_t Index1;
//    for(Index1=0;Index1<nTriangles;Index1+=3)
//        {
//        unsigned a=aTriangles[Index1];
//        unsigned b=aTriangles[Index1+1];
//        unsigned c=aTriangles[Index1+2];
//        SGM::Point3D const &A=aPoints3D[a];
//        SGM::Point3D const &B=aPoints3D[b];
//        SGM::Point3D const &C=aPoints3D[c];
//
//        auto ABIter=aMap.find({a,b});
//        unsigned ab;
//        if(ABIter==aMap.end())
//            {
//            SGM::Point3D AB=SGM::MidPoint(A,B);
//            SGM::UnitVector3D UVAB=AB-Center;
//            AB=Center+UVAB*dRadius;
//            ab=(unsigned)aPoints.size();
//            aPoints.push_back(AB);
//            aMap[{b,a}]=ab;
//            }
//        else
//            {
//            ab=ABIter->second;
//            }
//
//        auto BCIter=aMap.find({b,c});
//        unsigned bc;
//        if(BCIter==aMap.end())
//            {
//            SGM::Point3D BC=SGM::MidPoint(B,C);
//            SGM::UnitVector3D UVBC=BC-Center;
//            BC=Center+UVBC*dRadius;
//            bc=(unsigned)aPoints.size();
//            aPoints.push_back(BC);
//            aMap[{c,b}]=bc;
//            }
//        else
//            {
//            bc=BCIter->second;
//            }
//
//        auto CAIter=aMap.find({c,a});
//        unsigned ca;
//        if(CAIter==aMap.end())
//            {
//            SGM::Point3D CA=SGM::MidPoint(C,A);
//            SGM::UnitVector3D UVCA=CA-Center;
//            CA=Center+UVCA*dRadius;
//            ca=(unsigned)aPoints.size();
//            aPoints.push_back(CA);
//            aMap[{a,c}]=ca;
//            }
//        else
//            {
//            ca=CAIter->second;
//            }
//
//        aTriangles[Index1]=ab;
//        aTriangles[Index1+1]=bc;
//        aTriangles[Index1+2]=ca;
//        aTriangles.push_back(a);
//        aTriangles.push_back(ab);
//        aTriangles.push_back(ca);
//        aTriangles.push_back(b);
//        aTriangles.push_back(bc);
//        aTriangles.push_back(ab);
//        aTriangles.push_back(c);
//        aTriangles.push_back(ca);
//        aTriangles.push_back(bc);
//        }
//    }

void FacetFace(SGM::Result                    &rResult,
               face                     const *pFace,
               FacetOptions             const &Options,
               std::vector<SGM::Point2D>      &aPoints2D,
               std::vector<SGM::Point3D>      &aPoints3D,
               std::vector<SGM::UnitVector3D> &aNormals,
               std::vector<unsigned int>      &aTriangles)
    {
    // Code added for code coverage.

    if(rResult.GetDebugFlag())
        {
        if(rResult.GetDebugFlag()==1)
            {
            aPoints2D.reserve(6);
            aPoints2D.emplace_back(0,0);
            aPoints2D.emplace_back(1,0);
            aPoints2D.emplace_back(0,1);
            aPoints2D.emplace_back(0,0);
            aPoints2D.emplace_back(1,0);
            aPoints2D.emplace_back(0,1);

            aPoints3D.reserve(6);
            aPoints3D.emplace_back(0,0,0);
            aPoints3D.emplace_back(1,0,0);
            aPoints3D.emplace_back(101,-100,100);
            aPoints3D.emplace_back(4,0,0);
            aPoints3D.emplace_back(2,0,0);
            aPoints3D.emplace_back(100,-100,100);

            aNormals.emplace_back(1,2,3);
            aNormals.emplace_back(1,2,3);

            aTriangles.reserve(6);
            aTriangles.push_back(0);
            aTriangles.push_back(1);
            aTriangles.push_back(2);
            aTriangles.push_back(3);
            aTriangles.push_back(4);
            aTriangles.push_back(5);
            return;
            }
        else if(rResult.GetDebugFlag()==2)
            {
            return;
            }
        }

    // Start of main code.
    
    // Added to make sure that UV boundaries are cached.
    std::set<edge *,EntityCompare> const &sEdges=pFace->GetEdges();
    for(edge *pEdge : sEdges)
        {
        ((face *)pFace)->SetUVBoundary(rResult,pEdge);
        }
    
    std::vector<unsigned> aAdjacencies;
    std::vector<std::vector<unsigned> > aaPolygons;
    std::vector<bool> aImprintFlags;
    if(!FacetFaceLoops(rResult,pFace,aPoints2D,aPoints3D,aaPolygons,nullptr,&aImprintFlags))
        {
        return;
        }

    // Check for a bounded face.

    SGM::Interval2D Bound(aPoints2D);
    if(Bound.IsBounded()==false)
        {
        // Attempt to repair the face.
        // Look for a seam edge that can be flipped.

        for(auto *pEdge : pFace->GetEdges())
            {
            SGM::EdgeSeamType nSeamType=pFace->GetSeamType(pEdge);
            if(nSeamType!=SGM::NotASeamType)
                {
                if(nSeamType==SGM::UpperUSeamType)
                    {
                    ((face *)pFace)->SetSeamType(pEdge,SGM::LowerUSeamType);
                    ((face *)pFace)->ClearUVBoundary(pEdge);
                    aAdjacencies.clear();
                    aaPolygons.clear();
                    aImprintFlags.clear();
                    aPoints2D.clear();
                    aPoints3D.clear();
                    if(!FacetFaceLoops(rResult,pFace,aPoints2D,aPoints3D,aaPolygons,nullptr,&aImprintFlags))
                        {
                        return;
                        }
                    break;
                    }
                }
            }
        }

    auto pSurface =pFace->GetSurface();
    if (!pSurface)
        {
        rResult.SetResult(SGM::ResultType::ResultTypeInconsistentData);
        rResult.SetMessage(std::string("No surface on face ID ") + std::to_string(pFace->GetID()));
        return;
        }

    switch(pSurface->GetSurfaceType())
        {
        case SGM::PlaneType:
            {
            // No scaling.
            // No refining.

            SGM::TriangulatePolygonWithHoles(rResult,aPoints2D,aaPolygons,aTriangles,aAdjacencies,pFace->HasBranchedVertex());
            aPoints3D.clear();
            aNormals.clear();
            FindNormalsAndPoints(pFace,aPoints2D,aNormals,aPoints3D);
            break;
            }
        case SGM::ConeType:
        case SGM::CylinderType:
        case SGM::ExtrudeType:
            {
            // Scaling is needed.
            // No refining.

            SGM::TriangulatePolygonWithHoles(rResult,aPoints2D,aaPolygons,aTriangles,aAdjacencies,pFace->HasBranchedVertex());
            FindNormals(pFace,aPoints2D,aNormals);
            std::vector<SGM::Point2D> aScaled;
            ScaledUVs(pFace,aPoints2D,aScaled);
            DelaunayFlips(aScaled,aTriangles,aAdjacencies);
            break;
            }
        case SGM::SphereType:
            {
            auto const *pSphere=(sphere const *)(pFace->GetSurface());
            std::vector<SGM::Point2D> aPolygonPoints=aPoints2D;
            aPoints2D.clear();
            SGM::CreateOctahedron(pSphere->m_dRadius,
                                  pSphere->m_Center,pSphere->m_ZAxis,pSphere->m_XAxis,
                                  aPoints3D,aTriangles,4);
            FindSpherePoints(pSphere,aPoints3D,aTriangles,aPoints2D,aNormals);

            if(aaPolygons.size())
                {
                std::vector<double> aDistances;
                FindDistances(aPoints2D,aTriangles,aDistances);
                size_t nDistances=aDistances.size();
                size_t Index1;
                for(Index1=0;Index1<nDistances;++Index1)
                    {
                    aDistances[Index1]*=0.5;
                    }
                std::vector<unsigned int> aRemovePoints;
                FindPointsToRemove(aPolygonPoints,aaPolygons,aImprintFlags,aPoints2D,aDistances,pSphere,aTriangles,aRemovePoints);

                for(auto nWhere : aRemovePoints)
                    {
                    std::vector<unsigned> aRemovedOrChanged,aReplacedTriangles;
                    SGM::RemovePointFromTriangles(rResult,nWhere,aPoints2D,aTriangles,aRemovedOrChanged,aReplacedTriangles);
                    }

                aPoints3D.clear();
                aNormals.clear();
                FindNormalsAndPoints(pFace,aPoints2D,aNormals,aPoints3D);

                SGM::Surface SurfID(pFace->GetSurface()->GetID());
                size_t nPolygons=aaPolygons.size();
                for(Index1=0;Index1<nPolygons;++Index1)
                    {
                    std::vector<bool> aFlags=FindPolygonImprintFlags(aImprintFlags,aaPolygons[Index1]);
                    std::vector<unsigned> aPolygonIndices;
                    SGM::InsertPolygon(rResult,SGM::PointsFromPolygon2D(aPolygonPoints,aaPolygons[Index1]),
                                       aPoints2D,aTriangles,aPolygonIndices,&SurfID,&aPoints3D,&aNormals,&aFlags);
                    aaPolygons[Index1]=aPolygonIndices;
                    }
                RemoveOutsideTriangles(rResult,aaPolygons,aPoints2D,aTriangles,0,&aPoints3D,&aNormals);
                }

            break;
            }
        case SGM::TorusType:
            {
            std::vector<SGM::Point2D> aOldPoints2D=aPoints2D;
            std::vector<SGM::Point3D> aOldPoints3D=aPoints3D;
            std::vector<std::vector<unsigned> > aaOldPoly=aaPolygons;

            SGM::Interval2D Box(aPoints2D);
            double dU=Box.m_UDomain.Length();
            double dV=Box.m_VDomain.Length();
            if(aPoints2D.size()>1 && (dU<0.1 || dV<0.1))
                {
                SGM::TriangulatePolygonWithHoles(rResult,aPoints2D,aaPolygons,aTriangles,aAdjacencies,pFace->HasBranchedVertex());
                aPoints3D.clear();
                aNormals.clear();
                FindNormalsAndPoints(pFace,aPoints2D,aNormals,aPoints3D);
                break;
                }
            auto const *pTorus=(torus const *)(pFace->GetSurface());
            std::vector<SGM::Point2D> aPolygonPoints=aPoints2D;
            aPoints2D.clear();
            FindTorusPoints(pTorus,Box,Options,aPoints3D,aTriangles,aPoints2D,aNormals);

            //SGM::Point3D Pole;
            //pTorus->Evaluate(pTorus->GetDomain().MidPoint(0,0),&Pole);
            //double dHight=Pole.Distance(pTorus->m_Center);
            //SGM::SquareBipyramid(pTorus->m_dMajorRadius+pTorus->m_dMinorRadius,dHight,
            //                      pTorus->m_Center,pTorus->m_ZAxis,pTorus->m_XAxis,
            //                      aPoints3D,aTriangles);
            //FindSpherePoints(pTorus,aPoints3D,aTriangles,aPoints2D,aNormals);
            //if(pFace)
            //    {
            //    return;
            //    }

            if(aaPolygons.size())
                {
                std::vector<double> aDistances;
                FindDistances(aPoints2D,aTriangles,aDistances);
                size_t nDistances=aDistances.size();
                size_t Index1;
                for(Index1=0;Index1<nDistances;++Index1)
                    {
                    aDistances[Index1]*=0.5;
                    }
                std::vector<unsigned int> aRemovePoints;
                FindPointsToRemove(aPolygonPoints,aaPolygons,aImprintFlags,aPoints2D,aDistances,pTorus,aTriangles,aRemovePoints);

                for(auto nWhere : aRemovePoints)
                    {
                    std::vector<unsigned> aRemovedOrChanged,aReplacedTriangles;
                    SGM::RemovePointFromTriangles(rResult,nWhere,aPoints2D,aTriangles,aRemovedOrChanged,aReplacedTriangles);
                    }
                aPoints3D.clear();
                aNormals.clear();
                FindNormalsAndPoints(pFace,aPoints2D,aNormals,aPoints3D);

                SGM::Surface SurfID(pFace->GetSurface()->GetID());
                size_t nPolygons=aaPolygons.size();
                bool bCouldNotImprint=false;
                for(Index1=0;Index1<nPolygons;++Index1)
                    {
                    std::vector<unsigned> aPolygonIndices;
                    std::vector<bool> aFlags=FindPolygonImprintFlags(aImprintFlags,aaPolygons[Index1]);
                    if(SGM::InsertPolygon(rResult,SGM::PointsFromPolygon2D(aPolygonPoints,aaPolygons[Index1]),
                                       aPoints2D,aTriangles,aPolygonIndices,&SurfID,&aPoints3D,&aNormals,&aFlags)==false)
                        {
                        bCouldNotImprint=true;
                        }
                    aaPolygons[Index1]=aPolygonIndices;
                    }
                if(bCouldNotImprint)
                    {
                    aTriangles.clear();
                    aAdjacencies.clear();
                    aPoints2D=aOldPoints2D;
                    aPoints3D=aOldPoints3D;
                    SGM::TriangulatePolygonWithHoles(rResult,aPoints2D,aaOldPoly,aTriangles,aAdjacencies,pFace->HasBranchedVertex());
                    aNormals.clear();
                    FindNormalsAndPoints(pFace,aPoints2D,aNormals,aPoints3D);
                    RefineTriangles(aPoints2D,aPoints3D,aNormals,aTriangles,pSurface); 
                    }
                else
                    {
                    RemoveOutsideTriangles(rResult,aaPolygons,aPoints2D,aTriangles,0,&aPoints3D,&aNormals);
                    }
                }

            break;
            }
        case SGM::RevolveType:
        case SGM::OffsetType:
        case SGM::NUBSurfaceType:
        case SGM::NURBSurfaceType:
            {
            // Find the number UV spread.

            std::vector<SGM::Point2D> aOldPoints2D=aPoints2D;
            std::vector<SGM::Point3D> aOldPoints3D=aPoints3D;
            std::vector<std::vector<unsigned> > aaOldPoly=aaPolygons;
            std::vector<SGM::Point2D> aPolygonPoints=aPoints2D;
            SGM::Interval2D Box(aPolygonPoints);
            aPoints2D.clear();
            aPoints3D.clear();
            FindParamPoints(pSurface,Box,Options,aPoints3D,aTriangles,aPoints2D,aNormals);

            if(aaPolygons.size())
                {
                std::vector<double> aDistances;
                FindDistances(aPoints2D,aTriangles,aDistances);
                size_t nDistances=aDistances.size();
                size_t Index1;
                for(Index1=0;Index1<nDistances;++Index1)
                    {
                    aDistances[Index1]*=0.5;
                    }
                std::vector<unsigned int> aRemovePoints;
                FindPointsToRemove(aPolygonPoints,aaPolygons,aImprintFlags,aPoints2D,aDistances,pFace->GetSurface(),aTriangles,aRemovePoints);

                for(auto nWhere : aRemovePoints)
                    {
                    std::vector<unsigned> aRemovedOrChanged,aReplacedTriangles;
                    SGM::RemovePointFromTriangles(rResult,nWhere,aPoints2D,aTriangles,aRemovedOrChanged,aReplacedTriangles);
                    }
                aPoints3D.clear();
                aNormals.clear();
                FindNormalsAndPoints(pFace,aPoints2D,aNormals,aPoints3D);

                SGM::Surface SurfID(pFace->GetSurface()->GetID());
                size_t nPolygons=aaPolygons.size();
                bool bCouldNotImprint=false;
                for(Index1=0;Index1<nPolygons;++Index1)
                    {
                    std::vector<unsigned> aPolygonIndices;
                    std::vector<bool> aFlags=FindPolygonImprintFlags(aImprintFlags,aaPolygons[Index1]);
                    if(SGM::InsertPolygon(rResult,SGM::PointsFromPolygon2D(aPolygonPoints,aaPolygons[Index1]),
                                       aPoints2D,aTriangles,aPolygonIndices,&SurfID,&aPoints3D,&aNormals,&aFlags)==false)
                        {
                        bCouldNotImprint=true;
                        break;
                        }
                    aaPolygons[Index1]=aPolygonIndices;
                    }
                if(bCouldNotImprint)
                    {
                    aTriangles.clear();
                    aAdjacencies.clear();
                    aPoints2D=aOldPoints2D;
                    aPoints3D=aOldPoints3D;
                    SGM::TriangulatePolygonWithHoles(rResult,aPoints2D,aaOldPoly,aTriangles,aAdjacencies,pFace->HasBranchedVertex());
                    aNormals.clear();
                    FindNormalsAndPoints(pFace,aPoints2D,aNormals,aPoints3D);
                    RefineTriangles(aPoints2D,aPoints3D,aNormals,aTriangles,pSurface); 
                    }
                else
                    {
                    RemoveOutsideTriangles(rResult,aaPolygons,aPoints2D,aTriangles,0,&aPoints3D,&aNormals);
                    size_t nTriangles=aTriangles.size();
                    bool bBadFacets=false;
                    for(Index1=0;Index1<nTriangles;Index1+=3)
                        {
                        if(CheckFacet(aPoints3D,aNormals,aTriangles,pSurface,Index1))
                            {
                            bBadFacets=true;
                            break;
                            }
                        }
                    if(bBadFacets)
                        {
                        aTriangles.clear();
                        aAdjacencies.clear();
                        aPoints2D=aOldPoints2D;
                        aPoints3D=aOldPoints3D;
                        SGM::TriangulatePolygonWithHoles(rResult,aPoints2D,aaOldPoly,aTriangles,aAdjacencies,pFace->HasBranchedVertex());
                        aNormals.clear();
                        FindNormalsAndPoints(pFace,aPoints2D,aNormals,aPoints3D);
                        RefineTriangles(aPoints2D,aPoints3D,aNormals,aTriangles,pSurface); 
                        }
                    }
                }

            break;
            }
        default:
            {
            // New surface needs to be considered.
            throw;
            }
        }
    pFace->GetUVBox(rResult);
    }

} // End of SGMInternal namespace