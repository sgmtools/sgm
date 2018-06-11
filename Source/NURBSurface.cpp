#include "SGMDataClasses.h"
#include "EntityClasses.h"
#include "Surface.h"
#include "Curve.h"
#include "Faceter.h"

namespace SGMInternal
{
NURBsurface::NURBsurface(SGM::Result                                   &rResult,
                         std::vector<std::vector<SGM::Point4D> > const &aControlPoints,
                         std::vector<double>                     const &aUKnots,
                         std::vector<double>                     const &aVKnots):
    surface(rResult,SGM::NURBSurfaceType),m_aaControlPoints(aControlPoints),m_aUKnots(aUKnots),m_aVKnots(aVKnots)
    {
    m_Domain.m_UDomain.m_dMin=aUKnots.front();
    m_Domain.m_UDomain.m_dMax=aUKnots.back();
    m_Domain.m_VDomain.m_dMin=aUKnots.front();
    m_Domain.m_VDomain.m_dMax=aUKnots.back();

    curve *pUCurve=UParamLine(rResult,m_Domain.m_UDomain.MidPoint(0.25));
    curve *pVCurve=VParamLine(rResult,m_Domain.m_VDomain.MidPoint(0.25));
    FacetOptions Options;
    Options.m_dEdgeAngleTol=0.17453292519943295769236907684886; // 10 degrees.
    std::vector<double> aUParams,aVParams;
    std::vector<SGM::Point3D> aUPoints,aVPoints;
    FacetCurve(pUCurve,pUCurve->GetDomain(),Options,aUPoints,aUParams);
    FacetCurve(pVCurve,pVCurve->GetDomain(),Options,aVPoints,aVParams);
    rResult.GetThing()->DeleteEntity(pUCurve);
    rResult.GetThing()->DeleteEntity(pVCurve);
    size_t Index1,Index2;
    m_nUParams=aUParams.size();
    m_nVParams=aVParams.size();
    size_t nParams=m_nUParams*m_nVParams;
    m_aSeedParams.reserve(nParams);
    m_aSeedPoints.reserve(nParams);
    for(Index1=0;Index1<m_nUParams;++Index1)
        {
        double u=aUParams[Index1];
        for(Index2=0;Index2<m_nVParams;++Index2)
            {
            double v=aVParams[Index2];
            SGM::Point2D uv(u,v);
            m_aSeedParams.push_back(uv);
            SGM::Point3D Pos;
            Evaluate(uv,&Pos);
            m_aSeedPoints.push_back(Pos);
            }
        }
    }

std::vector<SGM::Point3D> const &NURBsurface::GetSeedPoints() const
    {
    return m_aSeedPoints;
    }

std::vector<SGM::Point2D> const &NURBsurface::GetSeedParams() const
    {
    return m_aSeedParams;
    }
}