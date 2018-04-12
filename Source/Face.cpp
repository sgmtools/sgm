#include "SGMMathematics.h"
#include "SGMDataClasses.h"
#include "EntityClasses.h"
#include "Faceter.h"
#include "Graph.h"
#include "Topology.h"
#include "Query.h"
#include <cfloat>

///////////////////////////////////////////////////////////////////////////////
//
//  face methods
//
///////////////////////////////////////////////////////////////////////////////

face::face(SGM::Result &rResult):topology(rResult,SGM::EntityType::FaceType),
    m_pVolume(NULL),m_pSurface(NULL),m_bFlipped(false),m_nSides(1)
    {
    }

volume *face::GetVolume() const 
    {
    if(this)
        {
        return m_pVolume;
        }
    else
        {
        return NULL;
        }
    }

std::vector<SGM::Point2D> const &face::GetPoints2D(SGM::Result &rResult) const
    {
    if(m_aPoints2D.empty())
        {
        FacetOptions Options;
        FacetFace(rResult,this,Options,m_aPoints2D,m_aPoints3D,m_aNormals,m_aTriangles,m_aEntities);
        }
    return m_aPoints2D;
    }

std::vector<SGM::Point3D> const &face::GetPoints3D(SGM::Result &rResult) const
    {
    if(m_aPoints2D.empty())
        {
        FacetOptions Options;
        FacetFace(rResult,this,Options,m_aPoints2D,m_aPoints3D,m_aNormals,m_aTriangles,m_aEntities);
        }
    return m_aPoints3D;
    }

std::vector<size_t> const &face::GetTriangles(SGM::Result &rResult) const
    {
    if(m_aPoints2D.empty())
        {
        FacetOptions Options;
        FacetFace(rResult,this,Options,m_aPoints2D,m_aPoints3D,m_aNormals,m_aTriangles,m_aEntities);
        }
    return m_aTriangles;
    }

bool face::PointInFace(SGM::Result        &rResult,
                       SGM::Point2D const &uv,
                       SGM::Point3D       *ClosePos,    // The closest point.
                       entity            **pBoundary,   // The closest sub element.
                       SGM::Point3D       *pPos) const  // Found point on boundary.
    {
    // First check for the closed face case.

    if(m_sEdges.empty())
        {
        return true;
        }

    // Find the closest edge or vertex.

    SGM::Point3D Pos,CPos,TPos;
    entity *pEntity= (entity *)this;  //TODO: Why are casting away const'ness on "this" face*?
    double dDist=DBL_MAX;
    if(ClosePos)
        {
        Pos=*ClosePos;
        }
    else
        {
        m_pSurface->Evaluate(uv,&Pos);
        }
    std::set<edge *>::iterator iter=m_sEdges.begin();
    while(iter!=m_sEdges.end())
        {
        edge *pEdge=*iter;
        entity *pTempEnt=nullptr;
        FindClosestPointOnEdge(rResult,Pos,pEdge,TPos,pTempEnt);
        double dTempDist=Pos.DistanceSquared(TPos);
        if(dTempDist<dDist)
            {
            dDist=dTempDist;
            pEntity=pTempEnt;
            CPos=TPos;
            }
        ++iter;
        }

    // Find the parameter line to check sidedness from.

    if(pBoundary)
        {
        *pBoundary=pEntity;
        }
    SGM::Point2D Buv=m_pSurface->Inverse(CPos);
    if(pEntity->GetType()==SGM::EntityType::EdgeType)  //TODO: How can face pointer (pEntity=this) now be an EdgeType?
        {
        SGM::Vector3D VecU,VecV,Vec;
        m_pSurface->Evaluate(Buv,NULL,&VecU,&VecV);
        edge *pEdge=static_cast<edge *>(pEntity);
        curve const *pCurve=pEdge->GetCurve();
        double t=pCurve->Inverse(CPos);
        if(pPos)
            {
            *pPos=CPos;
            }
        pCurve->Evaluate(t,NULL,&Vec);
        SGM::Vector2D VecUV(Vec%VecU,Vec%VecV);
        SGM::Vector2D TestVec=uv-Buv;
        double dZ=VecUV.m_u*TestVec.m_v-VecUV.m_v*TestVec.m_u;
        if(-SGM_ZERO<=dZ)
            {
            return true;
            }
        else 
            {
            return false;
            }
        }
    else // The vertex case.
        {
        SGM::Point3D const &VertexPos=(static_cast<vertex *>(pEntity))->GetPoint();
        if(pPos)
            {
            *pPos=VertexPos;
            }
        if(SGM::NearEqual(VertexPos,Pos,SGM_ZERO))
            {
            return true;
            }
        else
            {
            // Have to find inside out side from one of the edges 
            // or the angle of the edges.
            throw;
            }
        return false;
        }
    }

size_t face::FindLoops(SGM::Result                                  &rResult,
                       std::vector<std::vector<edge *> >            &aaLoops,
                       std::vector<std::vector<SGM::EdgeSideType> > &aaFlipped) const
    {
    thing *pThing=rResult.GetThing();
    Graph graph(rResult,m_sEdges);
    std::vector<Graph> aComponents;
    size_t nLoops=graph.FindComponents(aComponents);
    aaLoops.reserve(nLoops);
    aaFlipped.reserve(nLoops);
    size_t Index1;
    for(Index1=0;Index1<nLoops;++Index1)
        {
        Graph const &comp=aComponents[Index1];
        std::set<GraphEdge> const &sGEdges=comp.GetEdges();
        std::set<edge *> sEdges;
        std::set<GraphEdge>::const_iterator GEdgeIter=sGEdges.begin();
        while(GEdgeIter!=sGEdges.end())
            {
            size_t ID=GEdgeIter->m_nID;
            sEdges.insert((edge *)pThing->FindEntity(ID));
            ++GEdgeIter;
            }
        std::vector<edge *> aEdges;
        std::vector<SGM::EdgeSideType> aFlips;
        OrderLoopEdges(rResult,this,sEdges,aEdges,aFlips);
        aaLoops.push_back(aEdges);
        aaFlipped.push_back(aFlips);
        }
    return aaLoops.size();
    }

void face::AddEdge(edge *pEdge,SGM::EdgeSideType nEdgeType)
    {
    m_sEdges.insert(pEdge);
    pEdge->AddFace(this);
    m_mFaceType[pEdge]=nEdgeType;
    }

void face::SetSurface(surface *pSurface)
    {
    if(m_pSurface)
        {
        m_pSurface->RemoveFace(this);
        }
    m_pSurface=pSurface;
    pSurface->AddFace(this);
    }

SGM::EdgeSideType face::GetEdgeType(edge const *pEdge) const 
    {
    std::map<edge *,SGM::EdgeSideType>::const_iterator iter=m_mFaceType.find((edge *)pEdge);
    return iter->second;
    }

bool face::GetFlipped() const
    {
    return m_bFlipped;
    }

