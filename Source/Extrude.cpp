#include "SGMVector.h"
#include "SGMInterval.h"
#include "SGMMathematics.h"
#include "SGMEntityClasses.h"
#include "SGMTransform.h"

#include "EntityClasses.h"
#include "Surface.h"
#include "Curve.h"

namespace SGMInternal
{

extrude::extrude(SGM::Result             &rResult,
                 SGM::UnitVector3D const &vAxis,
                 curve                   *pCurve): 
    surface(rResult, SGM::ExtrudeType)
    {
    m_vAxis  = vAxis;
    m_Domain.m_VDomain=SGM::Interval1D(-SGM_MAX,SGM_MAX);

    if (nullptr != pCurve)
        {
        SetCurve(pCurve);
        }
    }

extrude::~extrude()
    {
    if (m_pCurve)
        m_pCurve->RemoveOwner(this);
    }

void extrude::FindAllChildren(std::set<entity *, EntityCompare> &sChildren) const
    {
    sChildren.insert(m_pCurve);
    }

void extrude::Evaluate(SGM::Point2D const &uv,
                       SGM::Point3D       *Pos,
                       SGM::Vector3D      *Du,
                       SGM::Vector3D      *Dv,
                       SGM::UnitVector3D  *Norm,
                       SGM::Vector3D      *Duu,
                       SGM::Vector3D      *Duv,
                       SGM::Vector3D      *Dvv) const
    {
    SGM::Point3D   CurvePos;
    SGM::Vector3D  DuCurve;
    SGM::Vector3D *pDuCurve = &DuCurve;
    SGM::Vector3D  DuuCurve;
    SGM::Vector3D *pDuuCurve = &DuuCurve;

    if (nullptr == Dv && nullptr == Duv && nullptr == Duu && nullptr == Norm)
        pDuCurve = nullptr;
    if (nullptr == Duu)
        pDuuCurve = nullptr;

    // Evaluate the defining curve.

    m_pCurve->Evaluate(uv.m_u, &CurvePos, pDuCurve, pDuuCurve);

    // Fill in the answers.

    if(Pos)
        {
        *Pos=CurvePos+m_vAxis*uv.m_v;
        }
    if(Du)
        {
        *Du=*pDuCurve;
        }
    if(Dv)
        {
        *Dv=m_vAxis;
        }
    if(Norm)
        {
        *Norm=(*pDuCurve)*m_vAxis;
        }
    if(Duu)
        {
        *Duu=*pDuuCurve;
        }
    if(Duv)
        {
        *Duv=SGM::Vector3D(0,0,0);
        }
    if(Dvv)
        {
        *Dvv=SGM::Vector3D(0,0,0);
        }
    }

SGM::Point2D extrude::Inverse(SGM::Point3D const &Pos,
                              SGM::Point3D       *ClosePos,
                              SGM::Point2D const *pGuess) const
    {
    SGM::Point2D uv;

    uv.m_v=(Pos-m_Origin)%m_vAxis;
    SGM::Point3D PlanePos=Pos-m_vAxis*uv.m_v;
    if(pGuess)
        {
        uv.m_u=m_pCurve->Inverse(PlanePos,ClosePos,&(pGuess->m_u));
        }
    else
        {
        uv.m_u=m_pCurve->Inverse(PlanePos,ClosePos);
        }
    return uv;
    }

    
void extrude::Transform(SGM::Transform3D const &Trans)
    {
    m_vAxis=Trans*m_vAxis;
    if(m_pCurve->GetEdges().empty() && m_pCurve->GetOwners().size()==1)
        {
        m_pCurve->Transform(Trans);
        }
    else
        {
        //TODO: Make a copy and transform the copy.
        throw std::logic_error("Missing implementation of Transform() when curve has other owners");
        }
    }

curve *extrude::UParamLine(SGM::Result &, double) const
    { throw std::logic_error("Derived class of surface must override UParamLine()"); }

curve *extrude::VParamLine(SGM::Result &, double) const
    { throw std::logic_error("Derived class of surface must override VParamLine()"); }

void extrude::SetCurve(curve *pCurve)
    {
    pCurve->AddOwner(this);
    m_pCurve = pCurve;

    pCurve->Evaluate(pCurve->GetDomain().MidPoint(),&m_Origin);

    this->m_bClosedU = pCurve->GetClosed();
    m_Domain.m_UDomain=pCurve->GetDomain();
    }
}
