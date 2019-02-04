﻿#include <cfloat>
#include <cmath>
#include <vector>
#include <list>

#include "SGMVector.h"
#include "SGMTransform.h"
#include "SGMIntersector.h"

#include "Intersectors.h"
#include "FacetToBRep.h"
#include "Topology.h"

namespace SGMInternal
{

// Returns the two silhouette lines of a cylinder as seen 
// from a given point.  Note that the axises of the lines
// is the axis of the cylinder.  It is assumed that the 
// Eye position is not on the axis of the cylinder.

void FindCylinderSilhouette(cylinder     const *pCylinder,
                            SGM::Point3D const &EyePos,
                            SGM::Point3D       &Origin1,
                            SGM::Point3D       &Origin2)
    {
    SGM::UnitVector3D const &CylinderAxis=pCylinder->m_ZAxis;
    SGM::Point3D const &CylinderOrigin=pCylinder->m_Origin;
    double dRadius=pCylinder->m_dRadius;
    SGM::Point3D PointOnAxis=CylinderOrigin+CylinderAxis*(CylinderAxis%(EyePos-CylinderOrigin));
    double dDist=PointOnAxis.Distance(EyePos);
    double a=SGM_HALF_PI-SGM::SAFEasin(dRadius/dDist);
    SGM::UnitVector3D XAxis=EyePos-PointOnAxis;
    SGM::UnitVector3D YAxis=CylinderAxis*XAxis;
    double dCos=cos(a);
    double dSin=sin(a);
    Origin1=PointOnAxis+(dCos*XAxis+dSin*YAxis)*dRadius;
    Origin2=PointOnAxis+(dCos*XAxis-dSin*YAxis)*dRadius;
    }

// Returns point on two lines through the apex of the cone that
// are the silhouettes of the cone with respect to the given EyePos.

void FindConeSilhouette(cone         const *pCone,
                        SGM::Point3D const &InEyePos,
                        SGM::Point3D       &Origin1,
                        SGM::Point3D       &Origin2)
    {
    SGM::Point3D EyePos=InEyePos;
    SGM::UnitVector3D const &ConeAxis=pCone->m_ZAxis;
    SGM::Point3D const &ConeOrigin=pCone->m_Origin;
    SGM::Point3D PointOnAxis=ConeOrigin+ConeAxis*(ConeAxis%(EyePos-ConeOrigin));
    SGM::Point3D Apex=pCone->FindApex();
    if(SGM::NearEqual(PointOnAxis,Apex,SGM_MIN_TOL))
        {
        EyePos+=ConeAxis;
        PointOnAxis=ConeOrigin+ConeAxis*(ConeAxis%(EyePos-ConeOrigin));
        }
    double EyeDist=PointOnAxis.Distance(Apex);
    double ApexDist=Apex.Distance(ConeOrigin);
    double dConeRadius=pCone->m_dRadius;
    double dRadius=dConeRadius*EyeDist/ApexDist;
    double dDist=PointOnAxis.Distance(EyePos);
    double a=SGM_HALF_PI-SGM::SAFEasin(dRadius/dDist);
    SGM::UnitVector3D XAxis=EyePos-PointOnAxis;
    SGM::UnitVector3D YAxis=ConeAxis*XAxis;
    double dCos=cos(a);
    double dSin=sin(a);
    SGM::Vector3D TestVec=Apex-PointOnAxis;
    if(TestVec%ConeAxis<0)
        {
        PointOnAxis=PointOnAxis+TestVec*2;
        }
    Origin1=PointOnAxis+(dCos*XAxis+dSin*YAxis)*dRadius;
    Origin2=PointOnAxis+(dCos*XAxis-dSin*YAxis)*dRadius;
    }

// Given to conic equations of the form A*x^2+B*y^2+C*x*y+D*x+E*y+F=0
// return the up to four points where the equal.  The case where to
// two equations equal everywhere is not handled.

size_t SolveTwoConics(std::vector<double> const &aCoefficients1,
                      std::vector<double> const &aCoefficients2,
                      std::vector<SGM::Point2D> &aPoints,
                      double                     dTolerance)
    {
    if( fabs(aCoefficients1[1])<SGM_MIN_TOL ||
        fabs(aCoefficients2[1])<SGM_MIN_TOL)
        {
        // The degnerate case that needs to be solved in different way.
        }
    else
        {
        // Eliminate C from the first equation, solve for y in terms of x.
        // plug the x version of y into the first equation and find the
        // cubic of x to solve.  Once the x values are found then find the
        // the y values by pluging them into the original equation.

        double a1=aCoefficients1[0];
        double b1=aCoefficients1[1];
        double c1=aCoefficients1[2];
        double d1=aCoefficients1[3];
        double e1=aCoefficients1[4];
        double f1=aCoefficients1[5];

        double a2=aCoefficients2[0];
        double b2=aCoefficients2[1];
        double c2=aCoefficients2[2];
        double d2=aCoefficients2[3];
        double e2=aCoefficients2[4];
        double f2=aCoefficients2[5];

        double a3=a1*b2-a2*b1;
        double c3=c1*b2-c2*b1;
        double d3=d1*b2-d2*b1;
        double f3=f1*b2-f2*b1;
        double e3=e1*b2-e2*b1;

        // (a1*x2+d1*x+f1)(c3*x+e3)(c3*x+e3)+b1(a3*x2+d3*x+f3)(a3*x2+d3*x+f3)−(c1*x+e1)(c3*x+e3)(a3*x2+d3*x+f3)
        //
        // (a1*c3*x3+a1*e3*x2 + d1*c3*x2+d1*e3*x + f1*c3*x+f1*e3)(c3*x+e3)+
        // b1(a3*a3*x4+a3*d3*x3+a3*f3*x2 + d3*a3*x3+d3*d3*x2+d3*f3 + f3*a3*x2+f3*d3*x+f3*f3)-
        // (c1*c3*x2 + c1*e3*x + e1*c3*x + e1*e3)(a3*x2 + d3*x + f3)
        //
        // (c3*a1*c3*x4 + c3*a1*e3*x3 + c3*d1*c3*x3 + c3*d1*e3*x2 + c3*f1*c3*x2 + c3*f1*e3*x) +
        // (e3*a1*c3*x3 + e3*a1*e3*x2 + e3*d1*c3*x2 + e3*d1*e3*x  + e3*f1*c3*x  + e3*f1*e3)   +
        // (b1*a3*a3*x4 + b1*a3*d3*x3 + b1*a3*f3*x2 + b1*d3*a3*x3 + b1*d3*d3*x2 + b1*d3*f3    + b1*f3*a3*x2 + b1*f3*d3*x + b1*f3*f3)-
        // (a3*c1*c3*x4 + a3*c1*e3*x3 + a3*e1*c3*x3 + a3*e1*e3*x2 +
        //  d3*c1*c3*x3 + d3*c1*e3*x2 + d3*e1*c3*x2 + d3*e1*e3*x  +
        //  f3*c1*c3*x2 + f3*c1*e3*x  + f3*e1*c3*x  + f3*e1*e3)
        //
        double C4 = c3*a1*c3 + e3*a1*c3 + b1*a3*a3 - a3*c1*c3;
        double C3 = c3*a1*e3 + c3*d1*c3 + e3*a1*c3 + b1*a3*d3 + b1*d3*a3 - a3*c1*e3 - a3*e1*c3 - d3*c1*c3;
        double C2 = c3*d1*e3 + c3*f1*c3 + e3*a1*e3 + e3*d1*c3 + b1*a3*f3 + b1*d3*d3 + b1*f3*a3 - a3*e1*e3 - d3*c1*e3 - d3*e1*c3 - f3*c1*c3;
        double C1 = c3*f1*e3 + e3*d1*e3 + e3*f1*c3 + b1*f3*d3 - d3*e1*e3 - f3*c1*e3 - f3*e1*c3;
        double C0 = e3*f1*e3 + b1*d3*f3 + b1*f3*f3 - f3*e1*e3;

        std::vector<double> aRoots;
        SGM::Quartic(C4,C3,C2,C1,C0,aRoots,dTolerance);

        // a1*x*x + b1*y*y + c1*x*y + d1*x + e1*y + f1 = 0
        // b1*y*y + (c1*x + e1)*y + (a1*x*x + d1*x + f1) = 0

        for(double x : aRoots)
            {
            std::vector<double> aQRoots;
            SGM::Quadratic(b1,c1*x+e1,a1*x*x+d1*x+f1,aQRoots);
            for(double y : aQRoots)
                {
                aPoints.push_back(SGM::Point2D(x,y));
                }
            }
        SGM::RemoveDuplicates2D(aPoints,dTolerance);
        }
    return aPoints.size();
    }

// Returns <A,B,C,D,E,F> such that A*x^2+B*y^2+C*x*y+D*x+E*y+F=0.
// Divide through by A to get x^2+(B/A)*y^2+(C/A)*x*y+(D/A)*x+(E/A)*y+(F/A)=0.

bool FindConicCoefficient(std::vector<SGM::Point2D> const &aPoints,
                          std::vector<double>             &aCoefficients)
    {
    std::vector<std::vector<double> > aaMatrix;
    aaMatrix.reserve(5);
    for(SGM::Point2D const &Pos : aPoints)
        {
        double dX=Pos.m_u;
        double dY=Pos.m_v;
        std::vector<double> aRow;
        aRow.reserve(6);
        aRow.push_back(dY*dY);
        aRow.push_back(dX*dY);
        aRow.push_back(dX);
        aRow.push_back(dY);
        aRow.push_back(1);
        aRow.push_back(-dX*dX);
        aaMatrix.push_back(aRow);
        }
    bool bAnswer=SGM::LinearSolve(aaMatrix);
    aCoefficients.reserve(6);
    double B=aaMatrix[0].back();
    double C=aaMatrix[1].back();
    double D=aaMatrix[2].back();
    double E=aaMatrix[3].back();
    double F=aaMatrix[4].back();
    aCoefficients.push_back(1);
    aCoefficients.push_back(B);
    aCoefficients.push_back(C);
    aCoefficients.push_back(D);
    aCoefficients.push_back(E);
    aCoefficients.push_back(F);
    return bAnswer;
    }

bool PointOnCurves(SGM::Point3D         const &Pos,
                   std::vector<curve *> const &aCurves)
    {
    for(curve *pCurve : aCurves)
        {
        SGM::Point3D TestPos;
        pCurve->Inverse(Pos,&TestPos);
        double dDistSquared=Pos.DistanceSquared(TestPos);
        if(dDistSquared<SGM_MIN_TOL)
            {
            return true;
            }
        }
    return false;
    }

size_t OrderAndRemoveDuplicates(SGM::Point3D                 const &Origin,
                                SGM::UnitVector3D            const &Axis,
                                double                              dTolerance,
                                bool                                bUseWholeLine,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes)
    {
    if(size_t nAllHits=aPoints.size())
        {
        // Save all the points that occur in the right direction from the ray origin
        // Put them into aParams
        std::vector<std::pair<double,size_t> > aParams;
        aParams.reserve(nAllHits);
        for(size_t Index1=0;Index1<nAllHits;++Index1)
            {
            const double ray_distance = (aPoints[Index1]-Origin)%Axis;
            // compare to the following which ignores the axis
            // const double ray_distance = Origin.Distance(aPoints[Index1]);
            if(bUseWholeLine || -dTolerance<ray_distance)
                {
                aParams.emplace_back(ray_distance, Index1);
                }
            }

        // no points were valid
        if ( aParams.empty() )
            {
            aPoints.clear();
            aTypes.clear();
            return 0;    
            }
        // only one point was given, and it was valid, so no need to do anything
        else if ( aParams.size() == 1 && aPoints.size() == 1 )
            {
            return 1;
            }

        // Order them
        std::sort(aParams.begin(),aParams.end());

        // Remove duplicates. They are consecutive.
        const size_t nGoodHits=aParams.size();
        std::vector<SGM::Point3D> aTempPoints;
        std::vector<SGM::IntersectionType> aTempTypes;
        aTempPoints.reserve(nGoodHits);
        aTempTypes .reserve(nGoodHits);
        aTempPoints.push_back(aPoints[aParams[0].second]);
        aTempTypes .push_back( aTypes[aParams[0].second]);
        double dLastParam=aParams[0].first;
        for(size_t Index1=1;Index1<nGoodHits;++Index1)
            {
            if(dTolerance<aParams[Index1].first-dLastParam)
                {
                dLastParam=aParams[Index1].first;
                aTempPoints.push_back(aPoints[aParams[Index1].second]);
                aTempTypes .push_back( aTypes[aParams[Index1].second]);
                }
            }
        aPoints.swap(aTempPoints);
        aTypes .swap(aTempTypes);
        }
    return aPoints.size();
    }

size_t RayFireEdge(SGM::Result                        &rResult,
                   SGM::Point3D                 const &Origin,
                   SGM::UnitVector3D            const &Axis,
                   edge                         const *pEdge,
                   std::vector<SGM::Point3D>          &aPoints,
                   std::vector<SGM::IntersectionType> &aTypes,
                   double                              dTolerance,
                   bool                                bUseWholeLine)
    {
    std::vector<SGM::Point3D> aAllPoints;
    std::vector<SGM::IntersectionType> aAllTypes;
    SGM::Interval1D Domain(-dTolerance,SGM_MAX);
    curve const *pCurve=pEdge->GetCurve();
    std::vector<SGM::Point3D> aTempPoints;
    std::vector<SGM::IntersectionType> aTempTypes;
    size_t nHits=IntersectLineAndCurve(rResult,Origin,Axis,Domain,pCurve,dTolerance,aTempPoints,aTempTypes);
    size_t Index1;
    for(Index1=0;Index1<nHits;++Index1)
        {
        SGM::Point3D const &Pos=aTempPoints[Index1];
        double t=pCurve->Inverse(Pos);
        if(pEdge->GetDomain().InInterval(t,dTolerance))
            {
            aAllPoints.push_back(Pos);
            aAllTypes.push_back(aTempTypes[Index1]);
            }
        }
    size_t nAnswer=OrderAndRemoveDuplicates(Origin,Axis,dTolerance,bUseWholeLine,aAllPoints,aAllTypes);
    aPoints=aAllPoints;
    aTypes=aAllTypes;
    return nAnswer;
    }

size_t RayFireComplex(SGM::Result                        &,//rResult,
                      SGM::Point3D                 const &Origin,
                      SGM::UnitVector3D            const &Axis,
                      complex                      const *pComplex,
                      std::vector<SGM::Point3D>          &aInPoints,
                      std::vector<SGM::IntersectionType> &aInTypes,
                      double                              dTolerance,
                      bool                                bUseWholeLine)
    {
    std::vector<SGM::Point3D> aAllPoints;
    std::vector<SGM::IntersectionType> aAllTypes;
    SGM::BoxTree const &BTree=pComplex->GetTree();
    std::vector<SGM::BoxTree::BoundedItemType> aHits;
    SGM::Ray3D ray(Origin,Axis);
    SGM::Interval1D Domain;
    if(bUseWholeLine)
        {
        aHits=BTree.FindIntersectsLine(ray,dTolerance);
        Domain.m_dMin=-SGM_MAX;
        Domain.m_dMax=SGM_MAX;
        }
    else
        {
        aHits=BTree.FindIntersectsRay(ray,dTolerance);
        Domain.m_dMin=-dTolerance;
        Domain.m_dMax=SGM_MAX;
        }
    size_t nHits=aHits.size();
    size_t Index1;
    std::vector<unsigned int> const &aTriangles=pComplex->GetTriangles();
    std::vector<SGM::Point3D> const &aComplexPoints=pComplex->GetPoints();
    unsigned int const *pBase=&aTriangles[0];
    for(Index1=0;Index1<nHits;++Index1)
        {
        unsigned int const *pHit=(unsigned int const *)aHits[Index1].first;
        size_t nWhere=(size_t)(pHit-pBase);
        unsigned int a=aTriangles[nWhere];
        unsigned int b=aTriangles[nWhere+1];
        unsigned int c=aTriangles[nWhere+2];
        SGM::Point3D const &A=aComplexPoints[a];
        SGM::Point3D const &B=aComplexPoints[b];
        SGM::Point3D const &C=aComplexPoints[c];
        SGM::UnitVector3D Norm=(B-A)*(C-A);

        std::vector<SGM::Point3D> aIntPoints;
        std::vector<SGM::IntersectionType> aIntTypes;
        size_t nInts=IntersectLineAndPlane(Origin,Axis,Domain,A,Norm,dTolerance,aIntPoints,aIntTypes);

        if(nInts==1)
            {
            SGM::UnitVector3D XVec=B-A;
            SGM::UnitVector3D YVec=Norm*XVec;
            SGM::Point3D D=aIntPoints[0];
            SGM::Point2D Auv(0,0);
            SGM::Vector3D BVec=B-A;
            SGM::Point2D Buv(XVec%BVec,YVec%BVec);
            SGM::Vector3D CVec=C-A;
            SGM::Point2D Cuv(XVec%CVec,YVec%CVec);
            SGM::Vector3D DVec=D-A;
            SGM::Point2D Duv(XVec%DVec,YVec%DVec);
            if(SGM::InTriangle(Auv,Buv,Cuv,Duv))
                {
                aAllPoints.push_back(D);
                aAllTypes.push_back(SGM::PointType);
                }
            }
        }

    size_t nAnswer=OrderAndRemoveDuplicates(Origin,Axis,dTolerance,bUseWholeLine,aAllPoints,aAllTypes);
    aInPoints=aAllPoints;
    aInTypes=aAllTypes;
    return nAnswer;
    }

size_t RayFireFace(SGM::Result                        &rResult,
                   SGM::Point3D                 const &Origin,
                   SGM::UnitVector3D            const &Axis,
                   face                         const *pFace,
                   std::vector<SGM::Point3D>          &aPoints,
                   std::vector<SGM::IntersectionType> &aTypes,
                   double                              dTolerance,
                   bool                                bUseWholeLine)
    {
    std::vector<SGM::Point3D> aAllPoints;
    std::vector<SGM::IntersectionType> aAllTypes;
    SGM::Interval1D Domain(-dTolerance,SGM_MAX);
    surface const *pSurface=pFace->GetSurface();
    std::vector<SGM::Point3D> aTempPoints;
    std::vector<SGM::IntersectionType> aTempTypes;
    size_t nHits=IntersectLineAndSurface(rResult,Origin,Axis,Domain,pSurface,dTolerance,aTempPoints,aTempTypes);
    size_t Index1,Index2;
    for(Index1=0;Index1<nHits;++Index1)
        {
        SGM::Point3D const &Pos=aTempPoints[Index1];
        SGM::Point2D uv=pSurface->Inverse(Pos);
        if(aTempTypes[Index1]==SGM::CoincidentType)
            {
            uv=pSurface->Inverse(Origin);
            if(pFace->PointInFace(rResult,uv))
                {
                aAllPoints.push_back(Origin);
                aAllTypes.push_back(SGM::CoincidentType);
                }
            else
                {
                std::set<edge *,EntityCompare> const &sEdges=pFace->GetEdges();
                std::vector<SGM::Point3D> aRayFirePoints;
                std::vector<SGM::IntersectionType> aRayFireTypes;
                for(edge *pEdge : sEdges)
                    {
                    RayFireEdge(rResult,Origin,Axis,pEdge,aRayFirePoints,aRayFireTypes,dTolerance,bUseWholeLine);
                    size_t nTempPoints=aRayFirePoints.size();
                    for(Index2=0;Index2<nTempPoints;++Index2)
                        {
                        aAllPoints.push_back(aRayFirePoints[Index2]);
                        aAllTypes.push_back(SGM::CoincidentType);
                        }
                    }
                }
            }
        else
            {
            if(pFace->PointInFace(rResult,uv))
                {
                aAllPoints.push_back(Pos);
                aAllTypes.push_back(aTempTypes[Index1]);
                }
            }
        }
    
    size_t nAnswer=OrderAndRemoveDuplicates(Origin,Axis,dTolerance,bUseWholeLine,aAllPoints,aAllTypes);
    aPoints=aAllPoints;
    aTypes=aAllTypes;
    return nAnswer;
    }

size_t RayFireVolume(SGM::Result                        &rResult,
                     SGM::Point3D                 const &Origin,
                     SGM::UnitVector3D            const &Axis,
                     volume                       const *pVolume,
                     std::vector<SGM::Point3D>          &aPoints,
                     std::vector<SGM::IntersectionType> &aTypes,
                     double                              dTolerance,
                     bool                                bUseWholeLine)
    {
    // Find all ray hits for all faces.

    SGM::BoxTree const &FaceTree=pVolume->GetFaceTree(rResult);
    SGM::Ray3D Ray(Origin,Axis);
    std::vector<SGM::BoxTree::BoundedItemType> aHitFaces=FaceTree.FindIntersectsRay(Ray,dTolerance);

    std::vector<SGM::Point3D> aAllPoints;
    std::vector<SGM::IntersectionType> aAllTypes;
    for (auto boundedItem : aHitFaces)
        {
        face * pFace = (face*)boundedItem.first;
        std::vector<SGM::Point3D> aSubPoints;
        std::vector<SGM::IntersectionType> aSubTypes;
        size_t nHits=RayFireFace(rResult,Origin,Axis,pFace,aSubPoints,aSubTypes,dTolerance,bUseWholeLine);
        for(size_t Index1=0;Index1<nHits;++Index1)
            {
            aAllPoints.push_back(aSubPoints[Index1]);
            aAllTypes.push_back(aSubTypes[Index1]);
            }
        }

    size_t nAnswer=OrderAndRemoveDuplicates(Origin,Axis,dTolerance,bUseWholeLine,aAllPoints,aAllTypes);
    aPoints=aAllPoints;
    aTypes=aAllTypes;
    return nAnswer;
    }

size_t RayFireBody(SGM::Result                        &rResult,
                   SGM::Point3D                 const &Origin,
                   SGM::UnitVector3D            const &Axis,
                   body                         const *pBody,
                   std::vector<SGM::Point3D>          &aPoints,
                   std::vector<SGM::IntersectionType> &aTypes,
                   double                              dTolerance,
                   bool                                bUseWholeLine)
    {
    // Find all ray hits for all volumes.

    std::set<volume *,EntityCompare> const &sVolumes=pBody->GetVolumes();
    std::vector<SGM::Point3D> aAllPoints;
    std::vector<SGM::IntersectionType> aAllTypes;
    for (auto pVolume : sVolumes)
        {
        std::vector<SGM::Point3D> aSubPoints;
        std::vector<SGM::IntersectionType> aSubTypes;
        size_t nHits=RayFireVolume(rResult,Origin,Axis,pVolume,aSubPoints,aSubTypes,dTolerance,bUseWholeLine);
        for(size_t Index1=0;Index1<nHits;++Index1)
            {
            aAllPoints.push_back(aSubPoints[Index1]);
            aAllTypes.push_back(aSubTypes[Index1]);
            }
        }

    size_t nAnswer=OrderAndRemoveDuplicates(Origin,Axis,dTolerance,bUseWholeLine,aAllPoints,aAllTypes);
    aPoints=aAllPoints;
    aTypes=aAllTypes;
    return nAnswer;
    }

inline void MovePointsAndTypes(std::vector<SGM::Point3D>          &aSubPoints,
                               std::vector<SGM::IntersectionType> &aSubTypes,
                               std::vector<SGM::Point3D>          &aPoints,
                               std::vector<SGM::IntersectionType> &aTypes)
    {
    aPoints.reserve(aPoints.size() + aSubPoints.size());
    aPoints.insert(aPoints.end(), aSubPoints.begin(), aSubPoints.end());
    aSubPoints.clear();

    aTypes.reserve(aTypes.size() + aSubTypes.size());
    aTypes.insert(aTypes.end(), aSubTypes.begin(), aSubTypes.end());
    aSubTypes.clear();
    }

size_t RayFireThing(SGM::Result                        &rResult,
                    SGM::Point3D                 const &Origin,
                    SGM::UnitVector3D            const &Axis,
                    thing                        const *pThing,
                    std::vector<SGM::Point3D>          &aPoints,
                    std::vector<SGM::IntersectionType> &aTypes,
                    double                              dTolerance,
                    bool                                bUseWholeLine)
    {
    // Find all top level bodies, volumes, complexes, and faces.

    aPoints.clear();
    aTypes.clear();

    std::vector<SGM::Point3D> aSubPoints;
    std::vector<SGM::IntersectionType> aSubTypes;

    std::set<body *,EntityCompare> sBodies;
    FindBodies(rResult,pThing,sBodies,true);
    for (auto pBody : sBodies)
        {
        RayFireBody(rResult,Origin,Axis,pBody,aSubPoints,aSubTypes,dTolerance,bUseWholeLine);
        MovePointsAndTypes(aSubPoints, aSubTypes, aPoints, aTypes);
        }

    std::set<volume *,EntityCompare> sVolumes;
    FindVolumes(rResult,pThing,sVolumes,true);
    for (auto pVolume : sVolumes)
        {
        RayFireVolume(rResult,Origin,Axis,pVolume,aSubPoints,aSubTypes,dTolerance,bUseWholeLine);
        MovePointsAndTypes(aSubPoints, aSubTypes, aPoints, aTypes);
        }

    std::set<complex *,EntityCompare> sComplexes;
    FindComplexes(rResult,pThing,sComplexes,true);
    for (auto pComplex : sComplexes)
        {
        RayFireComplex(rResult,Origin,Axis,pComplex,aSubPoints,aSubTypes,dTolerance,bUseWholeLine);
        MovePointsAndTypes(aSubPoints, aSubTypes, aPoints, aTypes);
        }
    
    std::set<face *,EntityCompare> sFaces;
    FindFaces(rResult,pThing,sFaces,true);
    for (auto pFace : sFaces)
        {
        RayFireFace(rResult,Origin,Axis,pFace,aSubPoints,aSubTypes,dTolerance,bUseWholeLine);
        MovePointsAndTypes(aSubPoints, aSubTypes, aPoints, aTypes);
        }

    size_t nAnswer=OrderAndRemoveDuplicates(Origin,Axis,dTolerance,bUseWholeLine,aPoints,aTypes);
    return nAnswer;
    }

size_t IntersectSegment(SGM::Result               &rResult,
                        SGM::Segment3D      const &Segment,
                        entity              const *pEntity,
                        std::vector<SGM::Point3D> &aPoints,
                        double                     dTolerance)
    {
    std::vector<SGM::IntersectionType> aTypes;
    std::vector<SGM::Point3D> aTempPoints;
    SGM::Point3D Origin=Segment.m_Start;
    SGM::UnitVector3D Axis=Segment.m_End-Origin;
    size_t nHits=RayFire(rResult,Origin,Axis,pEntity,aTempPoints,aTypes,dTolerance);
    double dLengthSquared=Segment.LengthSquared();
    size_t Index1,nAnswer=0;
    for(Index1=0;Index1<nHits;++Index1)
        {
        double dTest=Origin.DistanceSquared(aTempPoints[Index1]);
        if(dTest<dLengthSquared+dTolerance)
            {
            aPoints.push_back(aTempPoints[Index1]);
            ++nAnswer;
            }
        }
    return nAnswer;
    }

size_t RayFire(SGM::Result                        &rResult,
               SGM::Point3D                 const &Origin,
               SGM::UnitVector3D            const &Axis,
               entity                       const *pEntity,
               std::vector<SGM::Point3D>          &aPoints,
               std::vector<SGM::IntersectionType> &aTypes,
               double                              dTolerance,
               bool                                bUseWholeLine)
    {
    switch(pEntity->GetType())
        {
        case SGM::ThingType:
            {
            return RayFireThing(rResult,Origin,Axis,(thing const *)pEntity,aPoints,aTypes,dTolerance,bUseWholeLine);
            }
        case SGM::BodyType:
            {
            return RayFireBody(rResult,Origin,Axis,(body const *)pEntity,aPoints,aTypes,dTolerance,bUseWholeLine);
            }
        case SGM::VolumeType:
            {
            return RayFireVolume(rResult,Origin,Axis,(volume const *)pEntity,aPoints,aTypes,dTolerance,bUseWholeLine);
            }
        case SGM::FaceType:
            {
            return RayFireFace(rResult,Origin,Axis,(face const *)pEntity,aPoints,aTypes,dTolerance,bUseWholeLine);
            }
        case SGM::EdgeType:
            {
            return RayFireEdge(rResult,Origin,Axis,(edge const *)pEntity,aPoints,aTypes,dTolerance,bUseWholeLine);
            }
        case SGM::ComplexType:
            {
            return RayFireComplex(rResult,Origin,Axis,(complex const *)pEntity,aPoints,aTypes,dTolerance,bUseWholeLine);
            }
        default:
            {
            throw;
            break;
            }
        }
    }

void IntersectNonParallelPlanes(SGM::Point3D      const &Origin1,
                                SGM::UnitVector3D const &Normal1,
                                SGM::Point3D      const &Origin2,
                                SGM::UnitVector3D const &Normal2,
                                SGM::Point3D            &Origin,
                                SGM::UnitVector3D       &Axis)
    {
    double AxisX=Normal1.m_y*Normal2.m_z-Normal1.m_z*Normal2.m_y;
    double AxisY=Normal1.m_z*Normal2.m_x-Normal1.m_x*Normal2.m_z;
    double AxisZ=Normal1.m_x*Normal2.m_y-Normal1.m_y*Normal2.m_x;

    double dx=fabs(AxisX);
    double dy=fabs(AxisY);
    double dz=fabs(AxisZ);

    double dLength=sqrt(AxisX*AxisX+AxisY*AxisY+AxisZ*AxisZ);
    Axis.m_x=AxisX/dLength;
    Axis.m_y=AxisY/dLength;
    Axis.m_z=AxisZ/dLength;

    double dPlaneDist1=-Origin1.m_x*Normal1.m_x-Origin1.m_y*Normal1.m_y-Origin1.m_z*Normal1.m_z;
    double dPlaneDist2=-Origin2.m_x*Normal2.m_x-Origin2.m_y*Normal2.m_y-Origin2.m_z*Normal2.m_z;
    if(dy<=dx && dz<=dx)
        {
        Origin.m_x=0.0;
        Origin.m_y=(dPlaneDist2*Normal1.m_z-dPlaneDist1*Normal2.m_z)/AxisX;
        Origin.m_z=(dPlaneDist1*Normal2.m_y-dPlaneDist2*Normal1.m_y)/AxisX;
        }
    else if(dz<dy)
        {
        Origin.m_x=(dPlaneDist1*Normal2.m_z-dPlaneDist2*Normal1.m_z)/AxisY;
        Origin.m_y=0.0;
        Origin.m_z=(dPlaneDist2*Normal1.m_x-dPlaneDist1*Normal2.m_x)/AxisY;
        }
    else
        {
        Origin.m_x=(dPlaneDist2*Normal1.m_y-dPlaneDist1*Normal2.m_y)/AxisZ;
        Origin.m_y=(dPlaneDist1*Normal2.m_x-dPlaneDist2*Normal1.m_x)/AxisZ;
        Origin.m_z=0.0;
        }
    }

size_t IntersectLineAndCylinder(SGM::Point3D                 const &Origin,
                                SGM::UnitVector3D            const &Axis,
                                SGM::Interval1D              const &Domain,
                                cylinder                     const *pCylinder,
                                double                              dTolerance,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes)
    {
    // IsEmpty, coincident, one tangent point, or two points.

    SGM::UnitVector3D const &CylinderAxis=pCylinder->m_ZAxis;
    SGM::Point3D const &Center=pCylinder->m_Origin;
    SGM::Point3D Pos=Origin+Axis;
    double dRadius=pCylinder->m_dRadius;
    if(fabs(fabs(CylinderAxis%Axis)-1.0)<dTolerance)
        {
        // Empty or coincident.

        double dDistanceFromCylinderAxis=Origin.Distance(Center+CylinderAxis*((Origin-Center)%CylinderAxis));
        if(fabs(dDistanceFromCylinderAxis-dRadius)<dTolerance)
            {
            aPoints.push_back(Origin);
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::CoincidentType);
            aTypes.push_back(SGM::CoincidentType);
            return 2;
            }
        else
            {
            return 0;
            }
        }
    else
        {
        SGM::Point3D ProjectedOrigin=Origin-CylinderAxis*((Origin-Center)%CylinderAxis);
        SGM::Point3D ProjectedPos=Pos-CylinderAxis*((Pos-Center)%CylinderAxis);
        SGM::UnitVector3D ProjectedAxis=ProjectedPos-ProjectedOrigin;
        size_t nHits=IntersectLineAndCircle(ProjectedOrigin,ProjectedAxis,Domain,Center,CylinderAxis,dRadius,dTolerance,aPoints,aTypes);
        size_t Index1;
        SGM::Interval1D MaxDomain(-SGM_MAX,SGM_MAX);
        for(Index1=0;Index1<nHits;++Index1)
            {
            std::vector<SGM::Point3D> aTempPoints;
            std::vector<SGM::IntersectionType> aTempTypes;
            IntersectLineAndLine(Origin,Axis,Domain,aPoints[Index1],CylinderAxis,MaxDomain,dTolerance,aTempPoints,aTempTypes);
            aPoints[Index1]=aTempPoints[0];
            }
        return nHits;
        }
    }

size_t IntersectLineAndLine(SGM::Point3D                 const &Origin1,
                            SGM::UnitVector3D            const &Axis1,
                            SGM::Interval1D              const &Domain1,
                            SGM::Point3D                 const &Origin2,
                            SGM::UnitVector3D            const &Axis2,
                            SGM::Interval1D              const &,//Domain2,
                            double                              dTolerance,
                            std::vector<SGM::Point3D>          &aPoints,
                            std::vector<SGM::IntersectionType> &aTypes)
    {
    SGM::Segment3D Seg1(Origin1,Origin1+Axis1);
    SGM::Segment3D Seg2(Origin2,Origin2+Axis2);
    SGM::Point3D Pos1,Pos2;
    Seg1.Intersect(Seg2,Pos1,Pos2);
    if(SGM::NearEqual(Pos1,Pos2,dTolerance))
        {
        if(fabs(Axis1%Axis2)<1.0-dTolerance)
            {
            aPoints.push_back(SGM::MidPoint(Pos1,Pos2));
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        else
            {
            aPoints.push_back(Origin1+Axis1*Domain1.m_dMin);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            aPoints.push_back(Origin1+Axis1*Domain1.m_dMax);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            }
        }
    return aPoints.size();
    }

size_t IntersectLineAndLine(SGM::Point3D                 const &Origin,
                            SGM::UnitVector3D            const &Axis,
                            SGM::Interval1D              const &Domain,
                            line                         const *pLine,
                            double                              dTolerance,
                            std::vector<SGM::Point3D>          &aPoints,
                            std::vector<SGM::IntersectionType> &aTypes)
    {
    SGM::Segment3D Seg1(Origin,Origin+Axis);
    SGM::Point3D const &Origin2=pLine->GetOrigin();
    SGM::UnitVector3D const &Axis2=pLine->GetAxis();
    SGM::Segment3D Seg2(Origin2,Origin2+Axis2);
    SGM::Point3D Pos1,Pos2;
    Seg1.Intersect(Seg2,Pos1,Pos2);
    if(SGM::NearEqual(Pos1,Pos2,dTolerance))
        {
        if(fabs(Axis%Axis2)<1.0-dTolerance)
            {
            aPoints.push_back(SGM::MidPoint(Pos1,Pos2));
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        else
            {
            aPoints.push_back(Origin+Axis*Domain.m_dMin);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            aPoints.push_back(Origin+Axis*Domain.m_dMax);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            }
        }
    return aPoints.size();
    }

size_t IntersectLineAndPlane(SGM::Point3D                 const &Origin,
                             SGM::UnitVector3D            const &Axis,
                             SGM::Interval1D              const &Domain,
                             SGM::Point3D                 const &PlaneOrigin,
                             SGM::UnitVector3D            const &PlaneNorm,
                             double                              dTolerance,
                             std::vector<SGM::Point3D>          &aPoints,
                             std::vector<SGM::IntersectionType> &aTypes)
    {
    // IsEmpty, coincident, or one point.

    SGM::Point3D Pos0=Origin+Axis*Domain.m_dMin;
    SGM::Point3D Pos1=Origin+Axis*Domain.m_dMax;
    double dDist0=(Pos0-PlaneOrigin)%PlaneNorm;
    double dDist1=(Pos1-PlaneOrigin)%PlaneNorm;
    if (fabs(Axis%PlaneNorm)<dTolerance) // line and plane are parallel
        {
        double dDist = (Origin-PlaneOrigin)%PlaneNorm;
        if(fabs(dDist)<dTolerance)
            {
            aPoints.push_back(Pos0);
            aPoints.push_back(Pos1);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            return 2;
            }
        else
            {
            return 0;
            }
        }
    if(dDist0*dDist1<0)
        {
        double t=-((Origin.m_x-PlaneOrigin.m_x)*PlaneNorm.m_x+
                   (Origin.m_y-PlaneOrigin.m_y)*PlaneNorm.m_y+
                   (Origin.m_z-PlaneOrigin.m_z)*PlaneNorm.m_z)/
            (Axis.m_x*PlaneNorm.m_x+Axis.m_y*PlaneNorm.m_y+Axis.m_z*PlaneNorm.m_z);
        aPoints.push_back(Origin+t*Axis);
        aTypes.push_back(SGM::IntersectionType::PointType);
        return 1;
        }
    else
        {
        if(fabs(dDist0)<dTolerance)
            {
            aPoints.push_back(Pos0);
            aTypes.push_back(SGM::IntersectionType::PointType);
            return 1;
            }
        else if(fabs(dDist1)<dTolerance)
            {
            aPoints.push_back(Pos1);
            aTypes.push_back(SGM::IntersectionType::PointType);
            return 1;
            }
        return 0;
        }
    }

size_t IntersectLineAndPlane(SGM::Point3D                 const &Origin,
                             SGM::UnitVector3D            const &Axis,
                             SGM::Interval1D              const &Domain,
                             plane                        const *pPlane,
                             double                              dTolerance,
                             std::vector<SGM::Point3D>          &aPoints,
                             std::vector<SGM::IntersectionType> &aTypes)
    {
    return IntersectLineAndPlane(Origin,Axis,Domain,pPlane->m_Origin,pPlane->m_ZAxis,
                                 dTolerance,aPoints,aTypes);
    }

size_t IntersectLineAndCircle(SGM::Point3D                 const &Origin,
                              SGM::UnitVector3D            const &Axis,
                              SGM::Interval1D              const &Domain,
                              SGM::Point3D                 const &Center,
                              SGM::UnitVector3D            const &Normal,
                              double                              dRadius,
                              double                              dTolerance,
                              std::vector<SGM::Point3D>          &aPoints,
                              std::vector<SGM::IntersectionType> &aTypes)
    {
    std::vector<SGM::Point3D> aPoints2;
    std::vector<SGM::IntersectionType> aTypes2;
    dTolerance=std::max(dTolerance,SGM_ZERO);
    IntersectLineAndPlane(Origin,Axis,Domain,Center,Normal,dTolerance,aPoints2,aTypes2);
    if(aPoints2.size()==2)
        {
        SGM::Point3D Pos=Origin+((Center-Origin)%Axis)*Axis;
        double dDist=Pos.Distance(Center);
        if(dDist<=dRadius-dTolerance)
            {
            double t=sqrt(dRadius*dRadius-dDist*dDist);
            aPoints.push_back(Pos-t*Axis);
            aTypes.push_back(SGM::IntersectionType::PointType);
            aPoints.push_back(Pos+t*Axis);
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        else if(fabs(dDist-dRadius)<dTolerance)
            {
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::TangentType);
            }
        }
    else if(aPoints2.size()==1 && !SGM::NearEqual(aPoints2[0], Center, SGM_ZERO))
        {
        SGM::UnitVector3D UVec=aPoints2[0]-Center;
        SGM::Point3D Pos=Center+UVec*dRadius;
        if(Pos.DistanceSquared(aPoints2[0])<dTolerance*dTolerance)
            {
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        }
    return aPoints.size();
    }

size_t IntersectLineAndCircle(SGM::Point3D                 const &Origin,
                              SGM::UnitVector3D            const &Axis,
                              SGM::Interval1D              const &Domain,
                              circle                       const *pCircle,
                              double                              dTolerance,
                              std::vector<SGM::Point3D>          &aPoints,
                              std::vector<SGM::IntersectionType> &aTypes)
    {
    SGM::Point3D const &Center=pCircle->m_Center;
    SGM::UnitVector3D const &Normal=pCircle->m_Normal;
    double dRadius=pCircle->m_dRadius;
    return IntersectLineAndCircle(Origin,Axis,Domain,Center,Normal,dRadius,dTolerance,aPoints,aTypes);
    }

size_t IntersectCoplanarLineAndParabola(SGM::Point3D                 const &LineOrigin,
                                        SGM::UnitVector3D            const &LineAxis,
                                        SGM::Interval1D              const &,//LineDomain,
                                        SGM::Point3D                 const &ParabolaCenter,
                                        SGM::UnitVector3D            const &ParabolaXAxis,
                                        SGM::UnitVector3D            const &ParabolaYAxis,
                                        double                              ParabolaA,
                                        double                              dTolerance,
                                        std::vector<SGM::Point3D>          &aPoints,
                                        std::vector<SGM::IntersectionType> &aTypes)
    {
    std::vector<SGM::Point3D> aPoints2;
    std::vector<SGM::IntersectionType> aTypes2;
    SGM::UnitVector3D ParabolaNormal = ParabolaXAxis*ParabolaYAxis;
    SGM::Point3D const &Center=ParabolaCenter;

    double a=ParabolaA;
    SGM::UnitVector3D XVec=ParabolaXAxis;
    SGM::UnitVector3D YVec=ParabolaYAxis;
    SGM::Vector3D Vec=LineOrigin-Center;
    SGM::Point2D Origin2D(Vec%XVec,Vec%YVec);
    SGM::Vector3D Vec2=(LineOrigin+LineAxis)-Center;
    SGM::Point2D Pos2D(Vec2%XVec,Vec2%YVec);
    SGM::UnitVector2D Axis2D=Pos2D-Origin2D;

    // Using y=a*x^2
    // x=Origin2D.m_u+Axis2D.m_u*t=c+d*t
    // y=Origin2D.m_v+Axis2D.m_v*t=e+f*t

    double c=Origin2D.m_u;
    double e=Origin2D.m_v;
    double d=Axis2D.m_u;
    double f=Axis2D.m_v;

    // a*(c+d*t)^2-(e+f*t)=0
    // (a*c^2-e) + (2*a*c*d-f)*t + (a*d^2)*t^2

    double A=a*d*d;
    double B=2.0*a*c*d-f;
    double C=a*c*c-e;

    std::vector<double> aRoots;
    size_t nRoots=SGM::Quadratic(A,B,C,aRoots);
    size_t Index1;
    for(Index1=0;Index1<nRoots;++Index1)
        {
        SGM::Point2D Pos=Origin2D+Axis2D*aRoots[Index1];
        SGM::Point3D Pos3D=Center+Pos.m_u*XVec+Pos.m_v*YVec;
        SGM::Point3D ClosePos;
        double dParabolaT = ParabolaInverse(Center, XVec, YVec, ParabolaA, Pos3D, &ClosePos);
        if(Pos3D.DistanceSquared(ClosePos)<dTolerance*dTolerance)
            {
            aPoints.push_back(Pos3D);
            if(nRoots==1)
                {
                SGM::Vector3D D1;
                ParabolaEvaluate(Center,XVec,YVec,ParabolaA,dParabolaT,nullptr,&D1);
                SGM::UnitVector3D UD1=D1;
                if(SGM::NearEqual(fabs(UD1%LineAxis),1.0,dTolerance,false))
                    {
                    aTypes.push_back(SGM::IntersectionType::TangentType);
                    }
                else
                    {
                    aTypes.push_back(SGM::IntersectionType::PointType);
                    }
                }
            else
                {
                aTypes.push_back(SGM::IntersectionType::PointType);
                }
            }
        }
    if(nRoots==0)
        {
        SGM::Point2D Pos=Origin2D-Axis2D*(B/(2.0*A));
        SGM::Point3D Pos3D=Center+Pos.m_u*XVec+Pos.m_v*YVec;
        SGM::Point3D CPos;
        ParabolaInverse(Center,XVec,YVec,ParabolaA,Pos3D,&CPos);
        if(Pos3D.DistanceSquared(CPos)<dTolerance*dTolerance)
            {
            aPoints.push_back(Pos3D);
            aTypes.push_back(SGM::IntersectionType::TangentType);
            }
        }
    return aPoints.size();
    }

size_t IntersectLineAndParabola(SGM::Point3D                 const &LineOrigin,
                                SGM::UnitVector3D            const &LineAxis,
                                SGM::Interval1D              const &LineDomain,
                                SGM::Point3D                 const &ParabolaCenter,
                                SGM::UnitVector3D            const &ParabolaXAxis,
                                SGM::UnitVector3D            const &ParabolaYAxis,
                                double                              ParabolaA,
                                double                              dTolerance,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes)
    {
    std::vector<SGM::Point3D> aPoints2;
    std::vector<SGM::IntersectionType> aTypes2;
    SGM::UnitVector3D ParabolaNormal = ParabolaXAxis*ParabolaYAxis;
    IntersectLineAndPlane(LineOrigin,LineAxis,LineDomain,ParabolaCenter,ParabolaNormal,dTolerance,aPoints2,aTypes2);
    if(aPoints2.size()==2)
        {
        return IntersectCoplanarLineAndParabola(LineOrigin, LineAxis, LineDomain,
                                                ParabolaCenter, ParabolaXAxis, ParabolaYAxis, ParabolaA,
                                                dTolerance, aPoints, aTypes);
        }
    else if(aPoints2.size()==1)
        {
        SGM::Point3D Pos;
        ParabolaInverse(ParabolaCenter,ParabolaXAxis,ParabolaYAxis,ParabolaA,aPoints2[0],&Pos);
        if(Pos.DistanceSquared(aPoints2[0])<dTolerance*dTolerance)
            {
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        }
    return aPoints.size();
    }

size_t IntersectLineAndParabola(SGM::Point3D                 const &Origin,
                                SGM::UnitVector3D            const &Axis,
                                SGM::Interval1D              const &Domain,
                                parabola                     const *pParabola,
                                double                              dTolerance,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes)
    {
    return IntersectLineAndParabola(Origin, Axis, Domain,
                                    pParabola->m_Center, pParabola->m_XAxis, pParabola->m_YAxis, pParabola->m_dA,
                                    dTolerance, aPoints, aTypes);
    }

size_t IntersectLineAndHyperbola(SGM::Point3D                 const &Origin,
                                 SGM::UnitVector3D            const &Axis,
                                 SGM::Interval1D              const &Domain,
                                 hyperbola                    const *pHyperbola,
                                 double                              dTolerance,
                                 std::vector<SGM::Point3D>          &aPoints,
                                 std::vector<SGM::IntersectionType> &aTypes)
    {
    std::vector<SGM::Point3D> aPoints2;
    std::vector<SGM::IntersectionType> aTypes2;
    IntersectLineAndPlane(Origin,Axis,Domain,pHyperbola->m_Center,pHyperbola->m_Normal,dTolerance,aPoints2,aTypes2);
    if(aPoints2.size()==2)
        {
        SGM::Point3D const &Center=pHyperbola->m_Center;
        double dA=pHyperbola->m_dA;
        double dB=pHyperbola->m_dB;
        SGM::UnitVector3D XVec=pHyperbola->m_XAxis;
        SGM::UnitVector3D YVec=pHyperbola->m_YAxis;
        SGM::Vector3D Vec=Origin-Center;
        SGM::Point2D Origin2D(Vec%XVec,Vec%YVec);
        SGM::Vector3D Vec2=(Origin+Axis)-Center;
        SGM::Point2D Pos2D(Vec2%XVec,Vec2%YVec);
        SGM::UnitVector2D Axis2D=Pos2D-Origin2D;

        // Using y^2/a^2-x^2/b^2=1
        // x=Origin2D.m_u+Axis2D.m_u*t=c+d*t
        // y=Origin2D.m_v+Axis2D.m_v*t=e+f*t

        double c=Origin2D.m_u;
        double e=Origin2D.m_v;
        double d=Axis2D.m_u;
        double f=Axis2D.m_v;
        double raa=1.0/(dA*dA);
        double rbb=1.0/(dB*dB);
        double A=f*f*raa-d*d*rbb;
        double B=2.0*e*f*raa-2*c*d*rbb;
        double C=e*e*raa-c*c*rbb-1.0;

        // (e+f*t)^2/a^2-(c+d*t)^2/b^2-1=0
        // (e^2/a^2 - c^2/b^2 - 1) + t ((2 e f)/a^2 - (2 c d)/b^2) + t^2 (f^2/a^2 - d^2/b^2) 

        std::vector<double> aRoots;
        size_t nRoots=SGM::Quadratic(A,B,C,aRoots);
        size_t Index1;
        for(Index1=0;Index1<nRoots;++Index1)
            {
            SGM::Point2D Pos=Origin2D+Axis2D*aRoots[Index1];
            SGM::Point3D Pos3D=Center+Pos.m_u*XVec+Pos.m_v*YVec;
            SGM::Point3D CPos;
            double t=pHyperbola->Inverse(Pos3D,&CPos);
            if((Pos3D-Center)%YVec > SGM_ZERO)
                {
                aPoints.push_back(Center+Pos.m_u*XVec+Pos.m_v*YVec);
                if(nRoots==1)
                    {
                    SGM::Vector3D D1;
                    pHyperbola->Evaluate(t,nullptr,&D1);
                    SGM::UnitVector3D UD1=D1;
                    if(SGM::NearEqual(fabs(UD1%Axis),1.0,dTolerance,false))
                        {
                        aTypes.push_back(SGM::IntersectionType::TangentType);
                        }
                    else
                        {
                        aTypes.push_back(SGM::IntersectionType::PointType);
                        }
                    }
                else
                    {
                    aTypes.push_back(SGM::IntersectionType::PointType);
                    }
                }
            }
        if(nRoots==0)
            {
            SGM::Point2D Pos=Origin2D-Axis2D*(B/(2.0*A));
            SGM::Point3D Pos3D=Center+Pos.m_u*XVec+Pos.m_v*YVec;
            SGM::Point3D CPos;
            pHyperbola->Inverse(Pos3D,&CPos);
            if(Pos3D.DistanceSquared(CPos)<dTolerance*dTolerance)
                {
                aPoints.push_back(Pos3D);
                aTypes.push_back(SGM::IntersectionType::TangentType);
                }
            }
        }
    else if(aPoints2.size()==1)
        {
        SGM::Point3D Pos;
        pHyperbola->Inverse(aPoints2[0],&Pos);
        if(Pos.DistanceSquared(aPoints2[0])<dTolerance*dTolerance)
            {
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        }
    return aPoints.size();
    }

size_t IntersectLineAndEllipse(SGM::Point3D                 const &Origin,
                               SGM::UnitVector3D            const &Axis,
                               SGM::Interval1D              const &Domain,
                               ellipse                      const *pEllipse,
                               double                              dTolerance,
                               std::vector<SGM::Point3D>          &aPoints,
                               std::vector<SGM::IntersectionType> &aTypes)
    {
    std::vector<SGM::Point3D> aPoints2;
    std::vector<SGM::IntersectionType> aTypes2;
    IntersectLineAndPlane(Origin,Axis,Domain,pEllipse->m_Center,pEllipse->m_Normal,dTolerance,aPoints2,aTypes2);
    if(aPoints2.size()==2)
        {
        SGM::Point3D const &Center=pEllipse->m_Center;
        double dA=pEllipse->m_dA;
        double dB=pEllipse->m_dB;
        SGM::UnitVector3D XVec=pEllipse->m_XAxis;
        SGM::UnitVector3D YVec=pEllipse->m_YAxis;
        SGM::Vector3D Vec=Origin-Center;
        SGM::Point2D Origin2D(Vec%XVec,Vec%YVec);
        SGM::Vector3D Vec2=(Origin+Axis)-Center;
        SGM::Point2D Pos2D(Vec2%XVec,Vec2%YVec);
        SGM::UnitVector2D Axis2D=Pos2D-Origin2D;

        // Using x^2/a^2+y^2/b^2=1
        // x=Origin2D.m_u+Axis2D.m_u*t=c+d*t
        // y=Origin2D.m_v+Axis2D.m_v*t=e+f*t

        double c=Origin2D.m_u;
        double e=Origin2D.m_v;
        double d=Axis2D.m_u;
        double f=Axis2D.m_v;
        double raa=1.0/(dA*dA);
        double rbb=1.0/(dB*dB);
        double A=d*d*raa+f*f*rbb;
        double B=2.0*c*d*raa+2*e*f*rbb;
        double C=c*c*raa+e*e*rbb-1.0;

        // (c+d*t)^2/a^2+(e+f*t)^2/b^2-1=0
        // (c^2/a^2 + e^2/b^2 - 1) + t ((2 c d)/a^2 + (2 e f)/b^2) + t^2 (d^2/a^2 + f^2/b^2) 

        std::vector<double> aRoots;
        size_t nRoots=SGM::Quadratic(A,B,C,aRoots);
        size_t Index1;
        for(Index1=0;Index1<nRoots;++Index1)
            {
            SGM::Point2D Pos=Origin2D+Axis2D*aRoots[Index1];
            aPoints.push_back(Center+Pos.m_u*XVec+Pos.m_v*YVec);
            if(nRoots==1)
                {
                aTypes.push_back(SGM::IntersectionType::TangentType);
                }
            else
                {
                aTypes.push_back(SGM::IntersectionType::PointType);
                }
            }
        if(nRoots==0)
            {
            SGM::Point2D Pos=Origin2D-Axis2D*(B/(2.0*A));
            SGM::Point3D Pos3D=Center+Pos.m_u*XVec+Pos.m_v*YVec;
            SGM::Point3D CPos;
            pEllipse->Inverse(Pos3D,&CPos);
            if(Pos3D.DistanceSquared(CPos)<dTolerance*dTolerance)
                {
                aPoints.push_back(Pos3D);
                aTypes.push_back(SGM::IntersectionType::TangentType);
                }
            }
        }
    else if(aPoints2.size()==1)
        {
        SGM::Point3D Pos;
        pEllipse->Inverse(aPoints2[0],&Pos);
        if(Pos.DistanceSquared(aPoints2[0])<dTolerance*dTolerance)
            {
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        }
    return aPoints.size();
    }

void FindStartingPointsForLineCurveIntersection(SGM::Point3D             const &Origin,
                                               SGM::UnitVector3D         const &Axis,
                                               std::vector<SGM::Point3D> const &aSeedPoints,
                                               double                           dTolerance,
                                               std::vector<SGM::Point3D>       &aStartPoints)
{
    SGM::Segment3D LineSeg(Origin,Origin+Axis);
    size_t nSeedPoints=aSeedPoints.size();
    double dTanHalfAngle=SEED_POINT_HALF_ANGLE_TANGENT;
    size_t Index1;
    for(Index1=1;Index1<nSeedPoints;++Index1)
    {
        SGM::Point3D const &Pos0=aSeedPoints[Index1-1];
        SGM::Point3D const &Pos1=aSeedPoints[Index1];
        SGM::Segment3D Seg(Pos0,Pos1);
        SGM::Point3D Pos2,Pos3;
        double s, t;
        bool bIntersect = Seg.Intersect(LineSeg,Pos2,Pos3,&s,&t);
        if(bIntersect)
        {
            aStartPoints.push_back(Pos2);
        }
        else
        {
            double dLength=Pos0.Distance(Pos1);
            double dTol=dTolerance+dLength*dTanHalfAngle;
            if ((Pos2.DistanceSquared(Pos3) < dTol*dTol) && (s>-SGM_ZERO && s<(1+SGM_ZERO)))
            {
                aStartPoints.push_back(Pos2);
            }
            SGM::Vector3D PointOnLine = SGM::Vector3D(Origin) + ((Pos0 - Origin) % Axis) * Axis;
            if(Pos0.DistanceSquared(SGM::Point3D(PointOnLine))<dTol*dTol)
            {
                aStartPoints.push_back(Pos0);
            }
            PointOnLine = SGM::Vector3D(Origin) + ((Pos1 - Origin) % Axis) * Axis;
            if(Pos1.DistanceSquared(SGM::Point3D(PointOnLine))<dTol*dTol)
            {
                aStartPoints.push_back(Pos1);
            }
        }
    }
}

void FindLineCurveIntersections(SGM::Point3D                           const &Origin,
                                SGM::UnitVector3D                      const &Axis,
                                curve                                  const *pCurve,
                                double                                        dTolerance,
                                std::vector<SGM::Point3D>              const &aStartPoints,
                                std::vector<std::pair<double,SGM::Point3D> > &aRefinedPoints)

{
    size_t nStartPoints=aStartPoints.size();
    size_t nCountLimit=100;
    SGM::Segment3D Seg1(Origin,Origin+Axis);
    size_t Index1;
    for(Index1=0;Index1<nStartPoints;++Index1)
    {
        SGM::Point3D Pos=aStartPoints[Index1];
        size_t nCount=0;
        double dOldDist=SGM_MAX;
        while(nCount<nCountLimit)
        {
            SGM::Point3D CPos;
            double dNUBt=pCurve->Inverse(Pos);
            SGM::Vector3D LocalTan;
            pCurve->Evaluate(dNUBt,&CPos,&LocalTan);
            SGM::Segment3D Seg2(CPos,CPos+LocalTan);
            SGM::Point3D Pos0,Pos1;
            Seg1.Intersect(Seg2,Pos0,Pos1);
            SGM::Point3D Temp=SGM::MidPoint(Pos0,Pos1);
            double dDist=Temp.Distance(CPos);
            if(dDist<dOldDist)
            {
                Pos=Temp;
            }
            else
            {
                // Newton lead us astray.
                double t=(CPos-Origin)%Axis;
                Pos=Origin+t*Axis;
                dDist=Pos.Distance(CPos);
            }
            double tNUB = pCurve->Inverse(Pos, nullptr, &dNUBt);
            if(dDist<SGM_ZERO || fabs(dDist-dOldDist)<SGM_ZERO)
            {
                aRefinedPoints.emplace_back(tNUB,Pos);
                break;
            }
            if(nCount==nCountLimit-1 && dDist<dTolerance)
            {
                aRefinedPoints.emplace_back(tNUB,Pos);
                break;
            }
            dOldDist=dDist;
            ++nCount;
        }
    }
}

void RemoveLineCurveIntersectionDuplicates(SGM::UnitVector3D                      const &LineDirection,
                                           curve                                  const *pCurve,
                                           double                                        dTolerance,
                                           std::vector<std::pair<double,SGM::Point3D> > &aRefinedPoints,
                                           std::vector<SGM::Point3D>                     &aPoints,
                                           std::vector<SGM::IntersectionType>            &aTypes)
{
    size_t Index1;
    if(size_t nRefinedPoints=aRefinedPoints.size())
    {
        double dDuplicatesTolerance=std::max(dTolerance,SGM_MIN_TOL);
        std::sort(aRefinedPoints.begin(),aRefinedPoints.end());
        for(Index1=0;Index1<nRefinedPoints;++Index1)
        {
            SGM::Point3D const &Pos=aRefinedPoints[Index1].second;
            if(Index1==0 || dDuplicatesTolerance<Pos.Distance(aPoints.back()))
            {
                double t=aRefinedPoints[Index1].first;
                SGM::Vector3D DPos;
                pCurve->Evaluate(t,nullptr,&DPos);
                aPoints.push_back(Pos);
                SGM::UnitVector3D Test(DPos);
                if(fabs(1.0-fabs(Test%LineDirection))<SGM_MIN_TOL)
                {
                    aTypes.push_back(SGM::IntersectionType::TangentType);
                }
                else
                {
                    aTypes.push_back(SGM::IntersectionType::PointType);
                }
            }
        }
    }
}

size_t IntersectLineAndNUBCurve(SGM::Point3D                 const &Origin,
                                SGM::UnitVector3D            const &Axis,
                                SGM::Interval1D              const &,//Domain,
                                NUBcurve                     const *pNUBCurve,
                                double                              dTolerance,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes)
    {
    std::vector<SGM::Point3D> const &aSeedPoints=pNUBCurve->GetSeedPoints();
    std::vector<SGM::Point3D> aStartPoints;
    SGMInternal::FindStartingPointsForLineCurveIntersection(Origin, Axis, aSeedPoints, dTolerance, aStartPoints);

    std::vector<std::pair<double,SGM::Point3D> > aRefinedPoints;
    SGMInternal::FindLineCurveIntersections(Origin, Axis, pNUBCurve, dTolerance, aStartPoints, aRefinedPoints);

    // Remove duplicates and find the types.
    SGMInternal::RemoveLineCurveIntersectionDuplicates(Axis, pNUBCurve, dTolerance, aRefinedPoints, aPoints, aTypes);

    return aPoints.size();
    }

size_t IntersectLineAndNURBCurve(SGM::Point3D                  const &Origin,
                                 SGM::UnitVector3D             const &Axis,
                                 SGM::Interval1D               const &,//Domain,
                                 NURBcurve                     const *pNURBCurve,
                                 double                               dTolerance,
                                 std::vector<SGM::Point3D>           &aPoints,
                                 std::vector<SGM::IntersectionType>  &aTypes)
    {
    // Find the starting points.
    std::vector<SGM::Point3D> const &aSeedPoints=pNURBCurve->GetSeedPoints();
    std::vector<SGM::Point3D> aStartPoints;
    SGMInternal::FindStartingPointsForLineCurveIntersection(Origin, Axis, aSeedPoints, dTolerance, aStartPoints);


    std::vector<std::pair<double,SGM::Point3D> > aRefinedPoints;
    SGMInternal::FindLineCurveIntersections(Origin, Axis, pNURBCurve, dTolerance, aStartPoints, aRefinedPoints);

    // Remove duplicates and find the types.
    SGMInternal::RemoveLineCurveIntersectionDuplicates(Axis, pNURBCurve, dTolerance, aRefinedPoints, aPoints, aTypes);

    return aPoints.size();
    }

size_t IntersectLineAndSphere(SGM::Point3D                 const &Origin,
                              SGM::UnitVector3D            const &Axis,
                              SGM::Interval1D              const &Domain,
                              sphere                       const *pSphere,
                              double                              dTolerance,
                              std::vector<SGM::Point3D>          &aPoints,
                              std::vector<SGM::IntersectionType> &aTypes)
    {
    // IsEmpty, one tangent point or two points.

    SGM::Point3D const &Center=pSphere->m_Center;
    SGM::Point3D Pos=Origin+((Center-Origin)%Axis)*Axis;
    double dDist=Pos.Distance(Center);
    double dRadius=pSphere->m_dRadius;
    if(dDist<=dRadius-dTolerance)
        {
        double t=sqrt(dRadius*dRadius-dDist*dDist);
        SGM::Point3D Pos0=Pos-t*Axis;
        if(Domain.InInterval((Pos0-Origin)%Axis,dTolerance))
            {
            aPoints.push_back(Pos0);
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        SGM::Point3D Pos1=Pos+t*Axis;
        if(Domain.InInterval((Pos1-Origin)%Axis,dTolerance))
            {
            aPoints.push_back(Pos1);
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        }
    else if(fabs(dDist-dRadius)<dTolerance)
        {
        if(Domain.InInterval((Pos-Origin)%Axis,dTolerance))
            {
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::TangentType);
            }
        }
    return aPoints.size();
    }

size_t IntersectLineAndCone(SGM::Point3D                 const &Origin,
                            SGM::UnitVector3D            const &Axis,
                            SGM::Interval1D              const &,//Domain,
                            cone                         const *pCone,
                            double                              dTolerance,
                            std::vector<SGM::Point3D>          &aPoints,
                            std::vector<SGM::IntersectionType> &aTypes)
    {
    //  IsEmpty, one point tangent or not, two points.

    // A point is on a cone with apex at the origin and axis the positive 
    // z-axis if and only if x^2+y^2=(sin(half angle)/cos(half angle))^2*z^2.

    SGM::Point3D Apex=pCone->FindApex();
    SGM::UnitVector3D XVec=pCone->m_XAxis;
    SGM::UnitVector3D YVec=pCone->m_YAxis;
    SGM::UnitVector3D ZVec=pCone->m_ZAxis;
    SGM::Transform3D Trans(XVec,YVec,ZVec,SGM::Vector3D(Apex));
    SGM::Transform3D Inverse;
    Trans.Inverse(Inverse);

    SGM::Point3D TOrigin=Inverse*Origin;
    SGM::UnitVector3D TAxis=Inverse*Axis;

    double dCosHalfAngle=pCone->m_dCosHalfAngle;
    double dSinHalfAngle=pCone->m_dSinHalfAngle;
    double dTan=dSinHalfAngle/dCosHalfAngle;
    double s=dTan*dTan;

    // x^2+y^2=s*z^2
    //
    // x=a+t*b
    // y=c+t*d
    // z=e+t*f
    //
    // (a+t*b)^2+(c+t*d)^2-s*(e+t*f)^2=0

    double a=TOrigin.m_x;
    double c=TOrigin.m_y;
    double e=TOrigin.m_z;
    double b=TAxis.m_x;
    double d=TAxis.m_y;
    double f=TAxis.m_z;

    double A = b*b+d*d-f*f*s;
    double B = 2*(a*b+c*d-e*f*s);
    double C = a*a+c*c-e*e*s;

    std::vector<SGM::Point3D> aHits;
    std::vector<SGM::IntersectionType> aTempTypes;
    std::vector<double> aRoots;
    size_t nRoots=SGM::Quadratic(A,B,C,aRoots);
    if(nRoots==0) 
        {
        if (SGM_ZERO<fabs(A)) 
            { 
            double x=-B/(2.0*A);
            aHits.push_back(TOrigin+x*TAxis);
            aTempTypes.push_back(SGM::IntersectionType::TangentType);
            }
        }
    else if(nRoots==1)
        {
        aHits.push_back(TOrigin+aRoots[0]*TAxis);
        aTempTypes.push_back(SGM::IntersectionType::TangentType);
        }
    else // nRoots==2
        {
        aHits.push_back(TOrigin+aRoots[0]*TAxis);
        aHits.push_back(TOrigin+aRoots[1]*TAxis);
        aTempTypes.push_back(SGM::IntersectionType::PointType);
        aTempTypes.push_back(SGM::IntersectionType::PointType);
        }

    // Check all the hits.

    size_t nHits=aHits.size();
    size_t Index1;
    for(Index1=0;Index1<nHits;++Index1)
        {
        SGM::Point3D Pos=aHits[Index1];
        Pos*=Trans;
        SGM::Point3D CPos;
        pCone->Inverse(Pos,&CPos);
        if(SGM::NearEqual(Pos,CPos,dTolerance))
            {
            aPoints.push_back(CPos);
            aTypes.push_back(aTempTypes[Index1]);
            }
        }
    return aPoints.size();
    }

// In this version of IntersectLineAndTorus the torus has been move to the origin.

size_t IntersectLineAndTorus(SGM::Point3D                 const &Origin,
                             SGM::UnitVector3D            const &Axis,
                             double                              dMajorRadius,
                             double                              dMinorRadius,
                             double                              dTolerance,
                             std::vector<SGM::Point3D>          &aPoints,
                             std::vector<SGM::IntersectionType> &aTypes)
    {
    // IsEmpty, one tangent point, two points, two tangents points, two points and one tangent point,
    // three points one tangent, four points.

    // A point is on a torus centered at the origin, with normal (0,0,1), major radius s and 
    // minor radius r if and only if (x^2+y^2+z^2+s^2-r^2)^2 = 4*s^2(x^2+y^2).

    // Finding x, y and z in terms of the parameter t of the given line we have the following;
    // x=a+t*b
    // y=c+t*d
    // z=e+t*f

    // Hence, the intersection of the line and the torus are at the roots of the following quartic;
    // ((a+t*b)^2+(c+t*d)^2+(e+t*f)^2+s^2-r^2)^2-4*s^2((a+t*b)^2+(c+t*d)^2)=0

    double a=Origin.m_x;
    double c=Origin.m_y;
    double e=Origin.m_z;
    double b=Axis.m_x;
    double d=Axis.m_y;
    double f=Axis.m_z;
    double r=dMinorRadius;
    double s=dMajorRadius;

    double a2=a*a,a3=a2*a,a4=a2*a2;
    double b2=b*b,b3=b2*b,b4=b2*b2;
    double c2=c*c,c3=c2*c,c4=c2*c2;
    double d2=d*d,d3=d2*d,d4=d2*d2;
    double e2=e*e,e3=e2*e,e4=e2*e2;
    double f2=f*f,f3=f2*f,f4=f2*f2;
    double r2=r*r,r4=r2*r2;
    double s2=s*s,s4=s2*s2;

    double E=a4+2*a2*c2+c4+2*a2*e2+2*c2*e2+e4-2*a2*r2-2*c2*r2-2*e2*r2+r4-2*a2*s2-2*c2*s2+2*e2*s2-2*r2*s2+s4;
    double D=4*a3*b+4*a*b*c2+4*a2*c*d+4*c3*d+4*a*b*e2+4*c*d*e2+4*a2*e*f+4*c2*e*f+4*e3*f-4*a*b*r2-4*c*d*r2-4*e*f*r2-4*a*b*s2-4*c*d*s2+4*e*f*s2;
    double C=6*a2*b2+2*b2*c2+8*a*b*c*d+2*a2*d2+6*c2*d2+2*b2*e2+2*d2*e2+8*a*b*e*f+8*c*d*e*f+2*a2*f2+2*c2*f2+6*e2*f2-2*b2*r2-2*d2*r2-2*f2*r2-2*b2*s2-2*d2*s2+2*f2*s2;
    double B=4*a*b3+4*b2*c*d+4*a*b*d2+4*c*d3+4*b2*e*f+4*d2*e*f+4*a*b*f2+4*c*d*f2+4*e*f3;
    double A=b4+2*b2*d2+d4+2*b2*f2+2*d2*f2+f4;

    std::vector<double> aRoots;
    size_t nRoots=SGM::Quartic(A,B,C,D,E,aRoots,dTolerance);
    size_t Index1;
    for(Index1=0;Index1<nRoots;++Index1)
        {
        double t=aRoots[Index1];
        double ftp=t*(t*(4*A*t+3*B)+2*C)+D;
        if(fabs(ftp)<dTolerance)
            {
            aTypes.push_back(SGM::IntersectionType::TangentType);
            }
        else
            {
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        aPoints.push_back(Origin+aRoots[Index1]*Axis);
        }
    return aPoints.size();
    }

size_t IntersectLineAndTorus(SGM::Point3D                 const &Origin,
                             SGM::UnitVector3D            const &Axis,
                             SGM::Interval1D              const &,//Domain,
                             SGM::Point3D                 const &Center,
                             SGM::UnitVector3D            const &XVec,
                             SGM::UnitVector3D            const &YVec,
                             SGM::UnitVector3D            const &ZVec,
                             double                              dMinorRadius,
                             double                              dMajorRadius,
                             double                              dTolerance,
                             std::vector<SGM::Point3D>          &aPoints,
                             std::vector<SGM::IntersectionType> &aTypes)
    {
    // IsEmpty, one tangent point, two points, two tangents points, two points and one tangent point,
    // three points one tangent, four points.

    // Set up the transforms.

    SGM::Transform3D Trans(XVec,YVec,ZVec,SGM::Vector3D(Center));
    SGM::Transform3D Inverse;
    Trans.Inverse(Inverse);

    SGM::Point3D TOrigin=Inverse*Origin;
    SGM::UnitVector3D TAxis=Inverse*Axis;

    return IntersectLineAndTorus(TOrigin,TAxis,dMajorRadius,dMinorRadius,dTolerance,aPoints,aTypes);
    }

size_t IntersectLineAndTorus(SGM::Point3D                 const &Origin,
                             SGM::UnitVector3D            const &Axis,
                             SGM::Interval1D              const &Domain,
                             torus                        const *pTorus,
                             double                              dTolerance,
                             std::vector<SGM::Point3D>          &aPoints,
                             std::vector<SGM::IntersectionType> &aTypes)
    {
    SGM::Point3D Center=pTorus->m_Center;
    SGM::UnitVector3D XVec=pTorus->m_XAxis;
    SGM::UnitVector3D YVec=pTorus->m_YAxis;
    SGM::UnitVector3D ZVec=pTorus->m_ZAxis;
    SGM::Transform3D Trans(XVec,YVec,ZVec,SGM::Vector3D(Center));
    SGM::Transform3D Inverse;
    Trans.Inverse(Inverse);

    SGM::Point3D TOrigin=Inverse*Origin;
    SGM::UnitVector3D TAxis=Inverse*Axis;

    double dMajorRadius=pTorus->m_dMajorRadius;
    double dMinorRadius=pTorus->m_dMinorRadius;

    size_t nHits=IntersectLineAndTorus(Origin,Axis,Domain,Center,XVec,YVec,ZVec,dMinorRadius,dMajorRadius,dTolerance,aPoints,aTypes);

    size_t Index1;
    for(Index1=0;Index1<nHits;++Index1)
        {
        SGM::Point3D Pos=aPoints[Index1];
        Pos*=Trans;
        SGM::Point3D SnappedPos;
        pTorus->Inverse(Pos,&SnappedPos);
        aPoints[Index1]=SnappedPos;
        }
    return nHits;
    }

size_t IntersectLineAndNUBSurface(SGM::Point3D                 const &Origin,
                                  SGM::UnitVector3D            const &Axis,
                                  SGM::Interval1D              const &Domain,
                                  NUBsurface                   const *pNUBSurface,
                                  double                              dTolerance,
                                  std::vector<SGM::Point3D>          &aPoints,
                                  std::vector<SGM::IntersectionType> &aTypes)
    {
    // Find the starting points.

    SGM::Segment3D LineSeg(Origin,Origin+Axis);
    std::vector<SGM::Point3D> const &aSeedPoints=pNUBSurface->GetSeedPoints();
    std::vector<SGM::Point2D> const &aSeedParams=pNUBSurface->GetSeedParams();
    size_t nUParams=pNUBSurface->GetUParams();
    size_t nVParams=pNUBSurface->GetVParams();
    std::vector<SGM::Point3D> aStartPoints;
    size_t Index1,Index2,Index3;
    for(Index1=0;Index1<nUParams;++Index1)
        {
        size_t nU=Index1*nUParams;
        for(Index2=0;Index2<nVParams;++Index2)
            {
            SGM::Point3D const &PlaneOrigin=aSeedPoints[nU+Index2];
            SGM::Point2D const uv=aSeedParams[nU+Index2];
            SGM::UnitVector3D Norm;
            pNUBSurface->Evaluate(uv,nullptr,nullptr,nullptr,&Norm);
            std::vector<SGM::Point3D> aHits;
            std::vector<SGM::IntersectionType> aHitTypes;
            size_t nHits=IntersectLineAndPlane(Origin,Axis,Domain,PlaneOrigin,Norm,dTolerance,aHits,aHitTypes);
            for(Index3=0;Index3<nHits;++Index3)
                {
                aStartPoints.push_back(aHits[Index3]);
                }
            }
        }

    // Find the intersection points.

    std::vector<std::pair<double,SGM::Point3D> > aRefinedPoints;
    size_t nStartPoints=aStartPoints.size();
    size_t nCountLimit=100;
    for(Index1=0;Index1<nStartPoints;++Index1)
        {
        SGM::Point3D Pos=aStartPoints[Index1];
        size_t nCount=0;
        double dOldDist=SGM_MAX;
        while(nCount<nCountLimit)
            {
            SGM::Point3D CPos;
            SGM::Point2D uv=pNUBSurface->Inverse(Pos);
            SGM::UnitVector3D LocalNorm;
            pNUBSurface->Evaluate(uv,&CPos,nullptr,nullptr,&LocalNorm);
            std::vector<SGM::Point3D> aTemp;
            std::vector<SGM::IntersectionType> aTempType;
            if(1==SGMInternal::IntersectLineAndPlane(Origin,Axis,Domain,CPos,LocalNorm,SGM_MIN_TOL,aTemp,aTempType))
                {
                double dDist=aTemp[0].Distance(CPos);
                if(dDist<dOldDist)
                    {
                    Pos=aTemp[0];
                    }
                else
                    {
                    // Newton lead us astray.
                    double t=(CPos-Origin)%Axis;
                    Pos=Origin+t*Axis;
                    dDist=Pos.Distance(CPos);
                    }
                double t=(Pos-Origin)%Axis;
                if(dDist<SGM_ZERO || fabs(dDist-dOldDist)<SGM_ZERO)
                    {
                    aRefinedPoints.emplace_back(t,Pos);
                    break;
                    }
                if(nCount==nCountLimit-1 && dDist<dTolerance)
                    {
                    aRefinedPoints.emplace_back(t,Pos);
                    break;
                    }
                dOldDist=dDist;
                }
            else
                {
                break;
                }
            ++nCount;
            }
        }

    // Remove duplicates and find the types.

    if(size_t nRefinedPoints=aRefinedPoints.size())
        {
        double dDuplicatesTolerance=std::max(dTolerance,SGM_MIN_TOL);
        std::sort(aRefinedPoints.begin(),aRefinedPoints.end());
        for(Index1=0;Index1<nRefinedPoints;++Index1)
            {
            SGM::Point3D const &Pos=aRefinedPoints[Index1].second;
            SGM::Point2D uv=pNUBSurface->Inverse(Pos);
            if(Index1==0 || dDuplicatesTolerance<Pos.Distance(aPoints.back()))
                {
                SGM::UnitVector3D Norm;
                pNUBSurface->Evaluate(uv,nullptr,nullptr,nullptr,&Norm);
                aPoints.push_back(Pos);
                if(fabs(Norm%Axis)<dDuplicatesTolerance)
                    {
                    aTypes.push_back(SGM::IntersectionType::TangentType);
                    }
                else
                    {
                    aTypes.push_back(SGM::IntersectionType::PointType);
                    }
                }
            }
        }

    return aPoints.size();
    }

size_t IntersectLineAndNURBSurface(SGM::Point3D                 const &Origin,
                                   SGM::UnitVector3D            const &Axis,
                                   SGM::Interval1D              const &Domain,
                                   NURBsurface                  const *pNURBSurface,
                                   double                              dTolerance,
                                   std::vector<SGM::Point3D>          &aPoints,
                                   std::vector<SGM::IntersectionType> &aTypes)
    {
    // Find the starting points.

    SGM::Segment3D LineSeg(Origin,Origin+Axis);
    std::vector<SGM::Point3D> const &aSeedPoints=pNURBSurface->GetSeedPoints();
    std::vector<SGM::Point2D> const &aSeedParams=pNURBSurface->GetSeedParams();
    size_t nUParams=pNURBSurface->GetUParams();
    size_t nVParams=pNURBSurface->GetVParams();
    std::vector<SGM::Point3D> aStartPoints;
    size_t Index1,Index2,Index3;
    for(Index1=0;Index1<nUParams;++Index1)
        {
        size_t nU=Index1*nUParams;
        for(Index2=0;Index2<nVParams;++Index2)
            {
            SGM::Point3D const &PlaneOrigin=aSeedPoints[nU+Index2];
            SGM::Point2D const uv=aSeedParams[nU+Index2];
            SGM::UnitVector3D Norm;
            pNURBSurface->Evaluate(uv,nullptr,nullptr,nullptr,&Norm);
            std::vector<SGM::Point3D> aHits;
            std::vector<SGM::IntersectionType> aHitTypes;
            size_t nHits=IntersectLineAndPlane(Origin,Axis,Domain,PlaneOrigin,Norm,dTolerance,aHits,aHitTypes);
            for(Index3=0;Index3<nHits;++Index3)
                {
                aStartPoints.push_back(aHits[Index3]);
                }
            }
        }

    // Find the intersection points.

    std::vector<std::pair<double,SGM::Point3D> > aRefinedPoints;
    size_t nStartPoints=aStartPoints.size();
    size_t nCountLimit=100;
    for(Index1=0;Index1<nStartPoints;++Index1)
        {
        SGM::Point3D Pos=aStartPoints[Index1];
        size_t nCount=0;
        double dOldDist=SGM_MAX;
        while(nCount<nCountLimit)
            {
            SGM::Point3D CPos;
            SGM::Point2D uv=pNURBSurface->Inverse(Pos);
            SGM::UnitVector3D LocalNorm;
            pNURBSurface->Evaluate(uv,&CPos,nullptr,nullptr,&LocalNorm);
            std::vector<SGM::Point3D> aTemp;
            std::vector<SGM::IntersectionType> aTempType;
            if(1==SGMInternal::IntersectLineAndPlane(Origin,Axis,Domain,CPos,LocalNorm,SGM_MIN_TOL,aTemp,aTempType))
                {
                double dDist=aTemp[0].Distance(CPos);
                if(dDist<dOldDist)
                    {
                    Pos=aTemp[0];
                    }
                else
                    {
                    // Newton lead us astray.
                    double t=(CPos-Origin)%Axis;
                    Pos=Origin+t*Axis;
                    dDist=Pos.Distance(CPos);
                    }
                double t=(Pos-Origin)%Axis;
                if(dDist<SGM_ZERO || fabs(dDist-dOldDist)<SGM_ZERO)
                    {
                    aRefinedPoints.emplace_back(t,Pos);
                    break;
                    }
                if(nCount==nCountLimit-1 && dDist<dTolerance)
                    {
                    aRefinedPoints.emplace_back(t,Pos);
                    break;
                    }
                dOldDist=dDist;
                }
            else
                {
                break;
                }
            ++nCount;
            }
        }

    // Remove duplicates and find the types.

    if(size_t nRefinedPoints=aRefinedPoints.size())
        {
        double dDuplicatesTolerance=std::max(dTolerance,SGM_MIN_TOL);
        std::sort(aRefinedPoints.begin(),aRefinedPoints.end());
        for(Index1=0;Index1<nRefinedPoints;++Index1)
            {
            SGM::Point3D const &Pos=aRefinedPoints[Index1].second;
            SGM::Point2D uv=pNURBSurface->Inverse(Pos);
            if(Index1==0 || dDuplicatesTolerance<Pos.Distance(aPoints.back()))
                {
                SGM::UnitVector3D Norm;
                pNURBSurface->Evaluate(uv,nullptr,nullptr,nullptr,&Norm);
                aPoints.push_back(Pos);
                if(fabs(Norm%Axis)<dDuplicatesTolerance)
                    {
                    aTypes.push_back(SGM::IntersectionType::TangentType);
                    }
                else
                    {
                    aTypes.push_back(SGM::IntersectionType::PointType);
                    }
                }
            }
        }

    return aPoints.size();
    }

size_t IntersectLineAndExtrude(SGM::Result                        &rResult,
                               SGM::Point3D                 const &Origin,
                               SGM::UnitVector3D            const &Direction,
                               SGM::Interval1D              const &Domain,
                               extrude                      const *pExtrude,
                               double                              dTolerance,
                               std::vector<SGM::Point3D>          &aPoints,
                               std::vector<SGM::IntersectionType> &aTypes)
    {
    SGM::UnitVector3D const &Axis=pExtrude->GetAxis();
    SGM::Point3D const &Pos=pExtrude->GetOrigin();
    curve *pCurve=pExtrude->GetCurve();
    SGM::Point3D PlanePos=Origin-Axis*((Origin-Pos)%Axis);
    size_t nAnswer=0;
    if(1<fabs(Direction%Axis)+SGM_MIN_TOL)
        {
        SGM::Point3D ClosePos;
        pCurve->Inverse(PlanePos,&ClosePos);
        double dDist=PlanePos.Distance(ClosePos);
        if(dDist<dTolerance)
            {
            aPoints.push_back(ClosePos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            ++nAnswer;
            }
        }
    else
        {
        SGM::UnitVector3D PlaneVec=Axis*(Direction*Axis);
        std::vector<SGM::Point3D> aTempPoints;
        std::vector<SGM::IntersectionType> aTempTypes;
        size_t nHits=IntersectLineAndCurve(rResult,PlanePos,PlaneVec,Domain,pCurve,dTolerance,aTempPoints,aTempTypes);
        size_t Index1;
        SGM::Segment3D Seg1(Origin,Origin+Direction);
        for(Index1=0;Index1<nHits;++Index1)
            {
            SGM::Segment3D Seg2(aTempPoints[Index1],aTempPoints[Index1]+Axis);
            SGM::Point3D Pos1,Pos2;
            Seg1.Intersect(Seg2,Pos1,Pos2);
            aPoints.push_back(Pos2);
            aTypes.push_back(aTempTypes[Index1]);
            ++nAnswer;
            }
        }
    return nAnswer;
    }

size_t IntersectLineAndRevolve(SGM::Result                        &rResult,
                               SGM::Point3D                 const &Origin,
                               SGM::UnitVector3D            const &Direction,
                               SGM::Interval1D              const &Domain,
                               revolve                      const *pRevolve,
                               double                              dTolerance,
                               std::vector<SGM::Point3D>          &aPoints,
                               std::vector<SGM::IntersectionType> &aTypes)
    {
    if (Direction % pRevolve->m_ZAxis < SGM_ZERO)
        {
        // find closest points on axis and line
        SGM::Segment3D RevolveAxisSeg(pRevolve->m_Origin, pRevolve->m_Origin + pRevolve->m_ZAxis);
        SGM::Segment3D LineSeg(Origin, Origin + Direction);
        SGM::Point3D PosAxis, PosLine;
        RevolveAxisSeg.Intersect(LineSeg,PosAxis,PosLine);

        // intersect curve with plane
        std::vector<SGM::Point3D> aCurvePlanePoints;
        std::vector<SGM::IntersectionType> aCurvePlaneTypes;
        IntersectCurveAndPlane(rResult, pRevolve->m_pCurve, PosAxis,pRevolve->m_ZAxis, aCurvePlanePoints, aCurvePlaneTypes, dTolerance);

        // if plane does not intersect we're done
        if (aCurvePlanePoints.empty())
            return 0;

        // intersect the line with the circle defined by each curve and plane intersection point
        for (SGM::Point3D Pos : aCurvePlanePoints)
            {
            double dRadius = PosAxis.Distance(Pos);
            IntersectLineAndCircle(Origin, Direction, Domain, PosAxis, pRevolve->m_ZAxis, dRadius, dTolerance, aPoints, aTypes);
            }
        }
    else
        {
          throw;
        }
    return aPoints.size();
    }

size_t IntersectLineAndSurface(SGM::Result                        &rResult,
                               SGM::Point3D                 const &Origin,
                               SGM::UnitVector3D            const &Axis,
                               SGM::Interval1D              const &Domain,
                               surface                      const *pSurface,
                               double                              dTolerance,
                               std::vector<SGM::Point3D>          &aPoints,
                               std::vector<SGM::IntersectionType> &aTypes)
    {
    switch(pSurface->GetSurfaceType())
        {
        case SGM::EntityType::CylinderType:
            {
            return IntersectLineAndCylinder(Origin,Axis,Domain,(cylinder const *)pSurface,dTolerance,aPoints,aTypes);
            }
        case SGM::EntityType::PlaneType:
            {
            return IntersectLineAndPlane(Origin,Axis,Domain,(plane const *)pSurface,dTolerance,aPoints,aTypes);
            }
        case SGM::EntityType::SphereType:
            {
            return IntersectLineAndSphere(Origin,Axis,Domain,(sphere const *)pSurface,dTolerance,aPoints,aTypes);
            }
        case SGM::EntityType::ConeType:
            {
            return IntersectLineAndCone(Origin,Axis,Domain,(cone const *)pSurface,dTolerance,aPoints,aTypes);
            }
        case SGM::EntityType::TorusType:
            {
            return IntersectLineAndTorus(Origin,Axis,Domain,(torus const *)pSurface,dTolerance,aPoints,aTypes);
            }
        case SGM::EntityType::NUBSurfaceType:
            {
            return IntersectLineAndNUBSurface(Origin,Axis,Domain,(NUBsurface const *)pSurface,dTolerance,aPoints,aTypes);
            }
        case SGM::EntityType::NURBSurfaceType:
            {
            return IntersectLineAndNURBSurface(Origin,Axis,Domain,(NURBsurface const *)pSurface,dTolerance,aPoints,aTypes);
            }
        case SGM::EntityType::RevolveType:
            {
            return IntersectLineAndRevolve(rResult,Origin,Axis,Domain,(revolve const *)pSurface,dTolerance,aPoints,aTypes);
            }
        case SGM::EntityType::ExtrudeType:
            {
            return IntersectLineAndExtrude(rResult,Origin,Axis,Domain,(extrude const *)pSurface,dTolerance,aPoints,aTypes);
            }
        default:
            {
            throw;
            }
        }
    }

size_t IntersectLineAndSurface(SGM::Result                        &rResult,
                               line                         const *pLine,
                               surface                      const *pSurface,
                               double                              dTolerance,
                               std::vector<SGM::Point3D>          &aPoints,
                               std::vector<SGM::IntersectionType> &aTypes,
                               edge                         const *pEdge,
                               face                         const *pFace)
    {
    std::vector<SGM::Point3D> aTempPoints;
    std::vector<SGM::IntersectionType> aTempTypes;
    IntersectLineAndSurface(rResult,pLine->m_Origin,pLine->m_Axis,pLine->GetDomain(),
                            pSurface,dTolerance,aTempPoints,aTempTypes);
    if(pEdge)
        {

        }
    if(pFace)
        {
        
        }
    aPoints=aTempPoints;
    aTypes=aTempTypes;
    return aPoints.size();
    }

// Returns a zero dRadius for a point and a negative dRadius for empty.

void IntersectPlaneAndSphere(SGM::Point3D         const &Origin,
                             SGM::UnitVector3D    const &Norm,
                             SGM::Point3D         const &SphereCenter,
                             double                      dSphereRadius,
                             double                      dTolerance,
                             SGM::Point3D               &Center,
                             double                     &dRadius)
    {
    SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN, SGM_INTERVAL_POS_MAX);
    std::vector<SGM::Point3D> aPoints;
    std::vector<SGM::IntersectionType> aTypes;
    IntersectLineAndPlane(SphereCenter,Norm,Domain,Origin,Norm,dTolerance,aPoints,aTypes);
    Center=aPoints[0];
    double dDist=Center.Distance(SphereCenter);
    if(SGM::NearEqual(dDist,dSphereRadius,dTolerance,false))
        {
        dRadius=0.0;
        }
    else if(dDist<dSphereRadius)
        {
        dRadius=sqrt(dSphereRadius*dSphereRadius-dDist*dDist);
        }
    else
        {
        dRadius=-1.0;
        }
    }

// Returns a zero dRadius for a point and a negative dRadius for empty.

void IntersectPlaneAndSphere(SGM::Point3D         const &Origin,
                             SGM::UnitVector3D    const &Norm,
                             sphere               const *pSphere,
                             double                      dTolerance,
                             SGM::Point3D               &Center,
                             double                     &dRadius)
    {
    IntersectPlaneAndSphere(Origin,Norm,pSphere->m_Center,pSphere->m_dRadius,dTolerance,Center,dRadius);
    }

 size_t IntersectCircleAndCircle(SGM::Point3D                 const &Center1,
                                 SGM::Point3D                 const &Center2,
                                 SGM::UnitVector3D            const &Norm1,
                                 SGM::UnitVector3D            const &Norm2,
                                 double                              dRadius1,
                                 double                              dRadius2,
                                 std::vector<SGM::Point3D>          &aPoints,
                                 std::vector<SGM::IntersectionType> &aTypes,
                                 double                              dTolerance)
    {
    SGM::Vector3D Vec=Center1-Center2;
    double dDist1=Vec%Norm1;
    double dDist2=Vec%Norm2;
    if(fabs(dDist1)<dTolerance && dDist2<dTolerance && SGM::NearEqual(1.0,fabs(Norm1%Norm2),dTolerance,false))
        {
        // Circle1 and Circle2 are in the same plane.

        double dDist=Center1.Distance(Center2);
        if(dDist+dRadius1+dTolerance<dRadius2 || dDist+dRadius2+dTolerance<dRadius1)
            {
            // One circle lies inside the other.
            }
        else if(SGM::NearEqual(dRadius1+dRadius2,dDist,dTolerance,false))
            {
            aPoints.push_back(Center1+(dRadius1/(dRadius1+dRadius2))*(Center2-Center1));
            aTypes.push_back(SGM::IntersectionType::TangentType);
            }
        else if(dDist<dRadius1+dRadius2)
            {
            double dCosC=(dRadius1*dRadius1+dDist*dDist-dRadius2*dRadius2)/(2*dRadius1*dDist);
            double dOffset1=dCosC*dRadius1;
            double dOffset2=sqrt(dRadius1*dRadius1-dOffset1*dOffset1);
            SGM::UnitVector3D UVec1=Center2-Center1;
            SGM::UnitVector3D UVec2=UVec1*Norm1;
            SGM::Point3D Pos1=Center1+dOffset1*UVec1+dOffset2*UVec2;
            SGM::Point3D Pos2=Center1+dOffset1*UVec1-dOffset2*UVec2;
            if(SGM::NearEqual(Pos1,Pos2,dTolerance))
                {
                aPoints.push_back(Pos1);
                aTypes.push_back(SGM::IntersectionType::TangentType);
                }
            else
                {
                aPoints.push_back(Pos1);
                aTypes.push_back(SGM::IntersectionType::PointType);
                aPoints.push_back(Pos2);
                aTypes.push_back(SGM::IntersectionType::PointType);
                }
            }
        }
    else
        {
        std::vector<SGM::Point3D> aHits;
        std::vector<SGM::IntersectionType> aTemp;
        size_t nHits=IntersectCircleAndPlane(Center1,Norm1,dRadius1,Center2,Norm2,dTolerance,aHits,aTemp);
        size_t Index1;
        for(Index1=0;Index1<nHits;++Index1)
            {
            SGM::Point3D const &Hit=aHits[Index1];
            if(Center1.Distance(Hit)<dTolerance)
                {
                aPoints.push_back(Hit);
                aTypes.push_back(SGM::IntersectionType::PointType);
                }
            }
        }
    return aPoints.size();
    }

 size_t IntersectCircleAndSphere(SGM::Point3D                 const &Center,
                                 SGM::UnitVector3D            const &Normal,
                                 double                              dRadius,
                                 SGM::Point3D                 const &SphereCenter,
                                 double                              dSphereRadius,
                                 double                              dTolerance,
                                 std::vector<SGM::Point3D>          &aPoints,
                                 std::vector<SGM::IntersectionType> &aTypes)
    {
    SGM::Point3D Center2;
    double dRadius2;
    IntersectPlaneAndSphere(Center,Normal,SphereCenter,dSphereRadius,dTolerance,Center2,dRadius2);
    if(dRadius2==0)
        {
        if(SGM::NearEqual(Center2.Distance(Center),dRadius,dTolerance,false))
            {
            aPoints.push_back(Center2);
            aTypes.push_back(SGM::IntersectionType::TangentType);
            }
        }
    else if(0<dRadius)
        {
        IntersectCircleAndCircle(Center,Center2,Normal,Normal,dRadius,dRadius2,aPoints,aTypes,dTolerance);
        }
    return aPoints.size();
    }

size_t IntersectCircleAndSphere(SGM::Point3D                 const &Center,
                                SGM::UnitVector3D            const &Normal,
                                double                              dRadius,
                                sphere                       const *pSphere,
                                double                              dTolerance,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes)
    {
    return IntersectCircleAndSphere(Center,Normal,dRadius,pSphere->m_Center,pSphere->m_dRadius,dTolerance,aPoints,aTypes);
    }

size_t IntersectCircleAndPlane(SGM::Point3D                 const &Center,
                               SGM::UnitVector3D            const &Normal,
                               double                              dRadius,
                               SGM::Point3D                 const &PlaneOrigin,
                               SGM::UnitVector3D            const &PlaneNormal,
                               double                              dTolerance,
                               std::vector<SGM::Point3D>          &aPoints,
                               std::vector<SGM::IntersectionType> &aTypes)
    {
    if(SGM::NearEqual(fabs(Normal%PlaneNormal),1.0,SGM_MIN_TOL,false))
        {
        if(fabs((Center-PlaneOrigin)%PlaneNormal)<dTolerance)
            {
            SGM::UnitVector3D XAxis=Normal.Orthogonal();
            SGM::Point3D CirclePos=Center+XAxis*dRadius;
            aPoints.push_back(CirclePos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            aPoints.push_back(CirclePos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            }
        }
    else
        {
        SGM::Point3D Origin;
        SGM::UnitVector3D Axis;
        IntersectNonParallelPlanes(Center,Normal,PlaneOrigin,PlaneNormal,Origin,Axis);
        SGM::Point3D Pos=Origin+((Center-Origin)%Axis)*Axis;
        double dDist=Pos.Distance(Center);
        if(dDist<=dRadius-dTolerance)
            {
            double t=sqrt(dRadius*dRadius-dDist*dDist);
            aPoints.push_back(Pos-t*Axis);
            aTypes.push_back(SGM::IntersectionType::PointType);
            aPoints.push_back(Pos+t*Axis);
            aTypes.push_back(SGM::IntersectionType::PointType);
            }
        else if(fabs(dDist-dRadius)<dTolerance)
            {
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::TangentType);
            }
        }
    return aPoints.size();
    }

size_t IntersectCircleAndSurface(SGM::Point3D                 const &Center,
                                 SGM::UnitVector3D            const &Normal,
                                 double                              dRadius,
                                 surface                      const *pSurface,
                                 double                              dTolerance,
                                 std::vector<SGM::Point3D>          &aPoints,
                                 std::vector<SGM::IntersectionType> &aTypes)
    {
    switch(pSurface->GetSurfaceType())
        {
        case SGM::EntityType::PlaneType:
            {
            auto pPlane=(plane const *)pSurface;
            IntersectCircleAndPlane(Center,Normal,dRadius,pPlane->m_Origin,pPlane->m_ZAxis,dTolerance,aPoints,aTypes);
            }
        case SGM::EntityType::CylinderType:
            {
            auto pCylinder=(cylinder const *)pSurface;
            IntersectCircleAndCylinder(Center,Normal,dRadius,pCylinder,dTolerance,aPoints,aTypes);
            }
        default:
            {
            
            }
        }
    return aPoints.size();
    }

size_t IntersectNUBCurveAndSurface(SGM::Result                        &rResult,
                                   NUBcurve                     const *pNUBcurve,
                                   surface                      const *pSurface,
                                   std::vector<SGM::Point3D>          &aPoints,
                                   std::vector<SGM::IntersectionType> &aTypes,
                                   double                              dTolerance)
    {
    switch(pSurface->GetSurfaceType())
        {
        case SGM::PlaneType:
            {
            auto pPlane =(plane const *)pSurface;
            IntersectNUBCurveAndPlane(rResult, pNUBcurve, pPlane->m_Origin, pPlane->m_ZAxis, aPoints, aTypes, dTolerance);
            break;
            }
        default:
            throw;
        }
    return aPoints.size();
    }

size_t IntersectParabolaAndPlane(parabola                     const *pParabola,
                                 SGM::Point3D                 const &PlaneOrigin,
                                 SGM::UnitVector3D            const &PlaneNormal,
                                 double                              dTolerance,
                                 std::vector<SGM::Point3D>          &aPoints,
                                 std::vector<SGM::IntersectionType> &aTypes)
{
    IntersectParabolaAndPlane(pParabola->m_Center, pParabola->m_XAxis, pParabola->m_YAxis, pParabola->m_dA,
                              PlaneOrigin, PlaneNormal, dTolerance, aPoints, aTypes);
    if ((2 == aPoints.size()) &&
        (SGM::IntersectionType::CoincidentType == aTypes[0]) && (SGM::IntersectionType::CoincidentType == aTypes[1]))
    {
        if(pParabola->GetDomain().IsBounded())
        {
            aPoints.clear();
            aTypes.clear();
            SGM::Point3D Pos;
            pParabola->Evaluate(pParabola->GetDomain().m_dMin, &Pos);
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            pParabola->Evaluate(pParabola->GetDomain().m_dMax, &Pos);
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
        }
    }
    return aPoints.size();
}

size_t IntersectCurveAndSurface(SGM::Result                        &rResult,
                                curve                        const *pCurve,
                                surface                      const *pSurface,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes,
                                edge                         const *pEdge,
                                face                         const *pFace,
                                double                              dTolerance)
    {
    switch(pCurve->GetCurveType())
        {
        case SGM::LineType:
            {
            double dTol=pEdge ? pEdge->GetTolerance() : SGM_MIN_TOL;
            dTol=std::max(dTol,dTolerance);
            IntersectLineAndSurface(rResult,(line const *)pCurve,pSurface,dTol,aPoints,aTypes,pEdge,pFace);
            break;
            }
        case SGM::CircleType:
            {
            auto pCircle=(circle const *)pCurve;
            SGM::Point3D const &Center=pCircle->m_Center;
            SGM::UnitVector3D const &Normal=pCircle->m_Normal;
            double dRadius=pCircle->m_dRadius;
            IntersectCircleAndSurface(Center,Normal,dRadius,pSurface,dTolerance,aPoints,aTypes);
            break;
            }
        case SGM::EllipseType:
            {
            break;
            }
        case SGM::ParabolaType:
            {
            auto pParabola=(parabola const *)pCurve;
            if(pSurface->GetSurfaceType()==SGM::EntityType::PlaneType)
                {
                auto pPlane=(plane const *)pSurface;
                IntersectParabolaAndPlane(pParabola,pPlane->m_Origin,pPlane->m_ZAxis,dTolerance,aPoints,aTypes);
                }
            break;
            }
        case SGM::HyperbolaType:
            {
            break;
            }
        case SGM::NUBCurveType:
            {
            auto pNUB = (NUBcurve const *)pCurve;
            IntersectNUBCurveAndSurface(rResult, pNUB, pSurface, aPoints, aTypes, dTolerance);
            break;
            }
        case SGM::NURBCurveType:
            {
            break;
            }
        case SGM::PointCurveType:
            {
            auto pPointCurve=(PointCurve const *)pCurve;
            SGM::Point3D const &Pos=pPointCurve->m_Pos;
            SGM::Point3D CPos;
            pSurface->Inverse(Pos,&CPos);
            if(Pos.Distance(CPos)<dTolerance)
                {
                aPoints.push_back(CPos);
                aTypes.push_back(SGM::IntersectionType::CoincidentType);
                }
            break;
            }
        default:
            {
            rResult.SetResult(SGM::ResultType::ResultTypeUnknownType);
            }
        }
    return aPoints.size();
    }

 size_t IntersectLineAndCurve(SGM::Result                        &rResult,
                              SGM::Point3D                 const &Origin,
                              SGM::UnitVector3D            const &Axis,
                              SGM::Interval1D              const &Domain,
                              curve                        const *pCurve,
                              double                              dTolerance,
                              std::vector<SGM::Point3D>          &aPoints,
                              std::vector<SGM::IntersectionType> &aTypes)
     {
     switch(pCurve->GetCurveType())
         {
         case SGM::LineType:
             {
             return IntersectLineAndLine(Origin,Axis,Domain,(line const *)pCurve,dTolerance,aPoints,aTypes);
             }
         case SGM::CircleType:
             {
             return IntersectLineAndCircle(Origin,Axis,Domain,(circle const *)pCurve,dTolerance,aPoints,aTypes);
             }
         case SGM::EllipseType:
             {
             return IntersectLineAndEllipse(Origin,Axis,Domain,(ellipse const *)pCurve,dTolerance,aPoints,aTypes);
             }
         case SGM::ParabolaType:
             {
             return IntersectLineAndParabola(Origin,Axis,Domain,(parabola const *)pCurve,dTolerance,aPoints,aTypes);
             }
         case SGM::HyperbolaType:
             {
             return IntersectLineAndHyperbola(Origin,Axis,Domain,(hyperbola const *)pCurve,dTolerance,aPoints,aTypes);
             }
         case SGM::NUBCurveType:
             {
             return IntersectLineAndNUBCurve(Origin,Axis,Domain,(NUBcurve const *)pCurve,dTolerance,aPoints,aTypes);
             }
         case SGM::NURBCurveType:
             {
             break;
             }
         case SGM::PointCurveType:
             {
             break;
             }
         default:
             {
             rResult.SetResult(SGM::ResultType::ResultTypeUnknownType);
             }
         }
     return aPoints.size();
     }

 size_t IntersectConics(SGM::Result                        &,//rResult,
                        curve                        const *pCurve1,
                        curve                        const *pCurve2,
                        std::vector<SGM::Point3D>          &aPoints,
                        std::vector<SGM::IntersectionType> &aTypes,
                        double                              dTolerance)
    {
    // Get five points for each curve.

    SGM::Interval1D Domain1=pCurve1->GetDomain();
    SGM::Interval1D Domain2=pCurve2->GetDomain();
    if(Domain1.IsBounded()==false)
        {
        Domain1=SGM::Interval1D(-1,1);
        }
    if(Domain2.IsBounded()==false)
        {
        Domain2=SGM::Interval1D(-1,1);
        }
    std::vector<SGM::Point3D> aPoints1,aPoints2;
    aPoints1.reserve(5);
    aPoints2.reserve(5);
    size_t Index1;
    for(Index1=0;Index1<5;++Index1)
        {
        SGM::Point3D Pos1,Pos2;
        pCurve1->Evaluate(Domain1.MidPoint(Index1/5.0),&Pos1);
        pCurve2->Evaluate(Domain1.MidPoint(Index1/5.0),&Pos2);
        aPoints1.push_back(Pos1);
        aPoints2.push_back(Pos2);
        }

    // Move the points to the same plane.

    SGM::Point3D Origin1,Origin2;
    SGM::UnitVector3D XVec1,YVec1,ZVec1,XVec2,YVec2,ZVec2;
    SGM::FindLeastSquarePlane(aPoints1,Origin1,XVec1,YVec1,ZVec1);
    SGM::FindLeastSquarePlane(aPoints2,Origin2,XVec2,YVec2,ZVec2);
    if( fabs(ZVec2%(Origin2-Origin1))<dTolerance &&
        SGM::NearEqual(fabs(ZVec1%ZVec2),1.0,dTolerance,false))
        {
        // On the same plane.

        std::vector<SGM::Point2D> aPos1,aPos2,aPos3;
        aPos1.reserve(5);
        aPos2.reserve(5);
        for(Index1=0;Index1<5;++Index1)
            {
            double dx1=(aPoints1[Index1]-Origin1)%XVec1;
            double dy1=(aPoints1[Index1]-Origin1)%YVec1;
            double dx2=(aPoints2[Index1]-Origin1)%XVec1;
            double dy2=(aPoints2[Index1]-Origin1)%YVec1;
            aPos1.push_back(SGM::Point2D(dx1,dy1));
            aPos2.push_back(SGM::Point2D(dx2,dy2));
            }
        std::vector<double> aCoefficients1,aCoefficients2;
        FindConicCoefficient(aPos1,aCoefficients1);
        FindConicCoefficient(aPos2,aCoefficients2);
        SolveTwoConics(aCoefficients1,aCoefficients2,aPos3,dTolerance);
        for(SGM::Point2D uv : aPos3)
            {
            SGM::Point3D Pos=Origin1+uv.m_u*XVec1+uv.m_v*YVec1;
            SGM::Point3D Pos1,Pos2;
            double t1=pCurve1->Inverse(Pos,&Pos1);
            double t2=pCurve2->Inverse(Pos,&Pos2);
            if(Pos1.Distance(Pos)<SGM_MIN_TOL && Pos2.Distance(Pos)<SGM_MIN_TOL)
                {
                SGM::Vector3D Vec1,Vec2;
                pCurve1->Evaluate(t1,nullptr,&Vec1);
                pCurve2->Evaluate(t2,nullptr,&Vec2);
                aPoints.push_back(Pos);
                SGM::UnitVector3D UVec1=Vec1,UVec2=Vec2;
                if(SGM::NearEqual(fabs(UVec1%UVec2),1,dTolerance,false))
                    {
                    aTypes.push_back(SGM::IntersectionType::TangentType);
                    }
                else
                    {
                    aTypes.push_back(SGM::IntersectionType::PointType);
                    }
                }
            }
        }
    else
        {
        // On different planes.
        }
    return aPoints.size();
    }

size_t IntersectCircleAndCircle(SGM::Result                        &,//rResult,
                                circle                       const *pCircle1,
                                circle                       const *pCircle2,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes,
                                double                              dTolerance)
    {
    return IntersectCircleAndCircle(pCircle1->m_Center,pCircle2->m_Center,
                                    pCircle1->m_Normal,pCircle2->m_Normal,
                                    pCircle1->m_dRadius,pCircle2->m_dRadius,
                                    aPoints,aTypes,dTolerance);
    }

size_t IntersectCircleAndCylinder(SGM::Point3D                 const &Center,
                                  SGM::UnitVector3D            const &Normal,
                                  double                              dRadius,
                                  cylinder                     const *pCylinder,
                                  double                              dTolerance,
                                  std::vector<SGM::Point3D>          &aPoints,
                                  std::vector<SGM::IntersectionType> &aTypes)
    {
    // Test to see if the circle's normal and the cylinders normal match.
    if(SGM::NearEqual(fabs(Normal%pCylinder->m_ZAxis),1.0,dTolerance,false))
        {
        return IntersectCircleAndCircle(Center,pCylinder->m_Origin,
                                        Normal,pCylinder->m_ZAxis,
                                        dRadius,pCylinder->m_dRadius,
                                        aPoints,aTypes,dTolerance);
        }
    // Intersect the circle's plane and the cylinder, then
    // intersect the line(s), circle, or ellipse with the circle.
    return 0;
    }

size_t IntersectEllipseAndCurve(SGM::Result                        &rResult,
                                ellipse                      const *pEllipse,
                                curve                        const *pCurve,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes,
                                edge                         const *,//pEdge1,
                                edge                         const *,//pEdge2,
                                double                              dTolerance)
    {
    switch(pCurve->GetCurveType())
        {
        case SGM::CircleType:
        case SGM::EllipseType:
        case SGM::HyperbolaType:
            {
            IntersectConics(rResult,pEllipse,pCurve,aPoints,aTypes,dTolerance);
            break;
            }
        default:
            {
            throw;
            }
        }
    return aPoints.size();
    }

size_t IntersectCircleAndCurve(SGM::Result                        &rResult,
                               circle                       const *pCircle,
                               curve                        const *pCurve,
                               std::vector<SGM::Point3D>          &aPoints,
                               std::vector<SGM::IntersectionType> &aTypes,
                               edge                         const *,//pEdge1,
                               edge                         const *pEdge2,
                               double                              dTolerance)
    {
    switch(pCurve->GetCurveType())
        {
        case SGM::LineType:
            {
            auto pLine=(line const *)pCurve;
            SGM::Interval1D Domain=pCurve->GetDomain();
            if(pEdge2)
                {
                Domain&=pEdge2->GetDomain();
                }
            SGM::Point3D const &Origin=pLine->m_Origin;
            SGM::UnitVector3D const &Axis=pLine->m_Axis;
            return IntersectLineAndCircle(Origin,Axis,Domain,(circle const *)pCurve,dTolerance,aPoints,aTypes);
            }
        case SGM::CircleType:
            {
            return IntersectCircleAndCircle(rResult,pCircle,(circle const *)pCurve,aPoints,aTypes,dTolerance);
            }
        case SGM::PointCurveType:
            {
            auto pPointCurve=(PointCurve const *)pCurve;
            SGM::Point3D const &Pos=pPointCurve->m_Pos;
            SGM::Point3D CPos;
            pCircle->Inverse(Pos,&CPos);
            size_t nAnswer=0;
            if(Pos.Distance(CPos)<dTolerance)
                {
                aPoints.push_back(CPos);
                aTypes.push_back(SGM::IntersectionType::CoincidentType);
                nAnswer=1;
                }
            return nAnswer;
            }
        default:
            {
            throw;
            }
        }
    }

 size_t IntersectCurves(SGM::Result                        &rResult,
                        curve                        const *pCurve1,
                        curve                        const *pCurve2,
                        std::vector<SGM::Point3D>          &aPoints,
                        std::vector<SGM::IntersectionType> &aTypes,
                        edge                         const *pEdge1,
                        edge                         const *pEdge2,
                        double                              dTolerance)
      {
      double dTol1=pEdge1 ? pEdge1->GetTolerance() : SGM_MIN_TOL;
      double dTol2=pEdge1 ? pEdge2->GetTolerance() : SGM_MIN_TOL;
      double dTol=dTol1+dTol2+dTolerance;

      switch(pCurve1->GetCurveType())
         {
         case SGM::PointCurveType:
             {
             auto pPointCurve=(PointCurve const *)pCurve1;
             SGM::Point3D const &Pos=pPointCurve->m_Pos;
             SGM::Point3D CPos;
             pCurve2->Inverse(Pos,&CPos);
             if(Pos.Distance(CPos)<dTolerance)
                 {
                 aPoints.push_back(CPos);
                 aTypes.push_back(SGM::IntersectionType::CoincidentType);
                 }
             break;
             }
         case SGM::LineType:
             {
             auto pLine=(line const *)pCurve1;
             SGM::Interval1D Domain=pCurve1->GetDomain();
             if(pEdge1)
                 {
                 Domain&=pEdge1->GetDomain();
                 }
             IntersectLineAndCurve(rResult,pLine->GetOrigin(),pLine->GetAxis(),Domain,pCurve2,dTol,aPoints,aTypes);
             break;
             }
         case SGM::CircleType:
             {
             auto pCircle=(circle const *)pCurve1;
             IntersectCircleAndCurve(rResult,pCircle,pCurve2,aPoints,aTypes,pEdge1,pEdge2,dTol);
             break;
             }
         case SGM::EllipseType:
             {
             auto pEllipse=(ellipse const *)pCurve1;
             IntersectEllipseAndCurve(rResult,pEllipse,pCurve2,aPoints,aTypes,pEdge1,pEdge2,dTol);
             break;
             }
         case SGM::ParabolaType:
             {
             break;
             }
         case SGM::HyperbolaType:
             {
             break;
             }
         case SGM::NUBCurveType:
             {
             break;
             }
         case SGM::NURBCurveType:
             {
             break;
             }
         default:
             {
             rResult.SetResult(SGM::ResultType::ResultTypeUnknownType);
             }
         }
      return aPoints.size();
      }

 curve *FindConicCurve(SGM::Result             &rResult,
                       SGM::Point3D      const &Pos,
                       SGM::UnitVector3D const &Norm,
                       cone              const *pCone)
     {
     SGM::Interval1D Domain(-SGM_MAX,SGM_MAX);
     std::vector<SGM::Point3D> aPoints;
     std::vector<SGM::IntersectionType> aTypes;
     SGM::UnitVector3D Axis1=Norm.Orthogonal();
     IntersectLineAndCone(Pos,Axis1,Domain,pCone,SGM_MIN_TOL,aPoints,aTypes);
     SGM::UnitVector3D Axis2=Axis1*Norm;
     IntersectLineAndCone(Pos,Axis2,Domain,pCone,SGM_MIN_TOL,aPoints,aTypes);
     IntersectLineAndCone(Pos,Axis1+Axis2,Domain,pCone,SGM_MIN_TOL,aPoints,aTypes);
     IntersectLineAndCone(Pos,Axis2-Axis1,Domain,pCone,SGM_MIN_TOL,aPoints,aTypes);
     IntersectLineAndCone(Pos,Axis1-Axis2,Domain,pCone,SGM_MIN_TOL,aPoints,aTypes);
     return FindConic(rResult,aPoints,SGM_MIN_TOL);
     }

 bool AreLinesEqual(SGM::Point3D      const &Origin1,
                    SGM::UnitVector3D const &Axis1,
                    SGM::Point3D      const &Origin2,
                    SGM::UnitVector3D const &Axis2,
                    double                   dTolerance)
     {
     SGM::Point3D Pos1=Origin2+Axis2*(Axis2%(Origin1-Origin2));
     SGM::Point3D Pos2=Origin1+Axis1*(Axis1%(Origin2-Origin1));
     return SGM::NearEqual(fabs(Axis1%Axis2),1.0,dTolerance,false) && 
            Pos1.DistanceSquared(Origin1)<dTolerance*dTolerance &&
            Pos2.DistanceSquared(Origin2)<dTolerance*dTolerance;
     }

 SGM::Point3D ClosestPointOnLine(SGM::Point3D      const &Pos,
                                 SGM::Point3D      const &LineOrigin,
                                 SGM::UnitVector3D const &LineAxis)
     {
     return LineOrigin+LineAxis*(LineAxis%(Pos-LineOrigin));
     }

 size_t IntersectConeAndCone(SGM::Result                        &rResult,
                             cone                         const *pCone1,
                             cone                         const *pCone2,
                             std::vector<curve *>               &aCurves,
                             face                         const *,//pFace1,
                             face                         const *,//pFace2,
                             double                              dTolerance)
     {
     SGM::Point3D Apex1=pCone1->FindApex();
     SGM::Point3D Apex2=pCone2->FindApex();
     SGM::UnitVector3D const &Axis1=pCone1->m_ZAxis;
     SGM::UnitVector3D const &Axis2=pCone2->m_ZAxis;
     SGM::UnitVector3D const &XAxis1=pCone1->m_XAxis;
     double dHalfAngle1=pCone1->FindHalfAngle();
     double dHalfAngle2=pCone2->FindHalfAngle();
             
     if(SGM::NearEqual(Apex1,Apex2,dTolerance))
         {
         if(Axis1%Axis2<0)
             {
             aCurves.push_back(new PointCurve(rResult,Apex1));
             }
         else
             {
             double dAngle=Axis1.Angle(Axis2);
             if(SGM::NearEqual(dAngle,dHalfAngle1+dHalfAngle1,dTolerance,false))
                 {
                 // Line
                 
                 SGM::Point3D Pos;
                 pCone1->Inverse(pCone2->m_Origin,&Pos);
                 SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
                 curve *pCurve=new line(rResult,Apex1,Apex1+SGM::UnitVector3D(Pos-Apex1));
                 pCurve->SetDomain(Domain);
                 aCurves.push_back(pCurve);
                 }
             else if(dAngle<dHalfAngle1+dHalfAngle1)
                 {
                 // Two Lines

                 SGM::UnitVector3D UVec=Axis1*Axis2;
                 SGM::Point3D Pos0=Apex1+UVec;
                 SGM::Point3D Pos1=Apex1-UVec;
                 SGM::Point3D Pos2=ZoomInFrom(Pos0,pCone1,pCone2);
                 SGM::Point3D Pos3=ZoomInFrom(Pos1,pCone1,pCone2);
                 curve *pCurve1=new line(rResult,Apex1,Apex1+SGM::UnitVector3D(Pos2-Apex1));
                 curve *pCurve2=new line(rResult,Apex1,Apex1+SGM::UnitVector3D(Pos3-Apex1));
                 SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
                 pCurve1->SetDomain(Domain);
                 pCurve2->SetDomain(Domain);
                 aCurves.push_back(pCurve1);
                 aCurves.push_back(pCurve2);
                 }
             }
         }
     else if(AreLinesEqual(pCone1->m_Origin,Axis1,pCone2->m_Origin,Axis2,dTolerance))
         {
         if(Axis1%Axis2<0)
             {
             if(Axis1%(Apex2-Apex1)<0)
                 {
                 // Circle

                 SGM::Segment3D Seg1(Apex1,Apex1-pCone1->m_dCosHalfAngle*pCone1->m_ZAxis+pCone1->m_dSinHalfAngle*XAxis1);
                 SGM::Segment3D Seg2(Apex2,Apex2-pCone2->m_dCosHalfAngle*pCone2->m_ZAxis+pCone2->m_dSinHalfAngle*XAxis1);
                 SGM::Point3D Pos1,Pos2;
                 Seg1.Intersect(Seg2,Pos1,Pos2);
                 SGM::Point3D Center=ClosestPointOnLine(Pos1,Apex1,Axis1);
                 aCurves.push_back(new circle(rResult,Center,Axis1,Center.Distance(Pos1),&XAxis1));
                 }
             }
         else
             {
             if(Axis2%(Apex2-Apex1)<0)
                 {
                 if(dHalfAngle1<dHalfAngle2)
                     {
                     // Circle

                     SGM::Segment3D Seg1(Apex1,Apex1-pCone1->m_dCosHalfAngle*pCone1->m_ZAxis+pCone1->m_dSinHalfAngle*XAxis1);
                     SGM::Segment3D Seg2(Apex2,Apex2-pCone2->m_dCosHalfAngle*pCone2->m_ZAxis+pCone2->m_dSinHalfAngle*XAxis1);
                     SGM::Point3D Pos1,Pos2;
                     Seg1.Intersect(Seg2,Pos1,Pos2);
                     SGM::Point3D Center=ClosestPointOnLine(Pos1,Apex1,Axis1);
                     aCurves.push_back(new circle(rResult,Center,Axis1,Center.Distance(Pos1),&XAxis1));
                     }
                 }
             else
                 {
                 if(dHalfAngle2<dHalfAngle1)
                     {
                     // Circle

                     SGM::Segment3D Seg1(Apex1,Apex1-pCone1->m_dCosHalfAngle*pCone1->m_ZAxis+pCone1->m_dSinHalfAngle*XAxis1);
                     SGM::Segment3D Seg2(Apex2,Apex2-pCone2->m_dCosHalfAngle*pCone2->m_ZAxis+pCone2->m_dSinHalfAngle*XAxis1);
                     SGM::Point3D Pos1,Pos2;
                     Seg1.Intersect(Seg2,Pos1,Pos2);
                     SGM::Point3D Center=ClosestPointOnLine(Pos1,Apex1,Axis1);
                     aCurves.push_back(new circle(rResult,Center,Axis1,Center.Distance(Pos1),&XAxis1));
                     }
                 }
             }
         }
     else
         {
         // Check for lines and line segments.

         SGM::Point3D Pos1,Pos2;
         pCone1->Inverse(Apex2,&Pos1);
         pCone2->Inverse(Apex1,&Pos2);
         if( SGM::NearEqual(Apex2,Pos1,dTolerance) && 
             SGM::NearEqual(Apex1,Pos2,dTolerance))
             {
             // Line Segment from Apex1 to Apex2

             curve *pLine=new line(rResult,Apex1,Apex2);
             SGM::Interval1D Domain(0,Apex1.Distance(Apex2));
             pLine->SetDomain(Domain);
             aCurves.push_back(pLine);
             
             if(SGM::NearEqual(dHalfAngle1,dHalfAngle2,dTolerance,false))
                 {
                 SGM::Point3D Pos0=SGM::MidPoint(Apex1,Apex2);
                 SGM::Point2D uv1=pCone1->Inverse(Pos0);
                 SGM::Point2D uv2=pCone2->Inverse(Pos0);
                 SGM::UnitVector3D Norm1,Norm2;
                 pCone1->Evaluate(uv1,nullptr,nullptr,nullptr,&Norm1);
                 pCone2->Evaluate(uv2,nullptr,nullptr,nullptr,&Norm2);
                 if(0<Norm1%Norm2)
                     {
                     // Parabola

                     SGM::UnitVector3D Walk=Axis2*Axis1;
                     SGM::Point3D Pos1=ZoomInFrom(Pos0+0.5*Walk,pCone1,pCone2);
                     SGM::Point3D Pos2=ZoomInFrom(Pos0+Walk,pCone1,pCone2);
                     SGM::Point3D Pos3=ZoomInFrom(Pos0-0.5*Walk,pCone1,pCone2);
                     SGM::Point3D Pos4=ZoomInFrom(Pos0-Walk,pCone1,pCone2);
                     std::vector<SGM::Point3D> aPoints;
                     aPoints.reserve(5);
                     aPoints.push_back(Pos0);
                     aPoints.push_back(Pos1);
                     aPoints.push_back(Pos2);
                     aPoints.push_back(Pos3);
                     aPoints.push_back(Pos4);
                     aCurves.push_back(FindConic(rResult,aPoints,dTolerance));
                     }
                 }
             }
         else if(SGM::NearEqual(Apex2,Pos1,dTolerance))
             {
             SGM::Point3D Pos3=Apex1+2*(Apex2-Apex1);
             pCone2->Inverse(Pos3,&Pos1);
             if(Pos3.Distance(Pos1)<dTolerance)
                 {
                 // Line from Apex2 in the direction of Pos3

                 curve *pCurve=new line(rResult,Apex2,SGM::UnitVector3D(Pos3-Apex2));
                 SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
                 pCurve->SetDomain(Domain);
                 aCurves.push_back(pCurve);
                 }
             else
                 {
                 aCurves.push_back(new PointCurve(rResult,Apex2));
                 }
             }
         else if(SGM::NearEqual(Apex1,Pos2,dTolerance))
             {
             SGM::Point3D Pos4=Apex2+2*(Apex1-Apex2);
             pCone1->Inverse(Pos4,&Pos1);
             if(Pos4.Distance(Pos1)<dTolerance)
                 {
                 // Line from Apex1 in the direction of Pos3

                 curve *pCurve=new line(rResult,Apex1,SGM::UnitVector3D(Pos4-Apex1));
                 SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
                 pCurve->SetDomain(Domain);
                 aCurves.push_back(pCurve);
                 }
             else
                 {
                 aCurves.push_back(new PointCurve(rResult,Apex1));
                 }
             }
         else
             {
             // Look for tangent points

             SGM::Point3D Pos1,Pos2,Pos3,Pos4,Pos5,Pos6,Pos7,Pos8;
             FindConeSilhouette(pCone1,Apex2,Pos1,Pos2);
             FindConeSilhouette(pCone2,Apex1,Pos3,Pos4);
             SGM::Segment3D Seg1(Apex2,Pos3),Seg2(Apex2,Pos4),Seg3(Apex1,Pos1),Seg4(Apex1,Pos2);
             Seg1.Intersect(Seg3,Pos1,Pos2);
             Seg1.Intersect(Seg4,Pos3,Pos4);
             Seg2.Intersect(Seg3,Pos5,Pos6);
             Seg2.Intersect(Seg3,Pos7,Pos8);
             std::vector<SGM::Point3D> aTangents;
             double dTangentTol=0.005;
             if(SGM::NearEqual(Pos1,Pos2,dTangentTol))
                 {
                 aTangents.push_back(Pos1);
                 }
             if(SGM::NearEqual(Pos3,Pos4,dTangentTol))
                 {
                 aTangents.push_back(Pos3);
                 }
             if(SGM::NearEqual(Pos5,Pos6,dTangentTol))
                 {
                 aTangents.push_back(Pos5);
                 }
             if(SGM::NearEqual(Pos7,Pos8,dTangentTol))
                 {
                 aTangents.push_back(Pos7);
                 }
             SGM::RemoveDuplicates3D(aTangents,dTangentTol);
             if(aTangents.size()==1)
                 {
                 // Fire the ray on the opposite side of the cone from the tangent point to find walking points.

                 SGM::Point2D uv1=pCone1->Inverse(aTangents[0]);
                 SGM::Point2D uv2=pCone2->Inverse(aTangents[0]);
                 uv1.m_u+=SGM_PI;
                 uv2.m_u+=SGM_PI;
                 SGM::Point3D Pos1,Pos2;
                 pCone1->Evaluate(uv1,&Pos1);
                 pCone2->Evaluate(uv2,&Pos2);
                 SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
                 std::vector<SGM::Point3D> aWalk;
                 if(-dTangentTol<pCone1->PointInside(Pos2))
                     {
                     std::vector<SGM::Point3D> aPoints;
                     std::vector<SGM::IntersectionType> aTypes;
                     IntersectLineAndCone(Apex2,Apex2-Pos2,Domain,pCone1,dTolerance,aPoints,aTypes);
                     for(auto Pos : aPoints)
                         {
                         aWalk.push_back(Pos);
                         }
                     }
                 if(-dTangentTol<pCone2->PointInside(Pos1))
                     {
                     std::vector<SGM::Point3D> aPoints;
                     std::vector<SGM::IntersectionType> aTypes;
                     IntersectLineAndCone(Apex1,Apex1-Pos1,Domain,pCone2,dTolerance,aPoints,aTypes);
                     for(auto Pos : aPoints)
                         {
                         aWalk.push_back(Pos);
                         }
                     }
                 for(auto Pos : aWalk)
                     {
                     if(PointOnCurves(Pos,aCurves)==false)
                         {
                         aCurves.push_back(WalkFromTo(rResult,Pos,aTangents,pCone1,pCone2));
                         }
                     }

                 if(aCurves.empty())
                     {
                     aCurves.push_back(new PointCurve(rResult,aTangents[0]));
                     }
                 }
             else if(aTangents.size()==2)
                 {
                 SGM::UnitVector3D UpVec=Axis1*Axis2;
                 SGM::UnitVector3D XVec=UpVec*Axis1;
                 SGM::UnitVector3D Ray1=pCone1->m_dSinHalfAngle*XVec-pCone1->m_dCosHalfAngle*Axis1;
                 SGM::UnitVector3D Ray2=-pCone1->m_dSinHalfAngle*XVec-pCone1->m_dCosHalfAngle*Axis1;
                 SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
                 std::vector<SGM::Point3D> aPoints1,aPoints2,aPoints3,aPoints4;
                 std::vector<SGM::IntersectionType> aTypes1,aTypes2,aTypes3,aTypes4;
                 IntersectLineAndCone(Apex1,Ray1,Domain,pCone2,dTolerance,aPoints1,aTypes1);
                 IntersectLineAndCone(Apex1,Ray2,Domain,pCone2,dTolerance,aPoints2,aTypes2);
                 SGM::Point3D PosA1=aPoints1.front();
                 SGM::Point3D PosA2=aPoints2.back();
                 SGM::Point3D PosB1=aPoints1.back();
                 SGM::Point3D PosB2=aPoints2.front();
                 SGM::Point3D Center1=SGM::MidPoint(PosA1,PosA2);
                 SGM::Point3D Center2=SGM::MidPoint(PosB1,PosB2);
                 double dMajor1=PosA1.Distance(Center1);
                 double dMajor2=PosB1.Distance(Center2);
                 IntersectLineAndCone(Center1,UpVec,Domain,pCone2,dTolerance,aPoints3,aTypes3);
                 IntersectLineAndCone(Center2,UpVec,Domain,pCone2,dTolerance,aPoints4,aTypes4);
                 SGM::UnitVector3D MajorVec1=PosA1-Center1;
                 SGM::UnitVector3D MajorVec2=PosB1-Center2;
                 double dMinor1=Center1.Distance(aPoints3[0]);
                 double dMinor2=Center2.Distance(aPoints4[0]);
                 aCurves.push_back(new ellipse(rResult,Center1,MajorVec1,UpVec,dMajor1,dMinor1));
                 aCurves.push_back(new ellipse(rResult,Center2,MajorVec2,UpVec,dMajor2,dMinor2));
                 }
             else
                 {
                 // Check a hyperbola, an ellipse, or parabola

                 if(SGM::NearEqual(dHalfAngle1,dHalfAngle2,dTolerance,false))
                     {
                     if( SGM::NearEqual(Axis1%Axis2,1.0,dTolerance,false) &&
                         pCone1->PointInside(Apex2)<-dTolerance &&
                         pCone2->PointInside(Apex1)<-dTolerance)
                         {
                         // Hyperbola

                         SGM::Point3D Pos0=ZoomInFrom(SGM::MidPoint(Apex1,Apex2),pCone1,pCone2);
                         SGM::UnitVector3D UVec=Apex2-Apex1;
                         SGM::UnitVector3D Walk=UVec*Axis1;
                         SGM::Point3D Pos1=ZoomInFrom(Pos0+0.5*Walk,pCone1,pCone2);
                         SGM::Point3D Pos2=ZoomInFrom(Pos0+Walk,pCone1,pCone2);
                         SGM::Point3D Pos3=ZoomInFrom(Pos0-0.5*Walk,pCone1,pCone2);
                         SGM::Point3D Pos4=ZoomInFrom(Pos0-Walk,pCone1,pCone2);
                         std::vector<SGM::Point3D> aPoints;
                         aPoints.reserve(5);
                         aPoints.push_back(Pos0);
                         aPoints.push_back(Pos1);
                         aPoints.push_back(Pos2);
                         aPoints.push_back(Pos3);
                         aPoints.push_back(Pos4);
                         aCurves.push_back(FindConic(rResult,aPoints,dTolerance));
                         }
                     else if( SGM::NearEqual(Axis1%Axis2,-1.0,dTolerance,false))
                         {
                         // Ellipse

                         SGM::Vector3D Vec=Apex2-Apex1;
                         SGM::UnitVector3D UVec=Axis1*Vec*Axis1;
                         SGM::UnitVector3D Ray1=pCone1->m_dSinHalfAngle*UVec-pCone1->m_dCosHalfAngle*Axis1;
                         SGM::UnitVector3D Ray2=-pCone1->m_dSinHalfAngle*UVec-pCone1->m_dCosHalfAngle*Axis1;
                         SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
                         std::vector<SGM::Point3D> aPoints1,aPoints2,aPoints3;
                         std::vector<SGM::IntersectionType> aTypes1,aTypes2,aTypes3;
                         IntersectLineAndCone(Apex1,Ray1,Domain,pCone2,dTolerance,aPoints1,aTypes1);
                         IntersectLineAndCone(Apex1,Ray2,Domain,pCone2,dTolerance,aPoints2,aTypes2);
                         SGM::Point3D Pos1=aPoints1[0];
                         SGM::Point3D Pos2=aPoints2[0];
                         SGM::Point3D Center=SGM::MidPoint(Pos1,Pos2);
                         SGM::UnitVector3D MajorVector=Pos2-Pos1;
                         SGM::UnitVector3D MinorVector=MajorVector*Axis1;
                         IntersectLineAndCone(Center,MinorVector,Domain,pCone2,dTolerance,aPoints3,aTypes3);
                         SGM::Point3D Pos3=aPoints3[0];
                         double dMajor=Center.Distance(Pos1);
                         double dMinor=Center.Distance(Pos3);
                         aCurves.push_back(new ellipse(rResult,Center,MajorVector,MinorVector,dMajor,dMinor));
                         }
                     }
                 else
                     {
                     // The general case.

                     SGM::Segment3D Seg1(Apex1,Apex1-Axis1),Seg2(Apex2,Apex2-Axis2);
                     SGM::Point3D Pos1,Pos2,Pos3,Pos4;
                     Seg1.Intersect(Seg2,Pos1,Pos2);
                     pCone1->Inverse(Pos1,&Pos3);
                     pCone2->Inverse(Pos2,&Pos4);
                     std::vector<SGM::Point3D> aPoints1,aPoints2,aWalkFrom;
                     std::vector<SGM::IntersectionType> aTypes1,aTypes2;
                     SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
                     IntersectLineAndCone(Apex1,Pos3-Apex1,Domain,pCone2,dTolerance,aPoints1,aTypes1);
                     IntersectLineAndCone(Apex2,Pos4-Apex2,Domain,pCone1,dTolerance,aPoints2,aTypes2);
                     aWalkFrom=aPoints1;
                     for(auto Pos : aPoints2)
                         {
                         aWalkFrom.push_back(Pos);
                         }
                     for(auto StartPos : aWalkFrom)
                         {
                         if(PointOnCurves(StartPos,aCurves)==false)
                             {
                             std::vector<SGM::Point3D> aEndPoints;
                             aEndPoints.push_back(StartPos);
                             aCurves.push_back(WalkFromTo(rResult,StartPos,aEndPoints,pCone1,pCone2));
                             }
                         }
                     }
                 }
             }
         }
     return aCurves.size();
     }

 size_t IntersectPlaneAndCone(SGM::Result                        &rResult,
                              plane                        const *pPlane,
                              cone                         const *pCone,
                              std::vector<curve *>               &aCurves,
                              face                         const *,//pFace1,
                              face                         const *,//pFace2,
                              double                              dTolerance)
     {
     SGM::Point3D Apex=pCone->FindApex();
     SGM::UnitVector3D const &Axis=pCone->m_ZAxis;
     SGM::UnitVector3D const &Norm=pPlane->m_ZAxis;
     SGM::Point3D const &Origin=pPlane->m_Origin;
     if(fabs((Apex-Origin)%Norm)<dTolerance)
         {
         // One or two line intersection.
         std::vector<SGM::Point3D> aPoints;
         std::vector<SGM::IntersectionType> aTypes;
         size_t nRoots=IntersectCircleAndPlane(pCone->m_Origin,Axis,
             pCone->m_dRadius,pPlane->m_Origin,pPlane->m_ZAxis,dTolerance,aPoints,aTypes);
         size_t Index1;
         for(Index1=0;Index1<nRoots;++Index1)
             {
             SGM::UnitVector3D LineAxis=aPoints[Index1]-Apex;
             aCurves.push_back(new line(rResult,aPoints[Index1],LineAxis));
             }
         if(nRoots==0)
             {
             // Single point intersection.
             aCurves.push_back(new PointCurve(rResult,Apex));
             }
         }
     else
         {
         double dSin=pCone->m_dSinHalfAngle;
         double dDot=fabs(Axis%Norm);
         if(dSin+SGM_MIN_TOL<dDot)
             {
             std::vector<SGM::Point3D> aPoints;
             std::vector<SGM::IntersectionType> aTypes;
             SGM::Interval1D Domain(-SGM_MAX,SGM_MAX);
             IntersectLineAndPlane(Apex,Axis,Domain,Origin,Norm,dTolerance,aPoints,aTypes);
             double h=(aPoints[0]-Apex)%Axis;
             if(h<0.0)
                 {
                 if(SGM::NearEqual(dDot,1.0,dTolerance,false))
                     {
                     // Circle intersection.
                     double dRadius=-h*(pCone->m_dCosHalfAngle)/dSin;
                     aCurves.push_back(new circle(rResult,aPoints[0],Axis,dRadius,&pCone->m_XAxis));
                     }
                 else
                     {
                     // Ellipse intersection.
                     aCurves.push_back(FindConicCurve(rResult,aPoints[0],Norm,pCone));
                     }
                 }
             }
         else if(dSin<dDot+dTolerance)
             {
             std::vector<SGM::Point3D> aPoints;
             std::vector<SGM::IntersectionType> aTypes;
             SGM::Interval1D Domain(-SGM_MAX,SGM_MAX);
             IntersectLineAndPlane(Apex,Axis,Domain,Origin,Norm,dTolerance,aPoints,aTypes);
             if((aPoints[0]-Apex)%Axis<0)
                 {
                 // Parabola intersecton.
                 aCurves.push_back(FindConicCurve(rResult,aPoints[0],Norm,pCone));
                 }
             }
         else
             {
             // Hyperbola intersection.
             SGM::UnitVector3D Vec=Norm*Axis*Norm;
             if(0.0<Vec%Axis)
                 {
                 Vec.Negate();
                 }
             std::vector<SGM::Point3D> aPoints;
             std::vector<SGM::IntersectionType> aTypes;
             SGM::Interval1D Domain(-SGM_MAX,SGM_MAX);
             IntersectLineAndCone(Origin,Vec,Domain,pCone,dTolerance,aPoints,aTypes);
             aCurves.push_back(FindConicCurve(rResult,aPoints[0]+Vec,Norm,pCone));
             }
         }
     return aCurves.size();
     }

size_t IntersectPlaneAndPlane(SGM::Result                &rResult,
                              plane                const *pPlane1,
                              plane                const *pPlane2,
                              std::vector<curve *>       &aCurves,
                              face                 const *,//pFace1,
                              face                 const *,//pFace2,
                              double                      dTolerance)
     {
     SGM::UnitVector3D const &Norm1=pPlane1->m_ZAxis;
     SGM::UnitVector3D const &Norm2=pPlane2->m_ZAxis;
     if(fabs(Norm1%Norm2)<1.0-dTolerance)
         {
         SGM::Point3D const &Origin1=pPlane1->m_Origin;
         SGM::Point3D const &Origin2=pPlane2->m_Origin;
         SGM::Point3D Origin;
         SGM::UnitVector3D Axis;
         IntersectNonParallelPlanes(Origin1,Norm1,Origin2,Norm2,Origin,Axis);
         aCurves.push_back(new line(rResult,Origin,Axis));
         }
     return aCurves.size();
     }

size_t IntersectPlaneAndSphere(SGM::Result                &rResult,
                               plane                const *pPlane,
                               sphere               const *pSphere,
                               std::vector<curve *>       &aCurves,
                               face                 const *,//pFace1,
                               face                 const *,//pFace2,
                               double                      dTolerance)
    {
    SGM::Point3D Center;
    double dRadius;
    IntersectPlaneAndSphere(pPlane->m_Origin,pPlane->m_ZAxis,pSphere,dTolerance,Center,dRadius);
    if(dRadius==0)
        {
        aCurves.push_back(new PointCurve(rResult,Center));
        }
    else if(0<dRadius)
        {
        aCurves.push_back(new circle(rResult,Center,pPlane->m_ZAxis,dRadius));
        }
    return aCurves.size();
    }

size_t IntersectPlaneAndCylinder(SGM::Result                &rResult,
                                 plane                const *pPlane,
                                 cylinder             const *pCylinder,
                                 std::vector<curve *>       &aCurves,
                                 face                 const *,//pFace1,
                                 face                 const *,//pFace2,
                                 double                      dTolerance)
    {
    // Return one or two lines, a circle, an ellipse, or nothing.

    double dRadius=pCylinder->m_dRadius;
    SGM::Point3D const &Center=pCylinder->m_Origin;
    SGM::Point3D const &Origin=pPlane->m_Origin;
    SGM::UnitVector3D const &Norm=pPlane->m_ZAxis;
    double dDist=fabs((Center-Origin)%Norm);
    if(SGM::NearEqual(dRadius,dDist,dTolerance,false))
        {
        // One line.
        SGM::Point3D Pos=Center-Norm*((Center-Origin)%Norm);
        aCurves.push_back(new line(rResult,Pos,pCylinder->m_ZAxis));
        }
    if(dDist<dRadius)
        {
        SGM::UnitVector3D const &Axis=pCylinder->m_ZAxis;
        double dFABSDot=fabs(Norm%Axis);
        if(dFABSDot<dTolerance)
            {
            // Two lines.
            SGM::Point3D Pos=Center-Norm*((Center-Origin)%Norm);
            SGM::UnitVector3D UVec=Norm*pCylinder->m_ZAxis;
            double dH=sqrt(dRadius*dRadius-dDist*dDist);
            SGM::Point3D Pos0=Pos+UVec*dH;
            SGM::Point3D Pos1=Pos-UVec*dH;
            aCurves.push_back(new line(rResult,Pos0,pCylinder->m_ZAxis));
            aCurves.push_back(new line(rResult,Pos1,pCylinder->m_ZAxis));
            }
        else if(SGM::NearEqual(dFABSDot,1.0,dTolerance,false))
            {
            // Circle.
            SGM::Point3D CircleCenter=Center+(pCylinder->m_ZAxis)*((Origin-Center)%pCylinder->m_ZAxis);
            aCurves.push_back(new circle(rResult,CircleCenter,pCylinder->m_ZAxis,dRadius,&(pCylinder->m_XAxis)));
            }
        else
            {
            // Ellipse.
            SGM::UnitVector3D Minor=Norm*pCylinder->m_ZAxis;
            SGM::UnitVector3D Major=Minor*Norm;
            SGM::Interval1D Domain(-SGM_MAX,SGM_MAX);
            std::vector<SGM::Point3D> aPoints;
            std::vector<SGM::IntersectionType> aTypes;
            IntersectLineAndPlane(Center,pCylinder->m_ZAxis,Domain,Origin,Norm,dTolerance,aPoints,aTypes);
            SGM::Point3D EllipseCenter=aPoints[0];
            double dA=dRadius/dFABSDot;
            aCurves.push_back(new ellipse(rResult,EllipseCenter,Major,Minor,dA,dRadius));
            }
        }
    return aCurves.size();
    }

bool PlaneIsTangentToTorus(plane        const *pPlane,
                           torus        const *pTorus,
                           double              dTolerance,
                           SGM::Point3D       &TangentPos,
                           bool               &bInside)
    {
    SGM::UnitVector3D const &Normal=pPlane->m_ZAxis;
    double dX=Normal%pTorus->m_XAxis;
    double dY=Normal%pTorus->m_YAxis;
    double dZ=Normal%pTorus->m_ZAxis;
    double dU=SGM::SAFEatan2(dY,dX);
    SGM::UnitVector3D Spoke=cos(dU)*pTorus->m_XAxis+sin(dU)*pTorus->m_YAxis;
    double dS=Normal%Spoke;
    double dV=SGM::SAFEatan2(dZ,dS);
    SGM::Point3D Pos0,Pos1,Pos2,Pos3;
    pTorus->Evaluate(SGM::Point2D(dU,dV),&Pos0);
    pTorus->Evaluate(SGM::Point2D(dU+SGM_PI,dV),&Pos1);
    pTorus->Evaluate(SGM::Point2D(dU,dV+SGM_PI),&Pos2);
    pTorus->Evaluate(SGM::Point2D(dU+SGM_PI,dV+SGM_PI),&Pos3);
    SGM::Point3D PlanePos0,PlanePos1,PlanePos2,PlanePos3;
    pPlane->Inverse(Pos0,&PlanePos0);
    pPlane->Inverse(Pos1,&PlanePos1);
    pPlane->Inverse(Pos2,&PlanePos2);
    pPlane->Inverse(Pos3,&PlanePos3);
    if(Pos0.Distance(PlanePos0)<dTolerance)
        {
        TangentPos=Pos0;
        bInside=false;
        return true;
        }
    if(Pos1.Distance(PlanePos1)<dTolerance)
        {
        TangentPos=Pos1;
        bInside=false;
        return true;
        }
    if(Pos2.Distance(PlanePos2)<dTolerance)
        {
        TangentPos=Pos2;
        bInside=true;
        return true;
        }
    if(Pos3.Distance(PlanePos3)<dTolerance)
        {
        TangentPos=Pos3;
        bInside=true;
        return true;
        }
    return false;
    }

size_t IntersectPlaneAndTorus(SGM::Result                &rResult,
                              plane                const *pPlane,
                              torus                const *pTorus,
                              std::vector<curve *>       &aCurves,
                              face                 const *,//pFace1,
                              face                 const *,//pFace2,
                              double                      dTolerance)
    {
    SGM::UnitVector3D TorusNormal=pTorus->m_ZAxis;
    SGM::UnitVector3D PlaneNormal=pPlane->m_ZAxis;
    SGM::Point3D PlaneOrigin=pPlane->m_Origin;
    SGM::Point3D TorusCenter=pTorus->m_Center;
    double dAngle=0<(TorusNormal%PlaneNormal) ? TorusNormal.Angle(PlaneNormal) : TorusNormal.Angle(-PlaneNormal);
    double dVillarceauAngle=SGM::SAFEasin(pTorus->m_dMinorRadius/pTorus->m_dMajorRadius);
    double dPlaneDistFromCenter=(TorusCenter-PlaneOrigin)%PlaneNormal;

    // First test for the three circle cases.

    if(fabs(dPlaneDistFromCenter)<SGM_MIN_TOL && SGM_MIN_TOL<=dAngle)
        {
        if( SGM::NearEqual(dAngle,dVillarceauAngle,SGM_MIN_TOL,false))      // Villarceau circles.
            {
            SGM::UnitVector3D XVec=TorusNormal*PlaneNormal;
            double dRadius=pTorus->m_dMajorRadius;
            double dOffset=pTorus->m_dMajorRadius-pTorus->m_dMinorRadius;
            SGM::Point3D Center1=TorusCenter+XVec*dOffset;
            SGM::Point3D Center2=TorusCenter-XVec*dOffset;
            aCurves.push_back(new circle(rResult,Center1,PlaneNormal,dRadius));
            aCurves.push_back(new circle(rResult,Center2,PlaneNormal,dRadius));
            }
        else if(SGM::NearEqual(dAngle,SGM_HALF_PI,SGM_MIN_TOL,false))       // Minor Radius circles.
            {
            SGM::UnitVector3D UVec=TorusNormal*PlaneNormal;
            aCurves.push_back(new circle(rResult,TorusCenter+UVec*pTorus->m_dMajorRadius,PlaneNormal,pTorus->m_dMinorRadius));
            aCurves.push_back(new circle(rResult,TorusCenter-UVec*pTorus->m_dMajorRadius,PlaneNormal,pTorus->m_dMinorRadius));
            }
        else                                                                // Two Curves 
            {
            std::vector<SGM::Point3D> aPoints;
            std::vector<SGM::IntersectionType> aTypes;
            SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN,SGM_INTERVAL_POS_MAX);
            SGM::UnitVector3D LineAxis=TorusNormal*PlaneNormal;
            IntersectLineAndTorus(TorusCenter,LineAxis,Domain,pTorus,dTolerance,aPoints,aTypes);
            for(SGM::Point3D const &Pos : aPoints)
                {
                std::vector<SGM::Point3D> aEndPoints;
                aEndPoints.push_back(Pos);
                aCurves.push_back(WalkFromTo(rResult,Pos,aEndPoints,pPlane,pTorus));
                }
            }
        }
    else if(dAngle<SGM_MIN_TOL)                                             // Major radius circles.
        {
        SGM::Point3D Center=TorusCenter-PlaneNormal*((TorusCenter-PlaneOrigin)%PlaneNormal);
        if(fabs(dPlaneDistFromCenter)<pTorus->m_dMinorRadius+dTolerance)
            {
            SGM::Point3D Center2=TorusCenter-PlaneNormal*dPlaneDistFromCenter;
            if(SGM::NearEqual(fabs(dPlaneDistFromCenter),pTorus->m_dMinorRadius,dTolerance,false))
                {
                aCurves.push_back(new circle(rResult,Center2,PlaneNormal,pTorus->m_dMajorRadius));
                }
            else
                {
                double dOffset=sqrt(pTorus->m_dMinorRadius*pTorus->m_dMinorRadius-dPlaneDistFromCenter*dPlaneDistFromCenter);
                aCurves.push_back(new circle(rResult,Center2,PlaneNormal,pTorus->m_dMajorRadius+dOffset));
                aCurves.push_back(new circle(rResult,Center2,PlaneNormal,pTorus->m_dMajorRadius-dOffset));
                }
            }
        }
    else
        {
        // Test for a tangent plane.

        SGM::Point3D TangentPos;
        bool bInside;
        if(PlaneIsTangentToTorus(pPlane,pTorus,dTolerance,TangentPos,bInside))
            {
            if(bInside)
                {
                std::vector<SGM::Point3D> aPoints;
                std::vector<SGM::IntersectionType> aTypes;
                SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN,SGM_INTERVAL_POS_MAX);
                SGM::UnitVector3D LineAxis=TorusNormal*PlaneNormal;
                SGM::Point3D LineOrigin;
                pPlane->Inverse(TorusCenter,&LineOrigin);
                IntersectLineAndTorus(LineOrigin,LineAxis,Domain,pTorus,dTolerance,aPoints,aTypes);
                std::vector<SGM::Point3D> aEndPoints;
                aEndPoints.push_back(TangentPos);
                aCurves.push_back(WalkFromTo(rResult,aPoints.front(),aEndPoints,pPlane,pTorus));
                aCurves.push_back(WalkFromTo(rResult,aPoints.back(),aEndPoints,pPlane,pTorus));
                }
            else
                {
                aCurves.push_back(new PointCurve(rResult,TangentPos));
                }
            }
        else
            {
            std::vector<SGM::Point3D> aPoints;
            std::vector<SGM::IntersectionType> aTypes;
            SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN,SGM_INTERVAL_POS_MAX);
            SGM::UnitVector3D LineAxis=TorusNormal*PlaneNormal;
            SGM::Point3D LineOrigin;
            pPlane->Inverse(TorusCenter,&LineOrigin);
            IntersectLineAndTorus(LineOrigin,LineAxis,Domain,pTorus,dTolerance,aPoints,aTypes);
            for(SGM::Point3D const &Pos : aPoints)
                {
                if(PointOnCurves(Pos,aCurves)==false)
                    {
                    std::vector<SGM::Point3D> aEndPoints;
                    aEndPoints.push_back(Pos);
                    aCurves.push_back(WalkFromTo(rResult,Pos,aEndPoints,pPlane,pTorus));
                    }
                }
            }
        }
    
    return aCurves.size();
    }

size_t IntersectPlaneAndSurface(SGM::Result                &rResult,
                                plane                const *pPlane,
                                surface              const *pSurface,
                                std::vector<curve *>       &aCurves,
                                face                 const *pFace1,
                                face                 const *pFace2,
                                double                      dTolerance)
     {
     switch(pSurface->GetSurfaceType())
        {
        case SGM::EntityType::PlaneType:
            {
            auto pPlane2=(plane const *)pSurface;
            return IntersectPlaneAndPlane(rResult,pPlane,pPlane2,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::SphereType:
            {
            auto pSphere=(sphere const *)pSurface;
            return IntersectPlaneAndSphere(rResult,pPlane,pSphere,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::CylinderType:
            {
            auto pCylinder=(cylinder const *)pSurface;
            return IntersectPlaneAndCylinder(rResult,pPlane,pCylinder,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::ConeType:
            {
            auto pCone=(cone const *)pSurface;
            return IntersectPlaneAndCone(rResult,pPlane,pCone,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::TorusType:
            {
            torus const *pTorus=(torus const *)pSurface;
            return IntersectPlaneAndTorus(rResult,pPlane,pTorus,aCurves,pFace1,pFace2,dTolerance);
            }
        //case SGM::EntityType::RevolveType:
        //    {
        //    throw;
        //    }
        default:
            {
            return IntersectPlaneAndSurface(rResult,pPlane,pSurface,aCurves,pFace1,pFace2,dTolerance);
            }
        }
     }

size_t IntersectSphereAndSphere(SGM::Result                &rResult,
                                sphere               const *pSphere1,
                                sphere               const *pSphere2,
                                std::vector<curve *>       &aCurves,
                                face                 const *,//pFace1,
                                face                 const *,//pFace2,
                                double                      dTolerance)
    {
    double dR1=pSphere1->m_dRadius;
    double dR2=pSphere2->m_dRadius;
    SGM::Point3D const &Center1=pSphere1->m_Center;
    SGM::Point3D const &Center2=pSphere2->m_Center;
    double dDist=Center1.Distance(Center2);
    if(SGM::NearEqual(dR1+dR2,dDist,dTolerance,false))
        {
        SGM::UnitVector3D UVec=Center1-Center2;
        SGM::Interval1D Domain(0,SGM_TWO_PI);
        aCurves.push_back(new PointCurve(rResult,Center2+UVec*dR2,&Domain));  // Tangent outside of each other.
        }
    else if(dDist<dR1+dR2)
        {
        SGM::UnitVector3D Norm=Center1-Center2;
        double S=(dR1+dR2+dDist)*0.5;
        double dArea=sqrt(S*(S-dR1)*(S-dR2)*(S-dDist));
        double dRadius=2*dArea/dDist;
        if(dRadius<dTolerance)
            {
            if(dR1<dR2)
                {
                SGM::UnitVector3D UVec=Center1-Center2;
                SGM::Interval1D Domain(0,SGM_TWO_PI);
                aCurves.push_back(new PointCurve(rResult,Center2+UVec*dR2,&Domain));  // Tangent pSphere1 is inside pSphere2.
                }
            else
                {
                SGM::UnitVector3D UVec=Center2-Center1;
                SGM::Interval1D Domain(0,SGM_TWO_PI);
                aCurves.push_back(new PointCurve(rResult,Center1+UVec*dR1,&Domain));  // Tangent pSphere2 is inside pSphere1.
                }
            }
        else
            {
            aCurves.push_back(new circle(rResult,Center2+Norm*sqrt(dR2*dR2-dRadius*dRadius),Norm,dRadius));
            }
        }
    return aCurves.size();
    }

size_t IntersectCylinders(SGM::Result                &rResult,
                          cylinder             const *pCylinder1,
                          cylinder             const *pCylinder2,
                          std::vector<curve *>       &aCurves,
                          face                 const *,//pFace1,
                          face                 const *,//pFace2,
                          double                      dTolerance)
    {
    SGM::UnitVector3D Axis1=pCylinder1->m_ZAxis;
    SGM::UnitVector3D Axis2=pCylinder2->m_ZAxis;
    double dAngle=SGM::SAFEacos(fabs(Axis1%Axis2));
    double dRadius1=pCylinder1->m_dRadius;
    double dRadius2=pCylinder2->m_dRadius;
    if(dAngle<dTolerance)   // Zero, One or Two Line Case
        {
        SGM::Point3D Center1=pCylinder1->m_Origin;
        SGM::Point3D Origin2=pCylinder2->m_Origin;
        SGM::Point3D Center2=Origin2+Axis2*((Center1-Origin2)%Axis2);
        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;
        IntersectCircleAndCircle(Center1,Center2,Axis1,Axis1,dRadius1,dRadius2,aPoints,aTypes,dTolerance);
        for(SGM::Point3D Pos:aPoints)
            {
            aCurves.push_back(new line(rResult,Pos,Axis1));
            }
        }
    else  
        {
        SGM::Point3D Origin1=pCylinder1->m_Origin;
        SGM::Point3D Origin2=pCylinder2->m_Origin;
        
        SGM::Segment3D Seg1(Origin1,Origin1+Axis1);
        SGM::Segment3D Seg2(Origin2,Origin2+Axis2);
        SGM::Point3D CPos1,CPos2;
        Seg1.Intersect(Seg2,CPos1,CPos2);
        double dDist=CPos1.Distance(CPos2);
        
        if( SGM::NearEqual(dDist,dRadius1+dRadius2,dTolerance,false))   // Single Point Case
            {
            SGM::Point3D Pos=SGM::MidPoint(CPos1,CPos2,dRadius1/(dRadius1+dRadius2));
            aCurves.push_back(new PointCurve(rResult,Pos));
            }
        else if( dDist<dTolerance &&                                       
            SGM::NearEqual(dRadius1,dRadius2,dTolerance,false))         // Two Ellipses Case 
            {
            // Find where the cylinders intersect in the plane defined by Center and Axis1*Axis2.

            SGM::Point3D Center=CPos1;
            SGM::UnitVector3D Up=Axis1*Axis2;
            SGM::UnitVector3D XVec1=Up*Axis1;
            SGM::Point3D Pos0=Center+XVec1*dRadius1;
            SGM::Point3D Pos1=Center-XVec1*dRadius1;
            SGM::UnitVector3D XVec2=Up*Axis2;
            SGM::Point3D Pos2=Center+XVec2*dRadius2;
            std::vector<SGM::Point3D> aPoints2;
            std::vector<SGM::IntersectionType> aTypes2;
            SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN,SGM_INTERVAL_POS_MAX);
            IntersectLineAndLine(Pos0,Axis1,Domain,Pos2,Axis2,Domain,dTolerance,aPoints2,aTypes2);
            SGM::Point3D H1=aPoints2[0];
            std::vector<SGM::Point3D> aPoints3;
            std::vector<SGM::IntersectionType> aTypes3;
            IntersectLineAndLine(Pos1,Axis1,Domain,Pos2,Axis2,Domain,dTolerance,aPoints3,aTypes3);
            SGM::Point3D H2=aPoints3[0];

            SGM::UnitVector3D XAxis1=H1-Center;
            SGM::UnitVector3D Normal1=Up*Axis1;
            double dA1=H1.Distance(Center);
            aCurves.push_back(new ellipse(rResult,Center,XAxis1,Up,dA1,dRadius1));

            SGM::UnitVector3D XAxis2=H2-Center;
            SGM::UnitVector3D Normal2=Up*Axis2;
            double dA2=H2.Distance(Center);
            aCurves.push_back(new ellipse(rResult,Center,XAxis2,Up,dA2,dRadius2));
            }
        else
            {
            SGM::Point3D const &Origin1=pCylinder1->m_Origin;
            SGM::Segment3D Seg1(Origin1,Origin1+Axis1);
            SGM::Point3D const &Origin2=pCylinder2->m_Origin;
            SGM::Segment3D Seg2(Origin2,Origin2+Axis2);
            SGM::Point3D Pos1,Pos2;
            Seg1.Intersect(Seg2,Pos1,Pos2);
            double dDist=Pos1.Distance(Pos2);
            std::vector<SGM::Point3D> aTangentPoints;
            if(SGM::NearEqual(dDist+dRadius1,dRadius2,dTolerance,false)==true)
                {
                SGM::UnitVector3D UVec=Pos1-Pos2;
                aTangentPoints.push_back(Pos2+UVec*dRadius2);
                }
            if(SGM::NearEqual(dDist+dRadius2,dRadius1,dTolerance,false)==true)
                {
                SGM::UnitVector3D UVec=Pos2-Pos1;
                aTangentPoints.push_back(Pos1+UVec*dRadius1);
                }
            pCylinder2->Inverse(Pos1,&Pos2);
            std::vector<SGM::Point3D> aPoints;
            std::vector<SGM::IntersectionType> aTypes;
            SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN, SGM_INTERVAL_POS_MAX);
            IntersectLineAndCylinder(Pos2,Axis2,Domain,pCylinder1,dTolerance,aPoints,aTypes);
            for(SGM::Point3D const &Pos : aPoints)
                {
                if(PointOnCurves(Pos,aCurves)==false)
                    {
                    std::vector<SGM::Point3D> aEndPoints=aTangentPoints;
                    aEndPoints.push_back(Pos);
                    aCurves.push_back(WalkFromTo(rResult,Pos,aEndPoints,pCylinder1,pCylinder2));
                    }
                }
            aPoints.clear();
            aTypes.clear();
            IntersectLineAndCylinder(Pos1,Axis1,Domain,pCylinder1,dTolerance,aPoints,aTypes);
            for(SGM::Point3D const &Pos : aPoints)
                {
                if(PointOnCurves(Pos,aCurves)==false)
                    {
                    std::vector<SGM::Point3D> aEndPoints;
                    aEndPoints.push_back(Pos);
                    aCurves.push_back(WalkFromTo(rResult,Pos,aEndPoints,pCylinder1,pCylinder2));
                    }
                }
            }
        }

    return aCurves.size();
    }

size_t IntersectSphereAndCone(SGM::Result                &rResult,
                              sphere               const *pSphere,
                              cone                 const *pCone,
                              std::vector<curve *>       &aCurves,
                              face                 const *,//pFace1,
                              face                 const *,//pFace2,
                              double                      dTolerance)
    {
    SGM::Point3D const &Center=pSphere->m_Center;
    SGM::Point3D const &Origin=pCone->m_Origin;
    SGM::UnitVector3D const &Axis=pCone->m_ZAxis;
    SGM::Point3D Apex=pCone->FindApex();
    SGM::Point3D AxisPos=Origin+Axis*(Axis%(Center-Origin));
    SGM::Point3D Pos;
    SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
    std::vector<SGM::Point3D> aPoints;
    std::vector<SGM::IntersectionType> aTypes;

    if(SGM::NearEqual(AxisPos,Center,dTolerance))
        {
        pCone->Evaluate(SGM::Point2D(0,0),&Pos);
        SGM::UnitVector3D LineAxis=Pos-Apex;
        IntersectLineAndSphere(Apex,LineAxis,Domain,pSphere,dTolerance,aPoints,aTypes);
        for(SGM::Point3D const &Pos : aPoints)
            {
            if(SGM::NearEqual(Apex,Pos,dTolerance))
                {
                aCurves.push_back(new PointCurve(rResult,Apex));
                }
            else
                {
                AxisPos=Origin+Axis*(Axis%(Pos-Origin));
                double dRadius=AxisPos.Distance(Pos);
                aCurves.push_back(new circle(rResult,AxisPos,Axis,dRadius,&pCone->m_XAxis));
                }
            }
        }
    else
        {
        pSphere->Inverse(Apex,&Pos);
        SGM::Point3D DropCenter=Center-Axis*(Axis%(Center-Origin));
        SGM::Point2D uv=pCone->Inverse(DropCenter,&Pos);
        IntersectLineAndSphere(Apex,Pos-Apex,Domain,pSphere,dTolerance,aPoints,aTypes);
        if(SGM::NearEqual(Apex.Distance(Center),pSphere->m_dRadius,dTolerance,false))
            {
            // Tangent Point, Maybe More

            SGM::Point3D DropCenter=Center-Axis*(Axis%(Center-Origin));
            pCone->Inverse(DropCenter,&Pos);
            if(1<aPoints.size())
                {
                std::vector<SGM::Point3D> aEndPoints;
                aEndPoints.push_back(Apex);
                aCurves.push_back(WalkFromTo(rResult,aPoints[1],aEndPoints,pSphere,pCone));
                }
            else
                {
                aCurves.push_back(new PointCurve(rResult,Apex));
                }
            }
        else
            {
            // Check for opposite tangent hits.

            uv.m_u+=SGM_PI;
            pCone->Evaluate(uv,&Pos);
            std::vector<SGM::Point3D> aPoints2;
            std::vector<SGM::IntersectionType> aTypes2;
            IntersectLineAndSphere(Apex,Pos-Apex,Domain,pSphere,dTolerance,aPoints2,aTypes2);

            // General Solution

            size_t nPoints=aPoints.size();
            size_t Index1;
            for(Index1=0;Index1<nPoints;++Index1)
                {
                SGM::Point3D const &Hit=aPoints[Index1];
                if(PointOnCurves(Hit,aCurves)==false)
                    {
                    // Check for tangent points.

                    if(aTypes[Index1]==SGM::IntersectionType::TangentType)
                        {
                        aCurves.push_back(new PointCurve(rResult,Hit));
                        }
                    else
                        {
                        std::vector<SGM::Point3D> aEndPoints;
                        if(aTypes2.size() && aTypes2[0]==SGM::IntersectionType::TangentType)
                            {
                            aEndPoints.push_back(aPoints2[0]);
                            }
                        else
                            {
                            aEndPoints.push_back(Hit);
                            }
                        aCurves.push_back(WalkFromTo(rResult,Hit,aEndPoints,pSphere,pCone));
                        }
                    }
                }
            }
        }
    return aCurves.size();
    }

size_t IntersectConeAndCylinder(SGM::Result                &rResult,
                                cone                 const *pCone,
                                cylinder             const *pCylinder,
                                std::vector<curve *>       &aCurves,
                                face                 const *,//pFace1,
                                face                 const *,//pFace2,
                                double                      dTolerance)
    {
    SGM::UnitVector3D const &ConeAxis=pCone->m_ZAxis;
    SGM::UnitVector3D CylinderAxis=pCylinder->m_ZAxis;
    SGM::Point3D const &ConeCenter=pCone->m_Origin;
    SGM::Point3D const &CylinderCenter=pCylinder->m_Origin;
    SGM::Point3D Apex=pCone->FindApex();
    double dAxisAngle=ConeAxis.Angle(CylinderAxis);
    std::vector<SGM::Point3D> aPoints;
    std::vector<SGM::IntersectionType> aTypes;
    SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN,SGM_INTERVAL_POS_MAX);
    IntersectLineAndLine(ConeCenter,ConeAxis,Domain,CylinderCenter,CylinderAxis,Domain,dTolerance,aPoints,aTypes);
    if(aPoints.size()==2)
        {
        // Circle Case.

        SGM::Vector3D Vec=Apex-ConeCenter;
        SGM::Point3D Center=ConeCenter+Vec*(pCylinder->m_dRadius/pCone->m_dRadius);
        aCurves.push_back(new circle(rResult,Center,ConeAxis,pCylinder->m_dRadius,&pCone->m_XAxis));
        }
    else if(aPoints.size()==1 && SGM::NearEqual(pCone->FindHalfAngle(),dAxisAngle,dTolerance,false))
        {
        // Cylinder is at the same angle ad the cone.
        // Check the distance from the apex to the line.

        SGM::Point3D LineOrigin=aPoints[0];
        SGM::Point3D ClosePos=LineOrigin+CylinderAxis*((Apex-LineOrigin)%CylinderAxis);
        double dApexDist=ClosePos.Distance(Apex);
        if(0<CylinderAxis%ConeAxis)
            {
            CylinderAxis.Negate();
            }

        if(SGM::NearEqual(dApexDist,pCylinder->m_dRadius,dTolerance,false))
            {
            // The Line Case.

            line *pLine=new line(rResult,Apex,CylinderAxis);
            SGM::Interval1D RayDomain(0,SGM_INTERVAL_POS_MAX);
            pLine->SetDomain(RayDomain);
            aCurves.push_back(pLine);

            // Check for and ellipse
            
            SGM::Point3D RayOrigin=Apex+(ClosePos-Apex)*2;
            std::vector<SGM::Point3D> aHits;
            std::vector<SGM::IntersectionType> aTypes;
            if(IntersectLineAndCone(RayOrigin,CylinderAxis,Domain,pCone,dTolerance,aHits,aTypes))
                {
                SGM::Point3D Pos0=aHits[0];
                SGM::UnitVector3D Vec=ConeAxis*CylinderAxis;
                SGM::Point3D Pos1=CylinderCenter+Vec*pCylinder->m_dRadius;
                SGM::Point3D Pos2=CylinderCenter-Vec*pCylinder->m_dRadius;

                aHits.clear();
                aTypes.clear();
                IntersectLineAndCone(Pos1,CylinderAxis,Domain,pCone,dTolerance,aHits,aTypes);
                Pos1=aHits[0];

                aHits.clear();
                aTypes.clear();
                IntersectLineAndCone(Pos2,CylinderAxis,Domain,pCone,dTolerance,aHits,aTypes);
                Pos2=aHits[0];

                SGM::Point3D Center=SGM::MidPoint(Pos1,Pos2);
                SGM::UnitVector3D Vec1=Pos1-Center;
                SGM::UnitVector3D Vec0=Pos0-Center;
                double dRadius1=Pos1.Distance(Center);
                double dRadius0=Pos0.Distance(Center);

                aCurves.push_back(new ellipse(rResult,Center,Vec0,Vec1,dRadius0,dRadius1));
                }
            }
        else
            {
            // Infinite case.

            SGM::UnitVector3D OutVec=ConeAxis*CylinderAxis;
            SGM::UnitVector3D OutVec2=OutVec*CylinderAxis;
            SGM::Point3D Pos2=CylinderCenter+OutVec2*pCylinder->m_dRadius;
            SGM::Point3D Pos3=CylinderCenter-OutVec2*pCylinder->m_dRadius;
            std::vector<SGM::Point3D> aHits2,aHits3;
            std::vector<SGM::IntersectionType> aTypes2,aTypes3;
            if(IntersectLineAndCone(Pos2,CylinderAxis,Domain,pCone,dTolerance,aHits2,aTypes2))
                {
                SGM::Point3D StartPos=aHits2[0];
                std::vector<SGM::Point3D> aEndPoints;
                aCurves.push_back(WalkFromTo(rResult,StartPos,aEndPoints,pCone,pCylinder));
                }
            else if(IntersectLineAndCone(Pos3,CylinderAxis,Domain,pCone,dTolerance,aHits3,aTypes3))
                {
                SGM::Point3D StartPos=aHits3[0];
                std::vector<SGM::Point3D> aEndPoints;
                aCurves.push_back(WalkFromTo(rResult,StartPos,aEndPoints,pCone,pCylinder));
                }
            }
        }
    else
        {
        // Check to see if the apex touches the cylinder.

        SGM::Point3D ProjectedApex=CylinderCenter+CylinderAxis*(CylinderAxis%(Apex-CylinderCenter));
        if(SGM::NearEqual(pCylinder->m_dRadius,ProjectedApex.Distance(Apex),dTolerance,false))
            {
            if(pCone->FindHalfAngle()+dTolerance<dAxisAngle)
                {
                // Non-tangent point at apex.
                // Check for other parts.

                std::vector<SGM::Point3D> aHits,aHits2,aHits3,aHits4;
                std::vector<SGM::IntersectionType> aTypes;
                SGM::UnitVector3D LineAxis=ConeAxis;
                LineAxis.Negate();
                IntersectLineAndCylinder(Apex,LineAxis,Domain,pCylinder,dTolerance,aHits,aTypes);
                if(2==aHits.size())
                    {
                    SGM::Point3D BelowApex=aHits[1];
                    aTypes.clear();
                    IntersectLineAndCone(BelowApex,CylinderAxis,Domain,pCone,dTolerance,aHits2,aTypes);
                    if(aHits2.size()==2)
                        {
                        // One curve around the cone and a point at the apex.

                        aCurves.push_back(new PointCurve(rResult,Apex));
                        std::vector<SGM::Point3D> aEndPoints;
                        aEndPoints.push_back(aHits2.back());
                        aCurves.push_back(WalkFromTo(rResult,aHits2.back(),aEndPoints,pCone,pCylinder));
                        }
                    else if(aHits2.size()==1)
                        {
                        aTypes.clear();
                        if(IntersectLineAndCone(pCylinder->m_Origin,pCylinder->m_ZAxis,Domain,pCone,dTolerance,aHits3,aTypes))
                            {
                            // A tear drop curve

                            aTypes.clear();
                            IntersectLineAndCylinder(Apex,aHits3[0]-Apex,Domain,pCylinder,dTolerance,aHits4,aTypes);
                            std::vector<SGM::Point3D> aEndPoints;
                            aEndPoints.push_back(Apex);
                            aCurves.push_back(WalkFromTo(rResult,aHits4.back(),aEndPoints,pCone,pCylinder));
                            }
                        else
                            {
                            // Non-tangent point outside.

                            aCurves.push_back(new PointCurve(rResult,Apex));
                            }
                        }
                    }
                else if(aHits.size())
                    {
                    // Find the tear drop curve.

                    SGM::UnitVector3D CylinderNorm;
                    SGM::Point2D uv=pCylinder->Inverse(Apex);
                    pCylinder->Evaluate(uv,nullptr,nullptr,nullptr,&CylinderNorm);
                    SGM::Point3D Pos0=Apex-CylinderNorm;
                    SGM::Point3D Pos1;
                    pCone->Inverse(Pos0,&Pos1);
                    SGM::UnitVector3D LineAxis2=Pos1-Apex;
                    std::vector<SGM::Point3D> aHits2;
                    std::vector<SGM::IntersectionType> aTypes2;
                    IntersectLineAndCylinder(Apex,LineAxis2,Domain,pCylinder,dTolerance,aHits2,aTypes2);

                    std::vector<SGM::Point3D> aEndPoints;
                    aEndPoints.push_back(Apex);
                    aCurves.push_back(WalkFromTo(rResult,aHits2[1],aEndPoints,pCone,pCylinder));
                    }
                else
                    {
                    aCurves.push_back(new PointCurve(rResult,Apex));
                    }
                }
            else if(SGM::NearEqual(dAxisAngle,pCone->FindHalfAngle(),dTolerance,false))
                {
                // One infinite end and line.

                if(0<CylinderAxis%ConeAxis)
                    {
                    CylinderAxis.Negate();
                    }
                line *pLine=new line(rResult,Apex,CylinderAxis);
                SGM::Interval1D Domain(0,SGM_INTERVAL_POS_MAX);
                pLine->SetDomain(Domain);
                aCurves.push_back(pLine);

                SGM::Point3D CylinderAxisAtApex=CylinderCenter+CylinderAxis*(CylinderAxis%(Apex-CylinderCenter));
                SGM::Vector3D Vec=CylinderAxisAtApex-Apex;
                SGM::UnitVector3D OffsetVec=Vec*CylinderAxis;
                SGM::Point3D TestPos1=Apex+OffsetVec*(pCylinder->m_dRadius*0.1);
                SGM::Point3D TestPos2=Apex-OffsetVec*(pCylinder->m_dRadius*0.1);
                SGM::Point3D Pos1=ZoomInFrom(TestPos1,pCone,pCylinder);
                SGM::Point3D Pos2=ZoomInFrom(TestPos2,pCone,pCylinder);
                double dDist1=Pos1.DistanceSquared(Apex);
                double dDist2=Pos2.DistanceSquared(Apex);
                std::vector<SGM::Point3D> aEndPoints;
                aEndPoints.push_back(Apex);
                if(dDist1<dDist2)
                    {
                    aCurves.push_back(WalkFromTo(rResult,Pos2,aEndPoints,pCone,pCylinder));
                    }
                else
                    {
                    aCurves.push_back(WalkFromTo(rResult,Pos1,aEndPoints,pCone,pCylinder));
                    }
                }
            }
        else
            {
            // Check for tangent points.

            SGM::Point3D Origin1,Origin2;
            FindCylinderSilhouette(pCylinder,Apex,Origin1,Origin2);
            std::vector<SGM::Point3D> aSilHits1,aSilHits2;
            std::vector<SGM::IntersectionType> aSilTypes1,aSilTypes2;
            SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN,SGM_INTERVAL_POS_MAX);
            IntersectLineAndCone(Origin1,CylinderAxis,Domain,pCone,dTolerance,aSilHits1,aSilTypes1);
            IntersectLineAndCone(Origin2,CylinderAxis,Domain,pCone,dTolerance,aSilHits2,aSilTypes2);
            std::vector<SGM::Point3D> aTangent,aHit;
            std::vector<double> aDot;
            for(SGM::Point3D Pos : aSilHits2)
                {
                aSilHits1.push_back(Pos);
                }
            for(SGM::Point3D Pos : aSilHits1)
                {
                SGM::Point2D uv1=pCone->Inverse(Pos);
                SGM::Point2D uv2=pCylinder->Inverse(Pos);
                SGM::UnitVector3D Norm1,Norm2;
                pCone->Evaluate(uv1,nullptr,nullptr,nullptr,&Norm1);
                pCylinder->Evaluate(uv2,nullptr,nullptr,nullptr,&Norm2);
                double dDot=Norm1%Norm2;
                if(SGM::NearEqual(fabs(dDot),1,dTolerance,false))
                    {
                    aDot.push_back(dDot);
                    aTangent.push_back(Pos);
                    }
                else
                    {
                    aHit.push_back(Pos);
                    }
                }

            if(aTangent.size()==2)
                {
                // Two ellipses
                
                std::vector<SGM::Point3D> aPoints3;
                std::vector<SGM::IntersectionType> aTypes3;
                SGM::Interval1D Domain2(0,SGM_INTERVAL_POS_MAX);
                SGM::UnitVector3D LineAxis=-ConeAxis;
                if(IntersectLineAndCylinder(Apex,LineAxis,Domain2,pCylinder,SGM_MIN_TOL,aPoints3,aTypes3)==2)
                    {
                    std::vector<SGM::Point3D> aPoints4,aPoints5;
                    std::vector<SGM::IntersectionType> aTypes4,aTypes5;
                    IntersectLineAndCone(aPoints3[0],CylinderAxis,Domain,pCone,SGM_MIN_TOL,aPoints4,aTypes4);
                    IntersectLineAndCone(aPoints3[1],CylinderAxis,Domain,pCone,SGM_MIN_TOL,aPoints5,aTypes5);
                    if(aPoints4.size()==2)
                        {
                        double dMinor=pCylinder->m_dRadius;
                        double dMajor=aPoints4[0].Distance(aPoints5[1])*0.5;
                        SGM::UnitVector3D MajorVec1=aPoints4[0]-aPoints5[1];
                        SGM::UnitVector3D MajorVec2=aPoints4[1]-aPoints5[0];
                        SGM::UnitVector3D MinorVec=CylinderAxis*ConeAxis;
                        SGM::Point3D Center1=SGM::MidPoint(aPoints4[0],aPoints5[1]);
                        SGM::Point3D Center2=SGM::MidPoint(aPoints4[1],aPoints5[0]);
                        aCurves.push_back(new ellipse(rResult,Center1,MinorVec,MajorVec1,dMinor,dMajor));
                        aCurves.push_back(new ellipse(rResult,Center2,MinorVec,MajorVec2,dMinor,dMajor));
                        }
                    }
                }
            else if(aTangent.size()==1)
                {
                if(aDot[0]<0)
                    {
                    // Tangent point

                    aCurves.push_back(new PointCurve(rResult,aTangent[0]));
                    }
                else if(aHit.size()==2)
                    {
                    // Tangent point, teardrop curves that do not contract on the cylinder.
                    
                    std::vector<SGM::Point3D> aEndPoints;
                    aEndPoints.push_back(aTangent[0]);
                    aCurves.push_back(WalkFromTo(rResult,aHit[0],aEndPoints,pCone,pCylinder));
                    aCurves.push_back(WalkFromTo(rResult,aHit[1],aEndPoints,pCone,pCylinder));
                    }
                else
                    {
                    // Intersect ray from apex to other origin, projected to the cone, to find aHits for curves.

                    SGM::Point3D Pos1=aSilHits1.empty() ? Origin1 : Origin2;
                    SGM::Point3D Pos2;
                    pCone->Inverse(Pos1,&Pos2);
                    std::vector<SGM::IntersectionType> aSilTypes3;
                    SGM::UnitVector3D Axis=Pos2-Apex;
                    IntersectLineAndCylinder(Apex,Axis,Domain,pCylinder,dTolerance,aHit,aSilTypes3);
                    std::vector<SGM::Point3D> aEndPoints;
                    aEndPoints.push_back(aTangent[0]);
                    aCurves.push_back(WalkFromTo(rResult,aHit[0],aEndPoints,pCone,pCylinder));
                    aCurves.push_back(WalkFromTo(rResult,aHit[1],aEndPoints,pCone,pCylinder));
                    }
                }
            else if(aHit.size())
                {
                // General Solution

                SGM::RemoveDuplicates3D(aHit,dTolerance);
                std::vector<SGM::Point3D> aEndPoints;
                aEndPoints.push_back(aHit[0]);
                curve *pCurve=WalkFromTo(rResult,aHit[0],aEndPoints,pCone,pCylinder);
                aCurves.push_back(pCurve);

                if(1<aHit.size())
                    {
                    SGM::Point3D StartPos=aHit[1];
                    SGM::Point3D TestPos;
                    pCurve->Inverse(StartPos,&TestPos);
                    double dDist=StartPos.Distance(TestPos);
                    if(SGM_FIT_SMALL*5<dDist)
                        {
                        aEndPoints.clear();
                        aEndPoints.push_back(StartPos);
                        aCurves.push_back(WalkFromTo(rResult,StartPos,aEndPoints,pCone,pCylinder));
                        }
                    }
                }
            else
                {
                // General case without hits.
                // Fire a ray on a cylinder and the cone to fine hits.

                SGM::Point3D Pos0;
                pCone->Evaluate(SGM::Point2D(0,0),&Pos0);
                std::vector<SGM::Point3D> aStart1;
                std::vector<SGM::IntersectionType> aTypeA,aTypeB;
                IntersectLineAndCylinder(Apex,Pos0-Apex,Domain,pCylinder,dTolerance,aStart1,aTypeA);
                if(aStart1.size()==2)
                    {
                    std::vector<SGM::Point3D> aEndPoints;
                    aEndPoints.push_back(aStart1[0]);
                    aCurves.push_back(WalkFromTo(rResult,aStart1[0],aEndPoints,pCone,pCylinder));
                    aEndPoints.clear();
                    aEndPoints.push_back(aStart1[1]);
                    aCurves.push_back(WalkFromTo(rResult,aStart1[1],aEndPoints,pCone,pCylinder));
                    }
                }
            }
        }
    return aCurves.size();
    }

void FindWalkPoints(torus                     const *pTorus,
                    cylinder                  const *pCylinder,
                    std::vector<SGM::Point3D> const &aTangents,
                    std::vector<SGM::Point3D>       &aWalkPoints)
    {
    // Add a points between the tangent points.
    
    SGM::UnitVector3D const &XVec=pCylinder->m_XAxis;
    SGM::UnitVector3D const &YVec=pCylinder->m_YAxis;
    SGM::UnitVector3D const &ZVec=pCylinder->m_ZAxis;
    SGM::Point3D const &Center=pCylinder->m_Origin;
    double dRadius=pCylinder->m_dRadius;
    std::vector<double> aAngles;
    for(SGM::Point3D const &Pos : aTangents)
        {
        SGM::Point3D Projected=Pos-ZVec*(ZVec%(Pos-Center));
        SGM::Vector3D Vec=Projected-Center;
        aAngles.push_back(SGM::SAFEatan2(Vec%YVec,Vec%XVec));
        }
    SGM::RemoveDuplicates1D(aAngles,SGM_FIT);
    size_t nAngles=aAngles.size();
    size_t Index1;
    std::vector<SGM::Point3D> aRayPoints;
    for(Index1=0;Index1<nAngles;++Index1)
        {
        double dLast=Index1 ? aAngles[Index1-1] : aAngles.back()-SGM_TWO_PI;
        double dAngle=(dLast+aAngles[Index1])*0.5;
        aRayPoints.push_back(Center+(cos(dAngle)*dRadius)*XVec+(sin(dAngle)*dRadius)*YVec);
        }

    // And add the closest point to the torus center.

    SGM::Point3D CPos;
    pCylinder->Inverse(pTorus->m_Center,&CPos);
    aRayPoints.push_back(CPos);

    // Fire all the rays to the torus and remove tangent points.

    SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN, SGM_INTERVAL_POS_MAX);
    for(SGM::Point3D const &Pos : aRayPoints)
        {
        std::vector<SGM::Point3D> aHits;
        std::vector<SGM::IntersectionType> aTypes;
        size_t nHits=IntersectLineAndTorus(Pos,ZVec,Domain,pTorus,SGM_FIT,aHits,aTypes);
        for(Index1=0;Index1<nHits;++Index1)
            {
            if(aTypes[Index1]!=SGM::IntersectionType::TangentType)
                {
                aWalkPoints.push_back(aHits[Index1]);
                }
            }
        }
    }

size_t IntersectTorusAndCylinder(SGM::Result                &rResult,
                                 torus                const *pTorus,
                                 cylinder             const *pCylinder,
                                 std::vector<curve *>       &aCurves,
                                 face                 const *,//pFace1,
                                 face                 const *,//pFace2,
                                 double                      dTolerance)
    {
    double dCylinderRadius=pCylinder->m_dRadius;
    double dMajor=pTorus->m_dMajorRadius;
    double dMinor=pTorus->m_dMinorRadius;
    SGM::UnitVector3D const &CylinderAxis=pCylinder->m_ZAxis;
    SGM::UnitVector3D const &TorusAxis=pTorus->m_ZAxis;
    SGM::Point3D const &Center=pTorus->m_Center;
    SGM::UnitVector3D const XAxis=pCylinder->m_XAxis;
    SGM::Point3D const &CylinderOrigin=pCylinder->m_Origin;
    SGM::Point3D ProjectedCenter=Center+TorusAxis*(TorusAxis%(CylinderOrigin-Center));
    double dAngle=0<(CylinderAxis%TorusAxis) ? CylinderAxis.Angle(TorusAxis) : CylinderAxis.Angle(-TorusAxis);
    double dVillarceauAngle=SGM::SAFEasin(pTorus->m_dMinorRadius/pTorus->m_dMajorRadius);
    SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN, SGM_INTERVAL_POS_MAX);
    
    // Check for Villarceau circles.

    bool bFoundAnswer=false;
    if(SGM::NearEqual(dAngle,dVillarceauAngle,dTolerance,true))
        {
        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;
        if(IntersectLineAndPlane(CylinderOrigin,CylinderAxis,Domain,Center,TorusAxis,dTolerance,aPoints,aTypes))
            {
            SGM::Point3D VCenter=aPoints[0];
            if( SGM::NearEqual(VCenter.Distance(Center),dMajor-dMinor,dTolerance,false) &&
                SGM::NearEqual(CylinderAxis%(VCenter-Center),0.0,dTolerance,false))
                {
                aCurves.push_back(new circle(rResult,VCenter,CylinderAxis,dMajor,&XAxis));
                SGM::UnitVector3D Vec0=VCenter-Center;
                SGM::Vector3D Vec1=Vec0*dMajor;
                SGM::Point3D Pos1=VCenter+Vec1;
                SGM::Point3D Pos2=VCenter-Vec1;
                SGM::UnitVector3D Vec2=Vec1*CylinderAxis;
                SGM::Vector3D Vec3=Vec2*dMajor;
                SGM::Point3D Pos3=VCenter+Vec3;
                SGM::Point3D Pos4=VCenter-Vec3;

                aPoints.clear();
                aTypes.clear();
                IntersectLineAndTorus(Pos3,CylinderAxis,Domain,pTorus,dTolerance,aPoints,aTypes);
                Pos3=VCenter.Distance(aPoints[0])<VCenter.Distance(aPoints[1]) ? aPoints[1] : aPoints[0];
                aPoints.clear();
                aTypes.clear();
                IntersectLineAndTorus(Pos4,CylinderAxis,Domain,pTorus,dTolerance,aPoints,aTypes);
                Pos4=VCenter.Distance(aPoints[0])<VCenter.Distance(aPoints[1]) ? aPoints[1] : aPoints[0];

                aPoints.clear();
                aPoints.push_back(Pos1);
                aPoints.push_back(Pos2);
                aCurves.push_back(WalkFromTo(rResult,Pos3,aPoints,pTorus,pCylinder));
                aCurves.push_back(WalkFromTo(rResult,Pos4,aPoints,pTorus,pCylinder));

                return 3;
                }
            }
        }

    if( dAngle<dTolerance && 
        CylinderOrigin.Distance(ProjectedCenter)<dTolerance)
        {
        if(SGM::NearEqual(dMajor+dMinor,dCylinderRadius,dTolerance,false))
            {
            // One outside circle.

            bFoundAnswer=true;
            aCurves.push_back(new circle(rResult,Center,TorusAxis,dCylinderRadius,&XAxis));
            }
        else if(SGM::NearEqual(dMajor-dMinor,dCylinderRadius,dTolerance,false))
            {
            // One inside circle.

            bFoundAnswer=true;
            aCurves.push_back(new circle(rResult,Center,TorusAxis,dCylinderRadius,&XAxis));
            }
        else if(dCylinderRadius<dMajor+dMinor && dMajor-dMinor<dCylinderRadius)
            {
            // Two circle.

            bFoundAnswer=true;
            double dOffset=sqrt(1.0-(dMajor-dCylinderRadius)*(dMajor-dCylinderRadius));
            aCurves.push_back(new circle(rResult,Center+TorusAxis*dOffset,TorusAxis,dCylinderRadius,&XAxis));
            aCurves.push_back(new circle(rResult,Center-TorusAxis*dOffset,TorusAxis,dCylinderRadius,&XAxis));
            }
        }
    else if(SGM_HALF_PI<dAngle+dTolerance)
        {
        double dDist=Center.Distance(ProjectedCenter);
        if(SGM::NearEqual(dMinor+pCylinder->m_dRadius,dDist,dTolerance,false))
            {
            // Zero, One or Two tangent point(s) on the top or bottom of the torus.

            bFoundAnswer=true;
            SGM::Vector3D OffsetVec= 0<(CylinderOrigin-Center)%TorusAxis ? TorusAxis*(-dCylinderRadius) : TorusAxis*dCylinderRadius;
            double dOffset=(ProjectedCenter-Center)%TorusAxis<0 ? -dMinor : dMinor;
            SGM::Point3D TanCenter=Center+TorusAxis*dOffset;
            std::vector<SGM::Point3D> aPoints;
            std::vector<SGM::IntersectionType> aTypes;
            IntersectLineAndCircle(CylinderOrigin+OffsetVec,CylinderAxis,Domain,TanCenter,TorusAxis,dMajor,dTolerance,aPoints,aTypes);
            for(SGM::Point3D Pos : aPoints)
                {
                aCurves.push_back(new PointCurve(rResult,Pos));
                }
            }
        else if(dDist<dTolerance)
            {
            // Cylinder axis lies in the plane of the torus.

            if(SGM::NearEqual(pCylinder->m_dRadius,dMinor,dTolerance,false))
                {
                bFoundAnswer=true;
                SGM::Point3D Pos=CylinderOrigin+CylinderAxis*(CylinderAxis%(Center-CylinderOrigin));
                if(SGM::NearEqual(dMajor,Pos.Distance(Center),dTolerance,false))
                    {
                    // Minor Circle Plus Case

                    SGM::UnitVector3D CP=Center-Pos;
                    SGM::Point3D Pos1=Pos+CP*dMinor;
                    SGM::Point3D Pos2=Pos+TorusAxis*dMinor;
                    SGM::Point3D Pos3=Pos-TorusAxis*dMinor;
                    aCurves.push_back(new circle(rResult,Pos,CylinderAxis,dCylinderRadius,&TorusAxis));

                    SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN, SGM_INTERVAL_POS_MAX);
                    std::vector<SGM::Point3D> aPoints;
                    std::vector<SGM::IntersectionType> aTypes;
                    IntersectLineAndTorus(Pos1,CylinderAxis,Domain,pTorus,dTolerance,aPoints,aTypes);

                    std::vector<SGM::Point3D> aEndPoints;
                    aEndPoints.push_back(Pos2);
                    aEndPoints.push_back(Pos3);
                    aCurves.push_back(WalkFromTo(rResult,aPoints.front(),aEndPoints,pTorus,pCylinder));
                    aCurves.push_back(WalkFromTo(rResult,aPoints.back(),aEndPoints,pTorus,pCylinder));
                    }
                else
                    {
                    // Look for tangent points which lie on four posible lines.

                    SGM::Interval1D Domain(SGM_INTERVAL_NEG_MIN, SGM_INTERVAL_POS_MAX);
                    std::vector<SGM::Point3D> aPoints1,aPoints2,aPoints3,aPoints4;
                    std::vector<SGM::IntersectionType> aTypes1,aTypes2,aTypes3,aTypes4;
                    IntersectLineAndTorus(CylinderOrigin+TorusAxis*dCylinderRadius,CylinderAxis,Domain,pTorus,dTolerance,aPoints1,aTypes1);
                    IntersectLineAndTorus(CylinderOrigin-TorusAxis*dCylinderRadius,CylinderAxis,Domain,pTorus,dTolerance,aPoints2,aTypes2);
                    SGM::UnitVector3D UVec=CylinderAxis*TorusAxis;
                    IntersectLineAndTorus(CylinderOrigin+UVec*dCylinderRadius,CylinderAxis,Domain,pTorus,dTolerance,aPoints3,aTypes3);
                    IntersectLineAndTorus(CylinderOrigin-UVec*dCylinderRadius,CylinderAxis,Domain,pTorus,dTolerance,aPoints4,aTypes4);

                    std::vector<SGM::Point3D> aTangents,aWalkFrom;
                    for(auto Pos : aPoints1)
                        {
                        aTangents.push_back(Pos);
                        }
                    for(auto Pos : aPoints2)
                        {
                        aTangents.push_back(Pos);
                        }
                    size_t Index1;
                    size_t nPoints3=aPoints3.size();
                    for(Index1=0;Index1<nPoints3;++Index1)
                        {
                        if(aTypes3[Index1]==SGM::IntersectionType::TangentType)
                            {
                            aTangents.push_back(aPoints3[Index1]);
                            }
                        else
                            {
                            aWalkFrom.push_back(aPoints3[Index1]);
                            }
                        }
                    size_t nPoints4=aPoints4.size();
                    for(Index1=0;Index1<nPoints4;++Index1)
                        {
                        if(aTypes4[Index1]==SGM::IntersectionType::TangentType)
                            {
                            aTangents.push_back(aPoints4[Index1]);
                            }
                        else
                            {
                            aWalkFrom.push_back(aPoints4[Index1]);
                            }
                        }
                    for(auto Pos : aWalkFrom)
                        {
                        aCurves.push_back(WalkFromTo(rResult,Pos,aTangents,pCylinder,pTorus));
                        }

                    // Look for additional walking points that lie on four posible lines.

                    std::vector<SGM::Point3D> aPoints5,aPoints6,aPoints7,aPoints8;
                    std::vector<SGM::IntersectionType> aTypes5,aTypes6,aTypes7,aTypes8;
                    SGM::UnitVector3D Vec1=TorusAxis+UVec;
                    SGM::UnitVector3D Vec2=TorusAxis-UVec;
                    IntersectLineAndTorus(CylinderOrigin+Vec1*dCylinderRadius,CylinderAxis,Domain,pTorus,dTolerance,aPoints5,aTypes5);
                    IntersectLineAndTorus(CylinderOrigin-Vec1*dCylinderRadius,CylinderAxis,Domain,pTorus,dTolerance,aPoints6,aTypes6);
                    IntersectLineAndTorus(CylinderOrigin+Vec2*dCylinderRadius,CylinderAxis,Domain,pTorus,dTolerance,aPoints7,aTypes7);
                    IntersectLineAndTorus(CylinderOrigin-Vec2*dCylinderRadius,CylinderAxis,Domain,pTorus,dTolerance,aPoints8,aTypes8);
                    aWalkFrom.clear();
                    for(auto Pos : aPoints5)
                        {
                        if(PointOnCurves(Pos,aCurves)==false)
                            {
                            aWalkFrom.push_back(Pos);
                            }
                        }
                    for(auto Pos : aPoints6)
                        {
                        if(PointOnCurves(Pos,aCurves)==false)
                            {
                            aWalkFrom.push_back(Pos);
                            }
                        }
                    for(auto Pos : aPoints7)
                        {
                        if(PointOnCurves(Pos,aCurves)==false)
                            {
                            aWalkFrom.push_back(Pos);
                            }
                        }
                    for(auto Pos : aPoints8)
                        {
                        if(PointOnCurves(Pos,aCurves)==false)
                            {
                            aWalkFrom.push_back(Pos);
                            }
                        }

                    for(auto Pos : aWalkFrom)
                        {
                        aCurves.push_back(WalkFromTo(rResult,Pos,aTangents,pCylinder,pTorus));
                        }
                    }
                }
            }
        }
   
    if(bFoundAnswer==false)
        {
        // Check for tangent points

        std::vector<SGM::Point3D> aPoints1,aPoints2;
        std::vector<SGM::IntersectionType> aTypes1,aTypes2;
        IntersectLineAndTorus(CylinderOrigin,CylinderAxis,Domain,Center,
                              pTorus->m_XAxis,pTorus->m_YAxis,TorusAxis,
                              dMinor+dCylinderRadius,dMajor,dTolerance,
                              aPoints1,aTypes1);
        IntersectLineAndTorus(CylinderOrigin,CylinderAxis,Domain,Center,
                              pTorus->m_XAxis,pTorus->m_YAxis,TorusAxis,
                              dMinor-dCylinderRadius,dMajor,dTolerance,
                              aPoints2,aTypes2);
        std::vector<SGM::Point3D> aMayBeTangent,aTangents;
        for(SGM::Point3D const &Pos : aPoints1)
            {
            SGM::Point3D Snap;
            pTorus->Inverse(Pos,&Snap);
            aMayBeTangent.push_back(Snap);
            }
        for(SGM::Point3D const &Pos : aPoints2)
            {
            SGM::Point3D Snap;
            pTorus->Inverse(Pos,&Snap);
            aMayBeTangent.push_back(Snap);
            }
        SGM::RemoveDuplicates3D(aMayBeTangent,SGM_FIT);
        for(SGM::Point3D const &Pos : aMayBeTangent)
            {
            SGM::UnitVector3D Norm1,Norm2;
            pCylinder->Evaluate(pCylinder->Inverse(Pos),nullptr,nullptr,nullptr,&Norm1);
            pTorus->Evaluate(pTorus->Inverse(Pos),nullptr,nullptr,nullptr,&Norm2);
            if(SGM::NearEqual(fabs(Norm1%Norm2),1,dTolerance,false))
                {
                aTangents.push_back(Pos);
                }
            }

        // Find Walk Points and walk from them.

        std::vector<SGM::Point3D> aWalkPoints;
        FindWalkPoints(pTorus,pCylinder,aTangents,aWalkPoints);

        for(SGM::Point3D const &Pos : aWalkPoints)
            {
            if(PointOnCurves(Pos,aCurves)==false)
                {
                std::vector<SGM::Point3D> aEndPoints;
                aEndPoints=aTangents;
                aEndPoints.push_back(Pos);
                aCurves.push_back(WalkFromTo(rResult,Pos,aEndPoints,pCylinder,pTorus));
                }
            }
        
        // Add in isolated tangent points.
        
        for(SGM::Point3D const &Pos : aTangents)
            {
            if(PointOnCurves(Pos,aCurves)==false)
                {
                aCurves.push_back(new PointCurve(rResult,Pos));
                }
            }
        }
    return aCurves.size();
    }

bool PointOnCircle(SGM::Point3D      const &Pos,
                   SGM::Point3D      const &Center,
                   double                   dRadius,
                   SGM::UnitVector3D const &Normal,
                   double                   dTolerance)
    {
    return fabs((Pos-Center)%Normal)<dTolerance && SGM::NearEqual(dRadius,Pos.Distance(Center),dTolerance,false);
    }

size_t IntersectSphereAndTorus(SGM::Result                &rResult,
                               sphere               const *pSphere,
                               torus                const *pTorus,
                               std::vector<curve *>       &aCurves,
                               face                 const *,//pFace1,
                               face                 const *,//pFace2,
                               double                      dTolerance)
    {
    SGM::Point3D AxisPoint=pTorus->m_Center+pTorus->m_ZAxis*(pTorus->m_ZAxis%(pSphere->m_Center-pTorus->m_Center));
    SGM::Point3D ClosePos;
    pTorus->Inverse(pSphere->m_Center,&ClosePos);
    if(SGM::NearEqual(pSphere->m_Center,pTorus->m_Center,dTolerance))
        {
        // Sphere Center is the Torus Center

        if( SGM::NearEqual(pSphere->m_dRadius,pTorus->m_dMajorRadius-pTorus->m_dMinorRadius,dTolerance,false) ||
            SGM::NearEqual(pSphere->m_dRadius,pTorus->m_dMajorRadius+pTorus->m_dMinorRadius,dTolerance,false))
            {
            aCurves.push_back(new circle(rResult,pSphere->m_Center,pTorus->m_ZAxis,pSphere->m_dRadius,&pTorus->m_XAxis));
            }
        else if( pTorus->m_dMajorRadius-pTorus->m_dMinorRadius<pSphere->m_dRadius &&
                 pSphere->m_dRadius<pTorus->m_dMajorRadius+pTorus->m_dMinorRadius )
            {
            std::vector<SGM::Point3D> aPoints;
            std::vector<SGM::IntersectionType> aTypes;
            IntersectCircleAndCircle(pTorus->m_Center,pTorus->m_Center+pTorus->m_XAxis*pTorus->m_dMajorRadius,
                                     pTorus->m_YAxis,pTorus->m_YAxis,pSphere->m_dRadius,pTorus->m_dMinorRadius,
                                     aPoints,aTypes,dTolerance);
            SGM::Point3D Center1=pTorus->m_Center+pTorus->m_ZAxis*(pTorus->m_ZAxis%(aPoints[0]-pTorus->m_Center));
            SGM::Point3D Center2=pTorus->m_Center+pTorus->m_ZAxis*(pTorus->m_ZAxis%(aPoints[1]-pTorus->m_Center));
            double dRadius=Center1.Distance(aPoints[0]);
            aCurves.push_back(new circle(rResult,Center1,pTorus->m_ZAxis,dRadius,&pTorus->m_XAxis));
            aCurves.push_back(new circle(rResult,Center2,pTorus->m_ZAxis,dRadius,&pTorus->m_XAxis));
            }
        }
    else if(SGM::NearEqual(pSphere->m_Center,AxisPoint,dTolerance))
        {
        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;
        IntersectCircleAndCircle(AxisPoint,pTorus->m_Center+pTorus->m_XAxis*pTorus->m_dMajorRadius,
                                 pTorus->m_YAxis,pTorus->m_YAxis,pSphere->m_dRadius,pTorus->m_dMinorRadius,
                                 aPoints,aTypes,dTolerance);
        for(SGM::Point3D const &Pos : aPoints)
            {
            SGM::Point3D Center=pTorus->m_Center+pTorus->m_ZAxis*(pTorus->m_ZAxis%(Pos-pTorus->m_Center));
            double dRadius=Center.Distance(Pos);
            aCurves.push_back(new circle(rResult,Center,pTorus->m_ZAxis,dRadius,&pTorus->m_XAxis));
            }
        }
    else if(SGM::NearEqual(pSphere->m_Center.Distance(ClosePos),pSphere->m_dRadius,dTolerance,false))
        {
        // Check for a single circle

        if( SGM::NearEqual(pTorus->m_dMinorRadius,pSphere->m_dRadius,dTolerance,false) && 
            PointOnCircle(pSphere->m_Center,pTorus->m_Center,pTorus->m_dMajorRadius,pTorus->m_ZAxis,dTolerance))
            {
            SGM::UnitVector3D Spoke=pSphere->m_Center-pTorus->m_Center;
            SGM::UnitVector3D Norm=Spoke*pTorus->m_ZAxis;
            aCurves.push_back(new circle(rResult,pSphere->m_Center,Norm,pSphere->m_dRadius,&Spoke));
            }
        else
            {
            aCurves.push_back(new PointCurve(rResult,ClosePos));
            }
        }
    else if(fabs(SGM::UnitVector3D(pSphere->m_Center-pTorus->m_Center)%pTorus->m_ZAxis)<dTolerance)
        {
        // Sphere center is in the plane of the torus.

        if( SGM::NearEqual(pSphere->m_Center.Distance(pTorus->m_Center),
                           pTorus->m_dMajorRadius-pTorus->m_dMinorRadius,
                           dTolerance,false) &&
            fabs(pSphere->m_dRadius-pTorus->m_dMajorRadius)<dTolerance)
            {
            // Villarceau Circles

            SGM::UnitVector3D Spoke=pSphere->m_Center-pTorus->m_Center;
            SGM::UnitVector3D UVec=Spoke*pTorus->m_ZAxis;
            double dSin=pTorus->m_dMinorRadius/pTorus->m_dMajorRadius;
            double dCos=sqrt(1-dSin*dSin);
            SGM::UnitVector3D Norm1=dSin*UVec+dCos*pTorus->m_ZAxis;
            SGM::UnitVector3D Norm2=-dSin*UVec+dCos*pTorus->m_ZAxis;
            aCurves.push_back(new circle(rResult,pSphere->m_Center,Norm1,pSphere->m_dRadius,&Spoke));
            aCurves.push_back(new circle(rResult,pSphere->m_Center,Norm2,pSphere->m_dRadius,&Spoke));
            }
        else
            {
            // Check of minor circles.

            // Find the tangent point(s) of the core circle of the torus
            // as viewed from the center of the sphere.

            double dDist=pSphere->m_Center.Distance(pTorus->m_Center);
            double dB=SGM_HALF_PI-SGM::SAFEasin(pTorus->m_dMajorRadius/dDist);
            double dCos=cos(dB)*pTorus->m_dMajorRadius;
            double dSin=sin(dB)*pTorus->m_dMajorRadius;
            SGM::UnitVector3D Spoke=pSphere->m_Center-pTorus->m_Center;
            SGM::UnitVector3D UVec=pTorus->m_ZAxis*Spoke;
            SGM::Point3D Pos1=pTorus->m_Center+dCos*Spoke+dSin*UVec;
            SGM::Point3D Pos2=pTorus->m_Center+dCos*Spoke-dSin*UVec;
            SGM::Point3D Pos1A=Pos1+SGM::UnitVector3D(Pos1-pTorus->m_Center)*pTorus->m_dMinorRadius;
            SGM::Point3D Pos1B=Pos1-SGM::UnitVector3D(Pos1-pTorus->m_Center)*pTorus->m_dMinorRadius;
            double dDist1A=Pos1A.Distance(pSphere->m_Center);
            double dDist1B=Pos1B.Distance(pSphere->m_Center);
            if( SGM::NearEqual(dDist1A,pSphere->m_dRadius,dTolerance,false) && 
                SGM::NearEqual(dDist1B,pSphere->m_dRadius,dTolerance,false))
                {
                SGM::UnitVector3D XAxis1=Pos1-pTorus->m_Center;
                SGM::UnitVector3D Norm1=pTorus->m_ZAxis*XAxis1;
                aCurves.push_back(new circle(rResult,Pos1,Norm1,pTorus->m_dMinorRadius,&XAxis1));
                SGM::UnitVector3D XAxis2=Pos2-pTorus->m_Center;
                SGM::UnitVector3D Norm2=pTorus->m_ZAxis*XAxis2;
                aCurves.push_back(new circle(rResult,Pos2,Norm2,pTorus->m_dMinorRadius,&XAxis2));
                }
            }
        }

    // The General Case.
            
    if(aCurves.empty())
        {
        // First look for tangent points.

        std::vector<SGM::Point3D> aPoints1,aPoints2,aPoints3,aTangents;
        std::vector<SGM::IntersectionType> aTypes1,aTypes2,aTypes3;
        if(pTorus->m_dMinorRadius<pSphere->m_dRadius)
            {
            size_t nHits=IntersectCircleAndSphere(pTorus->m_Center,pTorus->m_ZAxis,pTorus->m_dMajorRadius,
                                                  pSphere->m_Center,pSphere->m_dRadius-pTorus->m_dMinorRadius,
                                                  dTolerance,aPoints3,aTypes3);
            size_t Index1;
            for(Index1=0;Index1<nHits;++Index1)
                {
                if(aTypes3[Index1]==SGM::IntersectionType::TangentType)
                    {
                    SGM::Point3D Pos;
                    pSphere->Inverse(aPoints3[Index1],&Pos);
                    aTangents.push_back(Pos);
                    }
                }
            aPoints3.clear();
            aTypes3.clear();
            nHits=IntersectCircleAndSphere(pTorus->m_Center,pTorus->m_ZAxis,pTorus->m_dMajorRadius,
                                           pSphere->m_Center,pSphere->m_dRadius+pTorus->m_dMinorRadius,
                                           dTolerance,aPoints3,aTypes3);
            for(Index1=0;Index1<nHits;++Index1)
                {
                if(aTypes3[Index1]==SGM::IntersectionType::TangentType)
                    {
                    SGM::Point3D Pos;
                    pSphere->Inverse(aPoints3[Index1],&Pos);
                    aTangents.push_back(Pos);
                    }
                }
            }

        // Find walking points from the major and minor circle throught the point
        // on the torus that is closest to the center of the sphere.

        SGM::Point3D Pos;
        pTorus->Inverse(pSphere->m_Center,&Pos);
        SGM::Point3D AxisPos=pTorus->m_Center+pTorus->m_ZAxis*(pTorus->m_ZAxis%(Pos-pTorus->m_Center));
        double dRadius=AxisPos.Distance(Pos);
        IntersectCircleAndSphere(AxisPos,pTorus->m_ZAxis,dRadius,pSphere,dTolerance,aPoints1,aTypes1);
        SGM::Vector3D Vec=Pos-pTorus->m_Center;
        double dX=Vec%pTorus->m_XAxis;
        double dY=Vec%pTorus->m_YAxis;
        SGM::Point3D SpokePos=pTorus->m_Center+dX*pTorus->m_XAxis+dY*pTorus->m_YAxis;
        SGM::Point3D CirclePos=pTorus->m_Center+SGM::UnitVector3D(SpokePos-pTorus->m_Center)*pTorus->m_dMajorRadius;
        SGM::UnitVector3D Norm=(CirclePos-pTorus->m_Center)*pTorus->m_ZAxis;
        IntersectCircleAndSphere(CirclePos,Norm,pTorus->m_dMinorRadius,pSphere,dTolerance,aPoints2,aTypes2);
        aPoints3.clear();
        size_t nWhere;
        for(auto Hit : aPoints1)
            {
            if(dTolerance<SGM::DistanceToPoints(aTangents,Hit,nWhere))
                {
                aPoints3.push_back(Hit);
                }
            }
        for(auto Hit : aPoints2)
            {
            if(dTolerance<SGM::DistanceToPoints(aTangents,Hit,nWhere))
                {
                aPoints3.push_back(Hit);
                }
            }

        // If aTangents is of size_t, then extra minor circles between the two 
        // tangent points need to be used to find the all the walking points.

        if(aTangents.size()==2)
            {
            double v1=pTorus->Inverse(aTangents[0]).m_v;
            double v2=pTorus->Inverse(aTangents[1]).m_v;
            double v3=(v1+v2)/2;
            double dCos=cos(v3);
            double dSin=sin(v3);
            SGM::Vector3D XVec=pTorus->m_XAxis*(pTorus->m_dMajorRadius*dCos);
            SGM::Vector3D YVec=pTorus->m_YAxis*(pTorus->m_dMajorRadius*dSin);
            SGM::Point3D Center1=pTorus->m_Center+XVec+YVec;
            SGM::Point3D Center2=pTorus->m_Center-XVec-YVec;
            SGM::UnitVector3D Norm1=(Center1-pTorus->m_Center)*pTorus->m_ZAxis;
            SGM::UnitVector3D Norm2=(Center2-pTorus->m_Center)*pTorus->m_ZAxis;
            aPoints1.clear();
            aTypes1.clear();
            aPoints2.clear();
            aTypes2.clear();
            IntersectCircleAndSphere(Center1,Norm1,pTorus->m_dMinorRadius,pSphere,dTolerance,aPoints1,aTypes1);
            IntersectCircleAndSphere(Center2,Norm2,pTorus->m_dMinorRadius,pSphere,dTolerance,aPoints2,aTypes2);
            for(auto Hit : aPoints1)
                {
                aPoints3.push_back(Hit);
                }
            for(auto Hit : aPoints2)
                {
                aPoints3.push_back(Hit);
                }
            }

        for(auto Hit : aPoints3)
            {
            if(PointOnCurves(Hit,aCurves)==false)
                {
                std::vector<SGM::Point3D> aEndPoints;
                if(aTangents.size())
                    {
                    aEndPoints=aTangents;
                    }
                else
                    {
                    aEndPoints.push_back(Hit);
                    }
                aCurves.push_back(WalkFromTo(rResult,Hit,aEndPoints,pSphere,pTorus));
                }
            }
        }
    return aCurves.size();
    }

size_t IntersectSphereAndCylinder(SGM::Result                &rResult,
                                  sphere               const *pSphere,
                                  cylinder             const *pCylinder,
                                  std::vector<curve *>       &aCurves,
                                  face                 const *,//pFace1,
                                  face                 const *,//pFace2,
                                  double                      dTolerance)
    {
    double dSphereRadius=pSphere->m_dRadius;
    double dCylinderRadius=pCylinder->m_dRadius;
    SGM::Point3D const &Center=pSphere->m_Center;
    SGM::Point3D const &Origin=pCylinder->m_Origin;
    SGM::UnitVector3D const &Axis=pCylinder->m_ZAxis;
    SGM::Point3D AxisPos=Origin+Axis*((Center-Origin)%Axis);
    double dDist=Center.Distance(AxisPos);
    if(SGM::NearEqual(AxisPos,Center,dTolerance))
        {
        // Center on Axis
    
        SGM::UnitVector3D const &XAxis=pCylinder->m_XAxis;
        if(SGM::NearEqual(dSphereRadius,dCylinderRadius,dTolerance,false))
            {
            // One circle.
            aCurves.push_back(new circle(rResult,Center,Axis,dSphereRadius,&XAxis));
            }
        else if(dCylinderRadius<dSphereRadius)
            {
            // Two circles.
            double dOffset=sqrt(dSphereRadius*dSphereRadius-dCylinderRadius*dCylinderRadius);
            SGM::Vector3D Offset=Axis*dOffset;
            aCurves.push_back(new circle(rResult,Center+Offset,Axis,dCylinderRadius,&XAxis));
            aCurves.push_back(new circle(rResult,Center-Offset,Axis,dCylinderRadius,&XAxis));
            }
        }
    else if(SGM::NearEqual(dDist,dCylinderRadius+dSphereRadius,dTolerance,false))
        {
        // Sphere outside cylinder and tangent.
        SGM::Point3D Pos=SGM::MidPoint(AxisPos,Center,dCylinderRadius/dDist);
        aCurves.push_back(new PointCurve(rResult,Pos));
        }
    else if( dSphereRadius<dCylinderRadius &&
        SGM::NearEqual(dDist,dCylinderRadius-dSphereRadius,dTolerance,false))
        {
        // Smaller radius sphere is inside cylinder and tangent.
        SGM::UnitVector3D UVec=Center-AxisPos;
        SGM::Point3D Pos=AxisPos+UVec*dCylinderRadius;
        aCurves.push_back(new PointCurve(rResult,Pos));
        }
    else if(dDist<dSphereRadius)
        {
        // Cylinder axis intersects sphere.

        SGM::Point3D LineOrigin;
        pCylinder->Inverse(Center,&LineOrigin);
        SGM::UnitVector3D Normal=Axis*(Center-AxisPos);
        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;
        IntersectLineAndCircle(LineOrigin,Axis,pCylinder->GetDomain().m_VDomain,
                               Center,Normal,dSphereRadius,dTolerance,aPoints,aTypes);

        if(SGM::NearEqual(dDist+dCylinderRadius,dSphereRadius,dTolerance,false))
            {
            // Larger radius sphere is tangent and spanning cylinder,
            // forming a bent figure eight intersection.

            SGM::UnitVector3D UVec=AxisPos-Center;
            SGM::Point3D TanPos=AxisPos+UVec*dCylinderRadius;
            std::vector<SGM::Point3D> aEndPoints;
            aEndPoints.push_back(TanPos);
            aCurves.push_back(WalkFromTo(rResult,aPoints[0],aEndPoints,pSphere,pCylinder));
            aCurves.push_back(WalkFromTo(rResult,aPoints[1],aEndPoints,pSphere,pCylinder));
            }
        else if(dDist+dCylinderRadius<dSphereRadius)
            {
            // Larger radius sphere is intected with whole section of cylinder,
            // forming two bent closed curves.

            std::vector<SGM::Point3D> aEndPoints0;
            aEndPoints0.push_back(aPoints[0]);
            aCurves.push_back(WalkFromTo(rResult,aPoints[0],aEndPoints0,pSphere,pCylinder));
            std::vector<SGM::Point3D> aEndPoints1;
            aEndPoints1.push_back(aPoints[1]);
            aCurves.push_back(WalkFromTo(rResult,aPoints[1],aEndPoints1,pSphere,pCylinder));
            }
        }
    else 
        {
        // If the closest point on the cylinder to the sphere center is
        // inside the sphere then the sphere intersects but does not cut
        // the cylinder into two parts, resuling in a bent closed curve.

        SGM::Point3D CPos;
        pCylinder->Inverse(Center,&CPos);
        double dDist2=Center.Distance(CPos);
        if(dDist2<dSphereRadius)
            {
            // One closed curve.

            SGM::Point3D LineOrigin;
            pCylinder->Inverse(Center,&LineOrigin);
            SGM::UnitVector3D Normal=Axis*(Center-AxisPos);
            std::vector<SGM::Point3D> aPoints;
            std::vector<SGM::IntersectionType> aTypes;
            IntersectLineAndCircle(LineOrigin,Axis,pCylinder->GetDomain().m_VDomain,
                                   Center,Normal,dSphereRadius,dTolerance,aPoints,aTypes);
            std::vector<SGM::Point3D> aEndPoints;
            aEndPoints.push_back(aPoints[0]);
            aCurves.push_back(WalkFromTo(rResult,aPoints[0],aEndPoints,pSphere,pCylinder));
            }
        }
    return aCurves.size();
    }

size_t IntersectSphereAndSurface(SGM::Result                &rResult,
                                 sphere               const *pSphere,
                                 surface              const *pSurface,
                                 std::vector<curve *>       &aCurves,
                                 face                 const *pFace1,
                                 face                 const *pFace2,
                                 double                      dTolerance)
    {
    switch(pSurface->GetSurfaceType())
        {
        case SGM::EntityType::PlaneType:
            {
            auto pPlane=(plane const *)pSurface;
            return IntersectPlaneAndSphere(rResult,pPlane,pSphere,aCurves,pFace2,pFace1,dTolerance);
            }
        case SGM::EntityType::SphereType:
            {
            auto pSphere2=(sphere const *)pSurface;
            return IntersectSphereAndSphere(rResult,pSphere,pSphere2,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::CylinderType:
            {
            auto pCylinder=(cylinder const *)pSurface;
            return IntersectSphereAndCylinder(rResult,pSphere,pCylinder,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::TorusType:
            {
            auto pTorus=(torus const *)pSurface;
            return IntersectSphereAndTorus(rResult,pSphere,pTorus,aCurves,pFace1,pFace2,dTolerance);
            }
        default:
            {
            return IntersectSphereAndSurface(rResult,pSphere,pSurface,aCurves,pFace1,pFace2,dTolerance);
            }
        }
    }

size_t IntersectCylinderAndSurface(SGM::Result                &rResult,
                                   cylinder             const *pCylinder,
                                   surface              const *pSurface,
                                   std::vector<curve *>       &aCurves,
                                   face                 const *pFace1,
                                   face                 const *pFace2,
                                   double                      dTolerance)
    {
    switch(pSurface->GetSurfaceType())
        {
        case SGM::EntityType::SphereType:
            {
            auto pSphere=(sphere const *)pSurface;
            return IntersectSphereAndCylinder(rResult,pSphere,pCylinder,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::CylinderType:
            {
            auto pCylinder2=(cylinder const *)pSurface;
            return IntersectCylinders(rResult,pCylinder,pCylinder2,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::ConeType:
            {
            auto pCone=(cone const *)pSurface;
            return IntersectConeAndCylinder(rResult,pCone,pCylinder,aCurves,pFace1,pFace2,dTolerance);
            }
        default:
            {
            throw;
            }
        }
    }

size_t IntersectConeAndSurface(SGM::Result                &rResult,
                               cone                 const *pCone,
                               surface              const *pSurface,
                               std::vector<curve *>       &aCurves,
                               face                 const *pFace1,
                               face                 const *pFace2,
                               double                      dTolerance)
    {
    switch(pSurface->GetSurfaceType())
        {
        case SGM::EntityType::SphereType:
            {
            auto pSphere=(sphere const *)pSurface;
            return IntersectSphereAndCone(rResult,pSphere,pCone,aCurves,pFace2,pFace1,dTolerance);
            }
        case SGM::EntityType::CylinderType:
            {
            auto pCylinder=(cylinder const *)pSurface;
            return IntersectConeAndCylinder(rResult,pCone,pCylinder,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::PlaneType:
            {
            auto pPlane=(plane const *)pSurface;
            return IntersectPlaneAndCone(rResult,pPlane,pCone,aCurves,pFace2,pFace1,dTolerance);
            }
        case SGM::EntityType::ConeType:
            {
            auto pCone2=(cone const *)pSurface;
            return IntersectConeAndCone(rResult,pCone,pCone2,aCurves,pFace1,pFace2,dTolerance);
            }
        default:
            {
            throw;
            }
        }
    }

size_t IntersectTorusAndSurface(SGM::Result                &rResult,
                                torus                const *pTorus,
                                surface              const *pSurface,
                                std::vector<curve *>       &aCurves,
                                face                 const *pFace1,
                                face                 const *pFace2,
                                double                      dTolerance)
    {
    switch(pSurface->GetSurfaceType())
        {
        case SGM::EntityType::CylinderType:
            {
            auto pCylinder=(cylinder const *)pSurface;
            return IntersectTorusAndCylinder(rResult,pTorus,pCylinder,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::PlaneType:
            {
            auto pPlane=(plane const *)pSurface;
            return IntersectPlaneAndTorus(rResult,pPlane,pTorus,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::SphereType:
            {
            auto pSphere=(sphere const *)pSurface;
            return IntersectSphereAndTorus(rResult,pSphere,pTorus,aCurves,pFace1,pFace2,dTolerance);
            }
        default:
            {
            throw;
            }
        }
    }

size_t IntersectSurfaces(SGM::Result                &rResult,
                         surface              const *pSurface1,
                         surface              const *pSurface2,
                         std::vector<curve *>       &aCurves,
                         face                 const *pFace1,
                         face                 const *pFace2,
                         double                      dTolerance)
    {
    switch(pSurface1->GetSurfaceType())
        {
        case SGM::EntityType::PlaneType:
            {
            auto pPlane=(plane const *)pSurface1;
            return IntersectPlaneAndSurface(rResult,pPlane,pSurface2,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::SphereType:
            {
            auto pSphere=(sphere const *)pSurface1;
            return IntersectSphereAndSurface(rResult,pSphere,pSurface2,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::CylinderType:
            {
            auto pCylinder=(cylinder const *)pSurface1;
            return IntersectCylinderAndSurface(rResult,pCylinder,pSurface2,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::ConeType:
            {
            auto pCone=(cone const *)pSurface1;
            return IntersectConeAndSurface(rResult,pCone,pSurface2,aCurves,pFace1,pFace2,dTolerance);
            }
        case SGM::EntityType::TorusType:
            {
            auto pTorus=(torus const *)pSurface1;
            return IntersectTorusAndSurface(rResult,pTorus,pSurface2,aCurves,pFace1,pFace2,dTolerance);
            }
        default:
            {
            throw;
            break;
            }
        }
    }

SGM::Point3D ZoomInFrom(SGM::Point3D const &Pos,
                        surface      const *pSurface1,
                        surface      const *pSurface2)
    {
    SGM::Point3D Answer=Pos;
    double dDist=SGM_MAX;
    int nCount=0;
    while(SGM_ZERO<dDist && nCount<100)
        {
        SGM::Point3D OldAnswer=Answer;
        SGM::Point2D uv1=pSurface1->Inverse(Answer);
        SGM::Point2D uv2=pSurface2->Inverse(Answer);
        SGM::UnitVector3D Norm1,Norm2;
        SGM::Point3D Pos1,Pos2;
        pSurface1->Evaluate(uv1,&Pos1,nullptr,nullptr,&Norm1);
        pSurface2->Evaluate(uv2,&Pos2,nullptr,nullptr,&Norm2);
        SGM::Point3D Origin;
        SGM::UnitVector3D Axis;
        IntersectNonParallelPlanes(Pos1,Norm1,Pos2,Norm2,Origin,Axis);
        Pos1=Origin+Axis*((Pos1-Origin)%Axis);
        Pos2=Origin+Axis*((Pos2-Origin)%Axis);
        Answer=SGM::MidPoint(Pos1,Pos2);
        dDist=OldAnswer.Distance(Answer);
        ++nCount;
        }
    return Answer;
    }

class HermiteNode
    {
    public:

        HermiteNode() = default;

        HermiteNode(double               dParam,
                    SGM::Point3D  const &Pos,
                    SGM::Vector3D const &Tan):m_dParam(dParam),m_Pos(Pos),m_Tan(Tan){}

        double        m_dParam;
        SGM::Point3D  m_Pos;
        SGM::Vector3D m_Tan;
    };

bool MidPointIsOff(HermiteNode const &iter1,
                   HermiteNode const &iter2,
                   surface     const *pSurface1,
                   surface     const *pSurface2,
                   HermiteNode       &HNode)
    {
    SGM::Point3D Pos1=iter1.m_Pos;
    SGM::Point3D Pos2=iter2.m_Pos;
    SGM::Vector3D Vec1=iter1.m_Tan;
    SGM::Vector3D Vec2=iter2.m_Tan;
    double t1=iter1.m_dParam;
    double t2=iter2.m_dParam;
    double t3=(t1+t2)*0.5;
    double s=(t3-t1)/(t2-t1);
    double h1=(s*s)*(2*s-3)+1;
    double h2=1-h1;
    double h3=s*(s*(s-2)+1);
    double h4=(s*s)*(s-1);
    Vec1=Vec1*(t2-t1);
    Vec2=Vec2*(t2-t1);
    SGM::Point3D MidPos(h1*Pos1.m_x+h2*Pos2.m_x+h3*Vec1.m_x+h4*Vec2.m_x,
                        h1*Pos1.m_y+h2*Pos2.m_y+h3*Vec1.m_y+h4*Vec2.m_y,
                        h1*Pos1.m_z+h2*Pos2.m_z+h3*Vec1.m_z+h4*Vec2.m_z);

    SGM::Point3D ExactMidPos=ZoomInFrom(MidPos,pSurface1,pSurface2);
    double dDist2=ExactMidPos.DistanceSquared(MidPos);
    bool bAnswer=false;
    if(SGM_FIT_SMALL<dDist2)
        {
        SGM::Point2D uv1=pSurface1->Inverse(ExactMidPos);
        SGM::Point2D uv2=pSurface2->Inverse(ExactMidPos);
        SGM::UnitVector3D Norm1,Norm2;
        pSurface1->Evaluate(uv1,nullptr,nullptr,nullptr,&Norm1);
        pSurface2->Evaluate(uv2,nullptr,nullptr,nullptr,&Norm2);
        SGM::UnitVector3D WalkDir=Norm1*Norm2;
        HNode.m_dParam=t3;
        HNode.m_Pos=ExactMidPos;
        HNode.m_Tan=WalkDir;
        bAnswer=true;
        }
    return bAnswer;
    }

hermite *WalkFromToSub(SGM::Result                     &rResult,
                       SGM::Point3D              const &StartPos,
                       std::vector<SGM::Point3D> const &aEndPoints,
                       surface                   const *pSurface1,
                       surface                   const *pSurface2)
    {
    std::vector<SGM::Point3D> aPoints;
    std::vector<SGM::Vector3D> aTangents;
    std::vector<double> aParams;
    SGM::Point3D CurrentPos=StartPos;
    SGM::Point2D uv1=pSurface1->Inverse(CurrentPos);
    SGM::Point2D uv2=pSurface2->Inverse(CurrentPos);
    SGM::UnitVector3D Norm1,Norm2;
    pSurface1->Evaluate(uv1,nullptr,nullptr,nullptr,&Norm1);
    pSurface2->Evaluate(uv2,nullptr,nullptr,nullptr,&Norm2);
    SGM::UnitVector3D WalkDir=Norm1*Norm2;
    double dWalkFraction=0.5; 

    bool bFound=false;
    while(!bFound) 
        {
        // Find how far to walk.

        double c1=pSurface1->DirectionalCurvature(uv1,WalkDir);
        double c2=pSurface2->DirectionalCurvature(uv2,WalkDir);
        double dMax=std::max(fabs(c1),fabs(c2));
        double dR=dMax<SGM_ZERO ? SGM_MAX : 1.0/dMax;
        double dWalkDist=dR*dWalkFraction;
        
        aPoints.push_back(CurrentPos);
        aTangents.push_back(WalkDir);

        // Check to see if we are walking too far.
        // Causing us to move off pSurface1 or pSurface2.

        SGM::Point3D Pos=CurrentPos+WalkDir*dWalkDist;
        bool bCutWalk=true;
        while(bCutWalk)
            {
            SGM::Point3D Pos1;
            pSurface1->Inverse(Pos,&Pos1);
            double dDist1=Pos.Distance(Pos1);
            if(SGM_FIT<dDist1 && 0.1<dDist1/dWalkDist)
                {
                dWalkDist*=0.5;
                Pos=CurrentPos+WalkDir*dWalkDist;
                }
            else
                {
                bCutWalk=false;
                }
            }
        bCutWalk=true;
        while(bCutWalk)
            {
            SGM::Point3D Pos2;
            pSurface2->Inverse(Pos,&Pos2);
            double dDist2=Pos.Distance(Pos2);
            if(SGM_FIT<dDist2 && 0.1<dDist2/dWalkDist)
                {
                dWalkDist*=0.5;
                Pos=CurrentPos+WalkDir*dWalkDist;
                }
            else
                {
                bCutWalk=false;
                }
            }
        
        // Check to see if walking this far flips walking direction.
        // Find the new point and walking direction.

        bool bLooking=true;
        while(bLooking)
            {
            SGM::Point3D TestPoint=ZoomInFrom(Pos,pSurface1,pSurface2);
            uv1=pSurface1->Inverse(TestPoint);
            uv2=pSurface2->Inverse(TestPoint);
            pSurface1->Evaluate(uv1,nullptr,nullptr,nullptr,&Norm1);
            pSurface2->Evaluate(uv2,nullptr,nullptr,nullptr,&Norm2);
            SGM::UnitVector3D NewWalkDir=Norm1*Norm2;
            if(WalkDir%NewWalkDir<0)
                {
                dWalkDist*=0.5;
                Pos=CurrentPos+WalkDir*dWalkDist;
                }
            else
                {
                WalkDir=NewWalkDir;
                CurrentPos=TestPoint;
                bLooking=false;
                }
            }

        // Check to see if we are at the EndPos.
        
        if(1000<dWalkDist)
            {
            bFound=true;
            aPoints.push_back(CurrentPos);
            aTangents.push_back(WalkDir);
            }
        else if(aEndPoints.size())
            {
            size_t nWhere;
            SGM::DistanceToPoints(aEndPoints,CurrentPos,nWhere);
            SGM::Point3D EndPos=aEndPoints[nWhere];
            double dEndDist=EndPos.Distance(CurrentPos);
            bool bFoundEnd=false;
            if(dEndDist<dWalkDist)
                {
                SGM::UnitVector3D UVec=EndPos-CurrentPos;
                if(0<UVec%WalkDir)
                    {
                    bFoundEnd=true;
                    }
                }
            else if(aPoints.size())
                {
                // Check to see if we walked over the end point.

                SGM::Point3D LastPos=aPoints.back();
                if(SGM_MIN_TOL<LastPos.Distance(EndPos))
                    {
                    SGM::Segment3D Seg(LastPos,CurrentPos);
                    double dEndTol=dWalkDist*0.1;
                    if(Seg.PointOnSegment(EndPos,dEndTol))
                        {
                        bFoundEnd=true;
                        }
                    }
                }
            if(bFoundEnd)
                {
                aPoints.push_back(EndPos);
                if(SGM::NearEqual(StartPos,EndPos,SGM_ZERO))
                    {
                    aTangents.push_back(aTangents.front());
                    }
                else
                    {
                    uv1=pSurface1->Inverse(EndPos);
                    uv2=pSurface2->Inverse(EndPos);
                    bool bSingular1=pSurface1->IsSingularity(uv1,SGM_MIN_TOL);
                    bool bSingular2=pSurface2->IsSingularity(uv2,SGM_MIN_TOL);
                    pSurface1->Evaluate(uv1,nullptr,nullptr,nullptr,&Norm1);
                    pSurface2->Evaluate(uv2,nullptr,nullptr,nullptr,&Norm2);
                    SGM::Vector3D Vec=Norm1*Norm2;
                    if(bSingular1 || bSingular2 || Vec.Magnitude()<0.005)
                        {
                        SGM::Point3D StepBack=SGM::MidPoint(aPoints[aPoints.size()-2],EndPos,0.99);
                        uv1=pSurface1->Inverse(StepBack);
                        uv2=pSurface2->Inverse(StepBack);
                        pSurface1->Evaluate(uv1,nullptr,nullptr,nullptr,&Norm1);
                        pSurface2->Evaluate(uv2,nullptr,nullptr,nullptr,&Norm2);
                        Vec=Norm1*Norm2;
                        }
                    Vec=SGM::UnitVector3D(Vec);
                    aTangents.push_back(Vec);
                    }
                bFound=true;
                }
            }
        }

    // Refine points to meet SGM_FIT times the cord length of the curve tolerance.

    SGM::FindLengths3D(aPoints,aParams);

    //int a=1;
    //if(a)
    //    {
    //    return new hermite(rResult,aPoints,aTangents,aParams);
    //    }

    std::list<HermiteNode> lNodes;
    size_t nPoints=aPoints.size();
    size_t Index1;
    for(Index1=0;Index1<nPoints;++Index1)
        {
        lNodes.emplace_back(aParams[Index1],aPoints[Index1],aTangents[Index1]);
        }
    auto iter=lNodes.begin();
    auto LastIter=iter;
    ++iter;
    while(iter!=lNodes.end())
        {
        HermiteNode HNode;
        if(MidPointIsOff(*LastIter,*iter,pSurface1,pSurface2,HNode))
            {
            iter=lNodes.insert(iter,HNode);
            }
        else
            {
            ++LastIter;
            ++iter;
            }
        }
    
    // Create the Hermite curve.
    
    aPoints.clear();
    aTangents.clear();
    aParams.clear();
    iter=lNodes.begin();
    while(iter!=lNodes.end())
        {
        aPoints.push_back(iter->m_Pos);
        aParams.push_back(iter->m_dParam);
        aTangents.push_back(iter->m_Tan);
        ++iter;
        }

    return new hermite(rResult,aPoints,aTangents,aParams);
    }

hermite *WalkFromTo(SGM::Result                     &rResult,
                    SGM::Point3D              const &StartPos,
                    std::vector<SGM::Point3D> const &aEndPoints,
                    surface                   const *pSurface1,
                    surface                   const *pSurface2)
    {
    hermite *pCurve=WalkFromToSub(rResult,StartPos,aEndPoints,pSurface1,pSurface2);
    if(pCurve->GetClosed()==false)
        {
        hermite *pCurve2=WalkFromToSub(rResult,StartPos,aEndPoints,pSurface2,pSurface1);
        pCurve->Negate();
        pCurve->Concatenate(pCurve2);
        rResult.GetThing()->DeleteEntity(pCurve2);
        }
    return pCurve;
    }

void IntersectThreeSurfaces(SGM::Result               &rResult,
                            surface             const *pSurface1,
                            surface             const *pSurface2,
                            surface             const *pSurface3,
                            std::vector<SGM::Point3D> &aPoints)
    {
    std::vector<curve *> aCurves;
    size_t nCurves=IntersectSurfaces(rResult,pSurface1,pSurface2,aCurves,nullptr,nullptr,SGM_MIN_TOL);
    size_t Index1;
    for(Index1=0;Index1<nCurves;++Index1)
        {
        curve *pCurve=aCurves[Index1];
        std::vector<SGM::IntersectionType> aTypes;
        IntersectCurveAndSurface(rResult,pCurve,pSurface3,aPoints,aTypes,nullptr,nullptr,SGM_MIN_TOL);
        rResult.GetThing()->DeleteEntity(pCurve);
        }
    }

size_t IntersectCurveAndPlane(SGM::Result                        &rResult,
                              curve                        const *pCurve,
                              SGM::Point3D                 const &PlaneOrigin,
                              SGM::UnitVector3D            const &PlaneNorm,
                              std::vector<SGM::Point3D>          &aPoints,
                              std::vector<SGM::IntersectionType> &aTypes,
                              double                              dTolerance)
{
    switch(pCurve->GetCurveType())
        {
        case SGM::LineType:
            {
            auto pLine = (line const *)pCurve;
            IntersectLineAndPlane(pLine->m_Origin, pLine->m_Axis, SGM::Interval1D(-SGM_MAX, SGM_MAX),
                                  PlaneOrigin, PlaneNorm, dTolerance, aPoints, aTypes);
            break;
            }
        case SGM::CircleType:
            {
            auto pCircle=(circle const *)pCurve;
            IntersectCircleAndPlane(pCircle->m_Center, pCircle->m_Normal, pCircle->m_dRadius, 
                                    PlaneOrigin, PlaneNorm, dTolerance, aPoints, aTypes);
            break;
            }
        case SGM::EllipseType:
            {
            auto pEllipse=(ellipse const *)pCurve;
            IntersectEllipseAndPlane(pEllipse, PlaneOrigin, PlaneNorm, dTolerance, aPoints, aTypes);
            break;
            }
        case SGM::ParabolaType:
            {
            auto pParabola = (parabola const *)pCurve;
            IntersectParabolaAndPlane(pParabola, PlaneOrigin, PlaneNorm, dTolerance, aPoints, aTypes);
            break;
            }
        case SGM::HyperbolaType:
            {
            auto pHyperbola = (hyperbola const *)pCurve;
            IntersectHyperbolaAndPlane(pHyperbola, PlaneOrigin, PlaneNorm, dTolerance, aPoints, aTypes);
            break;
            }
        case SGM::NUBCurveType:
            {
            auto pNUBCurve = (NUBcurve const *)pCurve;
            IntersectNUBCurveAndPlane(rResult, pNUBCurve, PlaneOrigin, PlaneNorm, aPoints, aTypes, dTolerance);
            break;
            }
        case SGM::NURBCurveType:
            {
            auto pNURBCurve = (NURBcurve const *)pCurve;
            IntersectNURBCurveAndPlane(rResult, pNURBCurve, PlaneOrigin, PlaneNorm, aPoints, aTypes, dTolerance);
            break;
            }
        case SGM::PointCurveType:
            {
            auto pPointCurve=(PointCurve const *)pCurve;
            SGM::Point3D const &Pos=pPointCurve->m_Pos;
            double dDist = (Pos-PlaneOrigin)%PlaneNorm;
            if(fabs(dDist)<dTolerance)
                {
                SGM::Point3D PlanePoint = Pos + dDist*PlaneNorm;
                aPoints.push_back(PlanePoint);
                aTypes.push_back(SGM::IntersectionType::CoincidentType);
                }
            break;
            }
        default:
            {
            rResult.SetResult(SGM::ResultType::ResultTypeUnknownType);
            }
        }
     return aPoints.size();
}

size_t IntersectEdgeAndPlane(SGM::Result                        &rResult,
                             edge                         const *pEdge,
                             SGM::Point3D                 const &PlaneOrigin,
                             SGM::UnitVector3D            const &PlaneNorm,
                             std::vector<SGM::Point3D>          &aPoints,
                             std::vector<SGM::IntersectionType> &aTypes,
                             double                              dTolerance)
    {
    curve const *pCurve=pEdge->GetCurve();
    std::vector<SGM::Point3D> aTempPoints;
    std::vector<SGM::IntersectionType> aTempTypes;
    size_t nHits=IntersectCurveAndPlane(rResult,pCurve,PlaneOrigin,PlaneNorm,aTempPoints,aTempTypes,dTolerance);
    size_t nAnswer=0;
    size_t Index1;
    for(Index1=0;Index1<nHits;++Index1)
        {
        SGM::Point3D const &Pos=aTempPoints[Index1];
        if(pEdge->PointInEdge(Pos,dTolerance))
            {
            aPoints.push_back(Pos);
            aTypes.push_back(aTempTypes[Index1]);
            ++nAnswer;
            }
        }
    return nAnswer;
    }

void FindStartingPointsForCurvePlaneIntersection(SGM::Point3D              const &PlaneOrigin,
                                                 SGM::UnitVector3D         const &PlaneNorm,
                                                 double                          dTolerance,
                                                 std::vector<SGM::Point3D> const &aSeedPoints,
                                                 std::vector<SGM::Point3D>       &aStartPoints)
{
    size_t nSeedPoints=aSeedPoints.size();
    double dTanHalfAngle=SEED_POINT_HALF_ANGLE_TANGENT;
    size_t Index1;
    for(Index1=1;Index1<nSeedPoints;++Index1)
    {
        SGM::Point3D const &Pos0=aSeedPoints[Index1-1];
        SGM::Point3D const &Pos1=aSeedPoints[Index1];
        double dDist0 = ((Pos0 - PlaneOrigin) % PlaneNorm);
        double dDist1 = ((Pos1 - PlaneOrigin) % PlaneNorm);
        if ((dDist0 * dDist1) < SGM_ZERO) // opposite sides of the plane
        {
            double dFraction = fabs(dDist0) / (fabs(dDist0) + fabs(dDist1));
            SGM::Point3D Start = Pos0 + (dFraction * (Pos1 - Pos0)); 
            aStartPoints.push_back(Start);
        }
        else
        {
            double dLength=Pos0.Distance(Pos1);
            double dTol=dTolerance+dLength*dTanHalfAngle;
            if (fabs(dDist0) < dTol)
            {
                aStartPoints.push_back(Pos0);
            }
            if (fabs(dDist1) < dTol)
            {
                aStartPoints.push_back(Pos1);
            }
        }
    }
}

void FindCurvePlaneIntersections(curve                                  const *pCurve,
                                 SGM::Point3D                           const &PlaneOrigin,
                                 SGM::UnitVector3D                      const &PlaneNorm,
                                 double                                        dTolerance,
                                 std::vector<SGM::Point3D>              const &aStartPoints,
                                 std::vector<std::pair<double,SGM::Point3D> > &aRefinedPoints)
{
    size_t nStartPoints=aStartPoints.size();
    size_t nCountLimit=100;
    size_t Index1;

    for(Index1=0;Index1<nStartPoints;++Index1)
    {
        SGM::Point3D Pos=aStartPoints[Index1];
        size_t nCount=0;
        double dOldDist=SGM_MAX;
        while(nCount<nCountLimit)
        {
#if 0 // project back and forth
            // project point to NUBCurve
            double dCurvet=pCurve->Inverse(Pos);
            SGM::Point3D CPos;
            pCurve->Evaluate(dCurvet,&CPos);

            // project point to plane
            Pos = CPos + ((PlaneOrigin - CPos) % PlaneNorm) * PlaneNorm;

            double dDist = Pos.Distance(CPos);

            //if(dDist<dOldDist)
            //{
            //    Pos=Temp;
            //}
#endif
#if 0 // newton
            // project point to NUBCurve and get the tangent
            double dCurvet=pCurve->Inverse(Pos);
            SGM::Point3D CPos;
            SGM::Vector3D LocalTan;
            pCurve->Evaluate(dCurvet,&CPos,&LocalTan);

            SGM::UnitVector3D uLocalTan(LocalTan);
            double dT = (PlaneNorm % (PlaneOrigin - CPos)) / (PlaneNorm % uLocalTan);
            SGM::Point3D Temp = CPos + dT * uLocalTan;
            double dDist=Temp.Distance(CPos);
            if(dDist<dOldDist)
            {
                Pos=Temp;
            }
            else
            {
                // Newton led us astray.  Just project to plane.
                double Dist=(CPos-PlaneOrigin)%PlaneNorm;
                Pos=CPos - Dist*PlaneNorm;
                dDist=Pos.Distance(CPos);
            }
#endif
#if 1 // tangent parabola
            double dCurveT=pCurve->Inverse(Pos);
            SGM::Point3D CPos;
            SGM::Vector3D LocalTan;
            pCurve->Evaluate(dCurveT, &CPos, &LocalTan);
            SGM::Vector3D Curvature = pCurve->Curvature(dCurveT);
            double dA = Curvature.Magnitude() * 0.5;

            if (Curvature.Magnitude() > SGM_FIT)
            {
                std::vector<SGM::Point3D> aParabolaPoints;
                std::vector<SGM::IntersectionType> aParabolaTypes;
                IntersectParabolaAndPlane(CPos, LocalTan, Curvature, dA, PlaneOrigin, PlaneNorm, dTolerance, aParabolaPoints, aParabolaTypes);

                if (aParabolaPoints.size() == 2)
                {
                    if ( (aParabolaTypes[0] == SGM::IntersectionType::CoincidentType) &&
                         (aParabolaTypes[1] == SGM::IntersectionType::CoincidentType) )
                    {
                        // if parabola and plane are coincident
                        Pos = CPos; 
                    }
                    else
                    {
                        double dDS1 = CPos.DistanceSquared(aParabolaPoints[0]);
                        double dDS2 = CPos.DistanceSquared(aParabolaPoints[1]);
                        if (dDS1 <= dDS2)
                          Pos = aParabolaPoints[0];
                        else
                          Pos = aParabolaPoints[1];
                    }
                }
                else if (aParabolaPoints.size() == 1)
                {
                    Pos = aParabolaPoints[0];
                }
                else if (aParabolaPoints.empty())
                {
                    // check the distance of the point to the plane
                    double Dist = (CPos - PlaneOrigin) % PlaneNorm;
                    if (fabs(Dist) < dTolerance)
                    {
                        double dCurveTInverse=pCurve->Inverse(Pos);
                        aRefinedPoints.emplace_back(dCurveTInverse,Pos);
                    }
                    break;
                }
                else
                {
                    throw;
                }
            }
            else
            {
                // parabola is a line so fall back to Newton
                SGM::UnitVector3D uLocalTan(LocalTan);
                double dT = (PlaneNorm % (PlaneOrigin - CPos)) / (PlaneNorm % uLocalTan);
                SGM::Point3D Temp = CPos + dT * uLocalTan;
                double dDist=Temp.Distance(CPos);
                if(dDist<dOldDist)
                {
                    Pos=Temp;
                }
                else
                {
                    // Newton led us astray.  Just project to plane.
                    double Dist=(CPos-PlaneOrigin)%PlaneNorm;
                    Pos=CPos - Dist*PlaneNorm;
                }
            }
            double dDist=Pos.Distance(CPos);
#endif
            if(dDist<SGM_ZERO || fabs(dDist-dOldDist)<SGM_ZERO)
            {
                if (dDist<dTolerance)
                {
                    double dCurveTInverse=pCurve->Inverse(Pos);
                    aRefinedPoints.emplace_back(dCurveTInverse,Pos);
                }
                break;
            }
            if(nCount==nCountLimit-1 && dDist<dTolerance)
            {
                double dCurveTInverse=pCurve->Inverse(Pos);
                aRefinedPoints.emplace_back(dCurveTInverse,Pos);
                break;
            }
            dOldDist=dDist;
            ++nCount;
        }
    }
}

void RemoveCurvePlaneIntersectionDuplicates(curve const *pCurve,
                                            SGM::UnitVector3D                      const &PlaneNorm,
                                            double                                        dTolerance,
                                            std::vector<std::pair<double,SGM::Point3D> > &aRefinedPoints,
                                            std::vector<SGM::Point3D>                    &aPoints,
                                            std::vector<SGM::IntersectionType>           &aTypes)
{
    size_t Index1;
    if(size_t nRefinedPoints=aRefinedPoints.size())
    {
        double dDuplicatesTolerance=std::max(dTolerance,SGM_MIN_TOL);
        std::sort(aRefinedPoints.begin(),aRefinedPoints.end());
        for(Index1=0;Index1<nRefinedPoints;++Index1)
        {
            SGM::Point3D const &Pos=aRefinedPoints[Index1].second;
            if(Index1==0 || dDuplicatesTolerance<Pos.Distance(aPoints.back()))
            {
                double t=aRefinedPoints[Index1].first;
                SGM::Vector3D DPos;
                pCurve->Evaluate(t,nullptr,&DPos);
                aPoints.push_back(Pos);
                SGM::UnitVector3D Test(DPos);
                if(fabs(Test%PlaneNorm)<SGM_MIN_TOL)
                {
                    aTypes.push_back(SGM::IntersectionType::TangentType);
                }
                else
                {
                    aTypes.push_back(SGM::IntersectionType::PointType);
                }
            }
        }
    }
}

size_t IntersectNUBCurveAndPlane(SGM::Result                        &,//rResult,
                                 NUBcurve                     const *pCurve,
                                 SGM::Point3D                 const &PlaneOrigin,
                                 SGM::UnitVector3D            const &PlaneNorm,
                                 std::vector<SGM::Point3D>          &aPoints,
                                 std::vector<SGM::IntersectionType> &aTypes,
                                 double                              dTolerance)
{
    SGM::Point3D      CurvePlaneOrigin;
    SGM::UnitVector3D CurvePlaneNormal;

    if (SGM::ArePointsCoplanar(pCurve->GetControlPoints(), dTolerance, &CurvePlaneOrigin, &CurvePlaneNormal))
    {
        if(SGM::NearEqual(fabs(CurvePlaneNormal%PlaneNorm),1.0,SGM_MIN_TOL,false)) // planes are parallel
        {
            if(fabs((CurvePlaneOrigin-PlaneOrigin)%PlaneNorm)<dTolerance)
            {
                SGM::Point3D StartPos;
                SGM::Point3D EndPos;
                pCurve->Evaluate(pCurve->GetDomain().m_dMin, &StartPos);
                pCurve->Evaluate(pCurve->GetDomain().m_dMax, &EndPos);
                aPoints.push_back(StartPos);
                aTypes.push_back(SGM::IntersectionType::CoincidentType);
                aPoints.push_back(EndPos);
                aTypes.push_back(SGM::IntersectionType::CoincidentType);
            }
        }
        else
        {
            SGM::Point3D LineOrigin;
            SGM::UnitVector3D LineAxis;
            IntersectNonParallelPlanes(CurvePlaneOrigin, CurvePlaneNormal, PlaneOrigin, PlaneNorm, LineOrigin, LineAxis);
            IntersectLineAndNUBCurve(LineOrigin, LineAxis, SGM::Interval1D(-SGM_MAX, +SGM_MAX), pCurve, dTolerance, aPoints, aTypes);
        }
    }
    else
    {
        std::vector<SGM::Point3D> const &aSeedPoints=pCurve->GetSeedPoints();
        std::vector<SGM::Point3D> aStartPoints;
        SGMInternal::FindStartingPointsForCurvePlaneIntersection(PlaneOrigin, PlaneNorm, dTolerance, aSeedPoints, aStartPoints);

        std::vector<std::pair<double,SGM::Point3D> > aRefinedPoints;
        SGMInternal::FindCurvePlaneIntersections(pCurve, PlaneOrigin, PlaneNorm, dTolerance, aStartPoints, aRefinedPoints);

        RemoveCurvePlaneIntersectionDuplicates(pCurve, PlaneNorm, dTolerance, aRefinedPoints, aPoints, aTypes);
    }

    return aPoints.size();    
}

size_t IntersectNURBCurveAndPlane(SGM::Result                         &,//rResult,
                                  NURBcurve                     const *pCurve,
                                  SGM::Point3D                  const &PlaneOrigin,
                                  SGM::UnitVector3D             const &PlaneNorm,
                                  std::vector<SGM::Point3D>           &aPoints,
                                  std::vector<SGM::IntersectionType>  &aTypes,
                                  double                               dTolerance)
{
    SGM::Point3D      CurvePlaneOrigin;
    SGM::UnitVector3D CurvePlaneNormal;

    std::vector<SGM::Point4D> const &aControlPoints4D = pCurve->GetControlPoints();

    std::vector<SGM::Point3D> aControlPoints3D;
    aControlPoints3D.reserve(aControlPoints4D.size());
    for (SGM::Point4D Pos4D : aControlPoints4D)
        aControlPoints3D.emplace_back(Pos4D.m_x, Pos4D.m_y, Pos4D.m_z);

    if (SGM::ArePointsCoplanar(aControlPoints3D, dTolerance, &CurvePlaneOrigin, &CurvePlaneNormal))
    {
        if(SGM::NearEqual(fabs(CurvePlaneNormal%PlaneNorm),1.0,SGM_MIN_TOL,false)) // planes are parallel
        {
            if(fabs((CurvePlaneOrigin-PlaneOrigin)%PlaneNorm)<dTolerance)
            {
                SGM::Point3D StartPos;
                SGM::Point3D EndPos;
                pCurve->Evaluate(pCurve->GetDomain().m_dMin, &StartPos);
                pCurve->Evaluate(pCurve->GetDomain().m_dMax, &EndPos);
                aPoints.push_back(StartPos);
                aTypes.push_back(SGM::IntersectionType::CoincidentType);
                aPoints.push_back(EndPos);
                aTypes.push_back(SGM::IntersectionType::CoincidentType);
            }
        }
        else
        {
            SGM::Point3D LineOrigin;
            SGM::UnitVector3D LineAxis;
            IntersectNonParallelPlanes(CurvePlaneOrigin, CurvePlaneNormal, PlaneOrigin, PlaneNorm, LineOrigin, LineAxis);
            IntersectLineAndNURBCurve(LineOrigin, LineAxis, SGM::Interval1D(-SGM_MAX, +SGM_MAX), pCurve, dTolerance, aPoints, aTypes);
        }
    }
    else
    {
        std::vector<SGM::Point3D> const &aSeedPoints=pCurve->GetSeedPoints();
        std::vector<SGM::Point3D> aStartPoints;
        SGMInternal::FindStartingPointsForCurvePlaneIntersection(PlaneOrigin, PlaneNorm, dTolerance, aSeedPoints, aStartPoints);

        std::vector<std::pair<double,SGM::Point3D> > aRefinedPoints;
        SGMInternal::FindCurvePlaneIntersections(pCurve, PlaneOrigin, PlaneNorm, dTolerance, aStartPoints, aRefinedPoints);

        RemoveCurvePlaneIntersectionDuplicates(pCurve, PlaneNorm, dTolerance, aRefinedPoints, aPoints, aTypes);
    }

    return aPoints.size();    
}

size_t IntersectEllipseAndPlane(ellipse                      const *pEllipse,
                                SGM::Point3D                 const &PlaneOrigin,
                                SGM::UnitVector3D            const &PlaneNormal,
                                double                              dTolerance,
                                std::vector<SGM::Point3D>          &aPoints,
                                std::vector<SGM::IntersectionType> &aTypes)
{
    if(SGM::NearEqual(fabs(pEllipse->m_Normal%PlaneNormal),1.0,SGM_MIN_TOL,false))
        {
        if(fabs((pEllipse->m_Center-PlaneOrigin)%PlaneNormal)<dTolerance)
            {
            SGM::Point3D EllipsePos;
            pEllipse->Evaluate(0.0, &EllipsePos);
            aPoints.push_back(EllipsePos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            aPoints.push_back(EllipsePos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            }
        }
    else
        {
        SGM::Point3D LineOrigin;
        SGM::UnitVector3D LineDirection;
        IntersectNonParallelPlanes(pEllipse->m_Center,pEllipse->m_Normal,PlaneOrigin,PlaneNormal,LineOrigin,LineDirection);
        IntersectLineAndEllipse(LineOrigin,LineDirection,SGM::Interval1D(-SGM_MAX,SGM_MAX),pEllipse,dTolerance,aPoints,aTypes);
        }
    return aPoints.size();
}

size_t IntersectParabolaAndPlane(SGM::Point3D                 const &Center,
                                 SGM::UnitVector3D            const &XAxis,
                                 SGM::UnitVector3D            const &YAxis,
                                 double                             dA,
                                 SGM::Point3D                 const &PlaneOrigin,
                                 SGM::UnitVector3D            const &PlaneNormal,
                                 double                              dTolerance,
                                 std::vector<SGM::Point3D>          &aPoints,
                                 std::vector<SGM::IntersectionType> &aTypes)
{
    SGM::UnitVector3D ParabolaNormal = XAxis*YAxis;
    if(SGM::NearEqual(fabs((ParabolaNormal)%PlaneNormal),1.0,SGM_MIN_TOL,false))
        {
        if(fabs((Center-PlaneOrigin)%PlaneNormal)<dTolerance)
            {
            // for coincident plane, return ends as a coincident
            SGM::Point3D Pos;
            ParabolaEvaluate(Center, XAxis, YAxis, dA, SGM_INTERVAL_NEG_MIN, &Pos);
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            ParabolaEvaluate(Center, XAxis, YAxis, dA, SGM_INTERVAL_POS_MAX, &Pos);
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            }
        }
    else
        {
        SGM::Point3D LineOrigin;
        SGM::UnitVector3D LineDirection;
        IntersectNonParallelPlanes(Center,ParabolaNormal,PlaneOrigin,PlaneNormal,LineOrigin,LineDirection);
        IntersectCoplanarLineAndParabola(LineOrigin,LineDirection,SGM::Interval1D(-SGM_MAX,SGM_MAX),
                                         Center,XAxis,YAxis,dA,dTolerance,aPoints,aTypes);
        }
    return aPoints.size();
}

size_t IntersectHyperbolaAndPlane(hyperbola                     const *pHyperbola,
                                  SGM::Point3D                  const &PlaneOrigin,
                                  SGM::UnitVector3D             const &PlaneNormal,
                                  double                               dTolerance,
                                  std::vector<SGM::Point3D>           &aPoints,
                                  std::vector<SGM::IntersectionType>  &aTypes)
{
    if(SGM::NearEqual(fabs(pHyperbola->m_Normal%PlaneNormal),1.0,SGM_MIN_TOL,false))
        {
        if(fabs((pHyperbola->m_Center-PlaneOrigin)%PlaneNormal)<dTolerance)
            {
            SGM::UnitVector3D XAxis=pHyperbola->m_Normal.Orthogonal();

            SGM::Point3D Pos;
            pHyperbola->Evaluate(pHyperbola->GetDomain().m_dMin, &Pos);
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            pHyperbola->Evaluate(pHyperbola->GetDomain().m_dMax, &Pos);
            aPoints.push_back(Pos);
            aTypes.push_back(SGM::IntersectionType::CoincidentType);
            }
        }
    else
        {
        SGM::Point3D LineOrigin;
        SGM::UnitVector3D LineDirection;
        IntersectNonParallelPlanes(pHyperbola->m_Center,pHyperbola->m_Normal,PlaneOrigin,PlaneNormal,LineOrigin,LineDirection);
        IntersectLineAndHyperbola(LineOrigin,LineDirection,SGM::Interval1D(-SGM_MAX,SGM_MAX),pHyperbola,dTolerance,aPoints,aTypes);
        }
    return aPoints.size();
}

} // End of SGMInternal namespace
