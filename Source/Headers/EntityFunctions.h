#ifndef SGM_INTERNAL_ENTITY_FUNCTIONS_H
#define SGM_INTERNAL_ENTITY_FUNCTIONS_H

#include "SGMVector.h"
#include "SGMEntityClasses.h"
#include "SGMBoxTree.h"
#include "SGMResult.h"

#include "EntityClasses.h"

#include <vector>

namespace SGMInternal
{

void DeleteEntity(SGM::Result &rResult,
                  entity      *pEntity);

entity *CopyEntity(SGM::Result &rResult,
                   entity      *pEntity);

void TransformEntity(SGM::Result            &rResult,
                     SGM::Transform3D const &transform3D,
                     entity                 *pEntity);

class HealOptions
    {
    public:

    HealOptions():
        m_bReparamNURBS(true),
        m_bRemoveSlivers(true),
        m_bSimplifySurfaces(true),
        m_bSimplifyCurves(true),
        m_bSnapVertices(false),
        m_bRepairApples(true)
    {}

    bool m_bReparamNURBS;
    bool m_bRemoveSlivers;
    bool m_bSimplifySurfaces;
    bool m_bSimplifyCurves;
    bool m_bSnapVertices;
    bool m_bRepairApples;
    };

void Heal(SGM::Result           &rResult,
          std::vector<entity *> &aEntities,
          HealOptions     const &Options);

/// Return a box that contains all the entity objects in the range [first,last) using the entity->GetBox()
template< class InputIt >
inline void StretchBox(SGM::Result &rResult, SGM::Interval3D &box, InputIt first, InputIt last)
    {
    for (InputIt iter = first; iter != last; ++iter)
        box.operator+=((*iter)->GetBox(rResult));
    }

/// Add all the entity objects (with GetBox member function) in the range [first,last) to the BoxTree.
template< class InputIt >
inline void BoxTreeInsert(SGM::Result &rResult, SGM::BoxTree& rTree, InputIt first, InputIt last)
    {
    for (InputIt iter = first; iter != last; ++iter)
        rTree.Insert(*iter,(*iter)->GetBox(rResult));
    }

// Example:
// std::set<face*,Compare> faceSet = pSurface->GetFaces();
// std::set<entity*,Compare> entitySet = container_cast(faceSet);

template<class SourceContainer>
class ContainerConverter
    {
    const SourceContainer& m_SourceContainer;

public:
    explicit ContainerConverter(const SourceContainer& s) : m_SourceContainer(s) {}

    template<class TargetContainer>
    inline explicit operator TargetContainer() const
        {
        return TargetContainer(m_SourceContainer.begin(), m_SourceContainer.end());
        }
    };

template<class Container>
inline ContainerConverter<Container> container_cast(const Container& c)
    {
    return ContainerConverter<Container>(c);
    }

}  // End of SGMInternal namespace

#endif // SGM_INTERNAL_ENTITY_FUNCTIONS_H