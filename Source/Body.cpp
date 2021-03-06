#include <EntityFunctions.h>
#include "EntityClasses.h"
#include "Topology.h"

namespace SGMInternal
{

void body::FindAllChildren(std::set<entity *, EntityCompare> &sChildren) const
    {
    for (auto pVolume : GetVolumes())
        {
        sChildren.insert(pVolume);
        pVolume->FindAllChildren(sChildren);
        }
    }

void body::ReplacePointers(std::map<entity *,entity *> const &mEntityMap)
    {
    // Run though all the pointers and change them if they are in the map.
    
    std::set<volume *,EntityCompare> m_sFixedVolumes;
    for(auto pVolume : m_sVolumes)
        {
        auto MapValue=mEntityMap.find(pVolume);
        if(MapValue!=mEntityMap.end())
            {
            m_sFixedVolumes.insert((volume *)MapValue->second);
            }
        //else
        //    {
        //    m_sFixedVolumes.insert(pVolume);
        //    }
        }
    m_sVolumes=m_sFixedVolumes;

    OwnerAndAttributeReplacePointers(mEntityMap);
    }

void body::AddVolume(volume *pVolume) 
    {
    m_sVolumes.insert(pVolume);
    pVolume->SetBody(this);
    }

void body::RemoveVolume(volume *pVolume)
    {
    m_sVolumes.erase(pVolume);
    pVolume->SetBody(nullptr);
    }

void body::AddPoint(SGM::Point3D const &Pos)
    {
    m_aPoints.push_back(Pos);
    }

void body::SetPoints(std::vector<SGM::Point3D> const &aPoints)
    {
    m_aPoints=aPoints;
    }

SGM::Interval3D const &body::GetBox(SGM::Result &rResult,bool /*bContruct*/) const
    {
    if (m_Box.IsEmpty())
        {
        std::set<volume *,EntityCompare> const &sVolumes = GetVolumes();
        StretchBox(rResult,m_Box,sVolumes.begin(),sVolumes.end());
        }
    return m_Box;
    }

void body::SeverRelations(SGM::Result &)
    {
    std::set<volume *,EntityCompare> sVolumes=GetVolumes();
    for(volume *pVolume : sVolumes)
        RemoveVolume(pVolume);
    RemoveAllOwners();
    }

double body::FindVolume(SGM::Result &rResult,bool bApproximate) const
    {
    double dAnswer=0;
    for (auto &&pVolume : m_sVolumes)
        {
        dAnswer+=pVolume->FindVolume(rResult,bApproximate);
        }
    return dAnswer;
    }

bool body::IsSheetBody(SGM::Result &rResult) const
    {
    std::set<edge *,EntityCompare> sEdges;
    FindWireEdges(rResult,this,sEdges);
    if(!sEdges.empty())
        {
        return false;
        }
    std::set<face *,EntityCompare> sFaces;
    FindFaces(rResult,this,sFaces);
    auto iter=sFaces.begin();
    while(iter!=sFaces.end())
        {
        face *pFace=*iter;
        if(pFace->GetSides()!=2)
            {
            return false;
            }
        ++iter;
        }
    return !sFaces.empty();
    }

bool body::IsWireBody(SGM::Result &rResult) const
    {
    std::set<face *,EntityCompare> sFaces;
    FindFaces(rResult,this,sFaces);
    if(!sFaces.empty())
        {
        return false;
        }
    std::set<edge *,EntityCompare> sEdges;
    FindWireEdges(rResult,this,sEdges);
    return !sEdges.empty();
    }

}