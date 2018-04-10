#include "SGMDataClasses.h"
#include "SGMTranslators.h"
#include "EntityClasses.h"
#include "Topology.h"
#include "FileFunctions.h"
#include <string>

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#else
#define snprintf snprintf
#endif

#ifdef _MSC_VER
__pragma(warning(disable: 4996 ))
#endif

void WriteVertices(FILE                     *pFile,
                   size_t                   &nLine,
                   std::set<vertex *> const &sVertices,
                   std::map<size_t,size_t>  &mVertexMap)
    {
    std::set<vertex *>::const_iterator iter=sVertices.begin();
    while(iter!=sVertices.end())
        {
        vertex const *pVertex=*iter;
        SGM::Point3D const &Pos=pVertex->GetPoint();
        size_t nPos=nLine;
        fprintf(pFile,"#%ld=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
        mVertexMap[pVertex->GetID()]=nLine;
        fprintf(pFile,"#%ld=VERTEX_POINT('',#%ld);\n",nLine++,nPos);
        ++iter;
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
    double dScale=pLine->GetScale();

    size_t nPos=nLine;
    fprintf(pFile,"#%ld=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nDirection=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Axis.m_x,Axis.m_y,Axis.m_z);
    size_t nVector=nLine;
    fprintf(pFile,"#%ld=VECTOR('',#%ld,%#.15G);\n",nLine++,nDirection,dScale);
    mCurveMap[pLine->GetID()]=nLine;
    fprintf(pFile,"#%ld=LINE('',#%ld,#%ld);\n",nLine++,nPos,nVector);
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
    fprintf(pFile,"#%ld=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Center.m_x,Center.m_y,Center.m_z);
    size_t nDirection1=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,ZVec.m_x,ZVec.m_y,ZVec.m_z);
    size_t nDirection2=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XVec.m_x,XVec.m_y,XVec.m_z);
    size_t nAxis=nLine;
    fprintf(pFile,"#%ld=AXIS2_PLACEMENT_3D('',#%ld,#%ld,#%ld);\n",nLine++,nPos,nDirection1,nDirection2);
    mCurveMap[pCircle->GetID()]=nLine;
    fprintf(pFile,"#%ld=CIRCLE('',#%ld,%#.15G);\n",nLine++,nAxis,dRadius);
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
    // Degree (Control Points) Curve Form, Closed, Self Intersect, (Mulitplicity) (Knots) Knot Type.
    
    int nDegree=pNUBCurve->GetDegree();
    std::vector<SGM::Point3D> const &aControlPoints=pNUBCurve->GetControlPoints();
    std::vector<double> aUniqueKnots;
    std::vector<int> aMultiplity;
    size_t nUniqueKnots=pNUBCurve->FindMultiplity(aMultiplity,aUniqueKnots);

    // Write out the control points.

    size_t Index1;
    size_t nControlPoints=aControlPoints.size();
    std::string sControlPoints="(";
    for(Index1=0;Index1<nControlPoints;++Index1)
        {
        SGM::Point3D const &Pos=aControlPoints[Index1];
        size_t nPos=nLine;
        fprintf(pFile,"#%ld=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
        char Arg[10];
        snprintf(Arg,sizeof(Arg),"#%ld",nPos);
        sControlPoints+=Arg;
        if(Index1<nControlPoints-1)
            {
            sControlPoints+=",";
            }
        }
    sControlPoints+=")";

    // Set up the multiplity string.

    std::string sMultiplity="(";
    for(Index1=0;Index1<nUniqueKnots;++Index1)
        {
        char Arg[10];
        snprintf(Arg,sizeof(Arg),"%ld",(long)(aMultiplity[Index1]));
        sMultiplity+=Arg;
        if(Index1<nUniqueKnots-1)
            {
            sMultiplity+=",";
            }
        }
    sMultiplity=")";

    // Set up the unique knot string.

    std::string sUniqueKnots="(";
    for(Index1=0;Index1<nUniqueKnots;++Index1)
        {
        char Arg[10];
        snprintf(Arg,sizeof(Arg),"%#.15G",aUniqueKnots[Index1]);
        sUniqueKnots+=Arg;
        if(Index1<nUniqueKnots-1)
            {
            sUniqueKnots+=",";
            }
        }
    sUniqueKnots=")";

    // Write out the b-spline

    mCurveMap[pNUBCurve->GetID()]=nLine;
    fprintf(pFile,"#%ld=B_SPLINE_CURVE_WITH_KNOTS('',%ld,%s,.UNSPECIFIED.,.F.,.F.,%s,%s,.UNSPECIFIED.);\n",
        nLine++,(long)nDegree,sControlPoints.c_str(),sMultiplity.c_str(),sUniqueKnots.c_str());
    }

void WriteCurves(FILE                          *pFile,
                 size_t                        &nLine,
                 std::set<curve const *> const &sCurves,
                 std::map<size_t,size_t>       &mCurveMap)
    {
    std::set<curve const *>::const_iterator iter=sCurves.begin();
    while(iter!=sCurves.end())
        {
        curve const *pCurve=*iter;
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
        ++iter;
        }
    }

void WriteCoedges(FILE                                      *pFile,
                  size_t                                    &nLine,
                  std::set<edge *>                    const &sEdges,
                  std::map<size_t,size_t>                   &mVertexMap,
                  std::map<size_t,size_t>                   &mCurveMap,
                  std::map<std::pair<size_t,size_t>,size_t> &mCoedgeMap,
                  std::map<size_t,size_t>                   &mEdgeMap)
    {
    std::set<edge *>::const_iterator EdgeIter=sEdges.begin();
    while(EdgeIter!=sEdges.end())
        {
        edge *pEdge=*EdgeIter;
        curve const *pCurve=pEdge->GetCurve();
        size_t nCurve=mCurveMap[pCurve->GetID()];
        size_t nEdgeCurve=nLine;
        volume *pVolume=pEdge->GetVolume();

        if(pVolume==NULL)
            {
            // #55=EDGE_CURVE('',#44,#25,#54,.T.);
            // #56=ORIENTED_EDGE('',*,*,#55,.T.);

            vertex *pStart=pEdge->GetStart();
            vertex *pEnd=pEdge->GetEnd();
            size_t nStart=mVertexMap[pStart->GetID()];
            size_t nEnd=mVertexMap[pEnd->GetID()];
            fprintf(pFile,"#%ld=EDGE_CURVE('',#%ld,#%ld,#%ld,.T.);\n",nLine++,nStart,nEnd,nCurve);
            std::set<face *> const &sFaces=pEdge->GetFaces();
            std::set<face *>::const_iterator FaceIter=sFaces.begin();
            while(FaceIter!=sFaces.end())
                {
                face const *pFace=*FaceIter;
                SGM::EdgeSideType bFlipped=pFace->GetEdgeType(pEdge);
                mCoedgeMap[std::pair<size_t,size_t>(pEdge->GetID(),pFace->GetID())]=nLine;
                fprintf(pFile,"#%ld=ORIENTED_EDGE('',*,*,#%ld,.%c.));\n",nLine++,nEdgeCurve,bFlipped==SGM::FaceOnRightType ? 'F' : 'T');
                ++FaceIter;
                }
            }
        else
            {
            // #28=TRIMMED_CURVE('',#27,(PARAMETER_VALUE(0.000000000000000)),(PARAMETER_VALUE(5.19615242270663)),.T.,.UNSPECIFIED.);
            // #29=GEOMETRIC_CURVE_SET(' ',(#28));

            double dStart=pEdge->GetDomain().m_dMin;
            double dEnd=pEdge->GetDomain().m_dMax;
            mEdgeMap[pEdge->GetID()]=nLine;
            fprintf(pFile,"#%ld=TRIMMED_CURVE('',#%ld,(PARAMETER_VALUE(%#.15G)),(PARAMETER_VALUE(%#.15G)),.T.,.UNSPECIFIED.);\n",nLine++,nCurve,dStart,dEnd);
            }

        ++EdgeIter;
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
    fprintf(pFile,"#%ld=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.m_x,Norm.m_y,Norm.m_z);
    size_t nXAxis=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.m_x,XAxis.m_y,XAxis.m_z);
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%ld=AXIS2_PLACEMENT_3D('',#%ld,#%ld,#%ld);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pPlane->GetID()]=nLine;
    fprintf(pFile,"#%ld=PLANE('',#%ld);\n",nLine++,nAxis3D);
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
    fprintf(pFile,"#%ld=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.m_x,Norm.m_y,Norm.m_z);
    size_t nXAxis=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.m_x,XAxis.m_y,XAxis.m_z);
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%ld=AXIS2_PLACEMENT_3D('',#%ld,#%ld,#%ld);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pSphere->GetID()]=nLine;
    fprintf(pFile,"#%ld=SPHERICAL_SURFACE('',#%ld,%#.15G);\n",nLine++,nAxis3D,dRadius);
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
    fprintf(pFile,"#%ld=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.m_x,Norm.m_y,Norm.m_z);
    size_t nXAxis=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.m_x,XAxis.m_y,XAxis.m_z);
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%ld=AXIS2_PLACEMENT_3D('',#%ld,#%ld,#%ld);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pCylinder->GetID()]=nLine;
    fprintf(pFile,"#%ld=CYLINDRICAL_SURFACE('',#%ld,%#.15G);\n",nLine++,nAxis3D,dRadius);
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
    fprintf(pFile,"#%ld=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.m_x,Norm.m_y,Norm.m_z);
    size_t nXAxis=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.m_x,XAxis.m_y,XAxis.m_z);
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%ld=AXIS2_PLACEMENT_3D('',#%ld,#%ld,#%ld);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pTorus->GetID()]=nLine;
    if(nKind==SGM::TorusKindType::DonutType || nKind==SGM::TorusKindType::PinchedType)
        {
        fprintf(pFile,"#%ld=TOROIDAL_SURFACE('',#%ld,%#.15G,%#.15G);\n",nLine++,nAxis3D,dMajorRadius,dMinorRadius);
        }
    else if(nKind==SGM::TorusKindType::AppleType)
        {
        fprintf(pFile,"#%ld=DEGENERATE_TOROIDAL_SURFACE('',#%ld,%#.15G,%#.15G,.T.);\n",nLine++,nAxis3D,dMajorRadius,dMinorRadius);
        }
    else // (nKind==SGM::TorusKindType::LemonType)
        {
        fprintf(pFile,"#%ld=DEGENERATE_TOROIDAL_SURFACE('',#%ld,%#.15G,%#.15G,.F.);\n",nLine++,nAxis3D,dMajorRadius,dMinorRadius);
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
    fprintf(pFile,"#%ld=CARTESIAN_POINT('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Pos.m_x,Pos.m_y,Pos.m_z);
    size_t nNorm=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,Norm.m_x,Norm.m_y,Norm.m_z);
    size_t nXAxis=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(%#.15G,%#.15G,%#.15G));\n",nLine++,XAxis.m_x,XAxis.m_y,XAxis.m_z);
    size_t nAxis3D=nLine;
    fprintf(pFile,"#%ld=AXIS2_PLACEMENT_3D('',#%ld,#%ld,#%ld);\n",nLine++,nPos,nNorm,nXAxis);
    mSurfaceMap[pCone->GetID()]=nLine;
    fprintf(pFile,"#%ld=CONICAL_SURFACE('',#%ld,%#.15G,%#.15G);\n",nLine++,nAxis3D,dRadius,dHalfAngle);
    }

void WriteSurfaces(FILE                            *pFile,
                   size_t                          &nLine,
                   std::set<surface const *> const &sSurfaces,
                   std::map<size_t,size_t>         &mSurfaceMap)
    {
    std::set<surface const *>::const_iterator iter=sSurfaces.begin();
    while(iter!=sSurfaces.end())
        {
        surface const *pSurface=*iter;
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
            default:
                {
                throw;
                }
            }
        ++iter;
        }
    }

void WriteFaces(SGM::Result                               &rResult,
                FILE                                      *pFile,
                size_t                                    &nLine,
                std::set<face *>                    const &sFaces,
                std::map<size_t,size_t>                   &mSurfaceMap,
                std::map<std::pair<size_t,size_t>,size_t> &mCoedgeMap,
                std::map<size_t,size_t>                   &mFaceMap)
    {
    std::set<face *>::const_iterator iter=sFaces.begin();
    while(iter!=sFaces.end())
        {
        face const *pFace=*iter;
        size_t nFace=pFace->GetID();
        size_t nSurface=mSurfaceMap[pFace->GetSurface()->GetID()];
        bool nFlipped=pFace->GetFlipped();
        std::vector<std::vector<edge *> > aaLoops;
        std::vector<std::vector<SGM::EdgeSideType> >   aaFlipped;
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
                    snprintf(Buf,sizeof(Buf),",#%ld",mCoedgeMap[std::pair<size_t,size_t>(nEdge,nFace)]);
                    }
                else
                    {
                    snprintf(Buf,sizeof(Buf),"#%ld",mCoedgeMap[std::pair<size_t,size_t>(nEdge,nFace)]);
                    }
                sEdges+=Buf;
                }
            size_t nLoop=nLine;
            fprintf(pFile,"#%ld=EDGE_LOOP('',(%s));\n",nLine++,sEdges.c_str());
            size_t nBound=nLine;
            if(Index1)
                {
                fprintf(pFile,"#%ld=FACE_BOUND('',#%ld,.T.);\n",nLine++,nLoop);
                }
            else
                {
                fprintf(pFile,"#%ld=FACE_OUTER_BOUND('',#%ld,.T.);\n",nLine++,nLoop);
                }
            char Buf2[25];
            if(Index1)
                {
                snprintf(Buf2,sizeof(Buf2),",#%ld",nBound);
                }
            else
                {
                snprintf(Buf2,sizeof(Buf2),"#%ld",nBound);
                }
            sLoops+=Buf2;
            }
        mFaceMap[pFace->GetID()]=nLine;
        fprintf(pFile,"#%ld=ADVANCED_FACE('',(%s),#%ld,.%c.);\n",nLine++,sLoops.c_str(),nSurface,nFlipped ? 'F' : 'T');
        ++iter;
        }
    }

void WriteVolumes(SGM::Result              &rResult,
                  FILE                     *pFile,
                  size_t                   &nLine,
                  size_t                   nGeoRepContext,
                  size_t                   nProductDefShape,
                  std::set<volume *> const &sVolumes,
                  std::map<size_t,size_t>  &mFaceMap,
                  std::map<size_t,size_t>  &mEdgeMap)
    {
    std::string sVolumeList;
    std::set<volume *>::const_iterator VolumeIter=sVolumes.begin();
    bool bFirstVolume=true;
    int nSides=1;
    bool bWireBody=false;
    while(VolumeIter!=sVolumes.end())
        {
        volume const *pVolume=*VolumeIter;
        std::vector<std::set<face *> > aShells;
        size_t nShells=pVolume->FindShells(rResult,aShells);
        if(nShells>1)
            {
            throw;
            }
        if(nShells==1)
            {
            std::string sFaceList;
            std::set<face *> const &sFaces=aShells[0];
            std::set<face *>::const_iterator FaceIter=sFaces.begin();
            nSides=(*FaceIter)->GetSides();
            bool bFirst=true;
            while(FaceIter!=sFaces.end())
                {
                face *pFace=*FaceIter;
                char Buf[25];
                if(bFirst)
                    {
                    snprintf(Buf,sizeof(Buf),"#%ld",mFaceMap[pFace->GetID()]);
                    }
                else
                    {
                    snprintf(Buf,sizeof(Buf),",#%ld",mFaceMap[pFace->GetID()]);
                    }
                sFaceList+=Buf;
                bFirst=false;
                ++FaceIter;
                }
            size_t nShell=nLine;
            size_t nVolume;
            if(nSides==2)
                {
                fprintf(pFile,"#%ld=OPEN_SHELL('',(%s));\n",nLine++,sFaceList.c_str());
                nVolume=nLine;
                fprintf(pFile,"#%ld=SHELL_BASED_SURFACE_MODEL('',(#%ld));\n",nLine++,nShell);
                }
            else
                {
                fprintf(pFile,"#%ld=CLOSED_SHELL('',(%s));\n",nLine++,sFaceList.c_str());
                nVolume=nLine;
                fprintf(pFile,"#%ld=MANIFOLD_SOLID_BREP('',#%ld);\n",nLine++,nShell);
                }
            char Buf2[25];
            if(bFirstVolume)
                {
                snprintf(Buf2,sizeof(Buf2),"#%ld",nVolume);
                }
            else
                {
                snprintf(Buf2,sizeof(Buf2),",#%ld",nVolume);
                }
            sVolumeList+=Buf2;
            bFirstVolume=false;
            }
        std::set<edge *> const &sEdges=pVolume->GetEdges();
        if(sEdges.size())
            {
            std::set<edge *>::const_iterator EdgeIter=sEdges.begin();
            std::string sEdgeList;
            bool bFirst=true;
            while(EdgeIter!=sEdges.end())
                {
                edge *pEdge=*EdgeIter;
                char Buf[25];
                if(bFirst)
                    {
                    snprintf(Buf,sizeof(Buf),"#%ld",mEdgeMap[pEdge->GetID()]);
                    }
                else
                    {
                    snprintf(Buf,sizeof(Buf),",#%ld",mEdgeMap[pEdge->GetID()]);
                    }
                sEdgeList+=Buf;
                bFirst=false;
                ++EdgeIter;
                }
            char Buf2[25];
            snprintf(Buf2,sizeof(Buf2),"#%ld",nLine);
            sVolumeList+=Buf2;
            fprintf(pFile,"#%ld=GEOMETRIC_CURVE_SET('',(%s));\n",nLine++,sEdgeList.c_str());
            bWireBody=true;
            }
        ++VolumeIter;
        }

    size_t nPoint=nLine;
    fprintf(pFile,"#%ld=CARTESIAN_POINT('',(0.0,0.0,0.0));\n",nLine++);
    size_t nDirection1=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(0.0,0.0,1.0));\n",nLine++);
    size_t nDirection2=nLine;
    fprintf(pFile,"#%ld=DIRECTION('',(1.0,0.0,0.0));\n",nLine++);
    size_t nAxis=nLine;
    fprintf(pFile,"#%ld=AXIS2_PLACEMENT_3D('',#%ld,#%ld,#%ld);\n",nLine++,nPoint,nDirection1,nDirection2);
    size_t nBRep=nLine;
    if(nSides==2)
        {
        fprintf(pFile,"#%ld=MANIFOLD_SURFACE_SHAPE_REPRESENTATION('',(%s,#%ld),#%ld);\n",nLine++,sVolumeList.c_str(),nAxis,nGeoRepContext);
        }
    else if(bWireBody)
        {
        fprintf(pFile,"#%ld=GEOMETRICALLY_BOUNDED_WIREFRAME_SHAPE_REPRESENTATION('',(%s,#%ld),#%ld);\n",nLine++,sVolumeList.c_str(),nAxis,nGeoRepContext);
        }
    else
        {
        fprintf(pFile,"#%ld=ADVANCED_BREP_SHAPE_REPRESENTATION('',(%s,#%ld),#%ld);\n",nLine++,sVolumeList.c_str(),nAxis,nGeoRepContext);
        }
    fprintf(pFile,"#%ld=SHAPE_DEFINITION_REPRESENTATION(#%ld,#%ld);\n",nLine++,nProductDefShape,nBRep);
    }

void WriteDataHeader(FILE   *pFile,
                     size_t &nLine,
                     size_t &nGeoRepContext,
                     size_t &nProductDefShape)
    {
    size_t nApplicationContext=nLine;
    fprintf(pFile,"#%ld=APPLICATION_CONTEXT('automotive design');\n",nLine++);
    size_t nProductDefContext=nLine;
    fprintf(pFile,"#%ld=PRODUCT_DEFINITION_CONTEXT('',#%ld,'design');\n",nLine++,nApplicationContext);
    size_t nProductContext=nLine;
    fprintf(pFile,"#%ld=PRODUCT_CONTEXT('',#%ld,'mechanical');\n",nLine++,nApplicationContext);
    fprintf(pFile,"#%ld=APPLICATION_PROTOCOL_DEFINITION('International Standard','automotive_design',2001,#%ld);\n",nLine++,nApplicationContext);
    size_t nDimExp1=nLine;
    fprintf(pFile,"#%ld=DIMENSIONAL_EXPONENTS(1.0,0.0,0.0,0.0,0.0,0.0,0.0);\n",nLine++);
    size_t nDimExp2=nLine;
    fprintf(pFile,"#%ld=DIMENSIONAL_EXPONENTS(0.0,0.0,0.0,0.0,0.0,0.0,0.0);\n",nLine++);
    size_t nNamedUnits1=nLine;
    fprintf(pFile,"#%ld= (NAMED_UNIT(#%ld)LENGTH_UNIT()SI_UNIT(.MILLI.,.METRE.));\n",nLine++,nDimExp1);
    size_t nNamedUnits2=nLine;
    fprintf(pFile,"#%ld= (NAMED_UNIT(#%ld)PLANE_ANGLE_UNIT()SI_UNIT($,.RADIAN.));\n",nLine++,nDimExp2);
    size_t nNamedUnits3=nLine;
    fprintf(pFile,"#%ld= (NAMED_UNIT(#%ld)SOLID_ANGLE_UNIT()SI_UNIT($,.STERADIAN.));\n",nLine++,nDimExp2);
    size_t nLengthMeasure=nLine;
    fprintf(pFile,"#%ld=LENGTH_MEASURE_WITH_UNIT(LENGTH_MEASURE(25.4),#%ld);\n",nLine++,nNamedUnits1);
    size_t nConversionBaseUnit=nLine;
    fprintf(pFile,"#%ld= (CONVERSION_BASED_UNIT('INCH',#%ld)LENGTH_UNIT()NAMED_UNIT(#%ld));\n",nLine++,nLengthMeasure,nDimExp1);
    size_t nUncertaintyMesure=nLine;
    fprintf(pFile,"#%ld=UNCERTAINTY_MEASURE_WITH_UNIT(LENGTH_MEASURE(1.0E-006),#%ld,'','');\n",nLine++,nConversionBaseUnit);
    nGeoRepContext=nLine;
    fprintf(pFile,"#%ld= (GEOMETRIC_REPRESENTATION_CONTEXT(3)GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT((#%ld))GLOBAL_UNIT_ASSIGNED_CONTEXT((#%ld,#%ld,#%ld))REPRESENTATION_CONTEXT('NONE','WORKSPACE'));\n",
        nLine++,nUncertaintyMesure,nConversionBaseUnit,nNamedUnits2,nNamedUnits3);
    fprintf(pFile,"#%ld=MECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION('',(),#%ld);\n",nLine++,nGeoRepContext);
    size_t nProduct=nLine;
    fprintf(pFile,"#%ld=PRODUCT('', '', ' ',(#%ld));\n",nLine++,nProductContext);
    size_t nProductDefFormation=nLine;
    fprintf(pFile,"#%ld=PRODUCT_DEFINITION_FORMATION_WITH_SPECIFIED_SOURCE (' ',' ', #%ld,.NOT_KNOWN.);\n",nLine++,nProduct);
    size_t nProductCategory=nLine;
    fprintf(pFile,"#%ld=PRODUCT_CATEGORY('part','');\n",nLine++);
    size_t nProductRelatedCategory=nLine;
    fprintf(pFile,"#%ld=PRODUCT_RELATED_PRODUCT_CATEGORY('detail',' ',(#%ld));\n",nLine++,nProduct);
    fprintf(pFile,"#%ld=PRODUCT_CATEGORY_RELATIONSHIP(' ',' ',#%ld,#%ld);\n",nLine++,nProductCategory,nProductRelatedCategory);
    size_t nProductDef=nLine;
    fprintf(pFile,"#%ld=PRODUCT_DEFINITION('','',#%ld,#%ld);\n",nLine++,nProductDefFormation,nProductDefContext);
    nProductDefShape=nLine;
    fprintf(pFile,"#%ld=PRODUCT_DEFINITION_SHAPE('NONE','NONE',#%ld);\n",nLine++,nProductDef);
    }

void SaveSTEP(SGM::Result                  &rResult,
              std::string            const &FileName,
              entity                       *pEntity,
              SGM::TranslatorOptions const &)//Options)
    {
    // Open the file.

    FILE *pFile = fopen(FileName.c_str(),"wt");
    if(pFile==NULL)
        {
        rResult.SetResult(SGM::ResultType::ResultTypeFileOpen);
        return;
        }

    // Write out the header

    std::string sTime=GetDateAndTime();
    std::string sShortName=GetFileName(FileName);

    fprintf(pFile,"ISO-10303-21;\n");
    fprintf(pFile,"HEADER;\n");
    fprintf(pFile,"FILE_DESCRIPTION(('STEP AP214'),'1');\n");
    fprintf(pFile,"FILE_NAME('%s','%s',('SGM Viewer 1.0'),(' '),'SGM 1.0',' ',' ');\n",sShortName.c_str(),sTime.c_str());
    fprintf(pFile,"FILE_SCHEMA(('automotive_design'));\n");
    fprintf(pFile,"ENDSEC;\n");

    // Write out data

    fprintf(pFile,"DATA;\n");

    std::set<volume *> sVolumes;
    std::set<face *>   sFaces;
    std::set<edge *>   sEdges;
    std::set<vertex *> sVertices;

    FindVolumes(rResult,pEntity,sVolumes);
    FindFaces(rResult,pEntity,sFaces);
    FindEdges(rResult,pEntity,sEdges);
    FindVertices(rResult,pEntity,sVertices);

    std::set<surface const *> sSurfaces;
    std::set<face *>::iterator FaceIter=sFaces.begin();
    while(FaceIter!=sFaces.end())
        {
        sSurfaces.insert((*FaceIter)->GetSurface());
        ++FaceIter;
        }

    std::set<curve const *> sCurves;
    std::set<edge *>::iterator EdgeIter=sEdges.begin();
    while(EdgeIter!=sEdges.end())
        {
        sCurves.insert((*EdgeIter)->GetCurve());
        ++EdgeIter;
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

void SGM::SaveSTEP(SGM::Result                  &rResult,
                   std::string            const &FileName,
                   SGM::Entity            const &EntityID,
                   SGM::TranslatorOptions const &Options)
    {
    thing *pThing=rResult.GetThing();
    SaveSTEP(rResult,FileName,pThing->FindEntity(EntityID.m_ID),Options); 
    }
