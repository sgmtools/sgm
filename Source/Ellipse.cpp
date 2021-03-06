#include "SGMVector.h"
#include "SGMTransform.h"
#include "Curve.h"

namespace SGMInternal
{
ellipse::ellipse(SGM::Result             &rResult,
                 SGM::Point3D      const &Center,
                 SGM::UnitVector3D const &XAxis,
                 SGM::UnitVector3D const &YAxis,
                 double                   dA,
                 double                   dB):  
    curve(rResult,SGM::EllipseType),m_Center(Center),m_XAxis(XAxis),m_YAxis(YAxis),
    m_Normal(XAxis*YAxis),m_dA(dA),m_dB(dB)
    {
    m_Domain.m_dMin=0;
    m_Domain.m_dMax=SGM_TWO_PI;
    m_bClosed=true;
    }

bool ellipse::IsSame(curve const *pOther,double dTolerance) const
    {
    if(pOther->GetCurveType()!=m_CurveType)
        {
        return false;
        }
    auto pCurve2=(ellipse const *)pOther;
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

ellipse::ellipse(SGM::Result &rResult, ellipse const &other):
        curve(rResult, other),
        m_Center(other.m_Center),
        m_XAxis(other.m_XAxis),
        m_YAxis(other.m_YAxis),
        m_Normal(other.m_Normal),
        m_dA(other.m_dA),
        m_dB(other.m_dB)
{}

ellipse *ellipse::Clone(SGM::Result &rResult) const
{ return new ellipse(rResult, *this); }

void ellipse::Evaluate(double t,SGM::Point3D *Pos,SGM::Vector3D *D1,SGM::Vector3D *D2) const
    {
    double dCos=cos(t);
    double dSin=sin(t);
    double dCosA=dCos*m_dA;
    double dSinB=dSin*m_dB;

    if(Pos)
        {
        Pos->m_x=m_Center[0]+m_XAxis.X()*dCosA+m_YAxis.X()*dSinB;
        Pos->m_y=m_Center[1]+m_XAxis.Y()*dCosA+m_YAxis.Y()*dSinB;
        Pos->m_z=m_Center[2]+m_XAxis.Z()*dCosA+m_YAxis.Z()*dSinB;
        }
    if(D1)
        {
        double dCosB=dCos*m_dB;
        double dSinA=dSin*m_dA;
        D1->m_x=m_YAxis.X()*dCosB-m_XAxis.X()*dSinA;
        D1->m_y=m_YAxis.Y()*dCosB-m_XAxis.Y()*dSinA;
        D1->m_z=m_YAxis.Z()*dCosB-m_XAxis.Z()*dSinA;
        }
    if(D2)
        {
        D2->m_x=-m_XAxis.X()*dCosA-m_YAxis.X()*dSinB;
        D2->m_y=-m_XAxis.Y()*dCosA-m_YAxis.Y()*dSinB;
        D2->m_z=-m_XAxis.Z()*dCosA-m_YAxis.Z()*dSinB;
        }
    }

double ellipse::Inverse(SGM::Point3D const &Pos,
                        SGM::Point3D       *ClosePos,
                        double       const *pGuess) const
    {
    SGM::Point3D const &Center=m_Center;
    SGM::UnitVector3D const &XVec=m_XAxis;
    SGM::UnitVector3D const &YVec=m_YAxis;
    SGM::Vector3D Vec=Pos-Center;
    double dx=XVec%Vec;
    double dy=YVec%Vec;
    double dParam=SGM::SAFEatan2(dy,dx);
    while(dParam<m_Domain.m_dMin)
        {
        dParam+=SGM_TWO_PI;
        }
    while(m_Domain.m_dMax<dParam)
        {
        dParam-=SGM_TWO_PI;
        }
    double dAnswer=NewtonsMethod(dParam,Pos);
    while(dAnswer<m_Domain.m_dMin)
        {
        dAnswer+=SGM_TWO_PI;
        }
    while(m_Domain.m_dMax<dAnswer)
        {
        dAnswer-=SGM_TWO_PI;
        }
    if(ClosePos)
        {
        Evaluate(dAnswer,ClosePos);
        }
    if(pGuess)
        {
        if(SGM::NearEqual(dAnswer,m_Domain.m_dMin,SGM_MIN_TOL,false) &&
           SGM::NearEqual(*pGuess, m_Domain.m_dMax, SGM_MIN_TOL, false))
            {
            dAnswer=*pGuess;
            }
        else if(SGM::NearEqual(dAnswer, m_Domain.m_dMax, SGM_MIN_TOL, false) &&
                SGM::NearEqual(*pGuess, m_Domain.m_dMin, SGM_MIN_TOL, false))
            {
            dAnswer=*pGuess;
            }
        }
    return dAnswer;
    }

void ellipse::Transform(SGM::Result            &,//rResult,
                        SGM::Transform3D const &Trans)
    {
    m_Center=Trans*m_Center;
    m_XAxis=Trans*m_XAxis;
    m_YAxis=Trans*m_YAxis;
    m_Normal=Trans*m_Normal;
    if(double dScale=Trans.Scale())
        {
        m_dA*=dScale;
        m_dB*=dScale;
        }
    }

}
