#include "SGMVector.h"
#include "SGMTransform.h"

#include "EntityClasses.h"
#include "Curve.h"
#include "Surface.h"
#include "Intersectors.h"


#include <limits>
#include <algorithm>

namespace SGMInternal
{

void topology::TransformBox(SGM::Result& rResult, SGM::Transform3D const &transform3D)
{
    if (!transform3D.IsScaleAndTranslate())
        ResetBox(rResult);
    else
        m_Box *= transform3D;
}

void FindBodies(SGM::Result                    &,//rResult,
                entity                   const *pEntity,
                std::set<body *,EntityCompare> &sBodies,
                bool                            bTopLevel)
    {
    if(pEntity==nullptr)
        {
        return;
        }
    SGM::EntityType Type=pEntity->GetType();
    if(Type==SGM::EntityType::ThingType)
        {
        ((thing *)(pEntity))->GetBodies(sBodies,bTopLevel);
        }
    else if(Type==SGM::EntityType::BodyType)
        {
        sBodies.insert((body *)(pEntity));
        }
    else if(Type==SGM::EntityType::VolumeType)
        {
        if(body *pBody=((volume *)(pEntity))->GetBody())
            {
            sBodies.insert(pBody);
            }
        }
    else if(Type==SGM::EntityType::FaceType)
        {
        if(body *pBody=((face *)(pEntity))->GetVolume()->GetBody())
            {
            sBodies.insert(pBody);
            }
        }
    else if(Type==SGM::EntityType::EdgeType)
        {
        std::set<face *,EntityCompare> const &sFaces=((edge *)(pEntity))->GetFaces();
        if(!sFaces.empty())
            {
            face *pFace=*(sFaces.begin());
            sBodies.insert(pFace->GetVolume()->GetBody());
            }
        if(volume *pVolume=((edge *)(pEntity))->GetVolume())
            {
            sBodies.insert(pVolume->GetBody());
            }
        }
    else if(Type==SGM::EntityType::VertexType)
        {
        std::set<edge *,EntityCompare> const &sEdges=((vertex *)(pEntity))->GetEdges();
        if(!sEdges.empty())
            {
            edge *pEdge=*(sEdges.begin());
            std::set<face *,EntityCompare> const &sFaces=pEdge->GetFaces();
            if(!sFaces.empty())
                {
                face *pFace=*(sFaces.begin());
                sBodies.insert(pFace->GetVolume()->GetBody());
                }
            if(volume *pVolume=((edge *)(pEntity))->GetVolume())
                {
                sBodies.insert(pVolume->GetBody());
                }
            }
        }
    else if(Type==SGM::EntityType::SurfaceType)
        {
        std::set<face *,EntityCompare> const &sFaces=((surface *)(pEntity))->GetFaces();
        if(!sFaces.empty())
            {
            face *pFace=*(sFaces.begin());
            sBodies.insert(pFace->GetVolume()->GetBody());
            }
        }
    else if(Type==SGM::EntityType::CurveType)
        {
        std::set<edge *,EntityCompare> const &sEdges=((curve *)(pEntity))->GetEdges();
        if(!sEdges.empty())
            {
            edge *pEdge=*(sEdges.begin());
            std::set<face *,EntityCompare> const &sFaces=pEdge->GetFaces();
            if(!sFaces.empty())
                {
                face *pFace=*(sFaces.begin());
                sBodies.insert(pFace->GetVolume()->GetBody());
                }
            if(volume *pVolume=((edge *)(pEntity))->GetVolume())
                {
                sBodies.insert(pVolume->GetBody());
                }
            }
        }
    }

void FindVolumes(SGM::Result        &,//rResult,
                 entity       const *pEntity,
                 std::set<volume *,EntityCompare> &sVolumes,
                 bool                bTopLevel)
    {
    if(pEntity==nullptr)
        {
        return;
        }
    SGM::EntityType Type=pEntity->GetType();
    if(Type==SGM::EntityType::ThingType)
        {
        ((thing *)pEntity)->GetVolumes(sVolumes,bTopLevel);
        }
    else if(Type==SGM::EntityType::BodyType)
        {
        std::set<volume *,EntityCompare> const &sBodyVolumes=((body *)(pEntity))->GetVolumes();
        sVolumes.insert(sBodyVolumes.begin(),sBodyVolumes.end());
        }
    else if(Type==SGM::EntityType::VolumeType)
        {
        sVolumes.insert((volume *)(pEntity));
        }
    else if(Type==SGM::EntityType::FaceType)
        {
        sVolumes.insert(((face *)(pEntity))->GetVolume());
        }
    else if(Type==SGM::EntityType::EdgeType)
        {
        std::set<face *,EntityCompare> const &sFaces=((edge *)(pEntity))->GetFaces();
        if(!sFaces.empty())
            {
            face *pFace=*(sFaces.begin());
            sVolumes.insert(pFace->GetVolume());
            }
        if(volume *pVolume=((edge *)(pEntity))->GetVolume())
            {
            sVolumes.insert(pVolume);
            }
        }
    else if(Type==SGM::EntityType::VertexType)
        {
        std::set<edge *,EntityCompare> const &sEdges=((vertex *)(pEntity))->GetEdges();
        if(!sEdges.empty())
            {
            edge *pEdge=*(sEdges.begin());
            std::set<face *,EntityCompare> const &sFaces=pEdge->GetFaces();
            if(!sFaces.empty())
                {
                face *pFace=*(sFaces.begin());
                sVolumes.insert(pFace->GetVolume());
                }
            if(volume *pVolume=pEdge->GetVolume())
                {
                sVolumes.insert(pVolume);
                }
            }
        }
    else if(Type==SGM::EntityType::SurfaceType)
        {
        std::set<face *,EntityCompare> const &sFaces=((surface *)(pEntity))->GetFaces();
        if(!sFaces.empty())
            {
            face *pFace=*(sFaces.begin());
            sVolumes.insert(pFace->GetVolume());
            }
        }
    else if(Type==SGM::EntityType::CurveType)
        {
        std::set<edge *,EntityCompare> const &sEdges=((curve *)(pEntity))->GetEdges();
        if(!sEdges.empty())
            {
            edge *pEdge=*(sEdges.begin());
            std::set<face *,EntityCompare> const &sFaces=pEdge->GetFaces();
            if(!sFaces.empty())
                {
                face *pFace=*(sFaces.begin());
                sVolumes.insert(pFace->GetVolume());
                }
            if(volume *pVolume=pEdge->GetVolume())
                {
                sVolumes.insert(pVolume);
                }
            }
        }
    }

void FindAttributes(SGM::Result                         &,//rResult,
                    entity                        const *pEntity,
                    std::set<attribute *,EntityCompare> &sAttributes,
                    bool                                 bTopLevel)
    {
    if(pEntity==nullptr)
        {
        return;
        }
    SGM::EntityType Type=pEntity->GetType();
    if(Type==SGM::EntityType::ThingType)
        {
        ((thing *)pEntity)->GetAttributes(sAttributes,bTopLevel);
        }
    else
        {
        std::set<entity *,EntityCompare> const &sOwners=pEntity->GetOwners();
        auto iter=sOwners.begin();
        while(iter!=sOwners.end())
            {
            entity *pOwner=*iter;
            if(pOwner->GetType()==SGM::AttributeType)
                {
                sAttributes.insert((attribute *)pOwner);
                }
            ++iter;
            }
        }
    }

void FindFaces(SGM::Result                    &,//rResult,
               entity                   const *pEntity,
               std::set<face *,EntityCompare> &sFaces,
               bool                            bTopLevel)
    {
    if(pEntity==nullptr)
        {
        return;
        }
    SGM::EntityType Type=pEntity->GetType();
    if(Type==SGM::EntityType::ThingType)
        {
        ((thing *)pEntity)->GetFaces(sFaces,bTopLevel);
        }
    else if(Type==SGM::EntityType::BodyType)
        {
        std::set<volume *,EntityCompare> const &sBodyVolumes=((body *)(pEntity))->GetVolumes();
        std::set<volume *,EntityCompare>::const_iterator VolumeIter=sBodyVolumes.begin();
        while(VolumeIter!=sBodyVolumes.end())
            {
            volume *pVolume=*VolumeIter;
            std::set<face *,EntityCompare> const &sVolumeFaces=pVolume->GetFaces();
            sFaces.insert(sVolumeFaces.begin(),sVolumeFaces.end());
            ++VolumeIter;
            }
        }
    else if(Type==SGM::EntityType::VolumeType)
        {
        std::set<face *,EntityCompare> const &sVolumeFaces=((volume *)(pEntity))->GetFaces();
        sFaces.insert(sVolumeFaces.begin(),sVolumeFaces.end());
        }
    else if(Type==SGM::EntityType::FaceType)
        {
        sFaces.insert((face *)(pEntity));
        }
    else if(Type==SGM::EntityType::EdgeType)
        {
        std::set<face *,EntityCompare> const &sEdgeFaces=((edge *)(pEntity))->GetFaces();
        sFaces.insert(sEdgeFaces.begin(),sEdgeFaces.end());
        }
    else if(Type==SGM::EntityType::VertexType)
        {
        std::set<edge *,EntityCompare> const &sEdges=((vertex *)(pEntity))->GetEdges();
        for(auto pEdge : sEdges)
            {
            std::set<face *,EntityCompare> const &sVertexFaces=pEdge->GetFaces();
            sFaces.insert(sVertexFaces.begin(),sVertexFaces.end());
            }
        }
    else if(Type==SGM::EntityType::SurfaceType)
        {
        std::set<face *,EntityCompare> const &sSurfaceFaces=((surface *)(pEntity))->GetFaces();
        sFaces.insert(sSurfaceFaces.begin(),sSurfaceFaces.end());
        }
    else if(Type==SGM::EntityType::CurveType)
        {
        std::set<edge *,EntityCompare> const &sEdges=((curve *)(pEntity))->GetEdges();
        for(auto pEdge : sEdges)
            {
            std::set<face *,EntityCompare> const &sEdgeFaces=pEdge->GetFaces();
            sFaces.insert(sEdgeFaces.begin(),sEdgeFaces.end());
            }
        }
    }

void FindEdges(SGM::Result      &,//rResult,
               entity     const *pEntity,
               std::set<edge *,EntityCompare> &sEdges,
               bool              bTopLevel)
    {
    if(pEntity==nullptr)
        {
        return;
        }
    SGM::EntityType Type=pEntity->GetType();
    if(Type==SGM::EntityType::ThingType)
        {
        ((thing *)(pEntity))->GetEdges(sEdges,bTopLevel);
        }
    else if(Type==SGM::EntityType::BodyType)
        {
        std::set<volume *,EntityCompare> const &sBodyVolumes=((body *)(pEntity))->GetVolumes();
        std::set<volume *,EntityCompare>::const_iterator VolumeIter=sBodyVolumes.begin();
        while(VolumeIter!=sBodyVolumes.end())
            {
            volume *pVolume=*VolumeIter;
            std::set<face *,EntityCompare> const &sVolumeFaces=pVolume->GetFaces();
            std::set<face *,EntityCompare>::const_iterator FaceIter=sVolumeFaces.begin();
            while(FaceIter!=sVolumeFaces.end())
                {
                face *pFace=*FaceIter;
                std::set<edge *,EntityCompare> const &sFaceEdges=pFace->GetEdges();
                sEdges.insert(sFaceEdges.begin(),sFaceEdges.end());
                ++FaceIter;
                }
            std::set<edge *,EntityCompare> const &sVolumeEdges=pVolume->GetEdges();
            std::set<edge *,EntityCompare>::const_iterator EdgeIter=sVolumeEdges.begin();
            while(EdgeIter!=sVolumeEdges.end())
                {
                sEdges.insert(*EdgeIter);
                ++EdgeIter;
                }
            ++VolumeIter;
            }
        }
    else if(Type==SGM::EntityType::VolumeType)
        {
        std::set<face *,EntityCompare> const &sVolumeFaces=((volume *)(pEntity))->GetFaces();
        std::set<face *,EntityCompare>::const_iterator FaceIter=sVolumeFaces.begin();
        while(FaceIter!=sVolumeFaces.end())
            {
            face *pFace=*FaceIter;
            std::set<edge *,EntityCompare> const &sFaceEdges=pFace->GetEdges();
            sEdges.insert(sFaceEdges.begin(),sFaceEdges.end());
            ++FaceIter;
            }
        std::set<edge *,EntityCompare> const &sVolumeEdges=((volume *)(pEntity))->GetEdges();
        std::set<edge *,EntityCompare>::const_iterator EdgeIter=sVolumeEdges.begin();
        while(EdgeIter!=sVolumeEdges.end())
            {
            sEdges.insert(*EdgeIter);
            ++EdgeIter;
            }
        }
    else if(Type==SGM::EntityType::FaceType)
        {
        std::set<edge *,EntityCompare> const &sFaceEdges=((face *)(pEntity))->GetEdges();
        sEdges.insert(sFaceEdges.begin(),sFaceEdges.end());
        }
    else if(Type==SGM::EntityType::EdgeType)
        {
        sEdges.insert((edge *)pEntity);
        }
    else if(Type==SGM::EntityType::VertexType)
        {
        std::set<edge *,EntityCompare> const &sVertexEdges=((vertex *)(pEntity))->GetEdges();
        sEdges.insert(sVertexEdges.begin(),sVertexEdges.end());
        }
    else if(Type==SGM::EntityType::SurfaceType)
        {
        std::set<face *,EntityCompare> const &sSurfaceFaces=((surface *)(pEntity))->GetFaces();
        std::set<face *,EntityCompare>::const_iterator FaceIter=sSurfaceFaces.begin();
        while(FaceIter!=sSurfaceFaces.end())
            {
            face *pFace=*FaceIter;
            std::set<edge *,EntityCompare> const &sFaceEdges=pFace->GetEdges();
            sEdges.insert(sFaceEdges.begin(),sFaceEdges.end());
            ++FaceIter;
            }
        }
    else if(Type==SGM::EntityType::CurveType)
        {
        std::set<edge *,EntityCompare> const &sCurveEdges=((curve *)(pEntity))->GetEdges();
        sEdges.insert(sCurveEdges.begin(),sCurveEdges.end());
        }
    }

void FindWireEdges(SGM::Result                    &rResult,
                   entity                   const *pEntity,
                   std::set<edge *,EntityCompare> &sEdges)
    {
    std::set<edge *,EntityCompare> sAllEdges;
    FindEdges(rResult,pEntity,sAllEdges,false);
    for(auto pEdge : sAllEdges)
        {
        if(pEdge->GetFaces().empty())
            {
            sEdges.insert(pEdge);
            }
        }
    }

void FindVertices(SGM::Result        &,//rResult,
                  entity       const *pEntity,
                  std::set<vertex *,EntityCompare> &sVertices,
                  bool                bTopLevel)
    {
    if(pEntity==nullptr)
        {
        return;
        }
    SGM::EntityType Type=pEntity->GetType();
    if(Type==SGM::EntityType::ThingType)
        {
        ((thing *)pEntity)->GetVertices(sVertices,bTopLevel);
        }
    else if(Type==SGM::EntityType::BodyType)
        {
        std::set<volume *,EntityCompare> const &sBodyVolumes=((body *)pEntity)->GetVolumes();
        std::set<volume *,EntityCompare>::const_iterator VolumeIter=sBodyVolumes.begin();
        while(VolumeIter!=sBodyVolumes.end())
            {
            volume *pVolume=*VolumeIter;
            std::set<face *,EntityCompare> const &sVolumeFaces=pVolume->GetFaces();
            std::set<face *,EntityCompare>::const_iterator FaceIter=sVolumeFaces.begin();
            while(FaceIter!=sVolumeFaces.end())
                {
                face *pFace=*FaceIter;
                std::set<edge *,EntityCompare> const &sFaceEdges=pFace->GetEdges();
                std::set<edge *,EntityCompare>::const_iterator EdgeIter=sFaceEdges.begin();
                while(EdgeIter!=sFaceEdges.end())
                    {
                    edge *pEdge=*EdgeIter;
                    if(pEdge->GetStart())
                        {
                        sVertices.insert(pEdge->GetStart());
                        sVertices.insert(pEdge->GetEnd());
                        }
                    ++EdgeIter;
                    }
                ++FaceIter;
                }
            std::set<edge *,EntityCompare> const &sVolumeEdges=pVolume->GetEdges();
            std::set<edge *,EntityCompare>::const_iterator EdgeIter=sVolumeEdges.begin();
            while(EdgeIter!=sVolumeEdges.end())
                {
                edge *pEdge=*EdgeIter;
                if(pEdge->GetStart())
                    {
                    sVertices.insert(pEdge->GetStart());
                    sVertices.insert(pEdge->GetEnd());
                    }
                ++EdgeIter;
                }
            ++VolumeIter;
            }
        }
    else if(Type==SGM::EntityType::VolumeType)
        {
        std::set<face *,EntityCompare> const &sVolumeFaces=((volume *)pEntity)->GetFaces();
        std::set<face *,EntityCompare>::const_iterator FaceIter=sVolumeFaces.begin();
        while(FaceIter!=sVolumeFaces.end())
            {
            face *pFace=*FaceIter;
            std::set<edge *,EntityCompare> const &sFaceEdges=pFace->GetEdges();
            std::set<edge *,EntityCompare>::const_iterator EdgeIter=sFaceEdges.begin();
            while(EdgeIter!=sFaceEdges.end())
                {
                edge *pEdge=*EdgeIter;
                if(pEdge->GetStart())
                    {
                    sVertices.insert(pEdge->GetStart());
                    sVertices.insert(pEdge->GetEnd());
                    }
                ++EdgeIter;
                }
            ++FaceIter;
            }
        std::set<edge *,EntityCompare> const &sVolumeEdges=((volume *)pEntity)->GetEdges();
        std::set<edge *,EntityCompare>::const_iterator EdgeIter=sVolumeEdges.begin();
        while(EdgeIter!=sVolumeEdges.end())
            {
            edge *pEdge=*EdgeIter;
            if(pEdge->GetStart())
                {
                sVertices.insert(pEdge->GetStart());
                sVertices.insert(pEdge->GetEnd());
                }
            ++EdgeIter;
            }
        }
    else if(Type==SGM::EntityType::FaceType)
        {
        std::set<edge *,EntityCompare> const &sFaceEdges=((face *)pEntity)->GetEdges();
        std::set<edge *,EntityCompare>::const_iterator EdgeIter=sFaceEdges.begin();
        while(EdgeIter!=sFaceEdges.end())
            {
            edge *pEdge=*EdgeIter;
            if(pEdge->GetStart())
                {
                sVertices.insert(pEdge->GetStart());
                sVertices.insert(pEdge->GetEnd());
                }
            ++EdgeIter;
            }
        }
    else if(Type==SGM::EntityType::EdgeType)
        {
        edge *pEdge=(edge *)pEntity;
        if(pEdge->GetStart())
            {
            sVertices.insert(pEdge->GetStart());
            sVertices.insert(pEdge->GetEnd());
            }
        }
    else if(Type==SGM::EntityType::VertexType)
        {
        sVertices.insert((vertex *)pEntity);
        }
    else if(Type==SGM::EntityType::SurfaceType)
        {
        std::set<face *,EntityCompare> const &sSurfaceFaces=((surface *)(pEntity))->GetFaces();
        std::set<face *,EntityCompare>::const_iterator FaceIter=sSurfaceFaces.begin();
        while(FaceIter!=sSurfaceFaces.end())
            {
            face *pFace=*FaceIter;
            std::set<edge *,EntityCompare> const &sFaceEdges=pFace->GetEdges();
            std::set<edge *,EntityCompare>::const_iterator EdgeIter=sFaceEdges.begin();
            while(EdgeIter!=sFaceEdges.end())
                {
                edge *pEdge=*EdgeIter;
                if(pEdge->GetStart())
                    {
                    sVertices.insert(pEdge->GetStart());
                    sVertices.insert(pEdge->GetEnd());
                    }
                ++EdgeIter;
                }
            }
        }
    else if(Type==SGM::EntityType::CurveType)
        {
        std::set<edge *,EntityCompare> const &sCurveEdges=((curve *)(pEntity))->GetEdges();
        std::set<edge *,EntityCompare>::const_iterator EdgeIter=sCurveEdges.begin();
        while(EdgeIter!=sCurveEdges.end())
            {
            edge *pEdge=*EdgeIter;
            if(pEdge->GetStart())
                {
                sVertices.insert(pEdge->GetStart());
                sVertices.insert(pEdge->GetEnd());
                }
            ++EdgeIter;
            }
        }
    }

size_t FindEdgesOnFaceAtVertex(SGM::Result         &rResult,
                               vertex        const *pVertex,
                               face          const *pFace,
                               std::vector<edge *> &aEdges)
    {
    std::set<edge *,EntityCompare> sEdges;
    FindEdges(rResult,pVertex,sEdges,false);
    std::set<edge *,EntityCompare>::iterator EdgeIter=sEdges.begin();
    while(EdgeIter!=sEdges.end())
        {
        std::set<face *,EntityCompare> sFaces;
        edge *pEdge=*EdgeIter;
        FindFaces(rResult,pEdge,sFaces,false);
        if(sFaces.find((face *)pFace)!=sFaces.end())
            {
            aEdges.push_back(pEdge);
            }
        ++EdgeIter;
        }
    return aEdges.size();
    }

size_t OrderEdgesAboutVertexOnFace(SGM::Result         &rResult,
                                   vertex        const *pVertex,
                                   face          const *pFace,
                                   std::vector<edge *> &aEdges)
    {
    SGM::UnitVector3D Norm;
    surface const *pSurface=pFace->GetSurface();
    SGM::Point3D const &VertexPos=pVertex->GetPoint();
    SGM::Point2D uv=pSurface->Inverse(VertexPos);
    pSurface->Evaluate(uv,nullptr,nullptr,nullptr,&Norm);
    if(pFace->GetFlipped())
        {
        Norm.Negate();
        }
    SGM::UnitVector3D XAxis=Norm.Orthogonal();
    SGM::UnitVector3D YAxis=Norm*XAxis;
    size_t nEdges=FindEdgesOnFaceAtVertex(rResult,pVertex,pFace,aEdges);
    size_t Index1;
    std::vector<std::pair<double,edge *> > aEdgePair;
    for(Index1=0;Index1<nEdges;++Index1)
        {
        edge *pEdge=aEdges[Index1];
        SGM::Point3D Pos(0,0,0);
        if(pEdge->GetStart()==pVertex)
            {
            Pos=pEdge->FindMidPoint(0.001);
            }
        if(pEdge->GetEnd()==pVertex)
            {
            Pos=pEdge->FindMidPoint(0.999);
            }
        SGM::Vector3D Vec=Pos-VertexPos;
        double dAngle=SGM::SAFEatan2(Vec%YAxis,Vec%XAxis);
        aEdgePair.emplace_back(dAngle,pEdge);
        }
    std::sort(aEdgePair.begin(),aEdgePair.end());
    nEdges=aEdgePair.size();
    aEdges.clear();
    for(Index1=0;Index1<nEdges;++Index1)
        {
        aEdges.push_back(aEdgePair[Index1].second);
        }
    return nEdges;
    }

edge *NextOutEdgeFrom(face                const *pFace,
                      vertex              const *pVertex,
                      std::vector<edge *> const &aEdges,
                      size_t                     nStart)
    {
    size_t nEdges=aEdges.size();
    size_t Index1;
    for(Index1=1;Index1<nEdges;++Index1)
        {
        edge *pEdge=aEdges[(Index1+nStart)%nEdges];
        SGM::EdgeSideType nType=pFace->GetSideType(pEdge);
        if(pFace->GetFlipped())
            {
            if(nType==SGM::FaceOnLeftType)
                {
                if(pEdge->GetEnd()==pVertex)
                    {
                    return pEdge;
                    }
                }
            else if(nType==SGM::FaceOnRightType)
                {
                if(pEdge->GetStart()==pVertex)
                    {
                    return pEdge;
                    }
                }
            else if(nType==SGM::FaceOnBothSidesType)
                {
                return pEdge;
                }
            }
        else
            {
            if(nType==SGM::FaceOnLeftType)
                {
                if(pEdge->GetStart()==pVertex)
                    {
                    return pEdge;
                    }
                }
            else if(nType==SGM::FaceOnRightType)
                {
                if(pEdge->GetEnd()==pVertex)
                    {
                    return pEdge;
                    }
                }
            else if(nType==SGM::FaceOnBothSidesType)
                {
                return pEdge;
                }
            }
        }
    return NULL;
    }

edge *FindNextEdge(SGM::Result  &rResult,
                   face   const *pFace,
                   edge   const *pEdge,
                   vertex const *pVertex)
    {
    if(pVertex==nullptr || pVertex->GetEdges().size()==1)
        {
        return (edge *)pEdge;
        }
    std::vector<edge *> aEdges;
    size_t nEdges=OrderEdgesAboutVertexOnFace(rResult,pVertex,pFace,aEdges);
    if(pFace->GetFlipped())
        {
        std::reverse(aEdges.begin(),aEdges.end());
        }
    edge *pAnswer=nullptr;
    size_t Index1;
    for(Index1=0;Index1<nEdges;++Index1)
        {
        if(aEdges[Index1]==pEdge)
            {
            pAnswer=NextOutEdgeFrom(pFace,pVertex,aEdges,Index1);
            break;
            }
        }
    return pAnswer;
    }

void OrderLoopEdges(SGM::Result                          &rResult,
                    face                           const *pFace,
                    std::set<edge *,EntityCompare> const &sEdges,
                    std::vector<edge *>                  &aEdges,
                    std::vector<SGM::EdgeSideType>       &aFlips)
    {
    size_t nSize=sEdges.size();
    if(nSize==0)
        {
        return;
        }
    std::vector<edge *> aTempEdges;
    std::vector<SGM::EdgeSideType> aTempFlips;
    aTempEdges.reserve(nSize);
    aTempFlips.reserve(nSize);
    aEdges.reserve(nSize);
    aFlips.reserve(nSize);

    edge *pStartEdge=*(sEdges.begin());
    aTempEdges.push_back(pStartEdge);
    SGM::EdgeSideType nStartSide=pFace->GetSideType(pStartEdge);
    bool bFlipped=pFace->GetFlipped();
    if(nStartSide==SGM::FaceOnBothSidesType)
        {
        nStartSide=SGM::FaceOnLeftType;
        }
    aTempFlips.push_back(nStartSide);
    vertex *pVertex;
    if(bFlipped)
        {
        pVertex = nStartSide==SGM::FaceOnRightType ? pStartEdge->GetEnd() : pStartEdge->GetStart();
        }
    else
        {
        pVertex = nStartSide==SGM::FaceOnRightType ? pStartEdge->GetStart() : pStartEdge->GetEnd();
        }
    edge *pNextEdge=FindNextEdge(rResult,pFace,pStartEdge,pVertex);
    SGM::EdgeSideType nNextType=pFace->GetSideType(pNextEdge);
    if(nNextType==SGM::FaceOnBothSidesType)
        {
        if(bFlipped)
            {
            if(pNextEdge->GetStart()!=pVertex)
                {
                nNextType=SGM::FaceOnLeftType;
                }
            else 
                {
                nNextType=SGM::FaceOnRightType;
                }
            }
        else
            {
            if(pNextEdge->GetStart()!=pVertex)
                {
                nNextType=SGM::FaceOnRightType;
                }
            else 
                {
                nNextType=SGM::FaceOnLeftType;
                }
            }
        }
    while(pNextEdge!=pStartEdge || nNextType!=nStartSide)
        {
        vertex *pLastVertex=pVertex;
        aTempEdges.push_back(pNextEdge);
        aTempFlips.push_back(nNextType);
        if(pNextEdge->GetStart()==pLastVertex)
            {
            pVertex=pNextEdge->GetEnd();
            }
        else
            {
            pVertex=pNextEdge->GetStart();
            }
        pNextEdge=FindNextEdge(rResult,pFace,pNextEdge,pVertex);
        nNextType=pFace->GetSideType(pNextEdge);
        if(nNextType==SGM::FaceOnBothSidesType)
            {
            if(bFlipped)
                {
                if(pNextEdge->GetStart()!=pVertex)
                    {
                    nNextType=SGM::FaceOnLeftType;
                    }
                else 
                    {
                    nNextType=SGM::FaceOnRightType;
                    }
                }
            else
                {
                if(pNextEdge->GetStart()!=pVertex)
                    {
                    nNextType=SGM::FaceOnRightType;
                    }
                else 
                    {
                    nNextType=SGM::FaceOnLeftType;
                    }
                }
            }
        }

    // Order the edges of loop to start at the lowest ID edge.

    size_t nTempEdges=aTempEdges.size();
    size_t Index1;
    size_t nStart=0;
    size_t nStartID=std::numeric_limits<size_t>::max();
    for(Index1=0;Index1<nTempEdges;++Index1)
        {
        size_t nID=aTempEdges[Index1]->GetID();
        if(nID<nStartID)
            {
            nStartID=nID;
            nStart=Index1;
            }
        }
    for(Index1=0;Index1<nTempEdges;++Index1)
        {
        size_t nWhere=(Index1+nStart)%nTempEdges;
        aEdges.push_back(aTempEdges[nWhere]);
        aFlips.push_back(aTempFlips[nWhere]);
        }
    }

void FindComplexes(SGM::Result                       &,//rResult,
                   entity                      const *pEntity,
                   std::set<complex *,EntityCompare> &sComplexes,
                   bool                               bTopLevel)
    {
    SGM::EntityType nType=pEntity->GetType();
    switch(nType)
        {
        case SGM::ThingType:
            {
            ((thing *)pEntity)->GetComplexes(sComplexes,bTopLevel);
            break;
            }
        case SGM::ComplexType:
            {
            sComplexes.insert((complex *)const_cast<entity *>(pEntity));
            break;
            }
        default:
            break;
        }
    }

void FindSurfaces(SGM::Result                       &rResult,
                  entity                      const *pEntity,
                  std::set<surface *,EntityCompare> &sSurfaces,
                  bool                               bTopLevel)
    {
    if(pEntity->GetType()==SGM::EntityType::ThingType)
        {
        ((thing *)pEntity)->GetSurfaces(sSurfaces,bTopLevel);
        return;
        }
    std::set<face *,EntityCompare> sFaces;
    FindFaces(rResult,pEntity,sFaces,false);
    std::set<face *,EntityCompare>::iterator iter=sFaces.begin();
    while(iter!=sFaces.end())
        {
        sSurfaces.insert((surface *)((*iter)->GetSurface()));
        ++iter;
        }
    }

void FindCurves(SGM::Result                     &rResult,
                entity                    const *pEntity,
                std::set<curve *,EntityCompare> &sCurves,
                bool                             bTopLevel)
    {
    if(pEntity->GetType()==SGM::EntityType::ThingType)
        {
        ((thing *)pEntity)->GetCurves(sCurves,bTopLevel);
        return;
        }
    std::set<edge *,EntityCompare> sEdges;
    FindEdges(rResult,pEntity,sEdges,false);
    std::set<edge *,EntityCompare>::iterator iter=sEdges.begin();
    while(iter!=sEdges.end())
        {
        sCurves.insert((curve *)((*iter)->GetCurve()));
        ++iter;
        }
    }

void ImprintVerticesOnClosedEdges(SGM::Result &rResult)
    {
    std::set<edge *,EntityCompare> sEdges;
    FindEdges(rResult, rResult.GetThing(), sEdges, false);

    for(SGMInternal::edge *pEdge : sEdges)
        {
        if (nullptr == pEdge->GetStart())
            {
            SGM::Point3D VertexPos;
            curve const *pCurve = pEdge->GetCurve();
            pCurve->Evaluate(pCurve->GetDomain().m_dMin,&VertexPos);
            vertex *pVertex=new vertex(rResult,VertexPos);
            pEdge->SetStart(pVertex);
            pEdge->SetEnd(pVertex);
            }
        }
    }

size_t FindCommonEdgesFromVertices(SGM::Result         &rResult,
                                   vertex        const *pVertex1, // Input
                                   vertex        const *pVertex2, // Input
                                   std::vector<edge *> &aEdges,   // Ouput
                                   face          const *pFace)    // Optional input
    {
    std::set<edge *,EntityCompare> sEdges,sBadEdges;
    if(pFace)
        {
        FindEdges(rResult,pFace,sEdges,false);
        std::set<edge *,EntityCompare>::iterator iter=sEdges.begin();
        while(iter!=sEdges.end())
            {
            edge *pEdge=*iter;
            if(pEdge->GetStart()!=pVertex1 && pEdge->GetEnd()!=pVertex1)
                {
                sBadEdges.insert(pEdge);
                }
            else if(pEdge->GetStart()!=pVertex2 && pEdge->GetEnd()!=pVertex2)
                {
                sBadEdges.insert(pEdge);
                }
            ++iter;
            }
        }
    else
        {
        FindEdges(rResult,pVertex1,sEdges,false);
        std::set<edge *,EntityCompare>::iterator iter=sEdges.begin();
        while(iter!=sEdges.end())
            {
            edge *pEdge=*iter;
            if(pEdge->GetStart()!=pVertex2 && pEdge->GetEnd()!=pVertex2)
                {
                sBadEdges.insert(pEdge);
                }
            ++iter;
            }
        }
    std::set<edge *,EntityCompare>::iterator EdgeIter=sEdges.begin();
    while(EdgeIter!=sEdges.end())
        {
        edge *pEdge=*EdgeIter;
        if(sBadEdges.find(pEdge)==sBadEdges.end())
            {
            aEdges.push_back(pEdge);
            }
        ++EdgeIter;
        }
    return aEdges.size();
    }

size_t FindAdjacentFaces(SGM::Result                    &rResult,
                         face                     const *pFace,
                         std::set<face *,EntityCompare> &sFaces)
    {
    std::set<vertex *,EntityCompare> sVertives;
    FindVertices(rResult,pFace,sVertives,false);
    std::set<vertex *,EntityCompare>::iterator VertexIter=sVertives.begin();
    while(VertexIter!=sVertives.end())
        {
        vertex *pVertex=*VertexIter;
        FindFaces(rResult,pVertex,sFaces,false);
        ++VertexIter;
        }
    std::set<edge *,EntityCompare> sEdges;
    FindEdges(rResult,pFace,sEdges,false);
    std::set<edge *,EntityCompare>::iterator EdgeIter=sEdges.begin();
    while(EdgeIter!=sEdges.end())
        {
        edge *pEdge=*EdgeIter;
        FindFaces(rResult,pEdge,sFaces,false);
        ++EdgeIter;
        }
    return sFaces.size();
    }

void FindFacesOfCell(SGM::Result                    &rResult,
                     face                           *pFace,
                     std::set<face *,EntityCompare> &sFaces)
    {
    FindFaces(rResult,pFace->GetVolume(),sFaces,false);
    }

size_t FindCommonEdgesFromFaces(SGM::Result         &rResult,
                                face          const *pFace1,   
                                face          const *pFace2,   
                                std::vector<edge *> &aEdges)
    {
    std::set<edge *,EntityCompare> sEdges1,sEdges2;
    FindEdges(rResult,pFace1,sEdges1,false);
    FindEdges(rResult,pFace2,sEdges2,false);
    std::set<edge *,EntityCompare>::iterator iter=sEdges1.begin();
    size_t nCount=0;
    while(iter!=sEdges1.end())
        {
        edge *pEdge=*iter;
        if(sEdges2.find(pEdge)!=sEdges2.end())
            {
            aEdges.push_back(pEdge);
            ++nCount;
            }
        ++iter;
        }
    return nCount;
    }

void RemoveFace(SGM::Result &rResult,
                face        *pFace)
    {
    std::set<edge *,EntityCompare> sEdges;
    FindEdges(rResult,pFace,sEdges,false);
    if(sEdges.size()==2)
        {
        std::set<face *,EntityCompare> sFaces;
        if(3==FindAdjacentFaces(rResult,pFace,sFaces))
            {
            // Three adjacent faces case.

            std::set<face *,EntityCompare>::iterator iter=sFaces.begin();
            face *pFace0=*iter;
            ++iter;
            face *pFace1=*iter;
            ++iter;
            face *pFace2=*iter;

            // Find the edges to extend.

            std::set<vertex *,EntityCompare> sVertices;
            FindVertices(rResult,pFace,sVertices,false);
            std::set<edge *,EntityCompare> sAllEdges;
            std::set<vertex *,EntityCompare>::iterator VertexIter=sVertices.begin();
            while(VertexIter!=sVertices.end())
                {
                FindEdges(rResult,*VertexIter,sAllEdges,false);
                ++VertexIter;
                }
            std::set<edge *,EntityCompare>::iterator EdgeIter=sAllEdges.begin();
            std::vector<edge *> aCommonEdges;
            while(EdgeIter!=sAllEdges.end())
                {
                edge *pEdge=*EdgeIter;
                if(sEdges.find(pEdge)==sEdges.end())
                    {
                    aCommonEdges.push_back(pEdge);
                    }
                ++EdgeIter;
                }

            // Extend the edges.

            size_t Index1,Index2;
            std::vector<SGM::Point3D> aPoints;
            IntersectThreeSurfaces(rResult,pFace0->GetSurface(),pFace1->GetSurface(),pFace2->GetSurface(),aPoints);
            size_t nCommonEdges=aCommonEdges.size();
            if(aPoints.size()>1)
                {
                SGM::Point3D BestPos=aPoints[0];
                double dBest=std::numeric_limits<double>::max();
                size_t nPoints=aPoints.size();
                for(Index1=0;Index1<nPoints;++Index1)
                    {
                    SGM::Point3D Pos=aPoints[Index1];
                    double dDist=0;
                    for(Index2=0;Index2<nCommonEdges;++Index2)
                        {
                        SGM::Point3D CPos;
                        edge *pEdge=aCommonEdges[Index2];
                        pEdge->GetCurve()->Inverse(Pos,&CPos);
                        dDist+=Pos.Distance(CPos);
                        }
                    if(dDist<dBest)
                        {
                        dBest=dDist;
                        BestPos=Pos;
                        }
                    }
                aPoints.clear();
                aPoints.push_back(BestPos);
                }
            if(aPoints.size()==1)
                {
                SGM::Point3D const &Pos=aPoints[0];
                vertex *pVertex=new vertex(rResult,Pos);
                for(Index1=0;Index1<nCommonEdges;++Index1)
                    {
                    edge *pEdge=aCommonEdges[Index1];
                    if(sVertices.find(pEdge->GetStart())!=sVertices.end())
                        {
                        double t=pEdge->GetCurve()->Inverse(Pos);
                        SGM::Interval1D Domain=pEdge->GetDomain();
                        Domain.m_dMin=t;
                        pEdge->SetDomain(rResult,Domain);
                        pEdge->SetStart(pVertex);
                        }                        
                    else 
                        {
                        double t=pEdge->GetCurve()->Inverse(Pos);
                        SGM::Interval1D Domain=pEdge->GetDomain();
                        Domain.m_dMax=t;
                        pEdge->SetDomain(rResult,Domain);
                        pEdge->SetEnd(pVertex);
                        }
                    }

                // Remove the faces and edges.

                std::set<entity *,EntityCompare> sChildren;
                pFace->FindAllChildren(sChildren);
                sChildren.insert(pFace);
                std::set<entity *,EntityCompare>::iterator ChildrenIter=sChildren.begin();
                while(ChildrenIter!=sChildren.end())
                    {
                    entity *pEntity=*ChildrenIter;
                    rResult.GetThing()->SeverOwners(pEntity);
                    ++ChildrenIter;
                    }
                ChildrenIter=sChildren.begin();
                while(ChildrenIter!=sChildren.end())
                    {
                    entity *pEntity=*ChildrenIter;
                    rResult.GetThing()->DeleteEntity(pEntity);
                    ++ChildrenIter;
                    }
                }
            }
        }
    }

void MergeFaces(SGM::Result &rResult,
                face        *pFace1,
                face        *pFace2)
    {
    // Keep the face with lower ID.  Make it pFace1.

    if(pFace2->GetID()<pFace1->GetID())
        {
        std::swap(pFace1,pFace2);
        }

    // Move all the edges from pFace2 to pFace1

    std::set<edge *,EntityCompare> sEdges=pFace2->GetEdges();
    std::set<edge *,EntityCompare>::iterator EdgeIter=sEdges.begin();
    while(EdgeIter!=sEdges.end())
        {
        edge *pEdge=*EdgeIter;
        SGM::EdgeSideType nType=pFace2->GetSideType(pEdge);
        pFace2->RemoveEdge(rResult,pEdge);
        pFace1->AddEdge(pEdge,nType);
        ++EdgeIter;
        }

    // Delete pFace2 and its surface if it is not used elsewhere.

    surface *pSurface=(surface *)(pFace2->GetSurface());
    if(pSurface->GetFaces().size()==1 && pSurface->GetOwners().empty())
        {
        pSurface->RemoveFace(pFace2);
        rResult.GetThing()->DeleteEntity(pSurface);
        }
    pFace2->GetVolume()->RemoveFace(pFace2);
    rResult.GetThing()->DeleteEntity(pFace2);
    }

void Merge(SGM::Result &rResult,
           entity      *pEntity)
    {
    // Find the mergable edges.

    std::set<edge *,EntityCompare> sEdges,sMergable;
    FindEdges(rResult,pEntity,sEdges,false);
    std::set<edge *,EntityCompare>::iterator EdgeIter=sEdges.begin();
    while(EdgeIter!=sEdges.end())
        {
        edge *pEdge=*EdgeIter;
        std::set<face *,EntityCompare> sFaces;
        FindFaces(rResult,pEdge,sFaces,false);
        if(sFaces.size()==2)
            {
            std::set<face *,EntityCompare>::iterator iter=sFaces.begin();
            face *pFace1=*iter;
            ++iter;
            face *pFace2=*iter;
            if(pFace1->GetSurface()->IsSame(pFace2->GetSurface(),SGM_MIN_TOL))
                {
                sMergable.insert(pEdge);
                }
            }
        ++EdgeIter;
        }

    // Merge out the edges.

    std::set<vertex *,EntityCompare> sVertices;
    EdgeIter=sMergable.begin();
    while(EdgeIter!=sMergable.end())
        {
        edge *pEdge=*EdgeIter;
        if(pEdge->GetStart())
            {
            sVertices.insert(pEdge->GetStart());
            }
        if(pEdge->GetEnd())
            {
            sVertices.insert(pEdge->GetEnd());
            }
        std::set<face *,EntityCompare> sFaces;
        FindFaces(rResult,pEdge,sFaces,false);
        pEdge->SeverRelations(rResult);
        rResult.GetThing()->DeleteEntity(pEdge);
        if(sFaces.size()==2)
            {
            std::set<face *,EntityCompare>::iterator iter=sFaces.begin();
            face *pFace1=*iter;
            ++iter;
            face *pFace2=*iter;
            MergeFaces(rResult,pFace1,pFace2);
            }
        ++EdgeIter;
        }

    // Remove vertices that are no longer needed.

    for(auto pVertex : sVertices)
        {
        if(pVertex->IsTopLevel())
            {
            rResult.GetThing()->DeleteEntity(pVertex);
            }
        }

    // Check for vertices with less than three edges that can be merged out.
    
    sVertices.clear();
    FindVertices(rResult,pEntity,sVertices,false);
    std::set<edge *,EntityCompare> sFixEdges;
    for(auto pVertex : sVertices)
        {
        std::set<edge *,EntityCompare> const &sVertexEdges=pVertex->GetEdges();
        if(sVertexEdges.size()==1)
            {
            edge *pEdge0=*(sVertexEdges.begin());
            pEdge0->SetStart(nullptr);
            pEdge0->SetEnd(nullptr);
            rResult.GetThing()->DeleteEntity(pVertex);
            sFixEdges.insert(pEdge0);
            }
        else if(sVertexEdges.size()==2)
            {
            edge *pEdge0=*(sVertexEdges.begin());
            edge *pEdge1=*(sVertexEdges.begin()++);
            if(pEdge0->GetCurve()->IsSame(pEdge1->GetCurve(),SGM_MIN_TOL))
                {
                if(pEdge0->GetStart()==pVertex)
                    {
                    if(pEdge1->GetStart()==pVertex)
                        {
                        pEdge0->SetStart(pEdge1->GetEnd());
                        }
                    else
                        {
                        pEdge0->SetStart(pEdge1->GetStart());
                        }
                    }
                else
                    {
                    if(pEdge1->GetStart()==pVertex)
                        {
                        pEdge0->SetEnd(pEdge1->GetEnd());
                        }
                    else
                        {
                        pEdge0->SetEnd(pEdge1->GetStart());
                        }
                    }
                curve *pCurve=pEdge1->GetCurve();
                pEdge1->SeverRelations(rResult);
                if(pCurve->IsTopLevel())
                    {
                    rResult.GetThing()->DeleteEntity(pCurve);
                    }
                rResult.GetThing()->DeleteEntity(pEdge1);
                pVertex->SeverRelations(rResult);
                rResult.GetThing()->DeleteEntity(pVertex);
                sFixEdges.insert(pEdge0);
                sFixEdges.erase(pEdge1);
                }
            }
        }
    for(auto pEdge : sFixEdges)
        {
        pEdge->FixDomain(rResult);
        }
    }

body *UnhookFaces(SGM::Result         &rResult,
                  std::vector<face *> &aFaces)
    {
    // Find all the surfaces, curves, edges and vertices on the given faces.

    std::set<surface *,EntityCompare> sSurfaces;
    std::set<curve *,EntityCompare> sCurves;
    std::set<edge *,EntityCompare> sEdges;
    std::set<vertex *,EntityCompare> sVertices;
    std::set<face *,EntityCompare> sFaces;
    size_t nFaces=aFaces.size();
    size_t Index1;
    for(Index1=0;Index1<nFaces;++Index1)
        {
        face *pFace=aFaces[Index1];
        sSurfaces.insert((surface *)pFace->GetSurface());
        FindCurves(rResult,pFace,sCurves,false);
        FindEdges(rResult,pFace,sEdges,false);
        FindVertices(rResult,pFace,sVertices,false);
        sFaces.insert(pFace);
        }

    // Find which surfaces, curves, edges and vertices are on other faces.

    std::set<entity *> sCopyEnts;
    for(auto pSurf : sSurfaces)
        {
        for(auto pFace : pSurf->GetFaces())
            {
            if(sFaces.find(pFace)==sFaces.end())
                {
                sCopyEnts.insert(pSurf);
                break;
                }
            }
        }
    for(auto pCurve : sCurves)
        {
        std::set<face *,EntityCompare> sCurveFaces;
        FindFaces(rResult,pCurve,sCurveFaces,false);
        for(auto pFace : sCurveFaces)
            {
            if(sFaces.find(pFace)==sFaces.end())
                {
                sCopyEnts.insert(pCurve);
                break;
                }
            }
        }
    for(auto pEdge : sEdges)
        {
        std::set<face *,EntityCompare> sEdgeFaces;
        FindFaces(rResult,pEdge,sEdgeFaces,false);
        for(auto pFace : sEdgeFaces)
            {
            if(sFaces.find(pFace)==sFaces.end())
                {
                sCopyEnts.insert(pEdge);
                break;
                }
            }
        }
    for(auto pVertex : sVertices)
        {
        std::set<face *,EntityCompare> sVertexFaces;
        FindFaces(rResult,pVertex,sVertexFaces,false);
        for(auto pFace : sVertexFaces)
            {
            if(sFaces.find(pFace)==sFaces.end())
                {
                sCopyEnts.insert(pVertex);
                break;
                }
            }
        }

    // Make a copy of all shaired entities.

    body *pAnswer=new body(rResult);
    volume *pAnswerVolume=new volume(rResult);
    pAnswer->AddVolume(pAnswerVolume);
    std::map<entity *,entity *> mCopyMap;
    for(entity *pEnt : sCopyEnts)
        {
        mCopyMap[pEnt]=pEnt->Clone(rResult);
        }

    // Replace the shaired entities with the copies in the unhooked faces.

    for(entity *pEnt : sCopyEnts)
        {
        switch(pEnt->GetType())
            {
            case SGM::EdgeType:
                {
                edge *pEdge=(edge *)pEnt;
                pEdge->ReplacePointers(mCopyMap);
                break;
                }
            case SGM::VertexType:
                {
                vertex *pVertex=(vertex *)pEnt;
                pVertex->ReplacePointers(mCopyMap);
                break;
                }
            case SGM::CurveType:
                {
                curve *pCurve=(curve *)pEnt;
                pCurve->ReplacePointers(mCopyMap);
                break;
                }
            case SGM::SurfaceType:
                {
                surface *pSurface=(surface *)pEnt;
                pSurface->ReplacePointers(mCopyMap);
                break;
                }
            case SGM::VolumeType:
                {
                volume *pVolume=(volume *)pEnt;
                pVolume->ReplacePointers(mCopyMap);
                break;
                }
            case SGM::BodyType:
                {
                body *pBody=(body *)pEnt;
                pBody->ReplacePointers(mCopyMap);
                break;
                }
            default:
                {
                throw;
                }
            }
        }

    return pAnswer;
    }

} // end namespace SGMInternal
