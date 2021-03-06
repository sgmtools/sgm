#include "SGMPrimitives.h"
#include "SGMMathematics.h"
#include "SGMInterval.h"
#include "SGMPolygon.h"

#include "Topology.h"
#include "EntityClasses.h"
#include "Surface.h"
#include "Modify.h"
#include "EntityFunctions.h"
#include "Faceter.h"
#include "Intersectors.h"

#include "Curve.h"
#include <cmath>
#include <algorithm>

namespace SGMInternal
{
edge *CreateEdge(SGM::Result           &rResult,
                 curve                 *pCurve,
                 SGM::Interval1D const *pDomain)
    {
    auto pEdge=new edge(rResult);
    pEdge->SetCurve(rResult,pCurve);
    SGM::Interval1D Domain;
    if(pDomain)
        {
        Domain=*pDomain;
        }
    else
        {
        Domain=pCurve->GetDomain();
        }
    
    pEdge->SetDomain(rResult,Domain);
    if( pCurve->GetCurveType()==SGM::EntityType::PointCurveType)
        {
        SGM::Point3D Pos;
        pCurve->Evaluate(Domain.m_dMin,&Pos);
        vertex *pVertex=new vertex(rResult,Pos);
        pEdge->SetStart(rResult,pVertex);
        pEdge->SetEnd(rResult,pVertex);
        }
    else if( pCurve->GetClosed()==false ||
        SGM::NearEqual(pCurve->GetDomain().Length(),Domain.Length(),SGM_MIN_TOL,false)==false)
        {
        SGM::Point3D StartPos,EndPos;
        pCurve->Evaluate(Domain.m_dMin,&StartPos);
        pCurve->Evaluate(Domain.m_dMax,&EndPos);
        vertex *pStart=new vertex(rResult,StartPos);
        vertex *pEnd=new vertex(rResult,EndPos);
        pEdge->SetStart(rResult,pStart);
        pEdge->SetEnd(rResult,pEnd);
        }
    else 
        {
        SGM::Point3D StartPos,EndPos;
        SGM::Vector3D StartVec,EndVec;
        pCurve->Evaluate(Domain.m_dMin,&StartPos,&StartVec);
        pCurve->Evaluate(Domain.m_dMax,&EndPos,&EndVec);
        SGM::UnitVector3D Vec0=StartVec,Vec1=EndVec;
        if(SGM::NearEqual(Vec0,Vec1,SGM_MIN_TOL)==false)
            {
            if(pCurve->GetClosed())
                {
                vertex *pVertex=new vertex(rResult,StartPos);
                pEdge->SetStart(rResult,pVertex);
                pEdge->SetEnd(rResult,pVertex);
                }
            else
                {
                vertex *pStart=new vertex(rResult,StartPos);
                vertex *pEnd=new vertex(rResult,EndPos);
                pEdge->SetStart(rResult,pStart);
                pEdge->SetEnd(rResult,pEnd);
                }
            }
        }
    return pEdge;
    }

edge *CreateEdge(SGM::Result        &rResult,
                 SGM::Point3D const &StartPos,
                 SGM::Point3D const &EndPos)
    {
    edge *pEdge=new edge(rResult);
    line *pLine=new line(rResult,StartPos,EndPos);
    pEdge->SetCurve(rResult,pLine);
    SGM::Interval1D Domain(0.0,StartPos.Distance(EndPos));
    pEdge->SetDomain(rResult,Domain);
    vertex *pStart=new vertex(rResult,StartPos);
    vertex *pEnd=new vertex(rResult,EndPos);
    pEdge->SetStart(rResult,pStart);
    pEdge->SetEnd(rResult,pEnd);
    return pEdge;
    }

edge *CreateEdge(SGM::Result &rResult,
                 curve       *pCurve,
                 vertex      *pStart,
                 vertex      *pEnd)
    {
    edge *pEdge=new edge(rResult);
    pEdge->SetCurve(rResult,pCurve);
    SGM::Interval1D domain;
    if (pStart != nullptr)
    {
        pEdge->SetStart(rResult,pStart);
        double uStart = pCurve->Inverse(pStart->GetPoint());
        if (pEnd != nullptr)
        {
            pEdge->SetEnd(rResult,pEnd);

            if (pEnd != pStart)
            {
                double uEnd = pCurve->Inverse(pEnd->GetPoint());
                pEdge->SetDomain(rResult, SGM::Interval1D(uStart, uEnd));
            }
            else
            {
                pEdge->SetDomain(rResult,pCurve->GetDomain());
            }
        }
        else
        {
            throw std::logic_error("Unhandled case in CreateEdge - start but no end vertex given");
        }
    }
    else
    {
        pEdge->SetDomain(rResult,pCurve->GetDomain());
    }
    return pEdge;
    }

body *CreateWireBody(SGM::Result            &rResult,
                     std::set<edge *> const &sEdges)
    {
    body   *pBody=new body(rResult); 
    volume *pVolume=new volume(rResult);
    pBody->AddVolume(pVolume);
    std::set<vertex *,EntityCompare> sVertices;
    for(auto pEdge : sEdges)
        {
        pVolume->AddEdge(rResult,pEdge);
        sVertices.insert(pEdge->GetStart());
        sVertices.insert(pEdge->GetEnd());
        }

    MergeVertexSet(rResult, sVertices);
    return pBody;
    }

body *CreatePointBody(SGM::Result                  &rResult,
                      std::set<SGM::Point3D> const &sPoints)
    {
    body *pBody=new body(rResult); 
    for(auto Pos : sPoints)
        {
        pBody->AddPoint(Pos);
        }
    return pBody;
    }

body *CreateTorus(SGM::Result             &rResult,
                  SGM::Point3D      const &Center,
                  SGM::UnitVector3D const &Axis,
                  double                   dMinorRadius,
                  double                   dMajorRadius,
                  bool                     bApple)
    {
    body   *pBody=new body(rResult); 
    volume *pVolume=new volume(rResult);

    torus *pTorus=new torus(rResult,Center,Axis,dMinorRadius,dMajorRadius,bApple);

    pBody->AddVolume(pVolume);
    face *pFace=new face(rResult);

    if(rResult.GetLog())
        {
        rResult.AddLog(SGM::Entity(pFace->GetID()),SGM::Entity(pFace->GetID()),SGM::LogType::LogMain);
        }

    pVolume->AddFace(rResult,pFace);
    pFace->SetSurface(rResult,pTorus);

    return pBody;
    }

body *CreateSphere(SGM::Result        &rResult,
                   SGM::Point3D const &Center,
                   double              dRadius)
    {
    ///////////////////////////////////////////////////////////////////////////
    //
    //
    //double L=6.3;
    //double M=1.75;
    //double d1=5,d2=6.5;
    //plane *pPlane=new plane(rResult,SGM::Point3D(0,0,0),SGM::UnitVector3D(0,0,1));
    //circle *pCircle=new circle(rResult,SGM::Point3D(0,0,0),SGM::UnitVector3D(0,0,1),M);
    //while(M)
    //    {
    //    double s=sqrt(1+(L+M)*(L+M));
    //    double a=SGM::SAFEasin(1/s);
    //    double b=SGM_HALF_PI-a;
    //    SGM::Point3D Pos1(M,0,0);
    //    SGM::Point3D Pos2(-L,0,1);
    //    double d=Pos1.Distance(Pos2);
    //    double dR=d*sin(a)/sin(b);
    //    cone *pCone=new cone(rResult,SGM::Point3D(M,0,0),SGM::Point3D(-L,0,1),dR,0);
    //    std::vector<curve *> aCurves;
    //    IntersectSurfaces(rResult,pCone,pPlane,aCurves,SGM_MIN_TOL);
    //    curve *pParabola=aCurves[0];
    //    std::vector<SGM::Point3D> aPoints;
    //    std::vector<SGM::IntersectionType> aTypes;
    //    IntersectCurves(rResult,pCircle,pParabola,aPoints,aTypes,SGM_MIN_TOL);
    //
    //    if(aPoints.size()==4)
    //        {
    //        d2=L;
    //        L=(L+d1)*0.5;
    //        }
    //    else
    //        {
    //        d1=L;
    //        L=(L+d2)*0.5;
    //        }
    //    }
    //
    //
    ///////////////////////////////////////////////////////////////////////////

    body   *pBody=new body(rResult); 
    volume *pVolume=new volume(rResult);

    sphere *pSphere=new sphere(rResult,Center,dRadius);

    pBody->AddVolume(pVolume);
    face *pFace=new face(rResult);

    if(rResult.GetLog())
        {
        rResult.AddLog(SGM::Entity(pFace->GetID()),SGM::Entity(pFace->GetID()),SGM::LogType::LogMain);
        }

    pVolume->AddFace(rResult,pFace);
    pFace->SetSurface(rResult,pSphere);

    return pBody;
    }

body *CreateCylinder(SGM::Result        &rResult,
                     SGM::Point3D const &BottomCenter,
                     SGM::Point3D const &TopCenter,
                     double              dRadius,
                     bool                bSheetBody)
    {
    SGM::UnitVector3D ZAxis=TopCenter-BottomCenter;
    SGM::UnitVector3D XAxis=ZAxis.Orthogonal();
    SGM::UnitVector3D YAxis=ZAxis*XAxis;

    body   *pBody=new body(rResult); 
    volume *pVolume=new volume(rResult);

    face *pSide=new face(rResult);
    face *pBottom=nullptr;
    face *pTop=nullptr;
    if(bSheetBody==false)
        {
        pBottom=new face(rResult);
        pTop=new face(rResult);
        }

    if(rResult.GetLog())
        {
        rResult.AddLog(SGM::Entity(pSide->GetID()),SGM::Entity(pSide->GetID()),SGM::LogType::LogMain);
        if(bSheetBody==false)
            {
            rResult.AddLog(SGM::Entity(pBottom->GetID()),SGM::Entity(pBottom->GetID()),SGM::LogType::LogBottom);
            rResult.AddLog(SGM::Entity(pTop->GetID()),SGM::Entity(pTop->GetID()),SGM::LogType::LogTop);
            }
        }

    edge *pEdgeBottom=new edge(rResult);
    edge *pEdgeTop=new edge(rResult);

    cylinder *pCylinder=new cylinder(rResult,BottomCenter,TopCenter,dRadius,&XAxis);

    circle *pCircleBottom=new circle(rResult,BottomCenter,-ZAxis,dRadius,&XAxis);
    circle *pCircleTop=new circle(rResult,TopCenter,ZAxis,dRadius,&XAxis);

    // Connect everything.

    pBody->AddVolume(pVolume);
    pVolume->AddFace(rResult,pSide);
    
    if(bSheetBody==false)
        {
        pVolume->AddFace(rResult,pBottom);
        pVolume->AddFace(rResult,pTop);

        plane *pPlaneBottom=new plane(rResult,BottomCenter,XAxis,-YAxis,-ZAxis);
        plane *pPlaneTop=new plane(rResult,TopCenter,XAxis,YAxis,ZAxis);

        pBottom->AddEdge(rResult,pEdgeBottom,SGM::FaceOnLeftType);
        pTop->AddEdge(rResult,pEdgeTop,SGM::FaceOnLeftType);

        pBottom->SetSurface(rResult,pPlaneBottom);
        pTop->SetSurface(rResult,pPlaneTop);
        }

    pSide->AddEdge(rResult,pEdgeBottom,SGM::FaceOnRightType);
    pSide->AddEdge(rResult,pEdgeTop,SGM::FaceOnRightType);
    pSide->SetSurface(rResult,pCylinder);

    pEdgeBottom->SetCurve(rResult,pCircleBottom);
    pEdgeTop->SetCurve(rResult,pCircleTop);

    pEdgeBottom->SetDomain(rResult,SGM::Interval1D(0,SGM_TWO_PI));
    pEdgeTop->SetDomain(rResult,SGM::Interval1D(0,SGM_TWO_PI));

    return pBody;
    }

body *CreateCone(SGM::Result        &rResult,
                 SGM::Point3D const &BottomCenter,
                 SGM::Point3D const &TopCenter,
                 double              dBottomRadius,
                 double              dTopRadius,
                 bool                bSheetBody)
    {
    SGM::UnitVector3D ZAxis=TopCenter-BottomCenter;
    SGM::UnitVector3D XAxis=ZAxis.Orthogonal();
    SGM::UnitVector3D YAxis=ZAxis*XAxis;

    body   *pBody=new body(rResult); 
    volume *pVolume=new volume(rResult);
    face *pSide=new face(rResult);

    double dy=dBottomRadius-dTopRadius;
    double dx=TopCenter.Distance(BottomCenter);
    double dHalfAngle=SGM::SAFEatan2(dy,dx);
    cone *pCone=new cone(rResult,BottomCenter,ZAxis,dBottomRadius,dHalfAngle,&XAxis);

    pBody->AddVolume(pVolume);
    pVolume->AddFace(rResult,pSide);
    pSide->SetSurface(rResult,pCone);
    if(bSheetBody)
        {
        pSide->SetSides(2);
        }
    
    if(SGM_MIN_TOL<dBottomRadius)
        {
        if(bSheetBody)
            {
            edge *pEdgeBottom=new edge(rResult);
            circle *pCircleBottom=new circle(rResult,BottomCenter,-ZAxis,dBottomRadius,&XAxis);

            pSide->AddEdge(rResult,pEdgeBottom,SGM::FaceOnRightType);
            pEdgeBottom->SetCurve(rResult,pCircleBottom);
            pEdgeBottom->SetDomain(rResult,SGM::Interval1D(0,SGM_TWO_PI));
            }
        else
            {
            face *pBottom=new face(rResult);
            edge *pEdgeBottom=new edge(rResult);
            plane *pPlaneBottom=new plane(rResult,BottomCenter,XAxis,-YAxis,-ZAxis);
            circle *pCircleBottom=new circle(rResult,BottomCenter,-ZAxis,dBottomRadius,&XAxis);

            pSide->AddEdge(rResult,pEdgeBottom,SGM::FaceOnRightType);
            pVolume->AddFace(rResult,pBottom);
            pBottom->AddEdge(rResult,pEdgeBottom,SGM::FaceOnLeftType);
            pBottom->SetSurface(rResult,pPlaneBottom);
            pEdgeBottom->SetCurve(rResult,pCircleBottom);
            pEdgeBottom->SetDomain(rResult,SGM::Interval1D(0,SGM_TWO_PI));
            }
        }
    
    if(SGM_MIN_TOL<dTopRadius)
        {
        if(bSheetBody)
            {
            edge *pEdgeTop=new edge(rResult);
            circle *pCircleTop=new circle(rResult,TopCenter,ZAxis,dTopRadius,&XAxis);

            pSide->AddEdge(rResult,pEdgeTop,SGM::FaceOnRightType);
            pEdgeTop->SetCurve(rResult,pCircleTop);
            pEdgeTop->SetDomain(rResult,SGM::Interval1D(0,SGM_TWO_PI));
            }
        else
            {
            face *pTop=new face(rResult);
            edge *pEdgeTop=new edge(rResult);
            plane *pPlaneTop=new plane(rResult,TopCenter,XAxis,YAxis,ZAxis);
            circle *pCircleTop=new circle(rResult,TopCenter,ZAxis,dTopRadius,&XAxis);

            pTop->AddEdge(rResult,pEdgeTop,SGM::FaceOnLeftType);
            pVolume->AddFace(rResult,pTop);
            pSide->AddEdge(rResult,pEdgeTop,SGM::FaceOnRightType);
            pTop->SetSurface(rResult,pPlaneTop);
            pEdgeTop->SetCurve(rResult,pCircleTop);
            pEdgeTop->SetDomain(rResult,SGM::Interval1D(0,SGM_TWO_PI));
            }
        }
    
    return pBody;
    }

body *CreateBlock(SGM::Result        &rResult,
                  SGM::Point3D const &Point1,
                  SGM::Point3D const &Point2)
    {
    // Create a one body, one volume, six faces, twelve edges, eight vertices, six planes, and twelve lines.
    //
    //      7-------6
    //     /|      /|
    //   4/------5  |
    //   |  3----|--2
    //   | /     | /
    //   |/      |/
    //   0-------1 

    double X0=std::min(Point1.m_x,Point2.m_x);
    double X1=std::max(Point1.m_x,Point2.m_x);
    double Y0=std::min(Point1.m_y,Point2.m_y);
    double Y1=std::max(Point1.m_y,Point2.m_y);
    double Z0=std::min(Point1.m_z,Point2.m_z);
    double Z1=std::max(Point1.m_z,Point2.m_z);

    body   *pBody=new body(rResult); 
    volume *pVolume=new volume(rResult);
    pBody->AddVolume(pVolume);

    if(std::abs(X0-X1)<SGM_MIN_TOL)
        {
        SGM::Point3D Pos0(X0,Y0,Z0);
        SGM::Point3D Pos1(X0,Y1,Z0);
        SGM::Point3D Pos2(X0,Y1,Z1);
        SGM::Point3D Pos3(X0,Y0,Z1);

        face *pFace0123=new face(rResult);

        edge *pEdge01=new edge(rResult);
        edge *pEdge12=new edge(rResult);
        edge *pEdge23=new edge(rResult);
        edge *pEdge30=new edge(rResult);

        vertex *pVertex0=new vertex(rResult,Pos0);
        vertex *pVertex1=new vertex(rResult,Pos1);
        vertex *pVertex2=new vertex(rResult,Pos2);
        vertex *pVertex3=new vertex(rResult,Pos3);

        plane *pPlane0123=new plane(rResult,Pos0,Pos1,Pos3);

        line *pLine01=new line(rResult,Pos0,Pos1);
        line *pLine12=new line(rResult,Pos1,Pos2);
        line *pLine23=new line(rResult,Pos2,Pos3);
        line *pLine30=new line(rResult,Pos3,Pos0);

        pVolume->AddFace(rResult,pFace0123);

        pFace0123->AddEdge(rResult,pEdge01,SGM::FaceOnLeftType);
        pFace0123->AddEdge(rResult,pEdge12,SGM::FaceOnLeftType);
        pFace0123->AddEdge(rResult,pEdge23,SGM::FaceOnLeftType);
        pFace0123->AddEdge(rResult,pEdge30,SGM::FaceOnLeftType);

        pEdge01->SetStart(rResult,pVertex0);
        pEdge12->SetStart(rResult,pVertex1);
        pEdge23->SetStart(rResult,pVertex2);
        pEdge30->SetStart(rResult,pVertex3);

        pEdge01->SetEnd(rResult,pVertex1);
        pEdge12->SetEnd(rResult,pVertex2);
        pEdge23->SetEnd(rResult,pVertex3);
        pEdge30->SetEnd(rResult,pVertex0);

        pEdge01->SetDomain(rResult,SGM::Interval1D(0,Pos0.Distance(Pos1)));
        pEdge12->SetDomain(rResult,SGM::Interval1D(0,Pos1.Distance(Pos2)));
        pEdge23->SetDomain(rResult,SGM::Interval1D(0,Pos2.Distance(Pos3)));
        pEdge30->SetDomain(rResult,SGM::Interval1D(0,Pos3.Distance(Pos0)));

        pFace0123->SetSurface(rResult,pPlane0123);
        pFace0123->SetSides(2);

        pEdge01->SetCurve(rResult,pLine01);
        pEdge12->SetCurve(rResult,pLine12);
        pEdge23->SetCurve(rResult,pLine23);
        pEdge30->SetCurve(rResult,pLine30);
        }
    else if(std::abs(Z0-Z1)<SGM_MIN_TOL)
        {
        SGM::Point3D Pos0(X0,Y0,Z0);
        SGM::Point3D Pos1(X1,Y0,Z0);
        SGM::Point3D Pos2(X1,Y1,Z0);
        SGM::Point3D Pos3(X0,Y1,Z0);

        face *pFace0123=new face(rResult);

        edge *pEdge01=new edge(rResult);
        edge *pEdge12=new edge(rResult);
        edge *pEdge23=new edge(rResult);
        edge *pEdge30=new edge(rResult);

        vertex *pVertex0=new vertex(rResult,Pos0);
        vertex *pVertex1=new vertex(rResult,Pos1);
        vertex *pVertex2=new vertex(rResult,Pos2);
        vertex *pVertex3=new vertex(rResult,Pos3);

        plane *pPlane0123=new plane(rResult,Pos0,Pos1,Pos3);

        line *pLine01=new line(rResult,Pos0,Pos1);
        line *pLine12=new line(rResult,Pos1,Pos2);
        line *pLine23=new line(rResult,Pos2,Pos3);
        line *pLine30=new line(rResult,Pos3,Pos0);

        pVolume->AddFace(rResult,pFace0123);

        pFace0123->AddEdge(rResult,pEdge01,SGM::FaceOnLeftType);
        pFace0123->AddEdge(rResult,pEdge12,SGM::FaceOnLeftType);
        pFace0123->AddEdge(rResult,pEdge23,SGM::FaceOnLeftType);
        pFace0123->AddEdge(rResult,pEdge30,SGM::FaceOnLeftType);

        pEdge01->SetStart(rResult,pVertex0);
        pEdge12->SetStart(rResult,pVertex1);
        pEdge23->SetStart(rResult,pVertex2);
        pEdge30->SetStart(rResult,pVertex3);

        pEdge01->SetEnd(rResult,pVertex1);
        pEdge12->SetEnd(rResult,pVertex2);
        pEdge23->SetEnd(rResult,pVertex3);
        pEdge30->SetEnd(rResult,pVertex0);

        pEdge01->SetDomain(rResult,SGM::Interval1D(0,Pos0.Distance(Pos1)));
        pEdge12->SetDomain(rResult,SGM::Interval1D(0,Pos1.Distance(Pos2)));
        pEdge23->SetDomain(rResult,SGM::Interval1D(0,Pos2.Distance(Pos3)));
        pEdge30->SetDomain(rResult,SGM::Interval1D(0,Pos3.Distance(Pos0)));

        pFace0123->SetSurface(rResult,pPlane0123);
        pFace0123->SetSides(2);

        pEdge01->SetCurve(rResult,pLine01);
        pEdge12->SetCurve(rResult,pLine12);
        pEdge23->SetCurve(rResult,pLine23);
        pEdge30->SetCurve(rResult,pLine30);
        }
    else if(std::abs(Y0-Y1)<SGM_MIN_TOL)
        {
        SGM::Point3D Pos0(X0,Y0,Z0);
        SGM::Point3D Pos1(X1,Y0,Z0);
        SGM::Point3D Pos2(X1,Y0,Z1);
        SGM::Point3D Pos3(X0,Y0,Z1);

        face *pFace0123=new face(rResult);

        edge *pEdge01=new edge(rResult);
        edge *pEdge12=new edge(rResult);
        edge *pEdge23=new edge(rResult);
        edge *pEdge30=new edge(rResult);

        vertex *pVertex0=new vertex(rResult,Pos0);
        vertex *pVertex1=new vertex(rResult,Pos1);
        vertex *pVertex2=new vertex(rResult,Pos2);
        vertex *pVertex3=new vertex(rResult,Pos3);

        plane *pPlane0123=new plane(rResult,Pos0,Pos1,Pos3);

        line *pLine01=new line(rResult,Pos0,Pos1);
        line *pLine12=new line(rResult,Pos1,Pos2);
        line *pLine23=new line(rResult,Pos2,Pos3);
        line *pLine30=new line(rResult,Pos3,Pos0);

        pVolume->AddFace(rResult,pFace0123);

        pFace0123->AddEdge(rResult,pEdge01,SGM::FaceOnLeftType);
        pFace0123->AddEdge(rResult,pEdge12,SGM::FaceOnLeftType);
        pFace0123->AddEdge(rResult,pEdge23,SGM::FaceOnLeftType);
        pFace0123->AddEdge(rResult,pEdge30,SGM::FaceOnLeftType);

        pEdge01->SetStart(rResult,pVertex0);
        pEdge12->SetStart(rResult,pVertex1);
        pEdge23->SetStart(rResult,pVertex2);
        pEdge30->SetStart(rResult,pVertex3);

        pEdge01->SetEnd(rResult,pVertex1);
        pEdge12->SetEnd(rResult,pVertex2);
        pEdge23->SetEnd(rResult,pVertex3);
        pEdge30->SetEnd(rResult,pVertex0);

        pEdge01->SetDomain(rResult,SGM::Interval1D(0,Pos0.Distance(Pos1)));
        pEdge12->SetDomain(rResult,SGM::Interval1D(0,Pos1.Distance(Pos2)));
        pEdge23->SetDomain(rResult,SGM::Interval1D(0,Pos2.Distance(Pos3)));
        pEdge30->SetDomain(rResult,SGM::Interval1D(0,Pos3.Distance(Pos0)));

        pFace0123->SetSurface(rResult,pPlane0123);
        pFace0123->SetSides(2);

        pEdge01->SetCurve(rResult,pLine01);
        pEdge12->SetCurve(rResult,pLine12);
        pEdge23->SetCurve(rResult,pLine23);
        pEdge30->SetCurve(rResult,pLine30);
        }
    else
        {
        SGM::Point3D Pos0(X0,Y0,Z0);
        SGM::Point3D Pos1(X1,Y0,Z0);
        SGM::Point3D Pos2(X1,Y1,Z0);
        SGM::Point3D Pos3(X0,Y1,Z0);
        SGM::Point3D Pos4(X0,Y0,Z1);
        SGM::Point3D Pos5(X1,Y0,Z1);
        SGM::Point3D Pos6(X1,Y1,Z1);
        SGM::Point3D Pos7(X0,Y1,Z1);

        face *pFace0321=new face(rResult);
        face *pFace0154=new face(rResult);
        face *pFace1265=new face(rResult);
        face *pFace2376=new face(rResult);
        face *pFace0473=new face(rResult);
        face *pFace4567=new face(rResult);

        edge *pEdge01=new edge(rResult);
        edge *pEdge12=new edge(rResult);
        edge *pEdge23=new edge(rResult);
        edge *pEdge03=new edge(rResult);
        edge *pEdge04=new edge(rResult);
        edge *pEdge15=new edge(rResult);
        edge *pEdge26=new edge(rResult);
        edge *pEdge37=new edge(rResult);
        edge *pEdge45=new edge(rResult);
        edge *pEdge56=new edge(rResult);
        edge *pEdge67=new edge(rResult);
        edge *pEdge47=new edge(rResult);

        vertex *pVertex0=new vertex(rResult,Pos0);
        vertex *pVertex1=new vertex(rResult,Pos1);
        vertex *pVertex2=new vertex(rResult,Pos2);
        vertex *pVertex3=new vertex(rResult,Pos3);
        vertex *pVertex4=new vertex(rResult,Pos4);
        vertex *pVertex5=new vertex(rResult,Pos5);
        vertex *pVertex6=new vertex(rResult,Pos6);
        vertex *pVertex7=new vertex(rResult,Pos7);

        plane *pPlane0321=new plane(rResult,Pos0,Pos3,Pos1);
        plane *pPlane0154=new plane(rResult,Pos1,Pos5,Pos0);
        plane *pPlane1265=new plane(rResult,Pos2,Pos6,Pos1);
        plane *pPlane2376=new plane(rResult,Pos2,Pos3,Pos6);
        plane *pPlane0473=new plane(rResult,Pos4,Pos7,Pos0);
        plane *pPlane4567=new plane(rResult,Pos5,Pos6,Pos4);

        line *pLine01=new line(rResult,Pos0,Pos1);
        line *pLine12=new line(rResult,Pos1,Pos2);
        line *pLine23=new line(rResult,Pos2,Pos3);
        line *pLine03=new line(rResult,Pos0,Pos3);
        line *pLine04=new line(rResult,Pos0,Pos4);
        line *pLine15=new line(rResult,Pos1,Pos5);
        line *pLine26=new line(rResult,Pos2,Pos6);
        line *pLine37=new line(rResult,Pos3,Pos7);
        line *pLine45=new line(rResult,Pos4,Pos5);
        line *pLine56=new line(rResult,Pos5,Pos6);
        line *pLine67=new line(rResult,Pos6,Pos7);
        line *pLine47=new line(rResult,Pos4,Pos7);

        // Connect everything.

        pVolume->AddFace(rResult,pFace0321);
        pVolume->AddFace(rResult,pFace0154);
        pVolume->AddFace(rResult,pFace1265);
        pVolume->AddFace(rResult,pFace2376);
        pVolume->AddFace(rResult,pFace0473);
        pVolume->AddFace(rResult,pFace4567);

        pFace0321->AddEdge(rResult,pEdge03,SGM::FaceOnLeftType);
        pFace0321->AddEdge(rResult,pEdge23,SGM::FaceOnRightType); 
        pFace0321->AddEdge(rResult,pEdge12,SGM::FaceOnRightType);
        pFace0321->AddEdge(rResult,pEdge01,SGM::FaceOnRightType);

        pFace0154->AddEdge(rResult,pEdge01,SGM::FaceOnLeftType);
        pFace0154->AddEdge(rResult,pEdge15,SGM::FaceOnLeftType);
        pFace0154->AddEdge(rResult,pEdge45,SGM::FaceOnRightType); 
        pFace0154->AddEdge(rResult,pEdge04,SGM::FaceOnRightType);

        pFace1265->AddEdge(rResult,pEdge12,SGM::FaceOnLeftType);
        pFace1265->AddEdge(rResult,pEdge26,SGM::FaceOnLeftType);
        pFace1265->AddEdge(rResult,pEdge56,SGM::FaceOnRightType); 
        pFace1265->AddEdge(rResult,pEdge15,SGM::FaceOnRightType);

        pFace2376->AddEdge(rResult,pEdge23,SGM::FaceOnLeftType);
        pFace2376->AddEdge(rResult,pEdge37,SGM::FaceOnLeftType);
        pFace2376->AddEdge(rResult,pEdge67,SGM::FaceOnRightType); 
        pFace2376->AddEdge(rResult,pEdge26,SGM::FaceOnRightType);

        pFace0473->AddEdge(rResult,pEdge04,SGM::FaceOnLeftType);
        pFace0473->AddEdge(rResult,pEdge47,SGM::FaceOnLeftType);
        pFace0473->AddEdge(rResult,pEdge37,SGM::FaceOnRightType); 
        pFace0473->AddEdge(rResult,pEdge03,SGM::FaceOnRightType);

        pFace4567->AddEdge(rResult,pEdge45,SGM::FaceOnLeftType);
        pFace4567->AddEdge(rResult,pEdge56,SGM::FaceOnLeftType);
        pFace4567->AddEdge(rResult,pEdge67,SGM::FaceOnLeftType);
        pFace4567->AddEdge(rResult,pEdge47,SGM::FaceOnRightType); 

        pEdge01->SetStart(rResult,pVertex0);
        pEdge12->SetStart(rResult,pVertex1);
        pEdge23->SetStart(rResult,pVertex2);
        pEdge03->SetStart(rResult,pVertex0);
        pEdge04->SetStart(rResult,pVertex0);
        pEdge15->SetStart(rResult,pVertex1);
        pEdge26->SetStart(rResult,pVertex2);
        pEdge37->SetStart(rResult,pVertex3);
        pEdge45->SetStart(rResult,pVertex4);
        pEdge56->SetStart(rResult,pVertex5);
        pEdge67->SetStart(rResult,pVertex6);
        pEdge47->SetStart(rResult,pVertex4);

        pEdge01->SetEnd(rResult,pVertex1);
        pEdge12->SetEnd(rResult,pVertex2);
        pEdge23->SetEnd(rResult,pVertex3);
        pEdge03->SetEnd(rResult,pVertex3);
        pEdge04->SetEnd(rResult,pVertex4);
        pEdge15->SetEnd(rResult,pVertex5);
        pEdge26->SetEnd(rResult,pVertex6);
        pEdge37->SetEnd(rResult,pVertex7);
        pEdge45->SetEnd(rResult,pVertex5);
        pEdge56->SetEnd(rResult,pVertex6);
        pEdge67->SetEnd(rResult,pVertex7);
        pEdge47->SetEnd(rResult,pVertex7);

        pEdge01->SetDomain(rResult,SGM::Interval1D(0,Pos0.Distance(Pos1)));
        pEdge12->SetDomain(rResult,SGM::Interval1D(0,Pos1.Distance(Pos2)));
        pEdge23->SetDomain(rResult,SGM::Interval1D(0,Pos2.Distance(Pos3)));
        pEdge03->SetDomain(rResult,SGM::Interval1D(0,Pos0.Distance(Pos3)));
        pEdge04->SetDomain(rResult,SGM::Interval1D(0,Pos0.Distance(Pos4)));
        pEdge15->SetDomain(rResult,SGM::Interval1D(0,Pos1.Distance(Pos5)));
        pEdge26->SetDomain(rResult,SGM::Interval1D(0,Pos2.Distance(Pos6)));
        pEdge37->SetDomain(rResult,SGM::Interval1D(0,Pos3.Distance(Pos7)));
        pEdge45->SetDomain(rResult,SGM::Interval1D(0,Pos4.Distance(Pos5)));
        pEdge56->SetDomain(rResult,SGM::Interval1D(0,Pos5.Distance(Pos6)));
        pEdge67->SetDomain(rResult,SGM::Interval1D(0,Pos6.Distance(Pos7)));
        pEdge47->SetDomain(rResult,SGM::Interval1D(0,Pos4.Distance(Pos7)));

        pFace0321->SetSurface(rResult,pPlane0321);
        pFace0154->SetSurface(rResult,pPlane0154);
        pFace1265->SetSurface(rResult,pPlane1265);
        pFace2376->SetSurface(rResult,pPlane2376);
        pFace0473->SetSurface(rResult,pPlane0473);
        pFace4567->SetSurface(rResult,pPlane4567);

        pEdge01->SetCurve(rResult,pLine01);
        pEdge12->SetCurve(rResult,pLine12);
        pEdge23->SetCurve(rResult,pLine23);
        pEdge03->SetCurve(rResult,pLine03);
        pEdge04->SetCurve(rResult,pLine04);
        pEdge15->SetCurve(rResult,pLine15);
        pEdge26->SetCurve(rResult,pLine26);
        pEdge37->SetCurve(rResult,pLine37);
        pEdge45->SetCurve(rResult,pLine45);
        pEdge56->SetCurve(rResult,pLine56);
        pEdge67->SetCurve(rResult,pLine67);
        pEdge47->SetCurve(rResult,pLine47);
        }

    if(rResult.GetDebugFlag()==5)   // Create a bad part for testing
        {
        pVolume->SetBody(nullptr);
        std::set<face *,EntityCompare> const &sFaces=pVolume->GetFaces();
        face *pFace=*(sFaces.begin());
        pFace->SetVolume(nullptr);
        std::set<edge *,EntityCompare> const &sEdges=pFace->GetEdges();
        edge *pEdge=*(sEdges.begin());
        pEdge->RemoveFace(pFace);
        pEdge->GetStart()->RemoveEdge(pEdge);
        pEdge->GetEnd()->RemoveEdge(pEdge);
        auto iter=sEdges.begin();
        ++iter;
        edge *pEdge1=*iter;
        ++iter;
        edge *pEdge2=*iter;
        pFace->RemoveEdge(rResult,pEdge1);
        pFace->AddEdge(rResult,pEdge1,SGM::EdgeSideType::FaceOnLeftType);
        pEdge2->SetStart(rResult,nullptr);
        pEdge2->SetEnd(rResult,nullptr);
        }

    return pBody;
    }

void FindDegree3Knots(std::vector<double> const &aLengths,
                      std::vector<double>       &aKnots,
                      size_t                    &nDegree)
    {
    size_t nLengths=aLengths.size();
    if(nLengths==2)
        {
        nDegree=1;
        aKnots.reserve(4);
        aKnots.push_back(0.0);
        aKnots.push_back(0.0);
        aKnots.push_back(1.0);
        aKnots.push_back(1.0);
        }
    else if(nLengths==3)
        {
        nDegree=2;
        aKnots.reserve(6);
        aKnots.push_back(0.0);
        aKnots.push_back(0.0);
        aKnots.push_back(0.0);
        aKnots.push_back(1.0);
        aKnots.push_back(1.0);
        aKnots.push_back(1.0);
        }
    else
        {
        nDegree=3;
        size_t nKnots=nLengths+4;
        aKnots.reserve(nKnots);
        aKnots.push_back(0.0);
        aKnots.push_back(0.0);
        aKnots.push_back(0.0);
        aKnots.push_back(0.0);
        size_t Index1;
        for(Index1=4;Index1<nKnots-4;++Index1)
            {
            double dKnot=(aLengths[Index1-3]+aLengths[Index1-2]+aLengths[Index1-1])/3.0;
            aKnots.push_back(dKnot);
            }
        aKnots.push_back(1.0);
        aKnots.push_back(1.0);
        aKnots.push_back(1.0);
        aKnots.push_back(1.0);
        }
    }

void FindDegree3KnotsWithEndDirections(std::vector<double> const &aLengths,
                                       std::vector<double>       &aKnots)
    {
    size_t nLengths=aLengths.size();
    size_t nKnots=nLengths+6;
    aKnots.reserve(nKnots);
    aKnots.push_back(0.0);
    aKnots.push_back(0.0);
    aKnots.push_back(0.0);
    aKnots.push_back(0.0);
    size_t Index1;
    for(Index1=0;Index1<nLengths-2;++Index1)
        {
        double dKnot=(aLengths[Index1]+aLengths[Index1+1]+aLengths[Index1+2])/3.0;
        aKnots.push_back(dKnot);
        }
    aKnots.push_back(1.0);
    aKnots.push_back(1.0);
    aKnots.push_back(1.0);
    aKnots.push_back(1.0);
    }

size_t FindKnots(std::vector<SGM::Point3D> const &aPoints,
                 std::vector<double>             &aKnots,
                 std::vector<double>             &aLengths,
                 std::vector<double>       const *pParams)
    {
    if(pParams)
        {
        aLengths=*pParams;
        }
    else
        {
        SGM::FindLengths3D(aPoints,aLengths,true);
        }
    size_t nDegree;
    FindDegree3Knots(aLengths,aKnots,nDegree);
    return nDegree;
    }

void FindControlPoints(std::vector<SGM::Point3D> const &aPoints,
                       std::vector<double>       const &aKnots,
                       std::vector<double>       const &aLengths,
                       size_t                           nDegree,
                       std::vector<SGM::Point3D>       &aControlPoints)
    {
    SGM::Interval1D Domain(0,1);

    // Set up the banded matrix.

    size_t nPoints=aPoints.size();
    std::vector<std::vector<double> > aaXMatrix;
    aaXMatrix.reserve(nPoints);
    size_t Index1;
    std::vector<double> aRow;
    aRow.reserve(6);
    aRow.push_back(0.0);
    aRow.push_back(0.0);
    aRow.push_back(1.0);
    aRow.push_back(0.0);
    aRow.push_back(0.0);
    aRow.push_back(aPoints[0].m_x);
    aaXMatrix.push_back(aRow);
    for(Index1=1;Index1<nPoints-1;++Index1)
        {
        aRow.clear();
        double *aaBasis[1];
        double dData[4];
        aaBasis[0]=dData;
        double t=aLengths[Index1];
        size_t nSpanIndex=FindSpanIndex(Domain,nDegree,t,aKnots);
        FindBasisFunctions(nSpanIndex,t,nDegree,0,&aKnots[0],aaBasis);
        if(nSpanIndex-Index1==1 && nDegree==3)
            {
            aRow.push_back(dData[0]);
            aRow.push_back(dData[1]);
            aRow.push_back(dData[2]);
            aRow.push_back(dData[3]);
            aRow.push_back(0.0);
            }
        else
            {
            aRow.push_back(0.0);
            aRow.push_back(dData[0]);
            aRow.push_back(dData[1]);
            aRow.push_back(dData[2]);
            aRow.push_back(dData[3]);
            }
        aRow.push_back(aPoints[Index1].m_x);
        aaXMatrix.push_back(aRow);
        }
    aRow.clear();
    aRow.push_back(0.0);
    aRow.push_back(0.0);
    aRow.push_back(1.0);
    aRow.push_back(0.0);
    aRow.push_back(0.0);
    aRow.push_back(aPoints[nPoints-1].m_x);
    aaXMatrix.push_back(aRow);

    // Solve for x, y, and z of the control points.

    std::vector<std::vector<double> > aaYMatrix=aaXMatrix;
    std::vector<std::vector<double> > aaZMatrix=aaXMatrix;
    for(Index1=0;Index1<nPoints;++Index1)
        {
        aaYMatrix[Index1][5]=aPoints[Index1].m_y;
        aaZMatrix[Index1][5]=aPoints[Index1].m_z;
        }
    SGM::BandedSolve(aaXMatrix);
    SGM::BandedSolve(aaYMatrix);
    SGM::BandedSolve(aaZMatrix);

    // Create the curve.

    aControlPoints.reserve(nPoints);
    for(Index1=0;Index1<nPoints;++Index1)
        {
        SGM::Point3D Pos(aaXMatrix[Index1].back(),aaYMatrix[Index1].back(),aaZMatrix[Index1].back());
        aControlPoints.push_back(Pos);
        }
    }

NUBcurve *CreateNUBCurve(SGM::Result                     &rResult,
                         std::vector<SGM::Point3D> const &aPoints,
                         std::vector<double>       const *pParams)
    {
    std::vector<SGM::Point3D> aControlPoints;
    std::vector<double> aKnots,aLengths;
    size_t dDegree=FindKnots(aPoints,aKnots,aLengths,pParams);
    FindControlPoints(aPoints,aKnots,aLengths,dDegree,aControlPoints);
    
    NUBcurve *pAnswer=new NUBcurve(rResult,std::move(aControlPoints),std::move(aKnots));

    return pAnswer;
    }

NUBcurve *CreateNUBCurveWithEndVectors(SGM::Result                     &rResult,
                                       std::vector<SGM::Point3D> const &aPoints,
                                       SGM::Vector3D             const &StartVec,
                                       SGM::Vector3D             const &EndVec,
                                       std::vector<double>       const *pParams)
    {
    // Find the knot vector.

    std::vector<double> aLengths,aKnots;
    if(pParams)
        {
        aLengths=*pParams;
        }
    else
        {
        SGM::FindLengths3D(aPoints,aLengths,true);
        }
    FindDegree3KnotsWithEndDirections(aLengths,aKnots);
    SGM::Interval1D Domain(0.0,1.0);
    
    // From "The NURBs Book" Algorithm A9.2, page 373.
    assert(!aPoints.empty());
    size_t nPoints=aPoints.size()-1;
    std::vector<SGM::Point3D> aControlPoints(nPoints+3,SGM::Point3D(0,0,0));
    aControlPoints[0]=aPoints[0];
    aControlPoints[1]=aPoints[0]+(aKnots[4]/3.0)*StartVec;
    aControlPoints[nPoints+1]=aPoints[nPoints]-((1-aKnots[nPoints+2])/3.0)*EndVec;
    aControlPoints[nPoints+2]=aPoints[nPoints];

    double *aaBasis[1];
    double dData[4];
    aaBasis[0]=dData;

    if(nPoints==2)
        {
        FindBasisFunctions(4,aKnots[4],3,0,&aKnots[0],aaBasis);
        aControlPoints[2].m_x=(aPoints[1].m_x-dData[2]*aControlPoints[1].m_x-dData[0]*aControlPoints[3].m_x)/dData[1];
        aControlPoints[2].m_y=(aPoints[1].m_y-dData[2]*aControlPoints[1].m_y-dData[0]*aControlPoints[3].m_y)/dData[1];
        aControlPoints[2].m_z=(aPoints[1].m_z-dData[2]*aControlPoints[1].m_z-dData[0]*aControlPoints[3].m_z)/dData[1];
        }
    else if(nPoints>2)
        {
        FindBasisFunctions(4,aKnots[4],3,0,&aKnots[0],aaBasis);
        double den=dData[1];
        aControlPoints[2].m_x=(aPoints[1].m_x-dData[0]*aControlPoints[1].m_x)/den;
        aControlPoints[2].m_y=(aPoints[1].m_y-dData[0]*aControlPoints[1].m_y)/den;
        aControlPoints[2].m_z=(aPoints[1].m_z-dData[0]*aControlPoints[1].m_z)/den;

        std::vector<double> dd(nPoints+1,0.0);
        size_t Index1;
        for(Index1=3;Index1<nPoints;++Index1)
            {
            dd[Index1]=dData[2]/den;
            FindBasisFunctions(Index1+2,aKnots[Index1+2],3,0,&aKnots[0],aaBasis);
            den=dData[1]-dData[0]*dd[Index1];
            aControlPoints[Index1].m_x=(aPoints[Index1-1].m_x-dData[0]*aControlPoints[Index1-1].m_x)/den;
            aControlPoints[Index1].m_y=(aPoints[Index1-1].m_y-dData[0]*aControlPoints[Index1-1].m_y)/den;
            aControlPoints[Index1].m_z=(aPoints[Index1-1].m_z-dData[0]*aControlPoints[Index1-1].m_z)/den;
            }
        dd[nPoints]=dData[2]/den;
        FindBasisFunctions(nPoints+2,aKnots[nPoints+2],3,0,&aKnots[0],aaBasis);
        den=dData[1]-dData[0]*dd[nPoints];
        aControlPoints[nPoints].m_x=(aPoints[nPoints-1].m_x-dData[2]*aControlPoints[nPoints+1].m_x-dData[0]*aControlPoints[nPoints-1].m_x)/den;
        aControlPoints[nPoints].m_y=(aPoints[nPoints-1].m_y-dData[2]*aControlPoints[nPoints+1].m_y-dData[0]*aControlPoints[nPoints-1].m_y)/den;
        aControlPoints[nPoints].m_z=(aPoints[nPoints-1].m_z-dData[2]*aControlPoints[nPoints+1].m_z-dData[0]*aControlPoints[nPoints-1].m_z)/den;
        for(Index1=nPoints-1;Index1>=2;--Index1)
            {
            aControlPoints[Index1].m_x-=dd[Index1+1]*aControlPoints[Index1+1].m_x;
            aControlPoints[Index1].m_y-=dd[Index1+1]*aControlPoints[Index1+1].m_y;
            aControlPoints[Index1].m_z-=dd[Index1+1]*aControlPoints[Index1+1].m_z;
            }
        }

    return new NUBcurve(rResult,std::move(aControlPoints),std::move(aKnots));
    }

complex *CreateComplex(SGM::Result                     &rResult,
                       std::vector<SGM::Point3D> const &aPoints,
                       std::vector<unsigned int> const &aSegments,
                       std::vector<unsigned int> const &aTriangles)
    {
    return new complex(rResult,aPoints,aSegments,aTriangles);
    }

complex *CreateComplex(SGM::Result  &rResult,
                       entity const *pEntity)
    {
    std::set<face *,EntityCompare> sFaces;
    FindFaces(rResult,pEntity,sFaces);
    std::vector<SGM::Point3D> aPoints;
    std::vector<unsigned int> aTriangles;
    for(face *pFace : sFaces)
        {
        std::vector<SGM::Point3D> const &aFacePoints=pFace->GetPoints3D(rResult);
        unsigned int nOffset=(unsigned int)aPoints.size();
        aPoints.insert(aPoints.end(),aFacePoints.begin(),aFacePoints.end());
        std::vector<unsigned int> const &aFaceTriangles=pFace->GetTriangles(rResult);
        for(auto nIndex : aFaceTriangles)
            {
            aTriangles.push_back(nIndex+nOffset);
            }
        }
    return new complex(rResult,aPoints,aTriangles);
    }

body *CreateDisk(SGM::Result             &rResult,
                 SGM::Point3D      const &Center,
                 SGM::UnitVector3D const &Normal,
                 double                   dRadius)
    {
    body   *pBody=new body(rResult); 
    volume *pVolume=new volume(rResult);
    pBody->AddVolume(pVolume);
    face *pFace=new face(rResult);
    pVolume->AddFace(rResult,pFace);
    SGM::UnitVector3D XAxis=Normal.Orthogonal();
    SGM::UnitVector3D YAxis=Normal*XAxis;
    surface *pSurface=new plane(rResult,Center,XAxis,YAxis,Normal);
    pFace->SetSurface(rResult,pSurface);
    curve *pCurve=new circle(rResult,Center,Normal,dRadius,&XAxis);
    edge *pEdge=SGMInternal::CreateEdge(rResult,pCurve,nullptr);
    pFace->AddEdge(rResult,pEdge,SGM::FaceOnLeftType);
    pFace->SetSides(2);
    return pBody;
    }


body *CoverPlanarWire(SGM::Result &rResult,
                      body        *pPlanarWire)
    {
    // Create the new body and move the edges onto a face.

    body *pBody=(body *)CopyEntity(rResult,pPlanarWire);

    face *pFace=new face(rResult);
    volume *pVolume=*(pBody->GetVolumes().begin());
    pVolume->AddFace(rResult,pFace);
    std::set<edge *,EntityCompare> sEdges=pVolume->GetEdges();
    for(edge *pEdge : sEdges)
        {
        pEdge->SetVolume(nullptr);
        pFace->AddEdge(rResult,pEdge,SGM::EdgeSideType::FaceOnUnknown);
        pVolume->RemoveEdge(rResult,pEdge);
        }

    // Order the edges.

    std::vector<edge *> aEdges;
    edge *pLastEdge=*sEdges.begin();
    aEdges.push_back(pLastEdge);
    sEdges.erase(pLastEdge);
    pFace->SetEdgeSideType(rResult,pLastEdge,SGM::EdgeSideType::FaceOnLeftType);
    vertex *pNextVertex=pLastEdge->GetEnd();
    while(sEdges.size())
        {
        bool bFound=false;
        for(edge *pEdge : sEdges)
            {
            if(pNextVertex==pEdge->GetStart())
                {
                pFace->SetEdgeSideType(rResult,pEdge,SGM::EdgeSideType::FaceOnLeftType);
                pLastEdge=pEdge;
                pNextVertex=pEdge->GetEnd();
                aEdges.push_back(pLastEdge);
                sEdges.erase(pEdge);
                bFound=true;
                break;
                }
            else if(pNextVertex==pEdge->GetEnd())
                {
                pFace->SetEdgeSideType(rResult,pEdge,SGM::EdgeSideType::FaceOnRightType);
                pLastEdge=pEdge;
                pNextVertex=pEdge->GetStart();
                aEdges.push_back(pLastEdge);
                sEdges.erase(pEdge);
                bFound=true;
                break;
                }
            }
        if(bFound==false)
            {
            return nullptr;
            }
        }

    // Find a bounding polygon and create plane in the correct direction.

    std::vector<SGM::Point3D> aPoints3D;
    for(edge *pEdge : aEdges)
        {
        FacetOptions Options;
        curve const *pCurve=pEdge->GetCurve();
        SGM::Interval1D const &Domain=pEdge->GetDomain();
        std::vector<double> aParams;
        std::vector<SGM::Point3D> aFacets;
        FacetCurve(pCurve,Domain,Options,aFacets,aParams);

        if(pFace->GetSideType(pEdge)==SGM::EdgeSideType::FaceOnRightType)
            {
            std::reverse(aFacets.begin(),aFacets.end());
            }
        aPoints3D.insert(aPoints3D.end(),++aFacets.begin(),aFacets.end());
        }
    SGM::Point3D Origin;
    SGM::UnitVector3D XVec,YVec,ZVec;
    SGM::FindLeastSquarePlane(aPoints3D,Origin,XVec,YVec,ZVec);
    std::vector<SGM::Point2D> aPoints2D;
    SGM::ProjectPointsToPlane(aPoints3D,Origin,XVec,YVec,ZVec,aPoints2D);
    double dArea=SGM::PolygonArea(aPoints2D);
    if(dArea<0)
        {
        YVec.Negate();
        ZVec.Negate();
        }
    pFace->SetSurface(rResult,new plane(rResult,Origin,XVec,YVec,ZVec));

    return pBody;
    }

void CreateFaceFromSurface(SGM::Result           &rResult,
                           surface               *pSurface,
                           face                  *pFace,
                           SGM::Interval2D const &Domain)
    {
    std::set<vertex*, EntityCompare> sVertices;
    if(pSurface->ClosedInV()==false)
        {
        double dStart=Domain.m_VDomain.m_dMin;
        double dEnd=Domain.m_VDomain.m_dMax;
        curve *pStart=pSurface->VParamLine(rResult,dStart);
        curve *pEnd=pSurface->VParamLine(rResult,dEnd);
        pStart->SetDomain(Domain.m_UDomain);
        pEnd->SetDomain(Domain.m_UDomain);
        edge *pEdgeStart=CreateEdge(rResult,pStart,nullptr);
        edge *pEdgeEnd=CreateEdge(rResult,pEnd,nullptr);
        if(pEdgeStart->GetStart())
            {
            sVertices.insert(pEdgeStart->GetStart());
            }
        if(pEdgeStart->GetEnd() && pEdgeStart->GetStart()!=pEdgeStart->GetEnd())
            {
            sVertices.insert(pEdgeStart->GetEnd());
            }
        if(pEdgeEnd->GetStart())
            {
            sVertices.insert(pEdgeEnd->GetStart());
            }
        if(pEdgeEnd->GetEnd() && pEdgeEnd->GetStart()!=pEdgeEnd->GetEnd())
            {
            sVertices.insert(pEdgeEnd->GetEnd());
            }
        pFace->AddEdge(rResult,pEdgeStart,SGM::EdgeSideType::FaceOnLeftType);
        pFace->AddEdge(rResult,pEdgeEnd,SGM::EdgeSideType::FaceOnRightType);
        }
    if(pSurface->ClosedInU()==false)
        {
        double dStart=Domain.m_UDomain.m_dMin;
        double dEnd=Domain.m_UDomain.m_dMax;
        curve *pStart=pSurface->UParamLine(rResult,dStart);
        curve *pEnd=pSurface->UParamLine(rResult,dEnd);
        pStart->SetDomain(Domain.m_VDomain);
        pEnd->SetDomain(Domain.m_VDomain);
        edge *pEdgeStart=CreateEdge(rResult,pStart,nullptr);
        edge *pEdgeEnd=CreateEdge(rResult,pEnd,nullptr);
        if(pEdgeStart->GetStart())
            {
            sVertices.insert(pEdgeStart->GetStart());
            }
        if(pEdgeStart->GetEnd() && pEdgeStart->GetStart()!=pEdgeStart->GetEnd())
            {
            sVertices.insert(pEdgeStart->GetEnd());
            }
        if(pEdgeEnd->GetStart())
            {
            sVertices.insert(pEdgeEnd->GetStart());
            }
        if(pEdgeEnd->GetEnd() && pEdgeEnd->GetStart()!=pEdgeEnd->GetEnd())
            {
            sVertices.insert(pEdgeEnd->GetEnd());
            }
        pFace->AddEdge(rResult,pEdgeStart,SGM::EdgeSideType::FaceOnRightType);
        pFace->AddEdge(rResult,pEdgeEnd,SGM::EdgeSideType::FaceOnLeftType);
        }
    if(pSurface->ClosedInU()==false && pSurface->ClosedInV()==false)
        {
        // Merge vertices.
        MergeVertexSet(rResult, sVertices);
        }
    }

face *CreateFaceFromSurface(SGM::Result                    &rResult,
                            surface                        *pSurface,
                            std::vector<edge *>            &aEdges,
                            std::vector<SGM::EdgeSideType> &aTypes,
                            SGM::Interval2D          const *pDomain)
    {
    face *pFace=new face(rResult);
    pFace->SetSurface(rResult,pSurface);
    size_t nEdges=aEdges.size();
    size_t Index1;
    if(nEdges)
        {
        std::set<vertex*, EntityCompare> sVertices;
        for(Index1=0;Index1<nEdges;++Index1)
            {
            edge *pEdge=aEdges[Index1];
            pFace->AddEdge(rResult,pEdge,aTypes[Index1]);
            if (pEdge->GetStart())
                sVertices.insert(pEdge->GetStart());
            if (pEdge->GetEnd())
                sVertices.insert(pEdge->GetEnd());
            }
        MergeVertexSet(rResult, sVertices);
        }
    else
        {
        SGM::Interval2D Domain=pSurface->GetDomain();
        if(pDomain)
            {
            Domain=*pDomain;
            }
        CreateFaceFromSurface(rResult,pSurface,pFace,Domain);
        }
    return pFace;
    }

body *CreateSheetBody(SGM::Result                    &rResult,
                      surface                        *pSurface,
                      std::vector<edge *>            &aEdges,
                      std::vector<SGM::EdgeSideType> &aTypes)
    {
    body   *pBody=new body(rResult); 
    volume *pVolume=new volume(rResult);
    pBody->AddVolume(pVolume);
    face *pFace=CreateFaceFromSurface(rResult,pSurface,aEdges,aTypes,nullptr);
    pVolume->AddFace(rResult,pFace);
    pFace->SetSurface(rResult,pSurface);
    pFace->SetSides(2);
    return pBody;
    }

body *CreateSheetBody(SGM::Result           &rResult,
                      surface               *pSurface,
                      SGM::Interval2D const &Domain)
    {
    body   *pBody=new body(rResult); 
    volume *pVolume=new volume(rResult);
    pBody->AddVolume(pVolume);
    std::vector<edge *> aEdges;
    std::vector<SGM::EdgeSideType> aTypes;
    face *pFace=CreateFaceFromSurface(rResult,pSurface,aEdges,aTypes,&Domain);
    pVolume->AddFace(rResult,pFace);
    pFace->SetSurface(rResult,pSurface);
    pFace->SetSides(2);
    return pBody;
    }

body *CreateRevolve(SGM::Result             &rResult,
                    SGM::Point3D      const &Origin,
                    SGM::UnitVector3D const &Axis,
                    curve                   *pCurve)
    {
    SGM::Point3D pCurveStart;
    SGM::Point3D pCurveEnd;

    // TODO
    // check for valid curve input
    // check for planar curve
    // check for axis in plane of curve
    // check for curve intersection with axis

    pCurve->Evaluate(pCurve->GetDomain().m_dMin, &pCurveStart);
    pCurve->Evaluate(pCurve->GetDomain().m_dMax, &pCurveEnd);

    body   *pBody=new body(rResult);
    volume *pVolume=new volume(rResult);

    face *pFace=new face(rResult);

    edge *pEdgeStart=new edge(rResult);
    edge *pEdgeEnd=new edge(rResult);

    revolve *pRevolve=new revolve(rResult,Origin,Axis,pCurve);

    SGM::Point3D StartCenter = Origin + ((pCurveStart - Origin) % Axis) * Axis;
    SGM::Point3D EndCenter = Origin + ((pCurveEnd - Origin) % Axis) * Axis;
    SGM::UnitVector3D XAxis(pCurveStart - StartCenter);
    double dRadiusStart = pCurveStart.Distance(StartCenter);
    double dRadiusEnd = pCurveEnd.Distance(EndCenter);

    circle *pCircleStart=new circle(rResult,StartCenter,-Axis,dRadiusStart,&XAxis);
    circle *pCircleEnd=new circle(rResult,EndCenter,Axis,dRadiusEnd,&XAxis);

    // Connect everything.

    pBody->AddVolume(pVolume);

    pVolume->AddFace(rResult,pFace);

    pFace->AddEdge(rResult,pEdgeStart,SGM::FaceOnRightType);
    pFace->AddEdge(rResult,pEdgeEnd,SGM::FaceOnRightType);

    pFace->SetSurface(rResult,pRevolve);
    pFace->SetSides(2);

    pEdgeStart->SetCurve(rResult,pCircleStart);
    pEdgeEnd->SetCurve(rResult,pCircleEnd);

    pEdgeStart->SetDomain(rResult,SGM::Interval1D(0,SGM_TWO_PI));
    pEdgeEnd->SetDomain(rResult,SGM::Interval1D(0,SGM_TWO_PI));

    return pBody;
    }

surface *CreateRevolveSurface(SGM::Result             &rResult,
                              SGM::Point3D      const &Origin,
                              SGM::UnitVector3D const &Axis,
                              curve                   *pCurve)
    {
    surface *pRevolve=new revolve(rResult,Origin,Axis,pCurve);
    return pRevolve;
    }

surface *CreateExtrudeSurface(SGM::Result             &rResult,
                              SGM::UnitVector3D const &Axis,
                              curve                   *pCurve)
    {
    surface *pExtrude=new extrude(rResult,Axis,pCurve);
    return pExtrude;
    }

} // end namespace SGMInternal
