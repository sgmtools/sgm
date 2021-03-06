#include "SGMVector.h"
#include "SGMTransform.h"

#include "Curve.h"

namespace SGMInternal
{
hyperbola::hyperbola(SGM::Result             &rResult,
                     SGM::Point3D      const &Center,
                     SGM::UnitVector3D const &XAxis,
                     SGM::UnitVector3D const &YAxis,
                     double                   dA,
                     double                   dB):  
    curve(rResult,SGM::HyperbolaType),m_Center(Center),m_XAxis(XAxis),m_YAxis(YAxis),
    m_Normal(XAxis*YAxis),m_dA(dA),m_dB(dB)
    {
    m_Domain.m_dMin=-SGM_MAX;
    m_Domain.m_dMax=SGM_MAX;
    }

bool hyperbola::IsSame(curve const *pOther,double dTolerance) const
    {
    if(pOther->GetCurveType()!=m_CurveType)
        {
        return false;
        }
    auto pCurve2=(hyperbola const *)pOther;
    if(!SGM::NearEqual(m_Center, pCurve2->m_Center, dTolerance))
        {
        return false;
        }
    if(!SGM::NearEqual(m_XAxis, pCurve2->m_XAxis, dTolerance))
        {
        return false;
        }
    if(!SGM::NearEqual(m_YAxis, pCurve2->m_YAxis, dTolerance))
        {
        return false;
        }
    if(!SGM::NearEqual(m_Normal, pCurve2->m_Normal, dTolerance))
        {
        return false;
        }
    if(!SGM::NearEqual(m_dA, pCurve2->m_dA, dTolerance, false))
        {
        return false;
        }
    return SGM::NearEqual(m_dB, pCurve2->m_dB, dTolerance, false);
    }

hyperbola::hyperbola(SGM::Result &rResult, hyperbola const &other):
        curve(rResult, other),
        m_Center(other.m_Center),
        m_XAxis(other.m_XAxis),
        m_YAxis(other.m_YAxis),
        m_Normal(other.m_Normal),
        m_dA(other.m_dA),
        m_dB(other.m_dB)
    {}

hyperbola *hyperbola::Clone(SGM::Result &rResult) const
    { return new hyperbola(rResult, *this); }

void hyperbola::Evaluate(double t,SGM::Point3D *Pos,SGM::Vector3D *D1,SGM::Vector3D *D2) const
    {
    // f(t)=a*sqrt(1+t^2/b^2)

    double dB2=m_dB*m_dB;
    double dR=1.0+t*t/(dB2);
    double dS=sqrt(dR);
    if(Pos)
        {
        double y=m_dA*dS;
        Pos->m_x=m_Center[0]+m_XAxis.X()*t+m_YAxis.X()*y;
        Pos->m_y=m_Center[1]+m_XAxis.Y()*t+m_YAxis.Y()*y;
        Pos->m_z=m_Center[2]+m_XAxis.Z()*t+m_YAxis.Z()*y;
        }
    if(D1)
        {
        double dy=m_dA*t/(dB2*dS);
        D1->m_x=m_XAxis.X()+m_YAxis.X()*dy;
        D1->m_y=m_XAxis.Y()+m_YAxis.Y()*dy;
        D1->m_z=m_XAxis.Z()+m_YAxis.Z()*dy;
        }
    if(D2)
        {
        double ddy=(m_dA*(1.0-t*t/(dR*dB2)))/(dS*dB2);
        D2->m_x=m_YAxis.X()*ddy;
        D2->m_y=m_YAxis.Y()*ddy;
        D2->m_z=m_YAxis.Z()*ddy;
        }
    }

double hyperbola::Inverse(SGM::Point3D const &Pos,
                          SGM::Point3D       *ClosePos,
                          double       const *) const
    {
    SGM::Point3D const &Center=m_Center;
    SGM::UnitVector3D const &XVec=m_XAxis;
    SGM::Vector3D Vec=Pos-Center;
    double dParam=XVec%Vec;
    double dAnswer=NewtonsMethod(dParam,Pos);
    if(ClosePos)
        {
        Evaluate(dAnswer,ClosePos);
        }
    return dAnswer;
    }

void hyperbola::Transform(SGM::Result            &,//rResult,
                          SGM::Transform3D const &Trans)
    {
    // f(t)=a*sqrt(1+t^2/b^2)
    m_Center=Trans*m_Center;
    m_XAxis=Trans*m_XAxis;
    m_YAxis=Trans*m_YAxis;
    m_Normal=Trans*m_Normal;
    if (double dScale=Trans.Scale())
        {
        m_dA*=dScale;
        m_dB*=dScale;
        }
    }

}