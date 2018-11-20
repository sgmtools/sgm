#include "Mathematics.h"
#include "SGMConstants.h"
#include "SGMVector.h"

namespace SGMInternal
{

///////////////////////////////////////////////////////////////////////////////
//
//  SortablePlane
//
///////////////////////////////////////////////////////////////////////////////

SortablePlane::SortablePlane(std::vector<SGM::Point3D> const &aPoints)
    {
    SGM::Point3D Origin,Zero(0,0,0);
    SGM::UnitVector3D XVec,YVec,ZVec;
    if(SGM::FindLeastSquarePlane(aPoints,Origin,XVec,YVec,ZVec))
        {
        SGM::Point3D XYZ=Zero-ZVec*((Zero-Origin)%ZVec);
        double dDist=Zero.Distance(XYZ);
        if((Origin-Zero)%ZVec<0)
            {
            ZVec.Negate();
            }
        aData[0]=ZVec.m_x;
        aData[1]=ZVec.m_y;
        aData[2]=ZVec.m_z;
        aData[3]=dDist;
        double dTol=SGM_ZERO;
        size_t nPoints=aPoints.size();
        size_t Index1;
        for(Index1=0;Index1<nPoints;++Index1)
            {
            double dOffPlane=fabs((aPoints[Index1]-XYZ)%ZVec);
            if(dTol<dOffPlane)
                {
                dTol=dOffPlane;
                }
            }
        aData[4]=dTol;
        bool bFlip=false;
        if(aData[3]<dTol)
            {
            if(fabs(aData[0])<-dTol)
                {
                bFlip=true;
                }
            else if(aData[0]<dTol)
                {
                if(fabs(aData[1])<-dTol)
                    {
                    bFlip=true;
                    }
                else if(aData[1]<dTol)
                    {
                    if(fabs(aData[2])<-dTol)
                        {
                        bFlip=true;
                        }
                    }
                }
            }
        if(bFlip)
            {
            aData[0]=-aData[0];
            aData[1]=-aData[1];
            aData[2]=-aData[2];
            aData[3]=-aData[3];
            }
        }
    else
        {
        aData[0]=0;
        aData[1]=0;
        aData[2]=0;
        aData[3]=0;
        aData[4]=0;
        }
    }

bool SortablePlane::Parallel(SortablePlane const &Other,
                             SGM::Vector3D       &Offset,
                             double               dTolerance) const
    {
    SGM::UnitVector3D Norm0=Normal();
    SGM::UnitVector3D Norm1=Other.Normal();
    if(fabs(Norm0%Norm1-1.0)<dTolerance)
        {
        double dOffset=Other.aData[3]-aData[3];
        Offset=Norm0*dOffset;
        return true;
        }
    return false;
    }

void SortablePlane::SetMinTolerance(double dMinTolerance)
    {
    if(aData[4]>dMinTolerance)
        {
        aData[4]=dMinTolerance;
        }
    }

bool SortablePlane::operator==(SortablePlane const &SPlane) const
    {
    if(*this<SPlane)
        {
        return false;
        }
    else if(SPlane<*this)
        {
        return false;
        }
    return true;
    }

bool SortablePlane::operator<(SortablePlane const &SPlane) const
    {
    double dTol=aData[4];
    if(aData[0]<SPlane.aData[0]-dTol)
        {
        return true;
        }
    else if(SPlane.aData[0]<aData[0]-dTol)
        {
        return false;
        }
    else
        {
        if(aData[1]<SPlane.aData[1]-dTol)
            {
            return true;
            }
        else if(SPlane.aData[1]<aData[1]-dTol)
            {
            return false;
            }
        else
            {
            if(aData[2]<SPlane.aData[2]-dTol)
                {
                return true;
                }
            else if(SPlane.aData[2]<aData[2]-dTol)
                {
                return false;
                }
            else
                {
                if(aData[3]<SPlane.aData[3]-dTol)
                    {
                    return true;
                    }
                return false;
                }
            }
        }
    }

SGM::Point3D SortablePlane::Origin() const
    {
    SGM::UnitVector3D Normal(aData[0],aData[1],aData[2]);
    SGM::Point3D Zero(0,0,0);
    return Zero+Normal*aData[3];
    }

SGM::UnitVector3D SortablePlane::Normal() const
    {
    return SGM::UnitVector3D(aData[0],aData[1],aData[2]);
    }

double SortablePlane::Tolerance() const
    {
    return aData[4];
    }

///////////////////////////////////////////////////////////////////////////////

//SGM::Vector3D Snap(SGM::Vector3D const &Vec)
//    {
//    SGM::Vector3D Answer=Vec;
//    if(fabs(Vec.m_x)<SGM_ZERO)
//        {
//        Answer.m_x=0.0;
//        }
//    if(fabs(Vec.m_y)<SGM_ZERO)
//        {
//        Answer.m_y=0.0;
//        }
//    if(fabs(Vec.m_z)<SGM_ZERO)
//        {
//        Answer.m_z=0.0;
//        }
//    return Answer;
//    }

///////////////////////////////////////////////////////////////////////////////
//
//  Internal Integration Functions
//
///////////////////////////////////////////////////////////////////////////////

typedef double (*SGMIntegrand)(SGM::Point2D const &uv,void const *pData);

class Integrate2DData
    {
    public:

        SGMIntegrand     m_f;
        double           m_y;
        double           m_dTolerance;
        void            *m_Data;
        SGM::Interval1D  m_Domain;
    };

double Integrate2DFx(double x,void const *pData)
    {
    Integrate2DData *XYData=(Integrate2DData *)pData;
    SGM::Point2D uv(x,XYData->m_y);
    return XYData->m_f(uv,XYData->m_Data);
    }

double Integrate2DFy(double y,void const *pData)
    {
    Integrate2DData *pSubData=(Integrate2DData *)pData;
    pSubData->m_y=y;
    return Integrate1D(Integrate2DFx,pSubData->m_Domain,pData,pSubData->m_dTolerance);
    }

class IntegrateTetraData
    {
    public:

        SGMIntegrand  m_f;
        double        m_y;
        double        m_dTolerance;
        void         *m_Data;
        SGM::Point2D  m_PosA;
        SGM::Point2D  m_PosB;
        SGM::Point2D  m_PosC;
        SGM::Point2D  m_PosD;
    };

double IntegrateTetraX(double x,void const *pData)
    {
    IntegrateTetraData *XYData=(IntegrateTetraData *)pData;
    SGM::Point2D uv(x,XYData->m_y);
    return XYData->m_f(uv,XYData->m_Data);
    }

double IntegrateTetraY(double y,void const *pData)
    {
    IntegrateTetraData *pSubData=(IntegrateTetraData *)pData;
    pSubData->m_y=y;
    SGM::Point2D const &PosA=pSubData->m_PosA;
    SGM::Point2D const &PosB=pSubData->m_PosB;
    SGM::Point2D const &PosC=pSubData->m_PosC;
    SGM::Point2D const &PosD=pSubData->m_PosD;
    double dFraction=(y-PosC.m_v)/(PosA.m_v-PosC.m_v);
    double x0=SGM::MidPoint(PosA,PosC,dFraction).m_u;
    double x1=SGM::MidPoint(PosB,PosD,dFraction).m_u;
    SGM::Interval1D Domain(x0,x1);
    return Integrate1D(IntegrateTetraX,Domain,pData,pSubData->m_dTolerance);
    }

double IntegrateTetra(double f(SGM::Point2D const &uv,void const *pData),
                      SGM::Point2D                         const &PosA,
                      SGM::Point2D                         const &PosB,
                      SGM::Point2D                         const &PosC,
                      SGM::Point2D                         const &PosD,
                      void                                 const *pData,
                      double                                      dTolerance)
    {
    //   A----------B         _ay  _Line(d,b)
    //    \        /         /    /
    //     \      /        _/   _/         f(x,y)dx,dy
    //      C----D        cy   Line(a,c)

    //     _Ay _Line(d,b)        _Ay
    //    /   /                 /
    //  _/  _/  f(x,y)dx,dy = _/  Integrate1D(f(x),Line(a,c),Line(d,b))(y) =
    //  Cy  Line(a,c)         Cy
    //
    //  Integrate1D(Integrate1D(f(x),Line(a,c),Line(d,b))(y),Cy,Ay)

    SGMInternal::IntegrateTetraData XYData;
    XYData.m_Data=(void *)pData;
    XYData.m_PosA=PosA;
    XYData.m_PosB=PosB;
    XYData.m_PosC=PosC;
    XYData.m_PosD=PosD;

    if(PosB.m_u<PosA.m_u)
        {
        std::swap(XYData.m_PosA,XYData.m_PosB);
        }
    if(PosD.m_u<PosC.m_u)
        {
        std::swap(XYData.m_PosC,XYData.m_PosD);
        }

    XYData.m_dTolerance=dTolerance;
    XYData.m_f=f;
    SGM::Interval1D Domain(PosC.m_v,PosA.m_v);
    return Integrate1D(SGMInternal::IntegrateTetraY,Domain,&XYData,dTolerance);
    }

double Integrate1D(double f(double x, void const *pData),
                   SGM::Interval1D const &Domain,
                   void const *pData,
                   double dTolerance)
    {
    std::vector<std::vector<double> > aaData;
    double dAnswer = 0;
    double dh = Domain.Length();
    double a = Domain.m_dMin;
    double b = Domain.m_dMax;
    std::vector<double> aData;
    aData.push_back(0.5 * dh * (f(a, pData) + f(b, pData)));
    aaData.push_back(aData);
    double dError = dTolerance + 1;
    size_t Index1 = 0, Index2;
    size_t nMin = 3;
    while (Index1 < nMin || dTolerance < dError)
        {
        ++Index1;
        dh *= 0.5;
        double dSum = 0.0;
        size_t nBound = (size_t) pow(2, Index1);
        for (Index2 = 1; Index2 < nBound; Index2 += 2)
            {
            dSum += f(a + Index2 * dh, pData);
            }
        aData.clear();
        aData.push_back(0.5 * aaData[Index1 - 1][0] + dSum * dh);
        for (Index2 = 1; Index2 <= Index1; ++Index2)
            {
            aData.push_back(aData[Index2 - 1] +
                            (aData[Index2 - 1] - aaData[Index1 - 1][Index2 - 1]) / (pow(4, Index2) - 1));
            }
        aaData.push_back(aData);
        dAnswer = aData.back();
        dError = fabs(aaData[Index1 - 1].back() - dAnswer);
        }
    return dAnswer;
    }

double Integrate2D(double f(SGM::Point2D const &uv, void const *pData),
                   SGM::Interval2D const &Domain,
                   void const *pData,
                   double dTolerance)
    {
    //     _b  _d                _b
    //    /   /                 /  
    //  _/  _/  f(x,y)dx,dy = _/  Integrate1D(f(x),c,d)(y) = Integrate1D(Integrate1D(f(x),c,d)(y),a,b)
    //  a   c                 a   

    SGMInternal::Integrate2DData XYData;
    XYData.m_Data = (void *) pData;
    XYData.m_Domain = Domain.m_UDomain;
    XYData.m_dTolerance = dTolerance;
    XYData.m_f = f;
    return Integrate1D(SGMInternal::Integrate2DFy, Domain.m_VDomain, &XYData, dTolerance);
    }

double IntegrateTriangle(double f(SGM::Point2D const &uv, void const *pData),
                         SGM::Point2D const &PosA,
                         SGM::Point2D const &PosB,
                         SGM::Point2D const &PosC,
                         void const *pData,
                         double dTolerance)
    {
    //  With A being the highest left point and C being the lowest right point 
    //  there are four case as follows;
    //
    //   Case 1
    //
    //       A   
    //     / |         _by  _Line(a,c)                _ay  _Line(a,c)         
    //   B   |        /    /                         /    /            
    //     \ |      _/   _/         f(x,y)dx,dy +  _/   _/         f(x,y)dx,dy
    //       C      cy   Line(b,c)                 by   Line(b,a)    
    //
    //   OR Case 2
    //
    //   A        
    //   | \           _by  _Line(b,c)                _ay  _Line(a,b)         
    //   |   B        /    /                         /    /            
    //   | /        _/   _/         f(x,y)dx,dy +  _/   _/         f(x,y)dx,dy
    //   C          cy   Line(a,c)                 by   Line(a,c)    
    //
    //   OR Case 3
    //
    //   A              
    //   | \              _ay  _Line(a,c)         
    //   |   \           /    /                   
    //   |     \       _/   _/         f(x,y)dx,dy
    //   B-------C     cy   Line(a,b)             
    //
    //   OR Case 4
    //
    //   A-------B
    //   |     /         _ay  _Line(b,c)         
    //   |   /          /    /                   
    //   | /          _/   _/         f(x,y)dx,dy
    //   C            cy   Line(a,c)             

    // Find the three points.

    SGM::Point2D PosHigh, PosLow, PosMid;
    if (PosB.m_v <= PosA.m_v && PosC.m_v <= PosA.m_v)
        {
        PosHigh = PosA;
        if (PosB.m_v <= PosC.m_v)
            {
            PosLow = PosB;
            PosMid = PosC;
            }
        else
            {
            PosLow = PosC;
            PosMid = PosB;
            }
        }
    else if (PosA.m_v <= PosB.m_v && PosC.m_v <= PosB.m_v)
        {
        PosHigh = PosB;
        if (PosA.m_v <= PosC.m_v)
            {
            PosLow = PosA;
            PosMid = PosC;
            }
        else
            {
            PosLow = PosC;
            PosMid = PosA;
            }
        }
    else
        {
        PosHigh = PosC;
        if (PosB.m_v <= PosA.m_v)
            {
            PosLow = PosB;
            PosMid = PosA;
            }
        else
            {
            PosLow = PosA;
            PosMid = PosB;
            }
        }

    // Figure out which case we have and call IntegrateTetra one or two times.

    //   A----------B         _ay  _Line(d,b)          
    //    \        /         /    /                    
    //     \      /        _/   _/         f(x,y)dx,dy 
    //      C----D        cy   Line(a,c)    

    double dAnswer;
    if (PosHigh.m_v == PosMid.m_v)
        {
        // Case 4

        dAnswer = SGMInternal::IntegrateTetra(f, PosHigh, PosMid, PosLow, PosLow, pData, dTolerance);
        }
    else if (PosLow.m_v == PosMid.m_v)
        {
        // Case 3

        dAnswer = SGMInternal::IntegrateTetra(f, PosHigh, PosHigh, PosLow, PosMid, pData, dTolerance);
        }
    else
        {
        // Case 1 or 2

        double dFraction = (PosMid.m_v - PosLow.m_v) / (PosHigh.m_v - PosLow.m_v);
        SGM::Point2D PosMidLine = MidPoint(PosLow, PosHigh, dFraction);
        dAnswer = SGMInternal::IntegrateTetra(f, PosHigh, PosHigh, PosMid, PosMidLine, pData, dTolerance) +
                  SGMInternal::IntegrateTetra(f, PosMid, PosMidLine, PosLow, PosLow, pData, dTolerance);
        }

    return dAnswer;
    }


} // End namespace SGMInternal