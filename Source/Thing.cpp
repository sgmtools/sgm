#include "EntityClasses.h"
#include "Surface.h"
#include "Curve.h"
#include <algorithm>
#include <cstdlib>

namespace SGMInternal
{
size_t entity::GetID() const
    {
    return m_ID;
    }

///////////////////////////////////////////////////////////////////////////////
//
//  thing methods
//
///////////////////////////////////////////////////////////////////////////////

thing::~thing()
    {
    while (!m_mAllEntities.empty()) {
      auto pEntity = m_mAllEntities.begin()->second;
      DeleteEntity(pEntity);
    }
    }

void thing::AddToMap(size_t nID,entity *pEntity)
    {
    m_mAllEntities[nID] = pEntity;
    }

void thing::DeleteEntity(entity *pEntity)
    {
    m_mAllEntities.erase(pEntity->GetID());
    switch(pEntity->GetType()) {
      case SGM::BodyType:
            delete reinterpret_cast<body*>(pEntity);
            break;
          case SGM::ComplexType:
            delete reinterpret_cast<complex*>(pEntity);
            break;
          case SGM::VolumeType:
            delete reinterpret_cast<volume*>(pEntity);
            break;
          case SGM::FaceType:
            delete reinterpret_cast<face*>(pEntity);
            break;
          case SGM::EdgeType:
            delete reinterpret_cast<edge*>(pEntity);
            break;
          case SGM::VertexType:
            delete reinterpret_cast<vertex*>(pEntity);
            break;
          case SGM::SurfaceType: {
            surface* pSurface = reinterpret_cast<surface*>(pEntity);
            switch (pSurface->GetSurfaceType()) {
              case SGM::PlaneType:
                delete reinterpret_cast<plane*>(pEntity);
                break;
              case SGM::CylinderType:
                delete reinterpret_cast<cylinder*>(pEntity);
                break;
              case SGM::ConeType:
                delete reinterpret_cast<cone*>(pEntity);
                break;
              case SGM::SphereType:
                delete reinterpret_cast<sphere*>(pEntity);
                break;
              case SGM::TorusType:
                delete reinterpret_cast<torus*>(pEntity);
                break;
              case SGM::NUBSurfaceType:
                delete reinterpret_cast<NUBsurface*>(pEntity);
                break;
              case SGM::NURBSurfaceType:
                delete reinterpret_cast<NUBsurface*>(pEntity);
                break;
              default:
                std::abort();
            }
            break;
          }
          case SGM::CurveType: {
            curve* pCurve = reinterpret_cast<curve*>(pEntity);
            switch(pCurve->GetCurveType()) {
              case SGM::LineType:
                delete reinterpret_cast<line*>(pEntity);
                break;
              case SGM::CircleType:
                delete reinterpret_cast<circle*>(pEntity);
                break;
              case SGM::EllipseType:
                delete reinterpret_cast<ellipse*>(pEntity);
                break;
              case SGM::NUBCurveType:
                delete reinterpret_cast<NUBcurve*>(pEntity);
                break;
              case SGM::NURBCurveType:
                delete reinterpret_cast<NURBcurve*>(pEntity);
                break;
              case SGM::PointCurveType:
                delete reinterpret_cast<PointCurve*>(pEntity);
                break;
              default:
                std::abort();
            }
            break;
          }
          default:
            std::abort();
        }
    }

entity *thing::FindEntity(size_t ID) const
    {
    if (ID == 0) return const_cast<thing*>(this);
    entity *pAnswer=nullptr;
    auto iter=m_mAllEntities.find(ID);
    if(iter!=m_mAllEntities.end())
        {
        pAnswer=iter->second;
        }
    return pAnswer;
    }

size_t thing::GetBodies(std::set<body *> &sBodies,bool bTopLevel) const
    {
    std::map<size_t,entity* >::const_iterator iter=m_mAllEntities.begin();
    while(iter!=m_mAllEntities.end())
        {
        entity *pEntity=iter->second;
        if(pEntity->GetType()==SGM::EntityType::BodyType)
            {
            body *pBody=(body *)pEntity;
            if(bTopLevel)
                {
                if(pBody->IsTopLevel())
                    {
                    sBodies.insert(pBody);
                    }
                }
            else
                {
                sBodies.insert(pBody);
                }
            }
        ++iter;
        }
    return sBodies.size();
    }

size_t thing::GetVolumes(std::set<volume *> &sVolumes,bool bTopLevel) const
    {
    std::map<size_t,entity* >::const_iterator iter=m_mAllEntities.begin();
    while(iter!=m_mAllEntities.end())
        {
        entity *pEntity=iter->second;
        if(pEntity->GetType()==SGM::EntityType::VolumeType)
            {
            volume *pVolume=(volume *)pEntity;
            if(bTopLevel)
                {
                if(pVolume->IsTopLevel())
                    {
                    sVolumes.insert(pVolume);
                    }
                }
            else
                {
                sVolumes.insert(pVolume);
                }
            }
        ++iter;
        }
    return sVolumes.size();
    }

size_t thing::GetFaces(std::set<face *> &sFaces,bool bTopLevel) const
    {
    std::map<size_t,entity* >::const_iterator iter=m_mAllEntities.begin();
    while(iter!=m_mAllEntities.end())
        {
        entity *pEntity=iter->second;
        if(pEntity->GetType()==SGM::EntityType::FaceType)
            {
            face *pFace=(face *)pEntity;
            if(bTopLevel)
                {
                if(pFace->IsTopLevel())
                    {
                    sFaces.insert(pFace);
                    }
                }
            else
                {
                sFaces.insert(pFace);
                }
            }
        ++iter;
        }
    return sFaces.size();
    }

size_t thing::GetEdges(std::set<edge *> &sEdges,bool bTopLevel) const
    {
    std::map<size_t,entity* >::const_iterator iter=m_mAllEntities.begin();
    while(iter!=m_mAllEntities.end())
        {
        entity *pEntity=iter->second;
        if(pEntity->GetType()==SGM::EntityType::EdgeType)
            {
            edge *pEdge=(edge *)pEntity;
            if(bTopLevel)
                {
                if(pEdge->IsTopLevel())
                    {
                    sEdges.insert(pEdge);
                    }
                }
            else
                {
                sEdges.insert(pEdge);
                }
            }
        ++iter;
        }
    return sEdges.size();
    }

size_t thing::GetVertices(std::set<vertex *> &sVertices,bool bTopLevel) const
    {
    std::map<size_t,entity* >::const_iterator iter=m_mAllEntities.begin();
    while(iter!=m_mAllEntities.end())
        {
        entity *pEntity=iter->second;
        if(pEntity->GetType()==SGM::EntityType::EdgeType)
            {
            vertex *pVertex=(vertex *)pEntity;
            if(bTopLevel)
                {
                if(pVertex->IsTopLevel())
                    {
                    sVertices.insert(pVertex);
                    }
                }
            else
                {
                sVertices.insert(pVertex);
                }
            }
        ++iter;
        }
    return sVertices.size();
    }

size_t thing::GetSurfaces(std::set<surface *> &sSurfaces,bool bTopLevel) const
    {
    std::map<size_t,entity* >::const_iterator iter=m_mAllEntities.begin();
    while(iter!=m_mAllEntities.end())
        {
        entity *pEntity=iter->second;
        if(pEntity->GetType()==SGM::EntityType::SurfaceType)
            {
            surface *pSurface=(surface *)pEntity;
            if(bTopLevel)
                {
                if(pSurface->IsTopLevel())
                    {
                    sSurfaces.insert(pSurface);
                    }
                }
            else
                {
                sSurfaces.insert(pSurface);
                }
            }
        ++iter;
        }
    return sSurfaces.size();
    }

size_t thing::GetCurves(std::set<curve *> &sCurves,bool bTopLevel) const
    {
    std::map<size_t,entity* >::const_iterator iter=m_mAllEntities.begin();
    while(iter!=m_mAllEntities.end())
        {
        entity *pEntity=iter->second;
        if(pEntity->GetType()==SGM::EntityType::ComplexType)
            {
            curve *pCurve=(curve *)pEntity;
            if(bTopLevel)
                {
                if(pCurve->IsTopLevel())
                    {
                    sCurves.insert(pCurve);
                    }
                }
            else
                {
                sCurves.insert(pCurve);
                }
            }
        ++iter;
        }
    return sCurves.size();
    }

size_t thing::GetComplexes(std::set<complex *> &sComplexes,bool bTopLevel) const
    {
    std::map<size_t,entity* >::const_iterator iter=m_mAllEntities.begin();
    while(iter!=m_mAllEntities.end())
        {
        entity *pEntity=iter->second;
        if(pEntity->GetType()==SGM::EntityType::ComplexType)
            {
            complex *pComplex=(complex *)pEntity;
            if(bTopLevel)
                {
                if(pComplex->IsTopLevel())
                    {
                    sComplexes.insert(pComplex);
                    }
                }
            else
                {
                sComplexes.insert(pComplex);
                }
            }
        ++iter;
        }
    return sComplexes.size();
    }
}