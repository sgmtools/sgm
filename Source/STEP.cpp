#include "SGMVector.h"
#include "SGMTranslators.h"
#include "SGMModify.h"

#include "EntityClasses.h"
#include "Topology.h"
#include "Surface.h"
#include "Curve.h"
#include "FileFunctions.h"

#include <string>

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#else
#define snprintf snprintf
#endif

// Lets us use fprintf
#ifdef _MSC_VER
__pragma(warning(disable: 4996 ))
__pragma(warning(disable: 4477 ))
#endif

namespace SGMInternal
{
void WriteVertices(FILE                     *pFile,
                   size_t                   &nLine,
                   std::set<vertex *,EntityCompare> const &sVertices,
                   std::map<size_t,size_t>  &mVertexMap)
    {
    for (auto pVertex : sVertices)
        {
        SGM::Point3D const &Pos=pVertex->GetPoint();
        size_t nPos=nLine;
        fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
        mVertexMap[pVertex->GetID()]=nLine;
        fprintf(pFile,"#%lu=VERTEX_POINT('',#%lu);\n",nLine++,nPos);
        }
    }

void WriteLine(FILE                    *pFile,
               size_t                  &nLine,
               line              const *pLine,
               std::map<size_t,size_t> &mCurveMap)
    {
    // #24=CARTESIAN_POINT('',(1.00000000000000,2.00000000000000,3.00000000000000));
    // #25=DIRECTION('',(0.577350269189626,0.577350269189626,0.577350269189626));
    // #26=VECTOR('',#25,1.00000000000000);
    // #27=LINE('',#24,#26);

    SGM::Point3D const &Pos=pLine->GetOrigin();
    SGM::UnitVector3D const &Axis=pLine->GetAxis();

    size_t nPos=nLine;
    fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nDirection=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Axis.X(),Axis.Y(),Axis.Z());
    size_t nVector=nLine;
    fprintf(pFile,"#%lu=VECTOR('',#%lu,%#.15G);\n",nLine++,nDirection,1.0);
    mCurveMap[pLine->GetID()]=nLine;
    fprintf(pFile,"#%lu=LINE('',#%lu,#%lu);\n",nLine++,nPos,nVector);
    }

void WriteCircle(FILE                    *pFile,
                 size_t                  &nLine,
                 circle            const *pCircle,
                 std::map<size_t,size_t> &mCurveMap)
    {
    // #24=CARTESIAN_POINT('',(1.00000000000000,2.00000000000000,3.00000000000000));
    // #25=DIRECTION('',(0.000000000000000,0.000000000000000,1.00000000000000));
    // #26=DIRECTION('',(1.00000000000000,0.000000000000000,0.000000000000000));
    // #27=AXIS2_PLACEMENT_3D('',#24,#25,#26);
    // #28=CIRCLE('',#27,5.00000000000000);

    SGM::Point3D const &Center=pCircle->GetCenter();
    SGM::UnitVector3D const &XVec=pCircle->GetXAxis();
    SGM::UnitVector3D const &ZVec=pCircle->GetNormal();
    double dRadius=pCircle->GetRadius();

    size_t nPos=nLine;
    fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Center.m_x,Center.m_y,Center.m_z);
    size_t nDirection1=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,ZVec.X(),ZVec.Y(),ZVec.Z());
    size_t nDirection2=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XVec.X(),XVec.Y(),XVec.Z());
    size_t nAxis=nLine;
    fprintf(pFile,"#%lu=AXIS2_PLACEMENT_3D('',#%lu,#%lu,#%lu);\n",nLine++,nPos,nDirection1,nDirection2);
    mCurveMap[pCircle->GetID()]=nLine;
    fprintf(pFile,"#%lu=CIRCLE('',#%lu,%#.15G);\n",nLine++,nAxis,dRadius);
    }

void WriteNUBCurve(FILE                    *pFile,
                   size_t                  &nLine,
                   NUBcurve          const *pNUBCurve,
                   std::map<size_t,size_t> &mCurveMap)
    {
    // #24=CARTESIAN_POINT('',(1.00000000000000,1.00000000000000,0.000000000000000));
    // #25=CARTESIAN_POINT('',(1.16666666666667,1.16666666666667,0.000000000000000));
    // #26=CARTESIAN_POINT('',(2.00000000000000,2.83333333333333,0.000000000000000));
    // #27=CARTESIAN_POINT('',(2.83333333333333,1.16666666666667,0.000000000000000));
    // #28=CARTESIAN_POINT('',(3.00000000000000,1.00000000000000,0.000000000000000));
    // #29=B_SPLINE_CURVE_WITH_KNOTS('',3,(#24,#25,#26,#27,#28),.UNSPECIFIED.,.F.,.F.,(4,1,4),(0.000000000000000,0.500000000000000,1.00000000000000),.UNSPECIFIED.);
    // Degree (Control Points) Curve Form, Closed, Self Intersect, (Multiplicity) (Knots) Knot Type.
    
    size_t nDegree=pNUBCurve->GetDegree();
    std::vector<SGM::Point3D> const &aControlPoints=pNUBCurve->GetControlPoints();
    std::vector<double> aUniqueKnots;
    std::vector<int> aMultiplicity;
    size_t nUniqueKnots= pNUBCurve->FindMultiplicity(aMultiplicity, aUniqueKnots);

    // Write out the control points.

    size_t Index1;
    size_t nControlPoints=aControlPoints.size();
    std::string sControlPoints="(";
    for(Index1=0;Index1<nControlPoints;++Index1)
        {
        SGM::Point3D const &Pos=aControlPoints[Index1];
        size_t nPos=nLine;
        fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
        char Arg[10];
        snprintf(Arg,sizeof(Arg),"#%lu",nPos);
        sControlPoints+=Arg;
        if(Index1<nControlPoints-1)
            {
            sControlPoints+=",";
            }
        }
    sControlPoints+=")";

    // Set up the multiplity string.

    std::string sMultiplicity="(";
    for(Index1=0;Index1<nUniqueKnots;++Index1)
        {
        char Arg[100];
        snprintf(Arg,sizeof(Arg),"%ld",(long)(aMultiplicity[Index1]));
        sMultiplicity+=Arg;
        if(Index1<nUniqueKnots-1)
            {
            sMultiplicity+=",";
            }
        }
    sMultiplicity+=")";

    // Set up the unique knot string.

    std::string sUniqueKnots="(";
    for(Index1=0;Index1<nUniqueKnots;++Index1)
        {
        char Arg[100];
        snprintf(Arg,sizeof(Arg),"%#.15G",aUniqueKnots[Index1]);
        sUniqueKnots+=Arg;
        if(Index1<nUniqueKnots-1)
            {
            sUniqueKnots+=",";
            }
        }
    sUniqueKnots+=")";

    // Write out the b-spline

    mCurveMap[pNUBCurve->GetID()]=nLine;
    fprintf(pFile,"#%lu=B_SPLINE_CURVE_WITH_KNOTS('',%ld,%s,.UNSPECIFIED.,.F.,.F.,%s,%s,.UNSPECIFIED.);\n",
        nLine++,(long)nDegree,sControlPoints.c_str(),sMultiplicity.c_str(),sUniqueKnots.c_str());
    }

void WriteCurves(FILE                          *pFile,
                 size_t                        &nLine,
                 std::set<curve const *> const &sCurves,
                 std::map<size_t,size_t>       &mCurveMap)
    {
    for (auto pCurve : sCurves)
        {
        switch(pCurve->GetCurveType())
            {
            case SGM::EntityType::LineType :
                {
                WriteLine(pFile,nLine,(line const *)pCurve,mCurveMap);
                break;
                }
            case SGM::EntityType::CircleType :
                {
                WriteCircle(pFile,nLine,(circle const *)pCurve,mCurveMap);
                break;
                }
            case SGM::EntityType::NUBCurveType :
                {
                WriteNUBCurve(pFile,nLine,(NUBcurve const *)pCurve,mCurveMap);
                break;
                }
            default:
                {
                throw;
                }
            }
        }
    }

void WriteCoedges(FILE                                      *pFile,
                  size_t                                    &nLine,
                  std::set<edge *,EntityCompare>                    const &sEdges,
                  std::map<size_t,size_t>                   &mVertexMap,
                  std::map<size_t,size_t>                   &mCurveMap,
                  std::map<std::pair<size_t,size_t>,size_t> &mCoedgeMap,
                  std::map<size_t,size_t>                   &mEdgeMap)
    {
    for (auto pEdge : sEdges)
        {
        curve const *pCurve=pEdge->GetCurve();
        size_t nCurve=mCurveMap[pCurve->GetID()];
        size_t nEdgeCurve=nLine;
        volume *pVolume=pEdge->GetVolume();

        if(pVolume==nullptr)
            {
            // #55=EDGE_CURVE('',#44,#25,#54,.T.);
            // #56=ORIENTED_EDGE('',*,*,#55,.T.);

            vertex *pStart=pEdge->GetStart();
            vertex *pEnd=pEdge->GetEnd();
            if (nullptr == pStart)
                {
                fprintf(pFile,"#%lu=EDGE_CURVE('',*,*,#%lu,.T.);\n",nLine++,nCurve);
                }
            else
                {
                size_t nStart=mVertexMap[pStart->GetID()];
                size_t nEnd=mVertexMap[pEnd->GetID()];
                fprintf(pFile,"#%lu=EDGE_CURVE('',#%lu,#%lu,#%lu,.T.);\n",nLine++,nStart,nEnd,nCurve);
                }
            std::set<face *,EntityCompare> const &sFaces=pEdge->GetFaces();
            for (auto const *pFace : sFaces)
                {
                SGM::EdgeSideType bFlipped=pFace->GetSideType(pEdge);
                mCoedgeMap[std::pair<size_t,size_t>(pEdge->GetID(),pFace->GetID())]=nLine;
                fprintf(pFile,"#%lu=ORIENTED_EDGE('',*,*,#%lu,.%c.));\n",nLine++,nEdgeCurve,bFlipped==SGM::FaceOnRightType ? 'F' : 'T');
                }
            }
        else
            {
            // #28=TRIMMED_CURVE('',#27,(PARAMETER_VALUE(0.000000000000000)),(PARAMETER_VALUE(5.19615242270663)),.T.,.UNSPECIFIED.);
            // #29=GEOMETRIC_CURVE_SET(' ',(#28));

            double dStart=pEdge->GetDomain().m_dMin;
            double dEnd=pEdge->GetDomain().m_dMax;
            mEdgeMap[pEdge->GetID()]=nLine;
            fprintf(pFile,"#%lu=TRIMMED_CURVE('',#%lu,(PARAMETER_VALUE(%#.15G)),(PARAMETER_VALUE(%#.15G)),.T.,.UNSPECIFIED.);\n",nLine++,nCurve,dStart,dEnd);
            }
        }
    }

void WritePlane(FILE                    *pFile,
                size_t                  &nLine,
                plane             const *pPlane,
                std::map<size_t,size_t> &mSurfaceMap)
    {
    // #114=CARTESIAN_POINT('',(0.000000000000000,10.0000000000000,0.000000000000000));
    // #115=DIRECTION('',(-1.00000000000000,0.000000000000000,0.000000000000000));
    // #116=DIRECTION('',(0.000000000000000,1.00000000000000,0.000000000000000));
    // #117=AXIS2_PLACEMENT_3D('',#114,#115,#116);
    // #118=PLANE('',#117);

    SGM::Point3D const &Pos=pPlane->m_Origin;
    SGM::UnitVector3D const &Norm=pPlane->m_ZAxis;
    SGM::UnitVector3D const &XAxis=pPlane->m_XAxis;
    
    size_t nPos=nLine;
    fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.X(),Norm.Y(),Norm.Z());
    size_t nXAxis=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.X(),XAxis.Y(),XAxis.Z());
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%lu=AXIS2_PLACEMENT_3D('',#%lu,#%lu,#%lu);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pPlane->GetID()]=nLine;
    fprintf(pFile,"#%lu=PLANE('',#%lu);\n",nLine++,nAxis3D);
    }

void WriteSphere(FILE                    *pFile,
                 size_t                  &nLine,
                 sphere            const *pSphere,
                 std::map<size_t,size_t> &mSurfaceMap)
    {
    // #38=CARTESIAN_POINT('',(1.00000000000000,2.00000000000000,3.00000000000000));
    // #39=DIRECTION('',(0.000000000000000,0.000000000000000,1.00000000000000));
    // #40=DIRECTION('',(1.00000000000000,0.000000000000000,0.000000000000000));
    // #41=AXIS2_PLACEMENT_3D('',#38,#39,#40);
    // #42=SPHERICAL_SURFACE('',#41,4.00000000000000);

    SGM::Point3D const &Pos=pSphere->m_Center;
    SGM::UnitVector3D const &Norm=pSphere->m_ZAxis;
    SGM::UnitVector3D const &XAxis=pSphere->m_XAxis;
    double dRadius=pSphere->m_dRadius;

    size_t nPos=nLine;
    fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.X(),Norm.Y(),Norm.Z());
    size_t nXAxis=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.X(),XAxis.Y(),XAxis.Z());
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%lu=AXIS2_PLACEMENT_3D('',#%lu,#%lu,#%lu);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pSphere->GetID()]=nLine;
    fprintf(pFile,"#%lu=SPHERICAL_SURFACE('',#%lu,%#.15G);\n",nLine++,nAxis3D,dRadius);
    }

void WriteCylinder(FILE                    *pFile,
                   size_t                  &nLine,
                   cylinder          const *pCylinder,
                   std::map<size_t,size_t> &mSurfaceMap)
    {
    // #139=CARTESIAN_POINT('',(0.000000000000000,5.00000000000000,0.000000000000000));
    // #140=DIRECTION('',(0.000000000000000,0.000000000000000,1.00000000000000));
    // #141=DIRECTION('',(-1.00000000000000,0.000000000000000,0.000000000000000));
    // #142=AXIS2_PLACEMENT_3D('',#139,#140,#141);
    // #143=CYLINDRICAL_SURFACE('',#142,9.00000000000000);

    SGM::Point3D const &Pos=pCylinder->m_Origin;
    SGM::UnitVector3D const &Norm=pCylinder->m_ZAxis;
    SGM::UnitVector3D const &XAxis=pCylinder->m_XAxis;
    double dRadius=pCylinder->m_dRadius;

    size_t nPos=nLine;
    fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.X(),Norm.Y(),Norm.Z());
    size_t nXAxis=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.X(),XAxis.Y(),XAxis.Z());
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%lu=AXIS2_PLACEMENT_3D('',#%lu,#%lu,#%lu);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pCylinder->GetID()]=nLine;
    fprintf(pFile,"#%lu=CYLINDRICAL_SURFACE('',#%lu,%#.15G);\n",nLine++,nAxis3D,dRadius);
    }

void WriteTorus(FILE                    *pFile,
                size_t                  &nLine,
                torus             const *pTorus,
                std::map<size_t,size_t> &mSurfaceMap)
    {
    // Donut or Pinched
    // #44=CARTESIAN_POINT('',(0.000000000000000,0.000000000000000,0.000000000000000));
    // #45=DIRECTION('',(0.000000000000000,0.000000000000000,1.00000000000000));
    // #46=DIRECTION('',(-1.00000000000000,0.000000000000000,0.000000000000000));
    // #47=AXIS2_PLACEMENT_3D('',#44,#45,#46);
    // #48=TOROIDAL_SURFACE('',#47,5.00000000000000,1.00000000000000);

    // Lemon
    // #38=CARTESIAN_POINT('',(0.000000000000000,0.000000000000000,0.000000000000000));
    // #39=DIRECTION('',(0.000000000000000,0.000000000000000,1.00000000000000));
    // #40=DIRECTION('',(-1.00000000000000,0.000000000000000,0.000000000000000));
    // #41=AXIS2_PLACEMENT_3D('',#38,#39,#40);
    // #42=DEGENERATE_TOROIDAL_SURFACE('',#41,0.500000000000000,1.00000000000000,.F.);
    // .F. for Lemon .T. for Apple.

    SGM::Point3D const &Pos=pTorus->m_Center;
    SGM::UnitVector3D const &Norm=pTorus->m_ZAxis;
    SGM::UnitVector3D const &XAxis=pTorus->m_XAxis;
    double dMinorRadius=pTorus->m_dMinorRadius;
    double dMajorRadius=pTorus->m_dMajorRadius;
    SGM::TorusKindType nKind=pTorus->GetKind();

    size_t nPos=nLine;
    fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.X(),Norm.Y(),Norm.Z());
    size_t nXAxis=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.X(),XAxis.Y(),XAxis.Z());
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%lu=AXIS2_PLACEMENT_3D('',#%lu,#%lu,#%lu);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pTorus->GetID()]=nLine;
    if(nKind==SGM::TorusKindType::DonutType || nKind==SGM::TorusKindType::PinchedType)
        {
        fprintf(pFile,"#%lu=TOROIDAL_SURFACE('',#%lu,%#.15G,%#.15G);\n",nLine++,nAxis3D,dMajorRadius,dMinorRadius);
        }
    else if(nKind==SGM::TorusKindType::AppleType)
        {
        fprintf(pFile,"#%lu=DEGENERATE_TOROIDAL_SURFACE('',#%lu,%#.15G,%#.15G,.T.);\n",nLine++,nAxis3D,dMajorRadius,dMinorRadius);
        }
    else // (nKind==SGM::TorusKindType::LemonType)
        {
        fprintf(pFile,"#%lu=DEGENERATE_TOROIDAL_SURFACE('',#%lu,%#.15G,%#.15G,.F.);\n",nLine++,nAxis3D,dMajorRadius,dMinorRadius);
        }
    }

void WriteCone(FILE                    *pFile,
               size_t                  &nLine,
               cone              const *pCone,
               std::map<size_t,size_t> &mSurfaceMap)
    {
    // #44=CARTESIAN_POINT('',(0.000000000000000,0.000000000000000,0.000000000000000));
    // #45=DIRECTION('',(-0.000000000000000,-0.000000000000000,-1.00000000000000));
    // #46=DIRECTION('',(-1.00000000000000,0.000000000000000,0.000000000000000));
    // #47=AXIS2_PLACEMENT_3D('',#44,#45,#46);
    // #48=CONICAL_SURFACE('',#47,1.00000000000000,0.785398163397448);

    SGM::Point3D const &Pos=pCone->m_Origin;
    SGM::UnitVector3D const &Norm=pCone->m_ZAxis;
    SGM::UnitVector3D const &XAxis=pCone->m_XAxis;
    double dRadius=pCone->m_dRadius;
    double dHalfAngle=pCone->FindHalfAngle();

    size_t nPos=nLine;
    fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.X(),Norm.Y(),Norm.Z());
    size_t nXAxis=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.X(),XAxis.Y(),XAxis.Z());
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%lu=AXIS2_PLACEMENT_3D('',#%lu,#%lu,#%lu);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pCone->GetID()]=nLine;
    fprintf(pFile,"#%lu=CONICAL_SURFACE('',#%lu,%#.15G,%#.15G);\n",nLine++,nAxis3D,dRadius,dHalfAngle);
    }

void WriteRevolve(FILE                    *pFile,
                  size_t                  &nLine,
                  revolve           const *pRevolve,
                  std::map<size_t,size_t> &mSurfaceMap)
    {
    //#914=CARTESIAN_POINT('',(0.E0,-1.307493617253E2,0.E0));
    //#915=DIRECTION('',(0.E0,1.E0,0.E0));
    //#916=AXIS1_PLACEMENT('',#914,#915);
    //#913=B_SPLINE_CURVE_WITH_KNOTS('',3,(#905,#906,#907,#908,#909,#910,#911,#912),
    //.UNSPECIFIED.,.F.,.F.,(4,1,1,1,1,4),(0.E0,1.025034494080E-2,5.905206385276E-2,
    //5.411131718827E-1,9.950198866069E-1,1.E0),.UNSPECIFIED.);
    //#917=SURFACE_OF_REVOLUTION('',#913,#916);

    SGM::Point3D const &Pos=pRevolve->m_Origin;
    SGM::UnitVector3D const &Norm=pRevolve->m_ZAxis;

    size_t nPos=nLine;
    fprintf(pFile,"#%lu=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.X(),Norm.Y(),Norm.Z());
    size_t nAxis1=nLine;
    fprintf(pFile,"#%lu=AXIS1_PLACEMENT('',#%lu,#%lu);\n",nLine++,nPos,nNorm);

    std::set<curve const *> sRevolveCurve;
    sRevolveCurve.insert(pRevolve->m_pCurve);
    std::map<size_t,size_t> CurveMap;
    WriteCurves(pFile, nLine, sRevolveCurve, CurveMap);
    size_t nCurve = CurveMap[pRevolve->m_pCurve->GetID()];

    mSurfaceMap[pRevolve->GetID()]=nLine;
    fprintf(pFile,"#%lu=SURFACE_OF_REVOLUTION('',#%lu,#%lu);\n",nLine++,nCurve,nAxis1);
    }

void WriteSurfaces(FILE                            *pFile,
                   size_t                          &nLine,
                   std::set<surface const *> const &sSurfaces,
                   std::map<size_t,size_t>         &mSurfaceMap)
    {
    for (auto pSurface : sSurfaces)
        {
        switch(pSurface->GetSurfaceType())
            {
            case SGM::EntityType::PlaneType :
                {
                WritePlane(pFile,nLine,(plane const *)pSurface,mSurfaceMap);
                break;
                }
            case SGM::EntityType::SphereType :
                {
                WriteSphere(pFile,nLine,(sphere const *)pSurface,mSurfaceMap);
                break;
                }
            case SGM::EntityType::CylinderType :
                {
                WriteCylinder(pFile,nLine,(cylinder const *)pSurface,mSurfaceMap);
                break;
                }
            case SGM::EntityType::TorusType :
                {
                WriteTorus(pFile,nLine,(torus const *)pSurface,mSurfaceMap);
                break;
                }
            case SGM::EntityType::ConeType :
                {
                WriteCone(pFile,nLine,(cone const *)pSurface,mSurfaceMap);
                break;
                }
            case SGM::EntityType::RevolveType :
                {
                WriteRevolve(pFile,nLine,(revolve const *)pSurface,mSurfaceMap);
                break;
                }
            default:
                {
                throw;
                }
            }
        }
    }

void WriteFaces(SGM::Result                               &rResult,
                FILE                                      *pFile,
                size_t                                    &nLine,
                std::set<face *,EntityCompare>                    const &sFaces,
                std::map<size_t,size_t>                   &mSurfaceMap,
                std::map<std::pair<size_t,size_t>,size_t> &mCoedgeMap,
                std::map<size_t,size_t>                   &mFaceMap)
    {
    for (auto pFace : sFaces)
        {
        size_t nFace=pFace->GetID();
        size_t nSurface=mSurfaceMap[pFace->GetSurface()->GetID()];
        bool nFlipped=pFace->GetFlipped();
        std::vector<std::vector<edge *> > aaLoops;
        std::vector<std::vector<SGM::EdgeSideType> > aaFlipped;
        size_t nLoops=pFace->FindLoops(rResult,aaLoops,aaFlipped);
        size_t Index1,Index2;
        std::string sLoops;
        for(Index1=0;Index1<nLoops;++Index1)
            {
            std::vector<edge *> const &aLoops=aaLoops[Index1];
            size_t nEdges=aLoops.size();
            std::string sEdges;
            for(Index2=0;Index2<nEdges;++Index2)
                {
                edge *pEdge=aLoops[Index2];
                size_t nEdge=pEdge->GetID();
                char Buf[25];
                if(Index2)
                    {
                    snprintf(Buf,sizeof(Buf),",#%lu",mCoedgeMap[std::pair<size_t,size_t>(nEdge,nFace)]);
                    }
                else
                    {
                    snprintf(Buf,sizeof(Buf),"#%lu",mCoedgeMap[std::pair<size_t,size_t>(nEdge,nFace)]);
                    }
                sEdges+=Buf;
                }
            size_t nLoop=nLine;
            fprintf(pFile,"#%lu=EDGE_LOOP('',(%s));\n",nLine++,sEdges.c_str());
            size_t nBound=nLine;
            if(Index1)
                {
                fprintf(pFile,"#%lu=FACE_BOUND('',#%lu,.T.);\n",nLine++,nLoop);
                }
            else
                {
                fprintf(pFile,"#%lu=FACE_OUTER_BOUND('',#%lu,.T.);\n",nLine++,nLoop);
                }
            char Buf2[25];
            if(Index1)
                {
                snprintf(Buf2,sizeof(Buf2),",#%lu",nBound);
                }
            else
                {
                snprintf(Buf2,sizeof(Buf2),"#%lu",nBound);
                }
            sLoops+=Buf2;
            }
        mFaceMap[pFace->GetID()]=nLine;
        fprintf(pFile,"#%lu=ADVANCED_FACE('',(%s),#%lu,.%c.);\n",nLine++,sLoops.c_str(),nSurface,nFlipped ? 'F' : 'T');
        }
    }

void WriteVolumes(SGM::Result              &rResult,
                  FILE                     *pFile,
                  size_t                   &nLine,
                  size_t                   nGeoRepContext,
                  size_t                   nProductDefShape,
                  std::set<volume *,EntityCompare> const &sVolumes,
                  std::map<size_t,size_t>  &mFaceMap,
                  std::map<size_t,size_t>  &mEdgeMap)
    {
    std::string sVolumeList;
    bool bFirstVolume=true;
    int nSides=1;
    bool bWireBody=false;
    for (auto pVolume : sVolumes)
        {
        std::vector<std::set<face *,EntityCompare> > aShells;
        size_t nShells=pVolume->FindShells(rResult,aShells);
        if(nShells>1)
            {
            throw;
            }
        if(nShells==1)
            {
            std::string sFaceList;
            std::set<face *,EntityCompare> const &sFaces=aShells[0];
            auto FaceIter=sFaces.begin();
            nSides=(*FaceIter)->GetSides();
            bool bFirst=true;
            while(FaceIter!=sFaces.end())
                {
                face *pFace=*FaceIter;
                char Buf[25];
                if(bFirst)
                    {
                    snprintf(Buf,sizeof(Buf),"#%lu",mFaceMap[pFace->GetID()]);
                    }
                else
                    {
                    snprintf(Buf,sizeof(Buf),",#%lu",mFaceMap[pFace->GetID()]);
                    }
                sFaceList+=Buf;
                bFirst=false;
                ++FaceIter;
                }
            size_t nShell=nLine;
            size_t nVolume;
            if(nSides==2)
                {
                fprintf(pFile,"#%lu=OPEN_SHELL('',(%s));\n",nLine++,sFaceList.c_str());
                nVolume=nLine;
                fprintf(pFile,"#%lu=SHELL_BASED_SURFACE_MODEL('',(#%lu));\n",nLine++,nShell);
                }
            else
                {
                fprintf(pFile,"#%lu=CLOSED_SHELL('',(%s));\n",nLine++,sFaceList.c_str());
                nVolume=nLine;
                fprintf(pFile,"#%lu=MANIFOLD_SOLID_BREP('',#%lu);\n",nLine++,nShell);
                }
            char Buf2[25];
            if(bFirstVolume)
                {
                snprintf(Buf2,sizeof(Buf2),"#%lu",nVolume);
                }
            else
                {
                snprintf(Buf2,sizeof(Buf2),",#%lu",nVolume);
                }
            sVolumeList+=Buf2;
            bFirstVolume=false;
            }
        std::set<edge *,EntityCompare> const &sEdges=pVolume->GetEdges();
        if(!sEdges.empty())
            {
            auto EdgeIter=sEdges.begin();
            std::string sEdgeList;
            bool bFirst=true;
            while(EdgeIter!=sEdges.end())
                {
                edge *pEdge=*EdgeIter;
                char Buf[25];
                if(bFirst)
                    {
                    snprintf(Buf,sizeof(Buf),"#%lu",mEdgeMap[pEdge->GetID()]);
                    }
                else
                    {
                    snprintf(Buf,sizeof(Buf),",#%lu",mEdgeMap[pEdge->GetID()]);
                    }
                sEdgeList+=Buf;
                bFirst=false;
                ++EdgeIter;
                }
            char Buf2[25];
            snprintf(Buf2,sizeof(Buf2),"#%lu",nLine);
            sVolumeList+=Buf2;
            fprintf(pFile,"#%lu=GEOMETRIC_CURVE_SET('',(%s));\n",nLine++,sEdgeList.c_str());
            bWireBody=true;
            }
        }

    size_t nPoint=nLine;
    fprintf(pFile,"#%lu=CARTESIAN_POINT('',(0.0,0.0,0.0));\n",nLine++);
    size_t nDirection1=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(0.0,0.0,1.0));\n",nLine++);
    size_t nDirection2=nLine;
    fprintf(pFile,"#%lu=DIRECTION('',(1.0,0.0,0.0));\n",nLine++);
    size_t nAxis=nLine;
    fprintf(pFile,"#%lu=AXIS2_PLACEMENT_3D('',#%lu,#%lu,#%lu);\n",nLine++,nPoint,nDirection1,nDirection2);
    size_t nBRep=nLine;
    if(nSides==2)
        {
        fprintf(pFile,"#%lu=MANIFOLD_SURFACE_SHAPE_REPRESENTATION('',(%s,#%lu),#%lu);\n",nLine++,sVolumeList.c_str(),nAxis,nGeoRepContext);
        }
    else if(bWireBody)
        {
        fprintf(pFile,"#%lu=GEOMETRICALLY_BOUNDED_WIREFRAME_SHAPE_REPRESENTATION('',(%s,#%lu),#%lu);\n",nLine++,sVolumeList.c_str(),nAxis,nGeoRepContext);
        }
    else
        {
        fprintf(pFile,"#%lu=ADVANCED_BREP_SHAPE_REPRESENTATION('',(%s,#%lu),#%lu);\n",nLine++,sVolumeList.c_str(),nAxis,nGeoRepContext);
        }
    fprintf(pFile,"#%lu=SHAPE_DEFINITION_REPRESENTATION(#%lu,#%lu);\n",nLine++,nProductDefShape,nBRep);
    }

void WriteDataHeader(FILE   *pFile,
                     size_t &nLine,
                     size_t &nGeoRepContext,
                     size_t &nProductDefShape)
    {
    size_t nApplicationContext=nLine;
    fprintf(pFile,"#%lu=APPLICATION_CONTEXT('automotive design');\n",nLine++);
    size_t nProductDefContext=nLine;
    fprintf(pFile,"#%lu=PRODUCT_DEFINITION_CONTEXT('',#%lu,'design');\n",nLine++,nApplicationContext);
    size_t nProductContext=nLine;
    fprintf(pFile,"#%lu=PRODUCT_CONTEXT('',#%lu,'mechanical');\n",nLine++,nApplicationContext);
    fprintf(pFile,"#%lu=APPLICATION_PROTOCOL_DEFINITION('International Standard','automotive_design',2001,#%lu);\n",nLine++,nApplicationContext);
    size_t nDimExp1=nLine;
    fprintf(pFile,"#%lu=DIMENSIONAL_EXPONENTS(1.0,0.0,0.0,0.0,0.0,0.0,0.0);\n",nLine++);
    size_t nDimExp2=nLine;
    fprintf(pFile,"#%lu=DIMENSIONAL_EXPONENTS(0.0,0.0,0.0,0.0,0.0,0.0,0.0);\n",nLine++);
    size_t nNamedUnits1=nLine;
    fprintf(pFile,"#%lu= (NAMED_UNIT(#%lu)LENGTH_UNIT()SI_UNIT(.MILLI.,.METRE.));\n",nLine++,nDimExp1);
    size_t nNamedUnits2=nLine;
    fprintf(pFile,"#%lu= (NAMED_UNIT(#%lu)PLANE_ANGLE_UNIT()SI_UNIT($,.RADIAN.));\n",nLine++,nDimExp2);
    size_t nNamedUnits3=nLine;
    fprintf(pFile,"#%lu= (NAMED_UNIT(#%lu)SOLID_ANGLE_UNIT()SI_UNIT($,.STERADIAN.));\n",nLine++,nDimExp2);
    size_t nLengthMeasure=nLine;
    fprintf(pFile,"#%lu=LENGTH_MEASURE_WITH_UNIT(LENGTH_MEASURE(25.4),#%lu);\n",nLine++,nNamedUnits1);
    size_t nConversionBaseUnit=nLine;
    fprintf(pFile,"#%lu= (CONVERSION_BASED_UNIT('INCH',#%lu)LENGTH_UNIT()NAMED_UNIT(#%lu));\n",nLine++,nLengthMeasure,nDimExp1);
    size_t nUncertaintyMeasure=nLine;
    fprintf(pFile,"#%lu=UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.0E-006),#%lu,'','');\n",nLine++,nConversionBaseUnit);
    nGeoRepContext=nLine;
    fprintf(pFile,"#%lu= (GEOMETRIC_REPRESENTATION_CONTEXT(3)GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT((#%lu))GLOBAL_UNIT_ASSIGNED_CONTEXT((#%lu,#%lu,#%lu))REPRESENTATION_CONTEXT('NONE','WORKSPACE'));\n",
        nLine++,nUncertaintyMeasure,nConversionBaseUnit,nNamedUnits2,nNamedUnits3);
    fprintf(pFile,"#%lu=MECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION('',(),#%lu);\n",nLine++,nGeoRepContext);
    size_t nProduct=nLine;
    fprintf(pFile,"#%lu=PRODUCT('', '', ' ',(#%lu));\n",nLine++,nProductContext);
    size_t nProductDefFormation=nLine;
    fprintf(pFile,"#%lu=PRODUCT_DEFINITION_FORMATION_WITH_SPECIFIED_SOURCE (' ',' ', #%lu,.NOT_KNOWN.);\n",nLine++,nProduct);
    size_t nProductCategory=nLine;
    fprintf(pFile,"#%lu=PRODUCT_CATEGORY('part','');\n",nLine++);
    size_t nProductRelatedCategory=nLine;
    fprintf(pFile,"#%lu=PRODUCT_RELATED_PRODUCT_CATEGORY('detail',' ',(#%lu));\n",nLine++,nProduct);
    fprintf(pFile,"#%lu=PRODUCT_CATEGORY_RELATIONSHIP(' ',' ',#%lu,#%lu);\n",nLine++,nProductCategory,nProductRelatedCategory);
    size_t nProductDef=nLine;
    fprintf(pFile,"#%lu=PRODUCT_DEFINITION('','',#%lu,#%lu);\n",nLine++,nProductDefFormation,nProductDefContext);
    nProductDefShape=nLine;
    fprintf(pFile,"#%lu=PRODUCT_DEFINITION_SHAPE('NONE','NONE',#%lu);\n",nLine++,nProductDef);
    }

void SaveSTEP(SGM::Result                  &rResult,
              std::string            const &FileName,
              entity                       *pEntity,
              SGM::TranslatorOptions const &)//Options)
    {
    // Open the file.

    FILE *pFile = fopen(FileName.c_str(),"wt");
    if(pFile==nullptr)
        {
        rResult.SetResult(SGM::ResultType::ResultTypeFileOpen);
        return;
        }

    // Add a vertex to any closed Edges
    SGM::ImprintVerticesOnClosedEdges(rResult);

    // Write out the header

    std::string sTime=GetDateAndTime(false);
    std::string sShortName=GetFileName(FileName);

    fprintf(pFile,"ISO-10303-21;\n");
    fprintf(pFile,"HEADER;\n");
    fprintf(pFile,"FILE_DESCRIPTION(('STEP AP214'),'1');\n");
    fprintf(pFile,"FILE_NAME('%s','%s',('SGM Viewer 1.0'),(' '),'SGM 1.0',' ',' ');\n",sShortName.c_str(),sTime.c_str());
    fprintf(pFile,"FILE_SCHEMA(('automotive_design'));\n");
    fprintf(pFile,"ENDSEC;\n");

    // Write out data

    fprintf(pFile,"DATA;\n");

    std::set<volume *,EntityCompare> sVolumes;
    std::set<face *,EntityCompare>   sFaces;
    std::set<edge *,EntityCompare>   sEdges;
    std::set<vertex *,EntityCompare> sVertices;

    FindVolumes(rResult,pEntity,sVolumes);
    FindFaces(rResult,pEntity,sFaces);
    FindEdges(rResult,pEntity,sEdges);
    FindVertices(rResult,pEntity,sVertices);

    std::set<surface const *> sSurfaces;
    for (auto pFace : sFaces)
        {
        sSurfaces.insert(pFace->GetSurface());
        }

    std::set<curve const *> sCurves;
    for (auto pEdge : sEdges)
        {
        sCurves.insert(pEdge->GetCurve());
        }

    size_t nLine=1;
    size_t nGeoRepContext;
    size_t nProductDefShape;
    WriteDataHeader(pFile,nLine,nGeoRepContext,nProductDefShape);
    std::map<size_t,size_t> mVertexMap;
    WriteVertices(pFile,nLine,sVertices,mVertexMap);
    std::map<size_t,size_t> mCurveMap;
    WriteCurves(pFile,nLine,sCurves,mCurveMap);
    std::map<std::pair<size_t,size_t>,size_t> mCoedgeMap;
    std::map<size_t,size_t> mEdgeMap;
    WriteCoedges(pFile,nLine,sEdges,mVertexMap,mCurveMap,mCoedgeMap,mEdgeMap);
    std::map<size_t,size_t> mSurfaceMap;
    WriteSurfaces(pFile,nLine,sSurfaces,mSurfaceMap);
    std::map<size_t,size_t> mFaceMap;
    WriteFaces(rResult,pFile,nLine,sFaces,mSurfaceMap,mCoedgeMap,mFaceMap);
    WriteVolumes(rResult,pFile,nLine,nGeoRepContext,nProductDefShape,sVolumes,mFaceMap,mEdgeMap);

    fprintf(pFile,"ENDSEC;\n");
    fprintf(pFile,"END-ISO-10303-21;\n");

    fclose(pFile);
    }

}
