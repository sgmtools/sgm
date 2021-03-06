#include "EntityClasses.h"

///////////////////////////////////////////////////////////////////////////////
//
//  entity method implementations
//
///////////////////////////////////////////////////////////////////////////////
namespace SGMInternal
{

void entity::ChangeColor(SGM::Result &rResult,
                         int nRed,int nGreen,int nBlue)
    {
    RemoveColor(rResult);
    std::vector<int> aData;
    aData.reserve(3);
    aData.push_back(nRed);
    aData.push_back(nGreen);
    aData.push_back(nBlue);
    IntegerAttribute *pColor=new IntegerAttribute(rResult,"SGM Color",aData);
    AddAttribute(pColor);
    pColor->AddOwner(this);
    }

void entity::OwnerAndAttributeReplacePointers(std::map<entity *,entity *> const &mEntityMap)
    {
    std::set<attribute *,EntityCompare> m_sFixedAttributes;
    for(auto pAttribute : m_sAttributes)
        {
        auto MapValue=mEntityMap.find(pAttribute);
        if(MapValue!=mEntityMap.end())
            {
            m_sFixedAttributes.insert((attribute *)MapValue->second);
            }
        //else
        //    {
        //    m_sFixedAttributes.insert(pAttribute);
        //    }
        }
    m_sAttributes=m_sFixedAttributes;

    std::set<entity *,EntityCompare> m_sFixedOwners;
    for(auto pEntity : m_sOwners)
        {
        auto MapValue=mEntityMap.find(pEntity);
        if(MapValue!=mEntityMap.end())
            {
            m_sFixedOwners.insert((attribute *)MapValue->second);
            }
        //else
        //    {
        //    m_sFixedOwners.insert(pEntity);
        //    }
        }
    m_sOwners=m_sFixedOwners;
    }

void entity::RemoveColor(SGM::Result &rResult)
    {
    auto iter=m_sAttributes.begin();
    while(iter!=m_sAttributes.end())
        {
        attribute *pAttribute=*iter;
        if(pAttribute->GetName()=="SGM Color")
            {
            RemoveAttribute(pAttribute);
            pAttribute->RemoveOwner(this);
            if(pAttribute->GetOwners().empty())
                {
                rResult.GetThing()->DeleteEntity(pAttribute);
                }
            break;
            }
        ++iter;
        }
    }

bool entity::GetColor(int &nRed, int &nGreen, int &nBlue) const
    {
    for (auto attr : m_sAttributes)
        {
        if (attr->GetName() == "SGM Color")
            {
            auto pIntegerAttribute = (IntegerAttribute *) (attr);
            std::vector<int> const &aData = pIntegerAttribute->GetData();
            nRed = aData[0];
            nGreen = aData[1];
            nBlue = aData[2];
            return true;
            }
        }
    return false;
    }

void entity::GetParents(std::set<entity *, EntityCompare> &sParents) const
    {
    for (entity *pOwner : m_sOwners)
        {
        sParents.emplace(pOwner);
        }
    }

void entity::RemoveParentsInSet(SGM::Result &,
                                std::set<entity *, EntityCompare> const &sEntities)
    {
    for (entity *pOwner : sEntities)
        {
        pOwner->DisconnectOwnedEntity(this);
        m_sOwners.erase(pOwner);
        }
    }

}
