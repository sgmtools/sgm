#include "SGMChecker.h"
#include "SGMPrimitives.h"
#include "SGMComplex.h"
#include "SGMMathematics.h"
#include "SGMInterval.h"
#include "SGMTranslators.h"
#include "SGMTopology.h"
#include "SGMVector.h"
#include "SGMGeometry.h"
#include "SGMInterrogate.h"
#include "SGMIntersector.h"
#include "SGMTransform.h"
#include "SGMTopology.h"
#include "SGMDisplay.h"
#include "SGMMeasure.h"
#include "SGMEntityFunctions.h"
#include "SGMModify.h"

#include "FileFunctions.h"
#include "EntityClasses.h"
#include "Intersectors.h"
#include "FacetToBRep.h"
#include "Primitive.h"

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cmath>

#ifdef _MSC_VER
__pragma(warning(disable: 4996 ))
#endif

class TestCommand
    {
    public:

        std::string              m_sOutput;
        std::string              m_sCommand;
        std::vector<double>      m_aDoubles;
        std::vector<std::string> m_aStrings;
        std::vector<std::string> m_aVariables;
    };

size_t ParseVariable(std::map<std::string,std::vector<size_t> > &mVariableMap,
                     std::string                          const &sVariable)
    {
    // Find the variable name and index.
    
    size_t nLength=sVariable.length();
    char const *str=sVariable.c_str();
    size_t Index1,Index2;
    bool bFound=false;
    for(Index1=0;Index1<nLength;++Index1)
        {
        if(str[Index1]=='[')
            {
            bFound=true;
            break;
            }
        }
    if(bFound)
        {
        std::string sIndex=str+Index1+1;
        int nIndex;
        std::stringstream(sIndex) >> nIndex;
        std::string sVariableName;
        for(Index2=0;Index2<Index1;++Index2)
            {
            sVariableName+=str[Index2];
            }
        return mVariableMap[sVariableName][nIndex];
        }
    else
        {
        return mVariableMap[sVariable][0];
        }
    }

bool RunCPPTest(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &,std::string const &,TestCommand &TestData)
    {
    return SGM::RunCPPTest(rResult,(size_t)TestData.m_aDoubles[0]);
    }

bool RunCreateBlock(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &mVariableMap,std::string const &,TestCommand &TestData)
    {
    SGM::Point3D Pos0(TestData.m_aDoubles[0],TestData.m_aDoubles[1],TestData.m_aDoubles[2]);
    SGM::Point3D Pos1(TestData.m_aDoubles[3],TestData.m_aDoubles[4],TestData.m_aDoubles[5]);
    SGM::Body BodyID=SGM::CreateBlock(rResult,Pos0,Pos1);
    std::vector<size_t> aEnts;
    aEnts.push_back(BodyID.m_ID);
    mVariableMap[TestData.m_sOutput]=aEnts;
    return true;
    }

bool RunCreateSphere(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &mVariableMap,std::string const &,TestCommand &TestData)
    {
    SGM::Point3D Center(TestData.m_aDoubles[0],TestData.m_aDoubles[1],TestData.m_aDoubles[2]);
    double dRadius=TestData.m_aDoubles[3];
    SGM::Body BodyID=SGM::CreateSphere(rResult,Center,dRadius);
    std::vector<size_t> aEnts;
    aEnts.push_back(BodyID.m_ID);
    mVariableMap[TestData.m_sOutput]=aEnts;
    return true;
    }

bool RunCreateTorus(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &mVariableMap,std::string const &,TestCommand &TestData)
    {
    SGM::Point3D Center(TestData.m_aDoubles[0],TestData.m_aDoubles[1],TestData.m_aDoubles[2]);
    SGM::UnitVector3D Normal(TestData.m_aDoubles[3],TestData.m_aDoubles[4],TestData.m_aDoubles[5]);
    double dMinorRadius=TestData.m_aDoubles[6];
    double dMajorRadius=TestData.m_aDoubles[7];
    SGM::Body BodyID=SGM::CreateTorus(rResult,Center,Normal,dMinorRadius,dMajorRadius);
    std::vector<size_t> aEnts;
    aEnts.push_back(BodyID.m_ID);
    mVariableMap[TestData.m_sOutput]=aEnts;
    return true;
    }

bool RunSaveSTEP(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &mVariableMap,std::string const &,TestCommand &TestData)
    {
    SGM::Entity Ent(mVariableMap[TestData.m_aVariables[0]][0]);
    SGM::TranslatorOptions Options;
    SGM::SaveSTEP(rResult,TestData.m_aStrings[0],Ent,Options);
    return true;
    }

bool RunSaveSTL(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &mVariableMap,std::string const &sDir,TestCommand &TestData)
    {
    size_t nEnt=ParseVariable(mVariableMap,TestData.m_aVariables[0]);
    SGM::Entity Ent(nEnt);
    SGM::TranslatorOptions Options;
    std::string sFullPath=sDir+"/"+TestData.m_aStrings[0];
    SGM::SaveSTL(rResult,sFullPath,Ent,Options);
    return true;
    }

bool RunReadFile(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &mVariableMap,std::string const &sDir,TestCommand &TestData)
    {
    std::vector<SGM::Entity> aEntities;
    std::vector<std::string> aLog;
    SGM::TranslatorOptions Options;
    std::string sFullPath=sDir+"/"+TestData.m_aStrings[0];
    size_t nEnts=SGM::ReadFile(rResult,sFullPath,aEntities,aLog,Options);
    std::vector<size_t> aEnts;
    aEnts.reserve(nEnts);
    size_t Index1;
    for(Index1=0;Index1<nEnts;++Index1)
        {
        aEnts.push_back(aEntities[Index1].m_ID);
        }
    mVariableMap[TestData.m_aVariables[0]]=aEnts;
    return true;
    }

bool RunCompareFiles(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &,std::string const &sDir,TestCommand &TestData)
    {
    std::string sFullPath0=sDir+"/"+TestData.m_aStrings[0];
    std::string sFullPath1=sDir+"/"+TestData.m_aStrings[1];
    return SGM::CompareFiles(rResult,sFullPath0,sFullPath1);
    }

bool RunCompareSizes(SGM::Result &,std::map<std::string,std::vector<size_t> > &mVariableMap,std::string const &,TestCommand &TestData)
    {
    std::vector<size_t> aSizes;
    size_t Index1;
    size_t nDoubles=TestData.m_aDoubles.size();
    for(Index1=0;Index1<nDoubles;++Index1)
        {
        aSizes.push_back((size_t)TestData.m_aDoubles[Index1]);
        }
    size_t nVariables=TestData.m_aVariables.size();
    for(Index1=0;Index1<nVariables;++Index1)
        {
        aSizes.push_back(ParseVariable(mVariableMap,TestData.m_aVariables[Index1]));
        }
    return aSizes[0]==aSizes[1];
    }

bool RunFindCloseFaces(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &mVariableMap,std::string const &,TestCommand &TestData)
    {
    std::vector<SGM::Face> aFaces;
    size_t nEnt=ParseVariable(mVariableMap,TestData.m_aVariables[0]);
    SGM::Entity Ent(nEnt);
    SGM::Point3D Pos(TestData.m_aDoubles[0],TestData.m_aDoubles[1],TestData.m_aDoubles[2]);
    double dDist=TestData.m_aDoubles[3];
    size_t nEnts=SGM::FindCloseFaces(rResult,Pos,Ent,dDist,aFaces);

    std::vector<size_t> aEnts;
    aEnts.reserve(nEnts);
    size_t Index1;
    for(Index1=0;Index1<nEnts;++Index1)
        {
        aEnts.push_back(aFaces[Index1].m_ID);
        }
    mVariableMap[TestData.m_aVariables[0]]=aEnts;
    mVariableMap[TestData.m_sOutput].push_back(nEnts);

    return true;
    }

bool RunFindCloseEdges(SGM::Result &rResult,std::map<std::string,std::vector<size_t> > &mVariableMap,std::string const &,TestCommand &TestData)
    {
    std::vector<SGM::Edge> aEdges;
    size_t nEnt=ParseVariable(mVariableMap,TestData.m_aVariables[0]);
    SGM::Entity Ent(nEnt);
    SGM::Point3D Pos(TestData.m_aDoubles[0],TestData.m_aDoubles[1],TestData.m_aDoubles[2]);
    double dDist=TestData.m_aDoubles[3];
    size_t nEnts=SGM::FindCloseEdges(rResult,Pos,Ent,dDist,aEdges);

    std::vector<size_t> aEnts;
    aEnts.reserve(nEnts);
    size_t Index1;
    for(Index1=0;Index1<nEnts;++Index1)
        {
        aEnts.push_back(aEdges[Index1].m_ID);
        }
    mVariableMap[TestData.m_aVariables[0]]=aEnts;
    mVariableMap[TestData.m_sOutput].push_back(nEnts);

    return true;
    }

typedef bool (*SGMFunction)(SGM::Result &,
                            std::map<std::string,std::vector<size_t> > &,
                            std::string const &,
                            TestCommand &); // function pointer type

void CreateFunctionMap(std::map<std::string,SGMFunction> &mFunctionMap)
    {
    mFunctionMap["CompareFiles"]=RunCompareFiles;
    mFunctionMap["CompareSizes"]=RunCompareSizes;
    mFunctionMap["CreateBlock"]=RunCreateBlock;
    mFunctionMap["CreateSphere"]=RunCreateSphere;
    mFunctionMap["CreateTorus"]=RunCreateTorus;
    mFunctionMap["FindCloseFaces"]=RunFindCloseFaces;
    mFunctionMap["FindCloseEdges"]=RunFindCloseEdges;
    mFunctionMap["ReadFile"]=RunReadFile;
    mFunctionMap["RunCPPTest"]=RunCPPTest;
    mFunctionMap["SaveSTEP"]=RunSaveSTEP;
    mFunctionMap["SaveSTL"]=RunSaveSTL;
    }

void FindOutput(std::string const &sFileLine,
                std::string       &sOutput,
                std::string       &sRight)
    {
    size_t nLength=sFileLine.length();
    size_t Index1,Index2;
    for(Index1=0;Index1<nLength;++Index1)
        {
        if(sFileLine.c_str()[Index1]=='=')
            {
            for(Index2=0;Index2<Index1;++Index2)
                {
                sOutput+=sFileLine.c_str()[Index2];
                }
            for(Index2=Index1+1;Index2<nLength;++Index2)
                {
                sRight+=sFileLine.c_str()[Index2];
                }
            return;
            }
        }
    sRight=sFileLine;
    }

void FindCommand(std::string const &sFileLine,
                 std::string       &sCommand,
                 std::string       &sRight)
    {
    size_t nLength=sFileLine.length();
    size_t Index1,Index2;
    for(Index1=0;Index1<nLength;++Index1)
        {
        char c=sFileLine.c_str()[Index1];
        if(c=='(' || c==';')
            {
            for(Index2=0;Index2<Index1;++Index2)
                {
                sCommand+=sFileLine.c_str()[Index2];
                }
            for(Index2=Index1+1;Index2<nLength;++Index2)
                {
                sRight+=sFileLine.c_str()[Index2];
                }
            return;
            }
        }
    }

bool FindFirstArgument(std::string const &sFileLine,
                       std::string       &sLeft,
                       std::string       &sRight)
    {
    size_t nLength=sFileLine.length();
    size_t Index1,Index2;
    for(Index1=0;Index1<nLength;++Index1)
        {
        if(sFileLine.c_str()[Index1]==',')
            {
            for(Index2=0;Index2<Index1;++Index2)
                {
                sLeft+=sFileLine.c_str()[Index2];
                }
            for(Index2=Index1+1;Index2<nLength;++Index2)
                {
                sRight+=sFileLine.c_str()[Index2];
                }
            return true;
            }
        if(sFileLine.c_str()[Index1]==')')
            {
            for(Index2=0;Index2<Index1;++Index2)
                {
                sLeft+=sFileLine.c_str()[Index2];
                }
            return true;
            }
        }
    return false;
    }

size_t FindArguments(std::string        const &sFileLine,
                     std::vector<std::string> &aArguments)
    {
    std::string sLeft,sRight,sString=sFileLine;
    bool bFound=true;
    while(bFound)
        {
        bFound=FindFirstArgument(sString,sLeft,sRight);
        if(bFound)
            {
            sString=sRight;
            sRight.clear();
            aArguments.push_back(sLeft);
            sLeft.clear();
            }
        }
    return aArguments.size();
    }

bool ParseLine(std::string const &sFileLine,
               TestCommand       &LineData)
    {
    if(sFileLine.empty() || sFileLine.c_str()[0]=='/')
        {
        return false;
        }

    // Anything in front of an '=' is the sOutput.
    // Anything after an '=' is the sCommand or if no '=' exists then the first 
    // up to the first (.
    // Break things up by ',' and ')' if they have a '"' then they are a string
    // else they are a double.

    std::string sRight,sArgs;
    FindOutput(sFileLine,LineData.m_sOutput,sRight);
    FindCommand(sRight,LineData.m_sCommand,sArgs);
    std::vector<std::string> aArguments;
    size_t nArguments=FindArguments(sArgs,aArguments);
    size_t Index1,Index2;
    for(Index1=0;Index1<nArguments;++Index1)
        {
        char c=aArguments[Index1].c_str()[0];
        if(c=='\"')  // String
            {
            size_t nLength=aArguments[Index1].length()-1;
            std::string sString;
            for(Index2=1;Index2<nLength;++Index2)
                {
                sString+=aArguments[Index1].c_str()[Index2];
                }
            LineData.m_aStrings.push_back(sString);
            }
        else if(c<'A' && c!='#') // Double
            {
            double dData;
            std::stringstream(aArguments[Index1]) >> dData;
            LineData.m_aDoubles.push_back(dData);
            }
        else // Variable
            {
            LineData.m_aVariables.push_back(aArguments[Index1]);
            }
        }
    return true;
    }

bool RunFileLine(SGM::Result                                &rResult,
                 std::map<std::string,SGMFunction>          &mFunctionMap,
                 std::string                          const &sTestDirectory,
                 std::map<std::string,std::vector<size_t> > &mVariableMap,
                 std::string                          const &sFileLine,
                 FILE                                       *)//pOutputFilep)
    {
    TestCommand LineData;
    if(ParseLine(sFileLine,LineData))
        {
        std::map<std::string,SGMFunction>::const_iterator iter=mFunctionMap.find(LineData.m_sCommand);
        if(iter==mFunctionMap.end())
            {
            rResult.SetResult(SGM::ResultType::ResultTypeUnknownCommand);
            rResult.SetMessage(LineData.m_sCommand);
            }
        if((*(iter->second))(rResult,mVariableMap,sTestDirectory,LineData)==false)
            {
            return false;
            }
        }

    return true;
    }

bool RunTestFile(SGM::Result                       &rResult,
                 std::map<std::string,SGMFunction> &mFunctionMap,
                 std::string                 const &sTestDirectory,
                 std::string                 const &sFileName,
                 FILE                              *pTestFile,
                 FILE                              *pOutputFile)
    {
    std::map<std::string,std::vector<size_t> > mVariableMap;
    bool bFound=true;
    bool bPassed=true;
    while(bFound)
        {
        std::string sFileLine;
        bFound=SGMInternal::ReadFileLine(pTestFile,sFileLine);
        if(bFound)
            {
            if(RunFileLine(rResult,mFunctionMap,sTestDirectory,mVariableMap,sFileLine,pOutputFile)==false)
                {
                bPassed=false;
                }
            }
        }
    if(pOutputFile)
        {
        if(bPassed)
            {
            fprintf(pOutputFile,"Passed  \"%s\"\n",sFileName.c_str());
            }
        else
            {
            fprintf(pOutputFile,"Failed  \"%s\"\n",sFileName.c_str());
            }
        }
    return bPassed;
    }

namespace SGMInternal
{

bool TestSurface(SGM::Result                &,//rResult,
                 SGMInternal::surface const *pSurface,
                 SGM::Point2D         const &uv1)
    {
    bool bAnswer=true;

    // Test to see if evaluate and inverse match.

    SGM::Point3D Pos,CPos;
    SGM::UnitVector3D Norm;
    SGM::Vector3D dU,dV,dUU,dUV,dVV;
    pSurface->Evaluate(uv1,&Pos,&dU,&dV,&Norm,&dUU,&dUV,&dVV);

    SGM::Point2D uv2=pSurface->Inverse(Pos,&CPos);
    if(SGM::NearEqual(uv1,uv2,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }
    if(SGM::NearEqual(Pos,CPos,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }

    // Test all the derivatives.

    double dx=SGM_FIT,dy=SGM_FIT;
    SGM::Vector3D dNU,dNV,dNUU,dNUV,dNVV;
    SGM::Point3D aMatrix[5][5];
    size_t Index1,Index2;
    for(Index1=0;Index1<5;++Index1)
        {
        double dX=uv1.m_u+dx*(Index1-2.0);
        for(Index2=0;Index2<5;++Index2)
            {
            double dY=uv1.m_v+dy*(Index2-2.0);
            SGM::Point2D uv3(dX,dY);
            SGM::Point3D GridPos;
            pSurface->Evaluate(uv3,&GridPos);
            aMatrix[Index1][Index2]=GridPos;
            }
        }
    SGM::PartialDerivatives<SGM::Point3D,SGM::Vector3D>(aMatrix,dx,dy,dNU,dNV,dNUU,dNUV,dNVV);

    int nContU=pSurface->UContinuity();
    int nContV=pSurface->VContinuity();

    if(0<nContU)
        {
        double dTol=std::max(1.0,dU.Magnitude())*SGM_FIT;
        if(SGM::NearEqual(dU,dNU,dTol)==false)
            {
            bAnswer=false;
            }
        }
    if(0<nContV)
        {
        double dTol=std::max(1.0,dV.Magnitude())*SGM_FIT;
        if(SGM::NearEqual(dV,dNV,dTol)==false)
            {
            bAnswer=false;
            }
        }
    if(1<nContU)
        {
        double dTol=std::max(1.0,dUU.Magnitude())*SGM_FIT;
        if(SGM::NearEqual(dUU,dNUU,dTol)==false)
            {
            bAnswer=false;
            }
        }
    if(0<nContU && 0<nContV)
        {
        double dTol=std::max(1.0,dUV.Magnitude())*SGM_FIT;
        if(SGM::NearEqual(dUV,dNUV,dTol)==false)
            {
            bAnswer=false;
            }
        }
    if(1<nContV)
        {
        double dTol=std::max(1.0,dVV.Magnitude())*SGM_FIT;
        if(SGM::NearEqual(dVV,dNVV,dTol)==false)
            {
            bAnswer=false;
            }
        }

    // Test parameter curves.
#if 0
    SGM::Interval2D const &Domain=pSurface->GetDomain();
    curve *pUParamLine=pSurface->UParamLine(rResult,Domain.m_UDomain.MidPoint());
    curve *pVParamLine=pSurface->VParamLine(rResult,Domain.m_VDomain.MidPoint());

    SGM::Point3D PosU,PosV;
    pUParamLine->Evaluate(pUParamLine->GetDomain().MidPoint(),&PosU);
    pVParamLine->Evaluate(pVParamLine->GetDomain().MidPoint(),&PosV);
    SGM::Point3D SurfPosU,SurfPosV;
    SGM::Point2D Uuv=pSurface->Inverse(PosU,&SurfPosU);
    SGM::Point2D Vuv=pSurface->Inverse(PosV,&SurfPosV);
    SGM::Point3D Zero(0,0,0);
    double dParamUTol=SGM_FIT*PosU.Distance(Zero);
    if(SGM::NearEqual(PosU,SurfPosU,dParamUTol)==false)
        {
        bAnswer=false;
        }
    double dParamVTol=SGM_FIT*PosV.Distance(Zero);
    if(SGM::NearEqual(PosV,SurfPosV,dParamVTol)==false)
        {
        bAnswer=false;
        }

    rResult.GetThing()->DeleteEntity(pUParamLine);
    rResult.GetThing()->DeleteEntity(pVParamLine);
#endif
    return bAnswer;
    }

bool TestCurve(SGMInternal::curve const *pCurve,
               double                    t1)
    {
    SGM::Point3D Pos;
    SGM::Vector3D Vec1,Vec2;
    pCurve->Evaluate(t1,&Pos,&Vec1,&Vec2);
    SGM::Point3D ClosePos;
    double t2=pCurve->Inverse(Pos,&ClosePos);

    bool bAnswer=true;
    if(SGM::NearEqual(Pos,ClosePos,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }
    if(SGM::NearEqual(t1,t2,SGM_MIN_TOL,false)==false)
        {
        bAnswer=false;
        }

    double h=SGM_MIN_TOL;
    SGM::Point3D Pos0,Pos1,Pos2,Pos3;
    pCurve->Evaluate(t1-2*h,&Pos0);
    pCurve->Evaluate(t1-h,&Pos1);
    pCurve->Evaluate(t1+h,&Pos2);
    pCurve->Evaluate(t1+2*h,&Pos3);

    SGM::Vector3D VecN1=SGM::FirstDerivative<SGM::Point3D,SGM::Vector3D>(Pos0,Pos1,Pos2,Pos3,h);
    SGM::Vector3D VecN2=SGM::SecondDerivative<SGM::Point3D,SGM::Vector3D>(Pos0,Pos1,Pos,Pos2,Pos3,h);

    double dDist=std::max(1.0,sqrt(Pos0.m_x*Pos0.m_x+Pos0.m_y*Pos0.m_y+Pos0.m_z*Pos0.m_z));
    if(0<pCurve->Continuity())
        {
        double dMag1=VecN1.Magnitude();
        double dTol1=std::max(dMag1,dDist)*SGM_FIT;
        if(SGM::NearEqual(Vec1,VecN1,dTol1)==false)
            {
            bAnswer=false;
            }
        }

    double dMag2=VecN2.Magnitude();
    double dTol2=std::max(dMag2,dDist)*SGM_FIT;
    if(1<pCurve->Continuity() && SGM::NearEqual(Vec2,VecN2,dTol2)==false)
        {
        bAnswer=false;
        }

    return bAnswer;
    }  
  
} // SGMIntneral namespace
  

bool TestIntersections(SGM::Result                &rResult,
                       SGMInternal::surface const *pSurface1,
                       SGMInternal::surface const *pSurface2,
                       size_t                      nExpectedCurves)
    {
    bool bAnswer=true;
    std::vector<SGMInternal::curve *> aCurves;
    size_t nCurves=SGMInternal::IntersectSurfaces(rResult,pSurface1,pSurface2,aCurves,nullptr,nullptr,SGM_MIN_TOL);
    if(nCurves!=nExpectedCurves)
        {
        bAnswer=false;
        }
    else
        {
        size_t Index1,Index2;
        size_t nTestPoint=10;
        for(Index1=0;bAnswer && Index1<nCurves;++Index1)
            {
            SGMInternal::curve *pCurve=aCurves[Index1];
            double dLength=pCurve->FindLength(pCurve->GetDomain(),SGM_MIN_TOL);
            double dTol=SGM_FIT*dLength;
            for(Index2=1;Index2<nTestPoint;++Index2)
                {
                double dFraction=Index2/(nTestPoint-1.0);
                double t=pCurve->GetDomain().MidPoint(dFraction);
                SGM::Point3D Pos;
                pCurve->Evaluate(t,&Pos);
                SGM::Point3D CPos1,CPos2;
                pSurface1->Inverse(Pos,&CPos1);
                pSurface2->Inverse(Pos,&CPos2);
                if(dTol<Pos.Distance(CPos1) && dTol<Pos.Distance(CPos2))
                    {
                    bAnswer=false;
                    break;
                    }
                }
            }
        }
    return bAnswer;
    }


bool SGM::CompareSizes(size_t nSize1,size_t nSize2)
    {
    return nSize1==nSize2;
    }

bool SGM::RunTestFile(SGM::Result       &rResult,
                      std::string const &sTestDirectory,
                      std::string const &sTestFileName,
                      std::string const &sOutputFileName)
    {
    std::map<std::string,SGMFunction> mFunctionMap;
    CreateFunctionMap(mFunctionMap);
    FILE *pOutputFile = fopen(sOutputFileName.c_str(),"w");
    if(pOutputFile==nullptr)
        {
        rResult.SetResult(SGM::ResultType::ResultTypeFileOpen);
        rResult.SetMessage(sOutputFileName);
        return false;
        }

    std::string sFullPathName=sTestDirectory+"/"+sTestFileName;
    FILE *pTestFile = fopen(sFullPathName.c_str(),"rt");
    if(pTestFile==nullptr)
        {
        rResult.SetResult(SGM::ResultType::ResultTypeFileOpen);
        rResult.SetMessage(sFullPathName);
        return false;
        }
    bool bAnswer=RunTestFile(rResult,mFunctionMap,sTestDirectory,sTestFileName,pTestFile,pOutputFile);
    fclose(pTestFile);
    fclose(pOutputFile);
    return bAnswer;
    }

void SGM::RunTestDirectory(SGM::Result       &rResult,
                           std::string const &sTestDirectory,
                           std::string const &sOutputFileName)
    {
    ///////////////////////////////////////////////////////////////////////////
    //
    //  Temp code to test STEP read.
    //
    ///////////////////////////////////////////////////////////////////////////

    //std::vector<Entity> aEntities;
    //std::vector<std::string> aLog;
    //SGM::TranslatorOptions Options;
    //SGM::ReadFile(rResult,sTestDirectory+"/Sphere.stp",aEntities,aLog,Options);

    ///////////////////////////////////////////////////////////////////////////
    //
    //  End of Temp code to test STEP read.
    //
    ///////////////////////////////////////////////////////////////////////////
    
    std::vector<std::string> aFileNames;
    SGMInternal::ReadDirectory(sTestDirectory,aFileNames);

    FILE *pOutputFile = fopen(sOutputFileName.c_str(),"w");
    std::map<std::string,SGMFunction> mFunctionMap;
    CreateFunctionMap(mFunctionMap);

    size_t nFiles=aFileNames.size();
    size_t nPassed=0,nFailed=0;
    size_t Index1;
    for(Index1=0;Index1<nFiles;++Index1)
        {
        if(aFileNames[Index1].c_str()[0]!='.')
            {
            std::string sExtension;
            SGMInternal::FindFileExtension(aFileNames[Index1],sExtension);
            if(sExtension=="txt")
                {
                std::string FullPath=sTestDirectory;
                FullPath+="/";
                FullPath+=aFileNames[Index1];
                FILE *pTestFile = fopen(FullPath.c_str(),"rt");
                try
                    {
                    if(RunTestFile(rResult,mFunctionMap,sTestDirectory,aFileNames[Index1],pTestFile,pOutputFile))
                        {
                        ++nPassed;
                        }
                    else
                        {
                        ++nFailed;
                        }
                    }
                catch(...)
                    {
                    ++nFailed;
                    }
                fclose(pTestFile);
                }
            }
        }
    fprintf(pOutputFile,"\n%lu Passed %lu Failed\n",nPassed,nFailed);
    fclose(pOutputFile);
    }

bool SGM::CompareFiles(SGM::Result       &rResult,
                       std::string const &sFile1,
                       std::string const &sFile2)
    {
    // Find the file types.

    std::string Ext1,Ext2;
    SGMInternal::FindFileExtension(sFile1,Ext1);
    SGMInternal::FindFileExtension(sFile2,Ext2);
    if(Ext1!=Ext2)
        {
        return false;
        }

    // Open the files.

    FILE *pFile1 = fopen(sFile1.c_str(),"rt");
    FILE *pFile2 = fopen(sFile2.c_str(),"rt");

    // Compare the files.

    bool bAnswer=false;
    if(Ext1=="stl")
        {
        bAnswer=true;
        SGM::TranslatorOptions Options;
        std::vector<std::string> aLog;
        std::vector<SGM::Entity> aEntities1,aEntities2;
        size_t nEntities1=ReadFile(rResult,sFile1,aEntities1,aLog,Options);
        size_t nEntities2=ReadFile(rResult,sFile2,aEntities2,aLog,Options);
        SGMInternal::thing *pThing=rResult.GetThing();
        std::vector<double> aAreas1,aAreas2;
        aAreas1.reserve(nEntities1);
        aAreas2.reserve(nEntities2);
        size_t Index1;
        for(Index1=0;Index1<nEntities1;++Index1)
            {
            SGMInternal::complex *pComplex=(SGMInternal::complex *)(pThing->FindEntity(aEntities1[Index1].m_ID));
            aAreas1.push_back(pComplex->Area());
            pThing->DeleteEntity(pComplex);
            }
        for(Index1=0;Index1<nEntities1;++Index1)
            {
            SGMInternal::complex *pComplex=(SGMInternal::complex *)(pThing->FindEntity(aEntities2[Index1].m_ID));
            aAreas2.push_back(pComplex->Area());
            pThing->DeleteEntity(pComplex);
            }
        if(nEntities1==nEntities2)
            {
            std::sort(aAreas1.begin(),aAreas1.end());
            std::sort(aAreas2.begin(),aAreas2.end());
            bAnswer=true;
            for(Index1=0;Index1<nEntities1;++Index1)
                {
                double dArea1=aAreas1[Index1];
                double dArea2=aAreas1[Index1];
                if(SGM::NearEqual(dArea1,dArea2,0.01,true)==false)
                    {
                    bAnswer=false;
                    break;
                    }
                }
            }
        }
    else if(Ext1=="spt")
        {
        SGMInternal::ReadToString(pFile1,"Data;");
        SGMInternal::ReadToString(pFile2,"Data;");
        bAnswer=true;
        while(bAnswer)
            {
            char data1,data2;
            fread(&data1,1,1,pFile1);
            fread(&data2,1,1,pFile2);
            if(data1!=data2)
                {
                bAnswer=false;
                }
            }
        }

    fclose(pFile1);
    fclose(pFile2);
    return bAnswer;
    }

double TestIntegrand(double x,void const *)
    {
    return 4.0/(1.0+x*x);
    }

double TestIntegrand2D(SGM::Point2D const &uv,void const *)
    {
    double x=uv.m_u;
    double y=uv.m_v;
    return x*x+4*y;
    }

bool Test_IntersectNonParallelPlanes(SGM::Point3D       const &Origin1,
                                     SGM::UnitVector3D  const &Norm1,
                                     SGM::Point3D       const &Origin2,
                                     SGM::UnitVector3D  const &Norm2)
{
        SGM::Point3D LineOrigin;
        SGM::UnitVector3D LineDirection;
        SGMInternal::IntersectNonParallelPlanes(Origin1, Norm1, Origin2, Norm2, LineOrigin, LineDirection);

        double dDistLineOriginToPlane1 = ((LineOrigin - Origin1) % Norm1);
        double dDistLineOriginToPlane2 = ((LineOrigin - Origin2) % Norm2);

        SGM::Point3D Pos2 = LineOrigin + 7 * LineDirection;

        double dDistPos2ToPlane1 = ((Pos2 - Origin1) % Norm1);
        double dDistPos2ToPlane2 = ((Pos2 - Origin2) % Norm2);

        return (dDistLineOriginToPlane1 < SGM_MIN_TOL &&
                dDistLineOriginToPlane2 < SGM_MIN_TOL &&
                dDistPos2ToPlane1       < SGM_MIN_TOL &&
                dDistPos2ToPlane2       < SGM_MIN_TOL);
}

bool SGM::RunCPPTest(SGM::Result &rResult,
                     size_t       nTestNumber)
    {
    if(nTestNumber==1)
        {
        // Test the quartic equation.

        bool bAnswer=true;

        // 2*(x-1)(x-2)(x-3)(x-4) -> 2*x^4-20*x^3+70*x^2-100*x+48 Four roots

        std::vector<double> aRoots;
        size_t nRoots=SGM::Quartic(2,-20,70,-100,48,aRoots,SGM_MIN_TOL);
        if( nRoots!=4 || 
            SGM_ZERO<fabs(aRoots[0]-1) ||
            SGM_ZERO<fabs(aRoots[1]-2) ||
            SGM_ZERO<fabs(aRoots[2]-3) ||
            SGM_ZERO<fabs(aRoots[3]-4))
            {
            bAnswer=false;
            }

        // (x-1)(x-2)(x-3)(x-3) -> x^4-9*x^3+29*x^2-39*x+18 Three roots, one double

        aRoots.clear();
        nRoots=SGM::Quartic(1,-9,29,-39,18,aRoots,SGM_MIN_TOL);
        if( nRoots!=3 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2) ||
            SGM_ZERO<fabs(aRoots[2]-3))
            {
            bAnswer=false;
            }

        // (x-1)(x-2)(x-2)(x-2) -> x^4-7*x^3+18*x^2-20*x+8 Two roots, one triple

        aRoots.clear();
        nRoots=SGM::Quartic(1,-7,18,-20,8,aRoots,SGM_MIN_TOL);
        if( nRoots!=2 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2))
            {
            bAnswer=false;
            }

        // (x-1)(x-1)(x-2)(x-2) -> x^4-6*x^3+13*x^2-12*x+4 Two double roots

        aRoots.clear();
        nRoots=SGM::Quartic(1,-6,13,-12,4,aRoots,SGM_MIN_TOL);
        if( nRoots!=2 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2))
            {
            bAnswer=false;
            }

        // (x-1)(x-2)(x^2+1) -> x^4-3*x^3+3*x^2-3*x+2 Two roots

        aRoots.clear();
        nRoots=SGM::Quartic(1,-3,3,-3,2,aRoots,SGM_MIN_TOL);
        if( nRoots!=2 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2))
            {
            bAnswer=false;
            }

        // (x-1)(x-1)(x^2+1) -> x^4-2*x^3+2*x^2-2*x+1 One double root.

        aRoots.clear();
        nRoots=SGM::Quartic(1,-2,2,-2,1,aRoots,SGM_MIN_TOL);
        if( nRoots!=1 || 
            SGM_ZERO<fabs(aRoots[0]-1))
            {
            bAnswer=false;
            }

        // (x-1)(x-1)(x-1)(x-1) -> x^4-4*x^3+6*x^2-4*x+1 One quadruple root.

        aRoots.clear();
        nRoots=SGM::Quartic(1,-4,6,-4,1,aRoots,SGM_MIN_TOL);
        if( nRoots!=1 || 
            SGM_ZERO<fabs(aRoots[0]-1))
            {
            bAnswer=false;
            }

        // (x^2+1)(x^2+1) -> x^4+2*x^2+1 No roots.

        aRoots.clear();
        nRoots=SGM::Quartic(1,0,2,0,1,aRoots,SGM_MIN_TOL);
        if( nRoots!=0 )
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==2)
        {
        // Test the cubic equation.

        bool bAnswer=true;

        // 2*(x-1)*(x-2)*(x-3)=0 -> 2*x^3-12*x^2+22*x-12=0 Three roots

        std::vector<double> aRoots;
        size_t nRoots=SGM::Cubic(2,-12,22,-12,aRoots);
        if( nRoots!=3 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2) ||
            SGM_ZERO<fabs(aRoots[2]-3))
            {
            bAnswer=false;
            }

        // (x-1)*(x-2)*(x-2)=0 -> x^3-5*x^2+8*x-4=0 Two roots, one double

        aRoots.clear();
        nRoots=SGM::Cubic(1,-5,8,-4,aRoots);
        if( nRoots!=2 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2))
            {
            bAnswer=false;
            }

        // (x-1)*(x^2+1)=0 -> x^3-x^2+x-1=0 One root

        aRoots.clear();
        nRoots=SGM::Cubic(1,-1,1,-1,aRoots);
        if( nRoots!=1 || 
            SGM_ZERO<fabs(aRoots[0]-1))
            {
            bAnswer=false;
            }

        // (x-1)*(x-1)*(x-1)=0 -> x^3-x^2+x-1=0 One triple root

        aRoots.clear();
        nRoots=SGM::Cubic(1,-3,3,-1,aRoots);
        if( nRoots!=1 || 
            SGM_ZERO<fabs(aRoots[0]-1))
            {
            bAnswer=false;
            }

        // (x-1)*(x-2)=0 -> x^2-3*x+2=0 Two roots and degenerate

        aRoots.clear();
        nRoots=SGM::Cubic(0,1,-3,2,aRoots);
        if( nRoots!=2 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2))
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==3)
        {
        // Test plane inverse.

        SGM::Point3D Origin(10,11,12);
        SGM::UnitVector3D XAxis(1,2,3);
        SGM::UnitVector3D YAxis=XAxis.Orthogonal();
        SGM::UnitVector3D ZAxis=XAxis*YAxis;
        SGMInternal::plane *pPlane=new SGMInternal::plane(rResult,Origin,XAxis,YAxis,ZAxis);

        bool bAnswer=SGMInternal::TestSurface(rResult,pPlane,SGM::Point2D(0.5,0.2));
        rResult.GetThing()->DeleteEntity(pPlane);

        return bAnswer;
        }

    if(nTestNumber==4)
        {
        // Test sphere inverse.

        SGM::Point3D Origin(10,11,12);
        SGM::UnitVector3D XAxis(1,2,3);
        SGM::UnitVector3D YAxis=XAxis.Orthogonal();
        double dRadius=2.5;
        SGMInternal::sphere *pSphere=new SGMInternal::sphere(rResult,Origin,dRadius,&XAxis,&YAxis);

        bool bAnswer=SGMInternal::TestSurface(rResult,pSphere,SGM::Point2D(0.5,0.2));
        rResult.GetThing()->DeleteEntity(pSphere);

        return bAnswer;
        }

    if(nTestNumber==5)
        {
        // Test cylinder inverse.

        SGM::Point3D Bottom(10,11,12),Top(13,14,15);
        double dRadius=2.5;
        SGMInternal::cylinder *pCylinder=new SGMInternal::cylinder(rResult,Bottom,Top,dRadius);

        bool bAnswer=SGMInternal::TestSurface(rResult,pCylinder,SGM::Point2D(0.5,0.2));
        rResult.GetThing()->DeleteEntity(pCylinder);

        return bAnswer;
        }

    if(nTestNumber==6)
        {
        // Test torus inverse.

        SGM::Point3D Origin(0,0,0);
        SGM::UnitVector3D ZAxis(0,0,1);
        SGMInternal::torus *pTorus=new SGMInternal::torus(rResult,Origin,ZAxis,2,5,true);

        bool bAnswer=SGMInternal::TestSurface(rResult,pTorus,SGM::Point2D(0.5,0.2));
        rResult.GetThing()->DeleteEntity(pTorus);

        return bAnswer;
        }

    if(nTestNumber==7)
        {
        // Test cone inverse.

        SGM::Point3D Origin(10,11,12);
        SGM::UnitVector3D ZAxis(1,2,3);
        SGMInternal::cone *pCone=new SGMInternal::cone(rResult,Origin,ZAxis,2,0.4);

        bool bAnswer=SGMInternal::TestSurface(rResult,pCone,SGM::Point2D(0.5,0.2));
        rResult.GetThing()->DeleteEntity(pCone);

        return bAnswer;
        }

    if(nTestNumber==8)
        {
        std::vector<double> aKnots={0,0,0,0,0.5,1,1,1,1};
        std::vector<SGM::Point3D> aControlPoints;
        aControlPoints.emplace_back(1,1,0);
        aControlPoints.emplace_back(1.166666666666666,1.166666666666666,0);
        aControlPoints.emplace_back(2,2.8333333333333333,0);
        aControlPoints.emplace_back(2.8333333333333333,1.166666666666666,0);
        aControlPoints.emplace_back(3,1,0);

        SGMInternal::NUBcurve *pNUB=new SGMInternal::NUBcurve(rResult,aControlPoints,aKnots);

        bool bAnswer=TestCurve(pNUB,0.45);
        rResult.GetThing()->DeleteEntity(pNUB);

        return bAnswer;
        }

    if(nTestNumber==9)
        {
        SGM::Point3D Pos0(1,2,3),Pos1(4,5,6);
        SGM::UnitVector3D Axis(7,8,9);

        SGMInternal::line *pLine1=new SGMInternal::line(rResult,Pos0,Pos1);
        bool bAnswer=TestCurve(pLine1,0.5);
        rResult.GetThing()->DeleteEntity(pLine1);

        SGMInternal::line *pLine2=new SGMInternal::line(rResult,Pos0,Axis);
        if(TestCurve(pLine2,0.5)==false)
            {
            bAnswer=false;
            }

        SGMInternal::line *pLine3=pLine2->Clone(rResult);
        if(TestCurve(pLine3,0.5)==false)
            {
            bAnswer=false;
            }
        rResult.GetThing()->DeleteEntity(pLine2);
        rResult.GetThing()->DeleteEntity(pLine3);

        return bAnswer;
        }

    if(nTestNumber==10)
        {
        SGM::Point3D Center(1,2,3);
        SGM::UnitVector3D Normal(4,5,6);
        double dRadius=2.1;
        SGM::Interval1D Domain(-1,1);
        SGM::UnitVector3D XAxis=Normal.Orthogonal();
   
        SGMInternal::circle *pCircle1=new SGMInternal::circle(rResult,Center,Normal,dRadius,&XAxis,&Domain);
        bool bAnswer=TestCurve(pCircle1,0.5);
        rResult.GetThing()->DeleteEntity(pCircle1);

        SGMInternal::circle *pCircle2=new SGMInternal::circle(rResult,Center,Normal,dRadius,&XAxis);
        if(TestCurve(pCircle2,0.5)==false)
            {
            bAnswer=false;
            }
        rResult.GetThing()->DeleteEntity(pCircle2);

        SGMInternal::circle *pCircle3=new SGMInternal::circle(rResult,Center,Normal,dRadius);
        if(TestCurve(pCircle3,0.5)==false)
            {
            bAnswer=false;
            }

        SGMInternal::circle *pCircle4=pCircle3->Clone(rResult);
        if(TestCurve(pCircle4,0.5)==false)
            {
            bAnswer=false;
            }
        rResult.GetThing()->DeleteEntity(pCircle3);
        rResult.GetThing()->DeleteEntity(pCircle4);

        return bAnswer;
        }

    if(nTestNumber==11)
        {
        bool bAnswer=true;

        SGM::Point3D Origin(0.0,0.0,0.0);
        SGM::UnitVector3D ZAxis(0.0,0.0,1.0);
        SGMInternal::torus *pTorus=new SGMInternal::torus(rResult,Origin,ZAxis,2,5,true);
        SGM::Point2D uv(0.0,0.0);
        SGM::UnitVector3D Vec1,Vec2;
        double k1,k2;
        pTorus->PrincipleCurvature(uv,Vec1,Vec2,k1,k2);
        rResult.GetThing()->DeleteEntity(pTorus);

        if(SGM::NearEqual(Vec1,SGM::UnitVector3D(0.0,1.0,0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(Vec2,SGM::UnitVector3D(0.0,0.0,1.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(k1,-7.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(k2,-2.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==12)
        {
        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints;
        aPoints.reserve(5);
        //aPoints.emplace_back(0,0,0);
        //aPoints.emplace_back(3,4,0);
        //aPoints.emplace_back(-1,4,0);
        //aPoints.emplace_back(-4,0,0);
        //aPoints.emplace_back(-4,-3,0);

        aPoints.emplace_back(-2,0,0);
        aPoints.emplace_back(-1,0.1,0);
        aPoints.emplace_back(0.0,0.0,0);
        aPoints.emplace_back(1,0.1,0);
        aPoints.emplace_back(2,0,0);

        SGM::Curve NUBID=SGM::CreateNUBCurve(rResult,aPoints);
        size_t Index1;
        for(Index1=0;Index1<5;++Index1)
            {
            SGM::Point3D ClosePos;
            SGM::CurveInverse(rResult,NUBID,aPoints[Index1],&ClosePos);
            if(SGM::NearEqual(aPoints[Index1],ClosePos,SGM_MIN_TOL)==false)
                {
                bAnswer=false;
                }
            }

        return bAnswer;
        }

    if(nTestNumber==13)
        {
        bool bAnswer=true;

        // if x=1,y=2,z=3,w=4,v=5,r=6 then
        //
        // 1x+2y+3z+0w+0v+0r=14
        // 2x+1y+1z+1w+0v+0r=11
        // 1x+2y+1z+1w+0v+0r=12
        // 0x+1y+2z-1w+1v+0r=9
        // 0x+0y-1z+1w+2v+1r=17
        // 0x+0y+0z-1w-1v+2r=3

        std::vector<std::vector<double> > aaMatrix;
        aaMatrix.reserve(5);
        std::vector<double> aRow;
        aRow.reserve(6);
        aRow.push_back(0);
        aRow.push_back(0);
        aRow.push_back(1);
        aRow.push_back(2);
        aRow.push_back(3);
        aRow.push_back(14);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(0);
        aRow.push_back(2);
        aRow.push_back(1);
        aRow.push_back(1);
        aRow.push_back(1);
        aRow.push_back(11);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(1);
        aRow.push_back(2);
        aRow.push_back(1);
        aRow.push_back(1);
        aRow.push_back(0);
        aRow.push_back(12);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(1);
        aRow.push_back(2);
        aRow.push_back(-1);
        aRow.push_back(1);
        aRow.push_back(0);
        aRow.push_back(9);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(-1);
        aRow.push_back(1);
        aRow.push_back(2);
        aRow.push_back(1);
        aRow.push_back(0);
        aRow.push_back(17);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(-1);
        aRow.push_back(-1);
        aRow.push_back(2);
        aRow.push_back(0);
        aRow.push_back(0);
        aRow.push_back(3);
        aaMatrix.push_back(aRow);

        if(SGM::BandedSolve(aaMatrix)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[0].back(),1,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[1].back(),2,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[2].back(),3,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[3].back(),4,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[4].back(),5,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==14)
        {
        bool bAnswer=true;

        // if x=1,y=2,z=3,w=4, then
        //
        // 1x+2y+0z+1w= 9
        // 2x+2y+2z+0w=12
        // 0x+2y-1z+3w=13
        // 1x+1y+2z-1w= 5

        std::vector<std::vector<double> > aaMatrix;
        aaMatrix.reserve(4);
        std::vector<double> aRow;
        aRow.reserve(4);
        aRow.push_back(1);
        aRow.push_back(2);
        aRow.push_back(0);
        aRow.push_back(1);
        aRow.push_back(9);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(2);
        aRow.push_back(2);
        aRow.push_back(2);
        aRow.push_back(0);
        aRow.push_back(12);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(0);
        aRow.push_back(2);
        aRow.push_back(-1);
        aRow.push_back(3);
        aRow.push_back(13);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(1);
        aRow.push_back(1);
        aRow.push_back(2);
        aRow.push_back(-1);
        aRow.push_back(5);
        aaMatrix.push_back(aRow);

        if(SGM::LinearSolve(aaMatrix)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[0].back(),1,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[1].back(),2,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[2].back(),3,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[3].back(),4,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==15)
        {
        // Fitting a NUB curve two three points with end vectors.

        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints;
        aPoints.reserve(3);
        aPoints.emplace_back(1,1,0);
        aPoints.emplace_back(2,2,0);
        aPoints.emplace_back(3,1,0);

        SGM::Vector3D StartVec(1,1,0),EndVec(1,-1,0);

        SGM::Curve NUBID=SGM::CreateNUBCurveWithEndVectors(rResult,aPoints,StartVec,EndVec);
        size_t Index1;
        for(Index1=0;Index1<3;++Index1)
            {
            SGM::Point3D ClosePos;
            SGM::CurveInverse(rResult,NUBID,aPoints[Index1],&ClosePos);
            if(SGM::NearEqual(aPoints[Index1],ClosePos,SGM_MIN_TOL)==false)
                {
                bAnswer=false;
                }
            }
        SGM::Vector3D Vec0,Vec1;
        SGM::EvaluateCurve(rResult,NUBID,0.0,nullptr,&Vec0);
        SGM::EvaluateCurve(rResult,NUBID,1.0,nullptr,&Vec1);
        if(SGM::NearEqual(Vec0,StartVec,SGM_MIN_TOL)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(Vec1,EndVec,SGM_MIN_TOL)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }
    
    if(nTestNumber==16)
        {
        // Fitting a NUB curve to three points with end vectors.

        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints;
        aPoints.reserve(4);
        aPoints.emplace_back(1,1,0);
        aPoints.emplace_back(2,2,0);
        aPoints.emplace_back(3,1,0);
        aPoints.emplace_back(5,0,0);

        SGM::Vector3D StartVec(1,1,0),EndVec(1,-1,0);

        SGM::Curve NUBID=SGM::CreateNUBCurveWithEndVectors(rResult,aPoints,StartVec,EndVec);
        size_t Index1;
        for(Index1=0;Index1<4;++Index1)
            {
            SGM::Point3D ClosePos;
            SGM::CurveInverse(rResult,NUBID,aPoints[Index1],&ClosePos);
            if(SGM::NearEqual(aPoints[Index1],ClosePos,SGM_MIN_TOL)==false)
                {
                bAnswer=false;
                }
            }
        SGM::Vector3D Vec0,Vec1;
        SGM::EvaluateCurve(rResult,NUBID,0.0,nullptr,&Vec0);
        SGM::EvaluateCurve(rResult,NUBID,1.0,nullptr,&Vec1);
        if(SGM::NearEqual(Vec0,StartVec,SGM_MIN_TOL)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(Vec1,EndVec,SGM_MIN_TOL)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==17)
        {
        // Fitting a NUB curve to two points with end vectors.

        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints;
        aPoints.reserve(2);
        aPoints.emplace_back(1,1,0);
        aPoints.emplace_back(3,1,0);

        SGM::Vector3D StartVec(1,1,0),EndVec(1,-1,0);

        SGM::Curve NUBID=SGM::CreateNUBCurveWithEndVectors(rResult,aPoints,StartVec,EndVec);
        size_t Index1;
        for(Index1=0;Index1<2;++Index1)
            {
            SGM::Point3D ClosePos;
            SGM::CurveInverse(rResult,NUBID,aPoints[Index1],&ClosePos);
            if(SGM::NearEqual(aPoints[Index1],ClosePos,SGM_MIN_TOL)==false)
                {
                bAnswer=false;
                }
            }
        SGM::Vector3D Vec0,Vec1;
        SGM::EvaluateCurve(rResult,NUBID,0.0,nullptr,&Vec0);
        SGM::EvaluateCurve(rResult,NUBID,1.0,nullptr,&Vec1);
        if(SGM::NearEqual(Vec0,StartVec,SGM_MIN_TOL)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(Vec1,EndVec,SGM_MIN_TOL)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==18)
        {
        // Fitting a NUB curve to three points.
        // Which requires it be degree two.

        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints;
        aPoints.reserve(3);
        aPoints.emplace_back(1,1,0);
        aPoints.emplace_back(2,2,0);
        aPoints.emplace_back(3,1,0);

        SGM::Curve NUBID=SGM::CreateNUBCurve(rResult,aPoints);
        size_t Index1;
        for(Index1=0;Index1<3;++Index1)
            {
            SGM::Point3D ClosePos;
            SGM::CurveInverse(rResult,NUBID,aPoints[Index1],&ClosePos);
            if(SGM::NearEqual(aPoints[Index1],ClosePos,SGM_MIN_TOL)==false)
                {
                bAnswer=false;
                }
            }
        return bAnswer;
        }

    if(nTestNumber==19)
        {
        // Fitting a NUB curve to two points.
        // Which requires it be degree one.

        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints;
        aPoints.reserve(2);
        aPoints.emplace_back(1,1,0);
        aPoints.emplace_back(3,1,0);

        SGM::Curve NUBID=SGM::CreateNUBCurve(rResult,aPoints);
        size_t Index1;
        for(Index1=0;Index1<2;++Index1)
            {
            SGM::Point3D ClosePos;
            SGM::CurveInverse(rResult,NUBID,aPoints[Index1],&ClosePos);
            if(SGM::NearEqual(aPoints[Index1],ClosePos,SGM_MIN_TOL)==false)
                {
                bAnswer=false;
                }
            }

        return bAnswer;
        }

    if(nTestNumber==20)
        {
        // Test NUB surface.

        std::vector<double> aUKnots,aVKnots;
        aUKnots.push_back(0.0);
        aUKnots.push_back(0.0);
        aUKnots.push_back(0.0);
        aUKnots.push_back(1.0);
        aUKnots.push_back(1.0);
        aUKnots.push_back(1.0);
        aVKnots=aUKnots;
        std::vector<std::vector<SGM::Point3D> > aaPoints;
        std::vector<SGM::Point3D> aPoints;
        aPoints.assign(3,SGM::Point3D(0,0,0));
        aaPoints.push_back(aPoints);
        aaPoints.push_back(aPoints);
        aaPoints.push_back(aPoints);
        aaPoints[0][0]=SGM::Point3D(0.0,0.0,1.0);
        aaPoints[0][1]=SGM::Point3D(0.0,1.0,0.0);
        aaPoints[0][2]=SGM::Point3D(0.0,2.0,-1.0);
        aaPoints[1][0]=SGM::Point3D(1.0,0.0,0.0);
        aaPoints[1][1]=SGM::Point3D(1.0,1.0,0.0);
        aaPoints[1][2]=SGM::Point3D(1.0,2.0,0.0);
        aaPoints[2][0]=SGM::Point3D(2.0,0.0,-1.0);
        aaPoints[2][1]=SGM::Point3D(2.0,1.0,0.0);
        aaPoints[2][2]=SGM::Point3D(2.0,2.0,1.0);
        SGMInternal::NUBsurface *pNUB=new SGMInternal::NUBsurface(rResult,std::move(aaPoints),std::move(aUKnots),std::move(aVKnots));

        bool bAnswer=SGMInternal::TestSurface(rResult,pNUB,SGM::Point2D(0.3,0.2));
        
        SGM::UnitVector3D Vec1,Vec2;
        double k1,k2;
        SGM::Point2D uv(0.5,0.5);
        pNUB->PrincipleCurvature(uv,Vec1,Vec2,k1,k2);

        if(SGM::NearEqual(Vec1,SGM::UnitVector3D(1.0,-1.0,0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(Vec2,SGM::UnitVector3D(1.0,1.0,0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(k1,-4.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(k2,4.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        rResult.GetThing()->DeleteEntity(pNUB);

        return bAnswer;
        }

    if(nTestNumber==21)
        {
        // Test Least Squared Plane Fitting.

        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints;
        aPoints.emplace_back(0.0,0.0,0.0);
        aPoints.emplace_back(2.0,0.0,0.0);
        aPoints.emplace_back(2.0,1.0,0.0);
        aPoints.emplace_back(0.0,1.0,0.0);
        aPoints.emplace_back(1.0,0.5,0.4);

        SGM::Point3D Origin;
        SGM::UnitVector3D XVec,YVec,ZVec;
        SGM::FindLeastSquarePlane(aPoints,Origin,XVec,YVec,ZVec);

        if(SGM::NearEqual(Origin,SGM::Point3D(1.0,0.5,0.08),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(XVec,SGM::UnitVector3D(1.0,0.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(YVec,SGM::UnitVector3D(0.0,1.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(ZVec,SGM::UnitVector3D(0.0,0.0,1.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        aPoints.emplace_back(1.0,0.0,0.0);
        aPoints.emplace_back(0.0,1.0,0.0);
        aPoints.emplace_back(3.0,2.0,0.0);
        aPoints.emplace_back(2.0,3.0,0.0);

        SGM::FindLeastSquarePlane(aPoints,Origin,XVec,YVec,ZVec);

        if(SGM::NearEqual(Origin,SGM::Point3D(1.5,1.5,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(XVec,SGM::UnitVector3D(1.0,1.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(YVec,SGM::UnitVector3D(1.0,-1.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(ZVec,SGM::UnitVector3D(0.0,0.0,-1.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        aPoints.emplace_back(1.0,0.0,0.0);
        aPoints.emplace_back(0.0,1.0,0.0);
        aPoints.emplace_back(1.0,1.0,0.0);
        aPoints.emplace_back(0.0,0.0,0.0);
        aPoints.emplace_back(1.0,0.0,1.0);
        aPoints.emplace_back(0.0,1.0,1.0);
        aPoints.emplace_back(1.0,1.0,1.0);
        aPoints.emplace_back(0.0,0.0,1.0);

        SGM::FindLeastSquarePlane(aPoints,Origin,XVec,YVec,ZVec);

        if(SGM::NearEqual(Origin,SGM::Point3D(0.5,0.5,0.5),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(XVec,SGM::UnitVector3D(1.0,0.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(YVec,SGM::UnitVector3D(0.0,1.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(ZVec,SGM::UnitVector3D(0.0,0.0,1.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        aPoints.emplace_back(0.0,0.0,0.0);
        aPoints.emplace_back(8.0,8.0,0.0);
        aPoints.emplace_back(4.0,4.0,0.0);

        SGM::FindLeastSquarePlane(aPoints,Origin,XVec,YVec,ZVec);

        if(SGM::NearEqual(Origin,SGM::Point3D(4.0,4.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(XVec,SGM::UnitVector3D(1.0,1.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(YVec,SGM::UnitVector3D(-1.0,1.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(ZVec,SGM::UnitVector3D(0.0,0.0,1.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        aPoints.emplace_back(0.0,0.0,0.0);
        aPoints.emplace_back(8.0,8.0,0.0);
        aPoints.emplace_back(4.0,4.0,0.1);

        SGM::FindLeastSquarePlane(aPoints,Origin,XVec,YVec,ZVec);

        if(SGM::NearEqual(Origin,SGM::Point3D(4.0,4.0,0.0333333333333333),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(XVec,SGM::UnitVector3D(1.0,1.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(YVec,SGM::UnitVector3D(0.0,0.0,1.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(ZVec,SGM::UnitVector3D(1.0,-1.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==22)
        {
        // Test Line Torus Intersections.

        bool bAnswer=true;

        SGM::Point3D Origin(-20,0,0);
        SGM::UnitVector3D Axis(1.0,0.0,0.0);
        SGM::Interval1D Domain(-20.0,20.0);
        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;

        SGM::Point3D Center(0.0,0.0,0.0);
        SGM::UnitVector3D ZAxis(0.0,0.0,1.0);
        SGMInternal::torus *pTorus=new SGMInternal::torus(rResult,Center,ZAxis,1.0,3.0,false);

        size_t nHits=IntersectLineAndTorus(Origin,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        if( nHits!=4 ||
            SGM::NearEqual(aPoints[0],SGM::Point3D(-4.0,0.0,0.0),SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[1],SGM::Point3D(-2.0,0.0,0.0),SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[2],SGM::Point3D( 2.0,0.0,0.0),SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[3],SGM::Point3D( 4.0,0.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        Origin.m_y=2.0;
        nHits=IntersectLineAndTorus(Origin,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        if( nHits!=3 ||
            SGM::NearEqual(aPoints[0],SGM::Point3D(-3.4641016151377545870548926830117,2.0,0.0),SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[1],SGM::Point3D(0.0,2.0,0.0),SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[2],SGM::Point3D(3.4641016151377545870548926830117,2.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        Origin.m_y=3.0;
        nHits=IntersectLineAndTorus(Origin,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        if( nHits!=2 ||
            SGM::NearEqual(aPoints[0],SGM::Point3D(-2.6457513110645905905016157536393,3.0,0.0),SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[1],SGM::Point3D(2.6457513110645905905016157536393,3.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        Origin.m_y=4.0;
        nHits=IntersectLineAndTorus(Origin,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        if( nHits!=1 ||
            SGM::NearEqual(aPoints[0],SGM::Point3D(0.0,4.0,0.0),SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        Origin.m_y=5.0;
        nHits=IntersectLineAndTorus(Origin,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        if(nHits!=0)
            {
            bAnswer=false;
            }

        rResult.GetThing()->DeleteEntity(pTorus);

        return bAnswer;
        }

    if(nTestNumber==23)
        {
        // Test Transforms

        bool bAnswer=true;

        SGM::UnitVector3D XAxis(2.0,3.0,4.0);
        SGM::UnitVector3D YAxis=XAxis.Orthogonal();
        SGM::UnitVector3D ZAxis=XAxis*YAxis;
        SGM::Vector3D Offset(5.0,6.0,7.0);
        SGM::Transform3D Trans1(XAxis,YAxis,ZAxis,Offset);
        SGM::Transform3D Trans2;
        Trans1.Inverse(Trans2);
        SGM::Transform3D Trans3=Trans1*Trans2;

        SGM::Point3D Pos(0.0,0.0,0.0);
        SGM::Vector3D Vec(1.0,0.0,0.0);
        SGM::UnitVector3D UVec(1.0,0.0,0.0);
        SGM::Point3D Pos1=Trans1*Pos;
        SGM::Point3D Pos2=Trans2*Pos1;
        SGM::Point3D Pos3=Trans3*Pos;
        SGM::Vector3D Vec1=Trans1*Vec;
        SGM::Vector3D Vec2=Trans2*Vec1;
        SGM::Vector3D Vec3=Trans3*Vec;
        SGM::UnitVector3D UVec1=Trans1*UVec;
        SGM::UnitVector3D UVec2=Trans2*UVec1;
        SGM::UnitVector3D UVec3=Trans3*UVec;

        if(SGM::NearEqual(Pos,Pos2,SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(Vec,Vec2,SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(UVec,UVec2,SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(Pos,Pos3,SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(Vec,Vec3,SGM_ZERO)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(UVec,UVec3,SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }

     if(nTestNumber==24)
        {
        // Test Line Torus Intersections.

        bool bAnswer=true;

        SGM::UnitVector3D XVec(1.0,2.0,3.0);
        SGM::UnitVector3D YVec=XVec.Orthogonal();
        SGM::UnitVector3D ZVec=XVec*YVec;
        SGM::Vector3D TVec(4.0,5.0,6.0);
        SGM::Transform3D Trans(XVec,YVec,ZVec,TVec);

        SGM::Point3D Origin1(-20,0,0);
        SGM::UnitVector3D Axis(1.0,0.0,0.0);
        SGM::Interval1D Domain(-20.0,20.0);
        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;

        SGM::Point3D Center(0.0,0.0,0.0);
        SGM::UnitVector3D ZAxis(0.0,0.0,1.0);
        SGMInternal::torus *pTorus=new SGMInternal::torus(rResult,Center,ZAxis,1.0,3.0,false);

        pTorus->Transform(Trans);
        Origin1*=Trans;
        Axis*=Trans;

        size_t nHits=IntersectLineAndTorus(Origin1,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        SGM::Point3D Answer0(-4.0,0.0,0.0);
        SGM::Point3D Answer1(-2.0,0.0,0.0);
        SGM::Point3D Answer2( 2.0,0.0,0.0);
        SGM::Point3D Answer3( 4.0,0.0,0.0);
        if( nHits!=4 ||
            SGM::NearEqual(aPoints[0],Trans*Answer0,SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[1],Trans*Answer1,SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[2],Trans*Answer2,SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[3],Trans*Answer3,SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        SGM::Point3D Origin2(-20,2.0,0);
        Origin2*=Trans;
        SGM::Point3D Answer4(-3.4641016151377545870548926830117,2.0,0.0);
        SGM::Point3D Answer5(0.0,2.0,0.0);
        SGM::Point3D Answer6(3.4641016151377545870548926830117,2.0,0.0);
        nHits=IntersectLineAndTorus(Origin2,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        if( nHits!=3 ||
            SGM::NearEqual(aPoints[0],Trans*Answer4,SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[1],Trans*Answer5,SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[2],Trans*Answer6,SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        SGM::Point3D Origin3(-20,3.0,0);
        Origin3*=Trans;
        SGM::Point3D Answer7(-2.6457513110645905905016157536393,3.0,0.0);
        SGM::Point3D Answer8(2.6457513110645905905016157536393,3.0,0.0);
        nHits=IntersectLineAndTorus(Origin3,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        if( nHits!=2 ||
            SGM::NearEqual(aPoints[0],Trans*Answer7,SGM_ZERO)==false ||
            SGM::NearEqual(aPoints[1],Trans*Answer8,SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        SGM::Point3D Origin4(-20,4.0,0);
        Origin4*=Trans;
        SGM::Point3D Answer9(0.0,4.0,0.0);
        nHits=IntersectLineAndTorus(Origin4,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        if( nHits!=1 ||
            SGM::NearEqual(aPoints[0],Trans*Answer9,SGM_ZERO)==false)
            {
            bAnswer=false;
            }

        aPoints.clear();
        SGM::Point3D Origin5(-20,5.0,0);
        Origin5*=Trans;
        nHits=IntersectLineAndTorus(Origin5,Axis,Domain,pTorus,SGM_MIN_TOL,aPoints,aTypes);
        if(nHits!=0)
            {
            bAnswer=false;
            }

        rResult.GetThing()->DeleteEntity(pTorus);

        return bAnswer;
        }

     if(nTestNumber==25)
        {
        // Test finding conic sections from five points

        bool bAnswer=true;

        double dTolerance=SGM_MIN_TOL;
        std::vector<SGM::Point3D> aPoints;
        aPoints.reserve(5);

        // y=ax^2 parabola
        // a=2 
        
        aPoints.emplace_back(0.0,0.0,0.0);
        aPoints.emplace_back(1.0,2.0,0.0);
        aPoints.emplace_back(-1.0,2.0,0.0);
        aPoints.emplace_back(2.0,8.0,0.0);
        aPoints.emplace_back(-3.0,18.0,0.0);

        SGMInternal::curve *pConic0=SGMInternal::FindConic(rResult,aPoints,dTolerance);
        if(pConic0)
            {
            bAnswer=false;
            rResult.GetThing()->DeleteEntity(pConic0);
            }

        // x^2/a^2-y^2/b^2=1 hyperbola
        // a=2 b=3
        
        aPoints.clear();
        aPoints.emplace_back(2.0,0.0,0.0);
        aPoints.emplace_back(-2.0,0.0,0.0);
        aPoints.emplace_back(6.0,8.4852813742385702928101323452582,0.0);
        aPoints.emplace_back(6.0,-8.4852813742385702928101323452582,0.0);
        aPoints.emplace_back(-6.0,8.4852813742385702928101323452582,0.0);

        SGMInternal::curve *pConic1=SGMInternal::FindConic(rResult,aPoints,dTolerance);
        rResult.GetThing()->DeleteEntity(pConic1);

        // x^2/a^2+y^2/b^2=1 ellipse
        // a=2 b=3

        aPoints.clear();
        aPoints.emplace_back(0.0,3.0,0.0);
        aPoints.emplace_back(2.0,0.0,0.0);
        aPoints.emplace_back(-2.0,0.0,0.0);
        aPoints.emplace_back(0.0,-3.0,0.0);
        aPoints.emplace_back(1.0,2.5980762113533159402911695122588,0.0);
        
        SGMInternal::curve *pConic2=SGMInternal::FindConic(rResult,aPoints,dTolerance);
        rResult.GetThing()->DeleteEntity(pConic2);

        return bAnswer;
        }

    if(nTestNumber==26)
        {
        // Test inverse on lemon and apple tori.

        bool bAnswer=true;

        SGM::Point3D Center;
        SGM::UnitVector3D Axis(0.0,0.0,1.0),XAxis(1.0,0.0,0.0);
        SGMInternal::torus *pApple=new SGMInternal::torus(rResult,Center,Axis,2.0,0.5,true,&XAxis);
        SGMInternal::torus *pLemon=new SGMInternal::torus(rResult,Center,Axis,2.0,0.5,false,&XAxis);

        SGM::Point3D Pos0(0.5,0.0,0.0);
        SGM::Point3D Pos1(1.5,0.0,2.0);
        SGM::Point3D CPos;

        pApple->Inverse(Pos0,&CPos);
        pLemon->Inverse(Pos1,&CPos);
        
        rResult.GetThing()->DeleteEntity(pApple);
        rResult.GetThing()->DeleteEntity(pLemon);

        pApple=new SGMInternal::torus(rResult,Center,Axis,2.0,1.5,true,&XAxis);
        pLemon=new SGMInternal::torus(rResult,Center,Axis,2.0,1.5,false,&XAxis);

        SGM::Point3D Pos2(1.5,0.0,0.0);
        SGM::Point3D Pos3(2.5,0.0,2.0);
        
        pApple->Inverse(Pos2,&CPos);
        pLemon->Inverse(Pos3,&CPos);

        rResult.GetThing()->DeleteEntity(pApple);
        rResult.GetThing()->DeleteEntity(pLemon);
        
        return bAnswer;
        }

    if(nTestNumber==27)
        {
        // Test the intersection of a line and NUBcurve.

        bool bAnswer=true;
    
        std::vector<SGM::Point3D> aPoints;
        size_t Index1;
        double d=0.2;
        for(Index1=0;Index1<100;++Index1)
            {
            aPoints.emplace_back(cos(Index1*d),sin(Index1*d),Index1*d*0.1);
            }
        SGMInternal::NUBcurve *pNUB=SGMInternal::CreateNUBCurve(rResult,aPoints);
        SGM::Point3D Pos0(1,0,0),Pos1(1,0,10);
        SGMInternal::line *pLine=new SGMInternal::line(rResult,Pos0,Pos1);
        std::vector<SGM::Point3D> aHits;
        std::vector<SGM::IntersectionType> aTypes;
        SGMInternal::IntersectCurves(rResult,pLine,pNUB,aHits,aTypes,nullptr,nullptr,SGM_FIT);

        size_t nHits=aHits.size();
        if(nHits!=4)
            {
            bAnswer=false;
            }
        for(Index1=0;Index1<nHits;++Index1)
            {
            SGM::Point3D const &Pos=aHits[Index1];
            SGM::Point3D CPos1,CPos2;
            pLine->Inverse(Pos,&CPos1);
            pNUB->Inverse(Pos,&CPos2);
            double dDist=CPos1.Distance(CPos2);
            if(SGM_FIT<dDist)
                {
                bAnswer=false;
                }
            }

        rResult.GetThing()->DeleteEntity(pNUB);
        rResult.GetThing()->DeleteEntity(pLine);

        return bAnswer;
        }

    if(nTestNumber==28)
        {
        // Test NUB surface line intersection.

        bool bAnswer=true;

        std::vector<double> aUKnots,aVKnots;
        aUKnots.push_back(0.0);
        aUKnots.push_back(0.0);
        aUKnots.push_back(0.0);
        aUKnots.push_back(1.0);
        aUKnots.push_back(1.0);
        aUKnots.push_back(1.0);
        aVKnots=aUKnots;
        std::vector<std::vector<SGM::Point3D> > aaPoints;
        std::vector<SGM::Point3D> aPoints;
        aPoints.assign(3,SGM::Point3D(0,0,0));
        aaPoints.push_back(aPoints);
        aaPoints.push_back(aPoints);
        aaPoints.push_back(aPoints);
        aaPoints[0][0]=SGM::Point3D(0.0,0.0,1.0);
        aaPoints[0][1]=SGM::Point3D(0.0,1.0,0.0);
        aaPoints[0][2]=SGM::Point3D(0.0,2.0,-1.0);
        aaPoints[1][0]=SGM::Point3D(1.0,0.0,0.0);
        aaPoints[1][1]=SGM::Point3D(1.0,1.0,0.0);
        aaPoints[1][2]=SGM::Point3D(1.0,2.0,0.0);
        aaPoints[2][0]=SGM::Point3D(2.0,0.0,-1.0);
        aaPoints[2][1]=SGM::Point3D(2.0,1.0,0.0);
        aaPoints[2][2]=SGM::Point3D(2.0,2.0,1.0);
        SGMInternal::NUBsurface *pNUB=new SGMInternal::NUBsurface(rResult,std::move(aaPoints),std::move(aUKnots),std::move(aVKnots));

        // Test with a line that hits the saddle point.

        SGM::Point3D Pos0(0,0,0.0),Pos1(2,2,0.0);
        SGMInternal::line *pLine1=new SGMInternal::line(rResult,Pos0,Pos1);

        std::vector<SGM::Point3D> aHits1;
        std::vector<SGM::IntersectionType> aTypes1;
        size_t nHits1=SGMInternal::IntersectCurveAndSurface(rResult,pLine1,pNUB,aHits1,aTypes1,nullptr,nullptr,0.0);

        if(nHits1!=1)
            {
            bAnswer=false;
            }
        else if(aTypes1[0]!=SGM::IntersectionType::TangentType)
            {
            bAnswer=false;
            }
        size_t Index1;
        for(Index1=0;Index1<nHits1;++Index1)
            {
            SGM::Point3D const &Pos=aHits1[Index1];
            SGM::Point3D CPos1,CPos2;
            pLine1->Inverse(Pos,&CPos1);
            pNUB->Inverse(Pos,&CPos2);
            double dDist=CPos1.Distance(CPos2);
            if(SGM_ZERO<dDist)
                {
                bAnswer=false;
                }
            }
        rResult.GetThing()->DeleteEntity(pLine1);

        // Test with a line that hits two points.

        SGM::Point3D Pos2(0,0,0.5),Pos3(2,2,0.5);
        SGMInternal::line *pLine2=new SGMInternal::line(rResult,Pos2,Pos3);

        std::vector<SGM::Point3D> aHits2;
        std::vector<SGM::IntersectionType> aTypes2;
        size_t nHits2=SGMInternal::IntersectCurveAndSurface(rResult,pLine2,pNUB,aHits2,aTypes2,nullptr,nullptr,0.0);

        if(nHits2!=2)
            {
            bAnswer=false;
            }
        for(Index1=0;Index1<nHits2;++Index1)
            {
            SGM::Point3D const &Pos=aHits2[Index1];
            SGM::Point3D CPos1,CPos2;
            pLine2->Inverse(Pos,&CPos1);
            pNUB->Inverse(Pos,&CPos2);
            double dDist=CPos1.Distance(CPos2);
            if(SGM_ZERO<dDist)
                {
                bAnswer=false;
                }
            }
        rResult.GetThing()->DeleteEntity(pLine2);

        // Test with a line that just misses the saddle but within tolernace.

        SGM::Point3D Pos4(2,0,0.0001),Pos5(0,2,0.0001);
        SGMInternal::line *pLine3=new SGMInternal::line(rResult,Pos4,Pos5);

        std::vector<SGM::Point3D> aHits3;
        std::vector<SGM::IntersectionType> aTypes3;
        double dTestTol=0.001;
        size_t nHits3=SGMInternal::IntersectCurveAndSurface(rResult,pLine3,pNUB,aHits3,aTypes3,nullptr,nullptr,dTestTol);

        if(nHits3!=1)
            {
            bAnswer=false;
            }
        else if(aTypes3[0]!=SGM::IntersectionType::TangentType)
            {
            bAnswer=false;
            }
        for(Index1=0;Index1<nHits3;++Index1)
            {
            SGM::Point3D const &Pos=aHits3[Index1];
            SGM::Point3D CPos1,CPos2;
            pLine3->Inverse(Pos,&CPos1);
            pNUB->Inverse(Pos,&CPos2);
            double dDist=CPos1.Distance(CPos2);
            if(dTestTol<dDist)
                {
                bAnswer=false;
                }
            }
        rResult.GetThing()->DeleteEntity(pLine3);

        rResult.GetThing()->DeleteEntity(pNUB);

        return bAnswer;
        }

    if(nTestNumber==29)
        {
        // Test the intersection of a line and NUBcurve.

        bool bAnswer=true;
    
        std::vector<SGM::Point3D> aPoints;
        size_t Index1;
        aPoints.emplace_back(SGM::Point3D(0,0,0));
        aPoints.emplace_back(SGM::Point3D(1.1,1,0));
        aPoints.emplace_back(SGM::Point3D(2,0,0));
        SGMInternal::NUBcurve *pNUB=SGMInternal::CreateNUBCurve(rResult,aPoints);

        // Test with two hits.

        SGM::Point3D Pos0(0,0.5,0),Pos1(2,0.5,0);
        SGMInternal::line *pLine1=new SGMInternal::line(rResult,Pos0,Pos1);
        std::vector<SGM::Point3D> aHits1;
        std::vector<SGM::IntersectionType> aTypes1;
        SGMInternal::IntersectCurves(rResult,pLine1,pNUB,aHits1,aTypes1,nullptr,nullptr,SGM_FIT);

        size_t nHits1=aHits1.size();
        if(nHits1!=2)
            {
            bAnswer=false;
            }
        for(Index1=0;Index1<nHits1;++Index1)
            {
            SGM::Point3D const &Pos=aHits1[Index1];
            SGM::Point3D CPos1,CPos2;
            pLine1->Inverse(Pos,&CPos1);
            pNUB->Inverse(Pos,&CPos2);
            double dDist=CPos1.Distance(CPos2);
            if(SGM_ZERO<dDist)
                {
                bAnswer=false;
                }
            }
        rResult.GetThing()->DeleteEntity(pLine1);

        // Test with one tangent hit.

        SGM::Point3D Pos2(0,1,0),Pos3(2,1,0);
        SGMInternal::line *pLine2=new SGMInternal::line(rResult,Pos2,Pos3);
        std::vector<SGM::Point3D> aHits2;
        std::vector<SGM::IntersectionType> aTypes2;
        SGMInternal::IntersectCurves(rResult,pLine2,pNUB,aHits2,aTypes2,nullptr,nullptr,SGM_FIT);

        size_t nHits2=aHits2.size();
        if(nHits2!=1)
            {
            bAnswer=false;
            }
        else if(aTypes2[0]!=SGM::IntersectionType::TangentType)
            {
            bAnswer=false;
            }
        for(Index1=0;Index1<nHits2;++Index1)
            {
            SGM::Point3D const &Pos=aHits2[Index1];
            SGM::Point3D CPos1,CPos2;
            pLine2->Inverse(Pos,&CPos1);
            pNUB->Inverse(Pos,&CPos2);
            double dDist=CPos1.Distance(CPos2);
            if(SGM_ZERO<dDist)
                {
                bAnswer=false;
                }
            }
        rResult.GetThing()->DeleteEntity(pLine2);

        rResult.GetThing()->DeleteEntity(pNUB);
        
        return bAnswer;
        }

    if(nTestNumber==30)
        {
        // Test sphere cylinder intersections

        bool bAnswer=true;

        SGM::Point3D Bottom(0,0,-10),Top(0,0,10),Pos0(1,0,0),Pos1(3,0,0),Pos2(0,0,0),Pos3(2,0,0),Pos4(4,0,0);
        double dRadius=2.0;
        SGMInternal::cylinder *pCylinder=new SGMInternal::cylinder(rResult,Bottom,Top,dRadius);

        SGMInternal::sphere *pSphere1=new SGMInternal::sphere(rResult,Pos2,1.0);
        SGMInternal::sphere *pSphere2=new SGMInternal::sphere(rResult,Pos2,2.0);
        SGMInternal::sphere *pSphere3=new SGMInternal::sphere(rResult,Pos2,3.0);
        SGMInternal::sphere *pSphere4=new SGMInternal::sphere(rResult,Pos0,1.0);
        SGMInternal::sphere *pSphere5=new SGMInternal::sphere(rResult,Pos1,1.0);
        SGMInternal::sphere *pSphere6=new SGMInternal::sphere(rResult,Pos3,2.0);
        SGMInternal::sphere *pSphere7=new SGMInternal::sphere(rResult,Pos3,4.0);
        SGMInternal::sphere *pSphere8=new SGMInternal::sphere(rResult,Pos3,6.0);
        SGMInternal::sphere *pSphere9=new SGMInternal::sphere(rResult,Pos4,1.0);

        if(!TestIntersections(rResult,pCylinder,pSphere1,0))
            {
            bAnswer=false;
            }
        if(!TestIntersections(rResult,pCylinder,pSphere2,1))
            {
            bAnswer=false;
            }
        if(!TestIntersections(rResult,pCylinder,pSphere3,2))
            {
            bAnswer=false;
            }
        if(!TestIntersections(rResult,pCylinder,pSphere4,1))
            {
            bAnswer=false;
            }
        if(!TestIntersections(rResult,pCylinder,pSphere5,1))
            {
            bAnswer=false;
            }
        if(!TestIntersections(rResult,pCylinder,pSphere6,1))
            {
            bAnswer=false;
            }
        if(!TestIntersections(rResult,pCylinder,pSphere7,2))
            {
            bAnswer=false;
            }
        if(!TestIntersections(rResult,pCylinder,pSphere8,2))
            {
            bAnswer=false;
            }
        if(!TestIntersections(rResult,pCylinder,pSphere9,0))
            {
            bAnswer=false;
            }

        rResult.GetThing()->DeleteEntity(pCylinder);
        rResult.GetThing()->DeleteEntity(pSphere1);
        rResult.GetThing()->DeleteEntity(pSphere2);
        rResult.GetThing()->DeleteEntity(pSphere3);
        rResult.GetThing()->DeleteEntity(pSphere4);
        rResult.GetThing()->DeleteEntity(pSphere5);
        rResult.GetThing()->DeleteEntity(pSphere6);
        rResult.GetThing()->DeleteEntity(pSphere7);
        rResult.GetThing()->DeleteEntity(pSphere8);
        rResult.GetThing()->DeleteEntity(pSphere9);
    
        return bAnswer;
        }

    if(nTestNumber==31)
        {
        // Test Revolve Surface

        bool bAnswer=true;

        // relatively easy testcase to debug
        // NUB curve is a straight line parallel to the axis
        // both have a slope of 2:1, and distance between is sqrt(5.0)
        std::vector<double> aKnots1={0,0,0,0,0.5,1,1,1,1};
        std::vector<SGM::Point3D> aControlPoints1;
        aControlPoints1.emplace_back(3.5,1,0);
        aControlPoints1.emplace_back(3.75,1.5,0);
        aControlPoints1.emplace_back(4,2,0);
        aControlPoints1.emplace_back(4.25,2.5,0);
        aControlPoints1.emplace_back(4.5,3,0);

        SGMInternal::NUBcurve *pNUB1=new SGMInternal::NUBcurve(rResult,aControlPoints1,aKnots1);

        SGM::Point3D Origin1(1.0,1.0,0.0);
        SGM::UnitVector3D Axis1(1.0,2.0,0.0);
        SGMInternal::revolve *pRevolve1 = new SGMInternal::revolve(rResult, Origin1, Axis1, pNUB1);

        SGM::Point3D Pos;
        SGM::Vector3D Du, Dv;
        pRevolve1->Evaluate(SGM::Point2D(SGM_PI/2.0, 0.5), &Pos, &Du, &Dv);

        bool bAnswer1 = true;
        double dDistance = sqrt(5.0);
        if(SGM::NearEqual(Pos, SGM::Point3D(2.0,3.0,-dDistance), SGM_ZERO)==false)
            {
            bAnswer1=false;
            }
        SGM::UnitVector3D uDuDirection(-2,1,0);
        SGM::UnitVector3D UnitDu = Du;
        if(SGM::NearEqual(uDuDirection,UnitDu,SGM_ZERO)==false)
            {
            bAnswer1=false;
            }
        SGM::UnitVector3D uDvDirection(1,2,0);
        SGM::UnitVector3D UnitDv = Dv;
        if(SGM::NearEqual(uDvDirection,UnitDv,SGM_ZERO)==false)
            {
            bAnswer1=false;
            }

        // create a more general NUB to revolve
        std::vector<double> aKnots2={0,0,0,0,0.5,1,1,1,1};
        std::vector<SGM::Point3D> aControlPoints2;
        aControlPoints2.emplace_back(1,1,0);
        aControlPoints2.emplace_back(1.166666666666666,1.166666666666666,0);
        aControlPoints2.emplace_back(2,2.8333333333333333,0);
        aControlPoints2.emplace_back(2.8333333333333333,2.8333333333333333,0);
        aControlPoints2.emplace_back(3,3,0);

        SGMInternal::NUBcurve *pNUB2=new SGMInternal::NUBcurve(rResult,aControlPoints2,aKnots2);

        SGM::Point3D Origin2(1.0,3.0,0.0);
        SGM::UnitVector3D Axis2(1.0,2.0,0.0);
        SGMInternal::revolve *pRevolve2 = new SGMInternal::revolve(rResult, Origin2, Axis2, pNUB2);

        bool bAnswer2 = SGMInternal::TestSurface(rResult,pRevolve2, SGM::Point2D(0.5,0.2));

        bAnswer = (bAnswer1 && bAnswer2);

        rResult.GetThing()->DeleteEntity(pRevolve1);
        rResult.GetThing()->DeleteEntity(pNUB1);
        rResult.GetThing()->DeleteEntity(pRevolve2);
        rResult.GetThing()->DeleteEntity(pNUB2);
        
        return bAnswer;
        }

    if(nTestNumber==32)
        {
        bool bAnswer=true;

        SGM::Point3D Bottom(0,0,-5),Top(0,0,5),Pos3(2,0,0);
        double dRadius=2.0;
        rResult.SetLog(true);
        SGM::CreateCylinder(rResult,Bottom,Top,dRadius);
        SGM::CreateSphere(rResult,Pos3,6.0);

        std::vector<SGM::Entity> const &aLog=rResult.GetLogEntities();
        SGM::Face CylinderFace(aLog[0].m_ID);
        SGM::Face SphereFace(aLog[3].m_ID);
        SGM::Surface CylinderSurf=SGM::GetSurfaceOfFace(rResult,CylinderFace);
        SGM::Surface SphereSurf=SGM::GetSurfaceOfFace(rResult,SphereFace);

        std::vector<SGM::Curve> aCurves;
        size_t Index1;
        size_t nCurves=SGM::IntersectSurfaces(rResult,CylinderSurf,SphereSurf,aCurves);
        for(Index1=0;Index1<nCurves;++Index1)
            {
            SGM::CreateEdge(rResult,aCurves[Index1]);
            }
        return bAnswer;
        }

    if(nTestNumber==33)
        {
        bool bAnswer=true;

        SGM::Interval2D Domain2D(11,14,7,10);
        double dValue2D=SGM::Integrate2D(TestIntegrand2D,Domain2D,nullptr,SGM_ZERO);
        if(SGM::NearEqual(dValue2D,1719,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        SGM::Point2D PosA(11,7),PosB(14,7),PosC(14,10),PosD(11,10);
        dValue2D=SGM::IntegrateTriangle(TestIntegrand2D,PosA,PosB,PosC,nullptr,SGM_ZERO);
        dValue2D+=SGM::IntegrateTriangle(TestIntegrand2D,PosA,PosC,PosD,nullptr,SGM_ZERO);
        if(SGM::NearEqual(dValue2D,1719,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        SGM::Point2D PosE(14,8.5),PosF(11,8.5);
        double dT1=SGM::IntegrateTriangle(TestIntegrand2D,PosA,PosB,PosE,nullptr,SGM_ZERO);
        double dT3=SGM::IntegrateTriangle(TestIntegrand2D,PosA,PosE,PosD,nullptr,SGM_ZERO);
        double dT2=SGM::IntegrateTriangle(TestIntegrand2D,PosD,PosC,PosE,nullptr,SGM_ZERO);
        dValue2D=dT1+dT2+dT3;
        if(SGM::NearEqual(dValue2D,1719,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        dT1=SGM::IntegrateTriangle(TestIntegrand2D,PosA,PosB,PosF,nullptr,SGM_ZERO);
        dT3=SGM::IntegrateTriangle(TestIntegrand2D,PosB,PosF,PosC,nullptr,SGM_ZERO);
        dT2=SGM::IntegrateTriangle(TestIntegrand2D,PosD,PosC,PosF,nullptr,SGM_ZERO);
        dValue2D=dT1+dT2+dT3;
        if(SGM::NearEqual(dValue2D,1719,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        SGM::Interval1D Domain(0.0,1.0);
        double dValue=SGM::Integrate1D(TestIntegrand,Domain,nullptr,SGM_ZERO);
        if(SGM::NearEqual(dValue,SGM_PI,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        SGM::Point3D Center(0,0,0);
        SGM::UnitVector3D Normal(0,0,1);
        SGMInternal::curve *pCurve=new SGMInternal::circle(rResult,Center,Normal,1.0);
        dValue=pCurve->FindLength(pCurve->GetDomain(),SGM_MIN_TOL);
        if(SGM::NearEqual(dValue,SGM_TWO_PI,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        rResult.GetThing()->DeleteEntity(pCurve);

        SGM::Body BodyID=SGM::CreateSphere(rResult,Center,1.0);
        std::set<SGM::Face> sFaces;
        SGM::FindFaces(rResult,BodyID,sFaces);
        SGM::Face FaceID=*(sFaces.begin());
        double dArea=SGM::FindArea(rResult,FaceID);
        if(SGM::NearEqual(dArea,12.566370614359172953850573533118,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        SGM::DeleteEntity(rResult,BodyID);
        
        return bAnswer;
        }

    if(nTestNumber==34)
        {
        std::vector<SGM::Point3D> aPoints;
        aPoints.emplace_back(-2,.5,0);
        aPoints.emplace_back(-1,1.5,0);
        aPoints.emplace_back(0,1,0);
        aPoints.emplace_back(1,1.5,0);
        aPoints.emplace_back(2,2,0);
        SGM::Curve CurveID = SGM::CreateNUBCurve(rResult, aPoints);
        CreateEdge(rResult,CurveID);
        //SGM::Point3D Origin(-1,0,0);
        //SGM::UnitVector3D Axis(1,0,0);
        //SGM::Body BodyID = SGM::CreateRevolve(rResult, Origin, Axis, CurveID);

        //std::set<SGM::Face> sFaces;
        //SGM::FindFaces(rResult, BodyID, sFaces);

        //for(Face FaceID : sFaces)
        //  SGM::GetFaceTriangles(rResult, FaceID);

        return true;
        } 

    if(nTestNumber==35)
        {
        bool bAnswer=true;

        SGM::Point3D Center(0,0,0);
        SGM::UnitVector3D Norm(1,1,1);
        SGM::Curve CurveID=SGM::CreateCircle(rResult,Center,Norm,1.0);
        SGM::Edge EdgeID=SGM::CreateEdge(rResult,CurveID);
        SGM::Surface SurfaceID=SGM::CreateSphereSurface(rResult,Center,1.0);
        std::vector<SGM::Edge> aEdges;
        aEdges.push_back(EdgeID);
        std::vector<SGM::EdgeSideType> aTypes;
        aTypes.push_back(SGM::EdgeSideType::FaceOnLeftType);
        SGM::Body BodyID=SGM::CreateSheetBody(rResult,SurfaceID,aEdges,aTypes);

        std::set<SGM::Face> sFaces;
        SGM::FindFaces(rResult,BodyID,sFaces);
        SGM::Face FaceID=*(sFaces.begin());
        double dArea=SGM::FindArea(rResult,FaceID);
        if(SGM::NearEqual(dArea,6.283185307179586476925286766559,SGM_MIN_TOL,true)==false)
            {
            bAnswer=false;
            }
        SGM::DeleteEntity(rResult,BodyID);

        return bAnswer;
        }

    if(nTestNumber==36)
        {
        bool bAnswer=true;

        SGM::Point3D Center(0,0,0);
        SGM::UnitVector3D Norm(0,0,1);
        SGM::Body BodyID=SGM::CreateTorus(rResult,Center,Norm,1,3);

        std::set<SGM::Face> sFaces;
        SGM::FindFaces(rResult,BodyID,sFaces);
        SGM::Face FaceID=*(sFaces.begin());
        double dArea=SGM::FindArea(rResult,FaceID);
        if(SGM::NearEqual(dArea,118.43525281307230342601389199851,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        SGM::DeleteEntity(rResult,BodyID);

        return bAnswer;
        }

    if(nTestNumber==37)
        {
        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints1;
        aPoints1.emplace_back(-2,.5,0);
        aPoints1.emplace_back(-1,1.5,0);
        aPoints1.emplace_back(0,1,0);
        aPoints1.emplace_back(1,1.5,0);
        aPoints1.emplace_back(2,2,0);

        // simple case
        //aPoints1.push_back(SGM::Point3D(-2,.5,0));
        //aPoints1.push_back(SGM::Point3D(-1,1.5,0));
        //aPoints1.push_back(SGM::Point3D(0,.5,0));
        //aPoints1.push_back(SGM::Point3D(1,.5,0));
        //aPoints1.push_back(SGM::Point3D(2,.5,0));

        SGM::Curve CurveID = SGM::CreateNUBCurve(rResult, aPoints1);

        SGM::Point3D Origin(-1,0,0);
        SGM::UnitVector3D Axis(1,0,0);

        SGMInternal::curve* pCurve = (SGMInternal::curve *)rResult.GetThing()->FindEntity(CurveID.m_ID);
        SGMInternal::revolve *pRevolve=new SGMInternal::revolve(rResult,Origin,Axis,pCurve);

        SGM::Point3D pCurveStart;
        SGM::Point3D pCurveEnd;
        pCurve->Evaluate(pCurve->GetDomain().m_dMin, &pCurveStart);
        pCurve->Evaluate(pCurve->GetDomain().m_dMax, &pCurveEnd);

        SGM::Point3D StartCenter = Origin + ((pCurveStart - Origin) % Axis) * Axis;
        SGM::Point3D EndCenter = Origin + ((pCurveEnd - Origin) % Axis) * Axis;
        SGM::UnitVector3D XAxis = pCurveStart - StartCenter;
        double dRadiusStart = pCurveStart.Distance(StartCenter);
        double dRadiusEnd = pCurveEnd.Distance(EndCenter);

        SGMInternal::circle *pCircleStart=new SGMInternal::circle(rResult,StartCenter,-Axis,dRadiusStart,&XAxis);
        SGMInternal::circle *pCircleEnd=new SGMInternal::circle(rResult,EndCenter,Axis,dRadiusEnd,&XAxis);


        SGMInternal::body   *pBody=new SGMInternal::body(rResult); 
        SGMInternal::volume *pVolume=new SGMInternal::volume(rResult);

        SGMInternal::face *pRevolveFace=new SGMInternal::face(rResult);

        SGMInternal::edge *pEdgeBottom=new SGMInternal::edge(rResult);
        SGMInternal::edge *pEdgeTop=new SGMInternal::edge(rResult);

        // Connect everything.

        pBody->AddVolume(pVolume);
        pVolume->AddFace(pRevolveFace);

        pRevolveFace->AddEdge(rResult,pEdgeBottom,SGM::FaceOnRightType);
        pRevolveFace->AddEdge(rResult,pEdgeTop,SGM::FaceOnRightType);

        pRevolveFace->SetSurface(pRevolve);
        pRevolveFace->SetSides(2);

        pEdgeBottom->SetCurve(pCircleStart);
        pEdgeTop->SetCurve(pCircleEnd);

        pEdgeBottom->SetDomain(rResult,SGM::Interval1D(0, SGM_PI*2));
        pEdgeTop->SetDomain(rResult,SGM::Interval1D(0, SGM_PI*2));

        SGM::TranslatorOptions TranslatorOpts;
        SGM::SaveSTEP(rResult, "revolve_sheet.stp", rResult.GetThing()->GetID(),TranslatorOpts);

        SGM::CheckOptions Options;
        std::vector<std::string> CheckStrings;

        bAnswer = pBody->Check(rResult, Options, CheckStrings,true);
        if (!bAnswer) return bAnswer;

        return true;
        } 

    if(nTestNumber==38)
        {
        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints1;
        aPoints1.emplace_back(-2,.5,0);
        aPoints1.emplace_back(-1,1.5,0);
        aPoints1.emplace_back(0,1,0);
        aPoints1.emplace_back(1,1.5,0);
        aPoints1.emplace_back(2,2,0);

        // simple case
        //aPoints1.push_back(SGM::Point3D(-2,.5,0));
        //aPoints1.push_back(SGM::Point3D(-1,1.5,0));
        //aPoints1.push_back(SGM::Point3D(0,.5,0));
        //aPoints1.push_back(SGM::Point3D(1,.5,0));
        //aPoints1.push_back(SGM::Point3D(2,.5,0));

        SGM::Curve CurveID = SGM::CreateNUBCurve(rResult, aPoints1);

        SGM::Point3D Origin(-1,0,0);
        SGM::UnitVector3D Axis(1,0,0);

        SGMInternal::curve* pCurve = (SGMInternal::curve *)rResult.GetThing()->FindEntity(CurveID.m_ID);
        SGMInternal::revolve *pRevolve=new SGMInternal::revolve(rResult,Origin,Axis,pCurve);

        SGM::Point3D pCurveStart;
        SGM::Point3D pCurveEnd;
        pCurve->Evaluate(pCurve->GetDomain().m_dMin, &pCurveStart);
        pCurve->Evaluate(pCurve->GetDomain().m_dMax, &pCurveEnd);

        SGM::Point3D StartCenter = Origin + ((pCurveStart - Origin) % Axis) * Axis;
        SGM::Point3D EndCenter = Origin + ((pCurveEnd - Origin) % Axis) * Axis;
        SGM::UnitVector3D XAxis = pCurveStart - StartCenter;
        double dRadiusStart = pCurveStart.Distance(StartCenter);
        double dRadiusEnd = pCurveEnd.Distance(EndCenter);

        SGMInternal::circle *pCircleStart=new SGMInternal::circle(rResult,StartCenter,-Axis,dRadiusStart,&XAxis);
        SGMInternal::circle *pCircleEnd=new SGMInternal::circle(rResult,EndCenter,Axis,dRadiusEnd,&XAxis);


        SGMInternal::body   *pBody=new SGMInternal::body(rResult); 
        SGMInternal::volume *pVolume=new SGMInternal::volume(rResult);

        SGMInternal::face *pRevolveFace=new SGMInternal::face(rResult);

        SGMInternal::edge *pEdgeBottom=new SGMInternal::edge(rResult);
        SGMInternal::edge *pEdgeTop=new SGMInternal::edge(rResult);

        SGMInternal::vertex *pBottom = new SGMInternal::vertex(rResult,SGM::Point3D(-2,.5,0));
        SGMInternal::vertex *pTop = new SGMInternal::vertex(rResult,SGM::Point3D(2,2,0));

        // Connect everything.

        pBody->AddVolume(pVolume);
        pVolume->AddFace(pRevolveFace);

        pRevolveFace->AddEdge(rResult,pEdgeBottom,SGM::FaceOnRightType);
        pRevolveFace->AddEdge(rResult,pEdgeTop,SGM::FaceOnRightType);

        pRevolveFace->SetSurface(pRevolve);
        pRevolveFace->SetSides(2);

        pEdgeBottom->SetCurve(pCircleStart);
        pEdgeTop->SetCurve(pCircleEnd);

        pEdgeBottom->SetDomain(rResult,SGM::Interval1D(0, SGM_PI*2));
        pEdgeTop->SetDomain(rResult,SGM::Interval1D(0, SGM_PI*2));

        pEdgeBottom->SetStart(pBottom);
        pEdgeBottom->SetEnd(pBottom);
        pEdgeTop->SetStart(pTop);
        pEdgeTop->SetEnd(pTop);

        SGM::TranslatorOptions TranslatorOpts;
        SGM::SaveSTEP(rResult, "revolve_sheet.stp", rResult.GetThing()->GetID(),TranslatorOpts);

        SGM::CheckOptions Options;
        std::vector<std::string> CheckStrings;

        bAnswer = pBody->Check(rResult, Options, CheckStrings,true);
        if (!bAnswer) return bAnswer;

        return true;
        } 

    if(nTestNumber==39)
        {
        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints1;
        aPoints1.emplace_back(-2,.5,0);
        aPoints1.emplace_back(-1,1.5,0);
        aPoints1.emplace_back(0,1,0);
        aPoints1.emplace_back(1,1.5,0);
        aPoints1.emplace_back(2,2,0);
        SGM::Curve CurveID = SGM::CreateNUBCurve(rResult, aPoints1);

        std::vector<SGM::Point3D> aPoints2;
        aPoints2.emplace_back(-2,0,.5);
        aPoints2.emplace_back(-1,0,1.5);
        aPoints2.emplace_back(0,0,1);
        aPoints2.emplace_back(1,0,1.5);
        aPoints2.emplace_back(2,0,2);
        SGM::Curve LeftCurveID = SGM::CreateNUBCurve(rResult, aPoints2);

        std::vector<SGM::Point3D> aPoints3;
        aPoints3.emplace_back(-2,0,-.5);
        aPoints3.emplace_back(-1,0,-1.5);
        aPoints3.emplace_back(0,0,-1);
        aPoints3.emplace_back(1,0,-1.5);
        aPoints3.emplace_back(2,0,-2);
        SGM::Curve RightCurveID = SGM::CreateNUBCurve(rResult, aPoints3);

        SGM::Point3D Origin(-1,0,0);
        SGM::UnitVector3D Axis(1,0,0);

        SGMInternal::curve* pCurve = (SGMInternal::curve *)rResult.GetThing()->FindEntity(CurveID.m_ID);
        SGMInternal::revolve *pRevolve=new SGMInternal::revolve(rResult,Origin,Axis,pCurve);

        SGM::Point3D pCurveStart;
        SGM::Point3D pCurveEnd;
        pCurve->Evaluate(pCurve->GetDomain().m_dMin, &pCurveStart);
        pCurve->Evaluate(pCurve->GetDomain().m_dMax, &pCurveEnd);

        SGM::Point3D StartCenter = Origin + ((pCurveStart - Origin) % Axis) * Axis;
        SGM::Point3D EndCenter = Origin + ((pCurveEnd - Origin) % Axis) * Axis;
        SGM::UnitVector3D XAxis = pCurveStart - StartCenter;
        double dRadiusStart = pCurveStart.Distance(StartCenter);
        double dRadiusEnd = pCurveEnd.Distance(EndCenter);

        SGMInternal::circle *pCircleStart=new SGMInternal::circle(rResult,StartCenter,-Axis,dRadiusStart,&XAxis);
        SGMInternal::circle *pCircleEnd=new SGMInternal::circle(rResult,EndCenter,Axis,dRadiusEnd,&XAxis);


        SGMInternal::body   *pBody=new SGMInternal::body(rResult); 
        SGMInternal::volume *pVolume=new SGMInternal::volume(rResult);

        SGMInternal::face *pRevolveFace=new SGMInternal::face(rResult);

        SGMInternal::edge *pEdgeBottom=new SGMInternal::edge(rResult);
        SGMInternal::edge *pEdgeTop=new SGMInternal::edge(rResult);
        SGMInternal::edge *pEdgeLeft=new SGMInternal::edge(rResult);
        SGMInternal::edge *pEdgeRight=new SGMInternal::edge(rResult);

        SGMInternal::vertex *pBottomLeft=new SGMInternal::vertex(rResult,SGM::Point3D(-2,0,.5));
        SGMInternal::vertex *pBottomRight=new SGMInternal::vertex(rResult,SGM::Point3D(-2,0,-.5));
        SGMInternal::vertex *pTopLeft=new SGMInternal::vertex(rResult,SGM::Point3D(2,0,2));
        SGMInternal::vertex *pTopRight=new SGMInternal::vertex(rResult,SGM::Point3D(2,0,-2));

        // Connect everything.

        pBody->AddVolume(pVolume);
        pVolume->AddFace(pRevolveFace);

        pRevolveFace->AddEdge(rResult,pEdgeBottom,SGM::FaceOnRightType);
        pRevolveFace->AddEdge(rResult,pEdgeTop,SGM::FaceOnRightType);
        pRevolveFace->AddEdge(rResult,pEdgeRight,SGM::FaceOnLeftType);
        pRevolveFace->AddEdge(rResult,pEdgeLeft,SGM::FaceOnRightType);

        pRevolveFace->SetSurface(pRevolve);
        pRevolveFace->SetSides(2);

        pEdgeBottom->SetStart(pBottomRight);
        pEdgeBottom->SetEnd(pBottomLeft);
        pEdgeRight->SetStart(pBottomRight);
        pEdgeRight->SetEnd(pTopRight);
        pEdgeTop->SetStart(pTopLeft);
        pEdgeTop->SetEnd(pTopRight);
        pEdgeLeft->SetStart(pBottomLeft);
        pEdgeLeft->SetEnd(pTopLeft);

        pEdgeBottom->SetCurve(pCircleStart);
        pEdgeTop->SetCurve(pCircleEnd);
        pEdgeLeft->SetCurve((SGMInternal::curve *)rResult.GetThing()->FindEntity(LeftCurveID.m_ID));
        pEdgeRight->SetCurve((SGMInternal::curve *)rResult.GetThing()->FindEntity(RightCurveID.m_ID));

        pEdgeBottom->SetDomain(rResult,SGM::Interval1D(SGM_PI/2.0, 3*SGM_PI/2.0));
        pEdgeTop->SetDomain(rResult,SGM::Interval1D(SGM_PI/2.0, 3*SGM_PI/2.0));
        pEdgeLeft->SetDomain(rResult,SGM::Interval1D(0,1));
        pEdgeRight->SetDomain(rResult,SGM::Interval1D(0,1));

        SGM::TranslatorOptions TranslatorOpts;
        SGM::SaveSTEP(rResult, "revolve_sheet.stp", rResult.GetThing()->GetID(),TranslatorOpts);
        /*
        SGM::CheckOptions Options;
        std::vector<std::string> CheckStrings;

        bAnswer = pRevolveFace->Check(rResult, Options, CheckStrings);
        if (!bAnswer) return bAnswer;

        bAnswer = pVolume->Check(rResult, Options, CheckStrings);
        if (!bAnswer) return bAnswer;

        bAnswer = pBody->Check(rResult, Options, CheckStrings);
        if (!bAnswer) return bAnswer;
        */
        return bAnswer;
        }

    if(nTestNumber==40)
        {
        // Test of NURB Curves.

        std::vector<SGM::Point4D> aControlPoints;
        aControlPoints.emplace_back(1,0,0,1);
        aControlPoints.emplace_back(1,1,0,sqrt(2)/2);
        aControlPoints.emplace_back(0,1,0,1);
        aControlPoints.emplace_back(-1,1,0,sqrt(2)/2);
        aControlPoints.emplace_back(-1,0,0,1);
        aControlPoints.emplace_back(-1,-1,0,sqrt(2)/2);
        aControlPoints.emplace_back(0,-1,0,1);
        aControlPoints.emplace_back(1,-1,0,sqrt(2)/2);
        aControlPoints.emplace_back(1,0,0,1);
        
        std::vector<double> aKnots;
        aKnots.push_back(0);
        aKnots.push_back(0);
        aKnots.push_back(0);
        aKnots.push_back(SGM_HALF_PI);
        aKnots.push_back(SGM_HALF_PI);
        aKnots.push_back(SGM_PI);
        aKnots.push_back(SGM_PI);
        aKnots.push_back(SGM_PI*1.5);
        aKnots.push_back(SGM_PI*1.5);
        aKnots.push_back(SGM_TWO_PI);
        aKnots.push_back(SGM_TWO_PI);
        aKnots.push_back(SGM_TWO_PI);

        SGMInternal::NURBcurve *pNURB=new SGMInternal::NURBcurve(rResult,aControlPoints,aKnots);
        bool bAnswer=SGMInternal::TestCurve(pNURB,1.234);

        SGM::Point3D Pos0,Pos1,Pos2;
        pNURB->Evaluate(1,&Pos0);
        pNURB->Evaluate(2,&Pos1);
        pNURB->Evaluate(3,&Pos2);
        double dDist0=(Pos0-SGM::Point3D(0,0,0)).Magnitude();
        double dDist1=(Pos1-SGM::Point3D(0,0,0)).Magnitude();
        double dDist2=(Pos2-SGM::Point3D(0,0,0)).Magnitude();

        if(SGM::NearEqual(dDist0,1.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(dDist1,1.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(dDist2,1.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        SGMInternal::CreateEdge(rResult,pNURB,nullptr);
        SGM::Point3D Center(0,0,0);
        SGM::UnitVector3D Normal(0,0,1);
        SGMInternal::circle *pCircle=new SGMInternal::circle(rResult,Center,Normal,1.0);
        SGMInternal::CreateEdge(rResult,pCircle,nullptr);

        return bAnswer;
        }

    if(nTestNumber==41)
        {
        // Test of NURB Surface.

        std::vector<std::vector<SGM::Point4D> > aaControlPoints;
        std::vector<SGM::Point4D> aControlPoints;
        aControlPoints.emplace_back(1,0,0,1);
        aControlPoints.emplace_back(1,1,0,sqrt(2)/2);
        aControlPoints.emplace_back(0,1,0,1);
        aControlPoints.emplace_back(-1,1,0,sqrt(2)/2);
        aControlPoints.emplace_back(-1,0,0,1);
        aControlPoints.emplace_back(-1,-1,0,sqrt(2)/2);
        aControlPoints.emplace_back(0,-1,0,1);
        aControlPoints.emplace_back(1,-1,0,sqrt(2)/2);
        aControlPoints.emplace_back(1,0,0,1);
        aaControlPoints.push_back(aControlPoints);
        aControlPoints.clear();
        aControlPoints.emplace_back(1,0,1,1);
        aControlPoints.emplace_back(1,1,1,sqrt(2)/2);
        aControlPoints.emplace_back(0,1,1,1);
        aControlPoints.emplace_back(-1,1,1,sqrt(2)/2);
        aControlPoints.emplace_back(-1,0,1,1);
        aControlPoints.emplace_back(-1,-1,1,sqrt(2)/2);
        aControlPoints.emplace_back(0,-1,1,1);
        aControlPoints.emplace_back(1,-1,1,sqrt(2)/2);
        aControlPoints.emplace_back(1,0,1,1);
        aaControlPoints.push_back(aControlPoints);
        
        std::vector<double> aUKnots;
        aUKnots.push_back(0);
        aUKnots.push_back(0);
        aUKnots.push_back(1);
        aUKnots.push_back(1);

        std::vector<double> aVKnots;
        aVKnots.push_back(0);
        aVKnots.push_back(0);
        aVKnots.push_back(0);
        aVKnots.push_back(SGM_HALF_PI);
        aVKnots.push_back(SGM_HALF_PI);
        aVKnots.push_back(SGM_PI);
        aVKnots.push_back(SGM_PI);
        aVKnots.push_back(SGM_PI*1.5);
        aVKnots.push_back(SGM_PI*1.5);
        aVKnots.push_back(SGM_TWO_PI);
        aVKnots.push_back(SGM_TWO_PI);
        aVKnots.push_back(SGM_TWO_PI);

        SGMInternal::NURBsurface *pNURB=new SGMInternal::NURBsurface(rResult,std::move(aaControlPoints),std::move(aUKnots),std::move(aVKnots));
        bool bAnswer=SGMInternal::TestSurface(rResult,pNURB,SGM::Point2D(0.245,0.678));

        SGM::Point3D Pos0,Pos1,Pos2;
        pNURB->Evaluate(SGM::Point2D(0.145,0.578),&Pos0);
        pNURB->Evaluate(SGM::Point2D(0.245,0.678),&Pos1);
        pNURB->Evaluate(SGM::Point2D(0.345,0.778),&Pos2);
        Pos0.m_z=0;
        Pos1.m_z=0;
        Pos2.m_z=0;
        double dDist0=(Pos0-SGM::Point3D(0,0,0)).Magnitude();
        double dDist1=(Pos1-SGM::Point3D(0,0,0)).Magnitude();
        double dDist2=(Pos2-SGM::Point3D(0,0,0)).Magnitude();

        if(SGM::NearEqual(dDist0,1.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(dDist1,1.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(dDist2,1.0,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==42)
        {
        // Test RayFire

        bool bAnswer=true;

        SGM::Point3D Bottom(0,0,0),Top(0,0,2);
        double dRadius=1.0;
        SGM::Body BodyID=SGM::CreateCylinder(rResult,Bottom,Top,dRadius);
        SGM::Point3D Origin(-2,0,1);
        SGM::UnitVector3D Axis(1,0,0);
        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;
        SGM::RayFire(rResult,Origin,Axis,BodyID,aPoints,aTypes,SGM_MIN_TOL);

        if(aPoints.size()==2)
            {
            if(SGM::NearEqual(aPoints[0],SGM::Point3D(-1,0,1),SGM_ZERO)==false)
                {
                bAnswer=false;
                }
            if(SGM::NearEqual(aPoints[1],SGM::Point3D(1,0,1),SGM_ZERO)==false)
                {
                bAnswer=false;
                }
            if(aTypes[0]!=SGM::IntersectionType::PointType)
                {
                bAnswer=false;
                }
            if(aTypes[1]!=SGM::IntersectionType::PointType)
                {
                bAnswer=false;
                }
            }
        else
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==43)
        {
        // Test Point in Body

        bool bAnswer=true;

        SGM::Point3D Bottom(0,0,0),Top(0,0,2);
        double dRadius=1.0;
        SGM::Body BodyID=SGM::CreateCylinder(rResult,Bottom,Top,dRadius);
        SGM::Point3D Pos1(-2,0,1);
        bool bInBody1=SGM::PointInEntity(rResult,Pos1,BodyID);
        SGM::Point3D Pos2(0,0,1);
        bool bInBody2=SGM::PointInEntity(rResult,Pos2,BodyID);

        if(bInBody1==false)
            {
            bAnswer=false;
            }
        if(bInBody2==true)
            {
            bAnswer=false;
            }
        return bAnswer;
        }
    
    if(nTestNumber==44)
        {
        // Test Point in Body

        bool bAnswer=true;

        SGM::Point3D Pos0(0,0,0),Pos1(10,10,10);
        SGM::Body BodyID1=SGM::CreateBlock(rResult,Pos0,Pos1);
        double dVolume1=SGM::FindVolume(rResult,BodyID1,true);
        if(SGM::NearEqual(dVolume1,1000,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        
        SGM::Point3D Pos3(0,0,0),Pos4(0,0,1);
        SGM::Body BodyID2=SGM::CreateCylinder(rResult,Pos3,Pos4,1.0);
        double dVolume2=SGM::FindVolume(rResult,BodyID2,false);
        if(SGM::NearEqual(dVolume2,3.1415926535897932384626433832795,SGM_MIN_TOL,true)==false)
            {
            bAnswer=false;
            }

        return bAnswer;
        }

    if(nTestNumber==47)
        {
        bool bAnswer = true;

        SGM::Point3D Origin(1,1,-1.2);
        SGM::UnitVector3D Direction(0,-1,0);
        SGM::Point3D Center(1,0,0);
        SGM::UnitVector3D Axis(1,0,0);
        double dRadius = 1.5;
        double dTolerance = SGM_MIN_TOL;

        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;
        SGMInternal::IntersectLineAndCircle(Origin, Direction, SGM::Interval1D(-SGM_MAX, SGM_MAX), Center, Axis, dRadius, SGM_MIN_TOL, aPoints, aTypes);

        if (aPoints.size() != 2)
            return false;

        for (SGM::IntersectionType IType : aTypes )
            if (IType != SGM::PointType)
                return false;

        std::vector<SGM::Point3D> aExpected(2);
        aExpected[0] = SGM::Point3D(1,0.9,-1.2);
        aExpected[1] = SGM::Point3D(1,-0.9,-1.2);

        int found=0;
        for (SGM::Point3D PosExpected : aExpected)
            {
            for (SGM::Point3D PosComputed : aPoints)
                if (SGM::NearEqual(PosExpected, PosComputed, dTolerance))
                    found++;
            }
        
        bAnswer = (found == 2);
        return bAnswer;
        }

    if(nTestNumber==48)
        {
        SGM::Point3D Origin1(1,0,0);
        SGM::UnitVector3D Norm1(1,0,0);
        SGM::Point3D Origin2(-1,1,0);
        SGM::UnitVector3D Norm2(0,0,-1);
        bool bAnswer1 = Test_IntersectNonParallelPlanes(Origin1, Norm1, Origin2, Norm2);


        Origin1 = SGM::Point3D(1,1,1);
        Norm1 = SGM::UnitVector3D(1,0,0);
        Origin2= SGM::Point3D(-1,1,0);
        Norm2 = SGM::UnitVector3D(0,0,-1);
        bool bAnswer2 = Test_IntersectNonParallelPlanes(Origin1, Norm1, Origin2, Norm2);

        return (bAnswer1 && bAnswer2);
        }

    if(nTestNumber==49)
        {
        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints1;
        aPoints1.emplace_back(-2,.5,0);
        aPoints1.emplace_back(-1,1.5,0);
        aPoints1.emplace_back(0,1,0);
        aPoints1.emplace_back(1,1.5,0);
        aPoints1.emplace_back(2,2,0);

        // simple case
        //aPoints1.emplace_back(-2,.5,0);
        //aPoints1.emplace_back(-1,1.5,0);
        //aPoints1.emplace_back(0,.5,0);
        //aPoints1.emplace_back(1,.5,0);
        //aPoints1.emplace_back(2,.5,0);

        SGM::Curve CurveID = SGM::CreateNUBCurve(rResult, aPoints1);

        SGM::Point3D Origin(1,0,0);
        SGM::Point3D XPos(1,0,-1);
        SGM::Point3D YPos(1,1,0);

        SGM::Surface PlaneID = SGM::CreatePlane(rResult, Origin, XPos, YPos);

        std::vector<SGM::Point3D> aPoints;
        std::vector<SGM::IntersectionType> aTypes;
        double dTolerance = SGM_MIN_TOL;
        SGM::IntersectCurveAndSurface(rResult, CurveID, PlaneID, aPoints, aTypes, nullptr, nullptr, dTolerance);

        if (aPoints.size() != 1)
            return false;
        else
            {
              SGM::Point3D Expected(1, 1.5, 0);
              bAnswer = SGM::NearEqual(aPoints[0], Expected, dTolerance);
            }

        return bAnswer;
        }

    if(nTestNumber==50)
        {
        bool bAnswer=true;

        std::vector<SGM::Point3D> aNUBPoints;
        aNUBPoints.emplace_back(-2,.5,0);
        aNUBPoints.emplace_back(-1,1.5,0);
        aNUBPoints.emplace_back(0,1,0);
        aNUBPoints.emplace_back(1,1.5,0);
        aNUBPoints.emplace_back(2,2,0);
        SGM::Curve CurveID = SGM::CreateNUBCurve(rResult, aNUBPoints);

        SGM::Point3D AxisOrigin(-1,0,0);
        SGM::UnitVector3D Axis(1,0,0);
        SGM::Surface RevolveID = SGM::CreateRevolveSurface(rResult, AxisOrigin, Axis, CurveID);

        SGM::Point3D LineOrigin1(1,0,0);
        SGM::UnitVector3D uDirection1(0,0,1);
        SGM::Curve Line1ID = SGM::CreateLine(rResult, LineOrigin1, uDirection1);

        std::vector<SGM::Point3D> aPoints1;
        std::vector<SGM::IntersectionType> aTypes1;
        double dTolerance = SGM_MIN_TOL;
        SGM::IntersectCurveAndSurface(rResult, Line1ID, RevolveID, aPoints1, aTypes1, nullptr, nullptr, dTolerance);

        if (aPoints1.size() != 2)
            return false;

        for (SGM::IntersectionType IType : aTypes1 )
            if (IType != SGM::PointType)
                return false;

        std::vector<SGM::Point3D> aExpected1(2);
        aExpected1[0] = SGM::Point3D(1,0,1.5);
        aExpected1[1] = SGM::Point3D(1,0,-1.5);
        int found1=0;
        for (SGM::Point3D PosExpected : aExpected1)
            {
            for (SGM::Point3D PosComputed : aPoints1)
                if (SGM::NearEqual(PosExpected, PosComputed, dTolerance))
                    found1++;
            }


        SGM::Point3D LineOrigin2(1,1,-1.2);
        SGM::UnitVector3D uDirection2(0,-1,0);
        SGM::Curve Line2ID = SGM::CreateLine(rResult, LineOrigin2, uDirection2);

        std::vector<SGM::Point3D> aPoints2;
        std::vector<SGM::IntersectionType> aTypes2;
        SGM::IntersectCurveAndSurface(rResult, Line2ID, RevolveID, aPoints2, aTypes2, nullptr, nullptr, dTolerance);

        if (aPoints2.size() != 2)
            return false;

        for (SGM::IntersectionType IType : aTypes2 )
            if (IType != SGM::PointType)
                return false;

        std::vector<SGM::Point3D> aExpected2(2);
        aExpected2[0] = SGM::Point3D(1,0.9,-1.2);
        aExpected2[1] = SGM::Point3D(1,-0.9,-1.2);

        int found2=0;
        for (SGM::Point3D PosExpected : aExpected2)
            {
            for (SGM::Point3D PosComputed : aPoints2)
                if (SGM::NearEqual(PosExpected, PosComputed, dTolerance))
                    found2++;
            }


        bAnswer = ((found1 == 2) && (found2 == 2));
        return bAnswer;
        }

    if(nTestNumber==51)
        {
        // Test transform of a block.

        bool bAnswer=true;

        SGM::Point3D Pos0(0,0,0),Pos1(10,10,10);
        SGM::CreateBlock(rResult,Pos0,Pos1);
        SGM::Body BodyID2=SGM::CreateBlock(rResult,Pos0,Pos1);
        SGM::Transform3D Trans(SGM::Vector3D(20,0,0));
        SGM::TransformEntity(rResult,Trans,BodyID2);

        return bAnswer;
        }

    if(nTestNumber==52)
        {
        // Test imprinting of points on a complex

        bool bAnswer=true;

        std::vector<SGM::Point3D> aPoints;
        aPoints.push_back(SGM::Point3D(0,0,0));
        aPoints.push_back(SGM::Point3D(10,0,0));
        aPoints.push_back(SGM::Point3D(10,10,0));
        std::vector<unsigned int> aSegments;
        aSegments.push_back(0);
        aSegments.push_back(1);
        aSegments.push_back(1);
        aSegments.push_back(2);

        std::vector<SGM::Point3D> aNewPoints;
        aNewPoints.push_back(SGM::Point3D(2,0,0));
        aNewPoints.push_back(SGM::Point3D(7,0,0));
        aNewPoints.push_back(SGM::Point3D(10,5,0));

        SGMInternal::complex *pComplex=new SGMInternal::complex(rResult,aSegments,aPoints);
        pComplex->SplitAtPoints(rResult,aNewPoints,SGM_MIN_TOL);

        return bAnswer;
        }

    if(nTestNumber==53)
        {
        // Test grid creation and polygon imprinting.

        bool bAnswer=true;

        std::vector<double> aValues;
        aValues.push_back(0);
        aValues.push_back(1);
        aValues.push_back(2);
        aValues.push_back(3);
        std::vector<SGM::Point2D> aPoints2D;
        std::vector<SGM::Point3D> aPoints3D;
        std::vector<unsigned int> aTriangles,aSegments;

        SGM::CreateTrianglesFromGrid(aValues,aValues,aPoints2D,aTriangles);

        std::vector<SGM::Point3D> aGridPoints3D;
        for(auto Pos : aPoints2D)
            {
            aGridPoints3D.push_back(SGM::Point3D(Pos.m_u,Pos.m_v,0.0));
            }
        //new SGMInternal::complex(rResult,aGridPoints3D,aTriangles);

        std::vector<SGM::Point2D> aPolyPoints2D;
        std::vector<SGM::Point3D> aPolyPoints3D;
        SGM::Interval1D Domain(0,SGM_TWO_PI);
        size_t Index1,nPoints=12;
        double dRadius=1.5;
        for(Index1=0;Index1<nPoints;++Index1)
            {
            double t=Domain.MidPoint(Index1/(double)nPoints);
            double x=cos(t)*dRadius+1.5;
            double y=sin(t)*dRadius+1.5;
            aPolyPoints2D.push_back(SGM::Point2D(x,y));
            aPolyPoints3D.push_back(SGM::Point3D(x,y,0));
            aSegments.push_back((unsigned int)Index1);
            aSegments.push_back((unsigned int)((Index1+1)%nPoints));
            }
        //new SGMInternal::complex(rResult,aSegments,aPolyPoints3D);

        std::vector<unsigned int> aPolygonIndices;
        SGM::InsertPolygon(rResult,aPolyPoints2D,aPoints2D,aTriangles,aPolygonIndices);
        for(auto Pos : aPoints2D)
            {
            aPoints3D.push_back(SGM::Point3D(Pos.m_u,Pos.m_v,0.0));
            }
        new SGMInternal::complex(rResult,aPoints3D,aTriangles);

        return bAnswer;
        }

    if(nTestNumber==54)
        {
        // Test grid creation and polygon imprinting.

        bool bAnswer=true;

        std::vector<double> aValues;
        aValues.push_back(0);
        aValues.push_back(1);
        aValues.push_back(2);
        aValues.push_back(3);
        std::vector<SGM::Point2D> aPoints2D;
        std::vector<unsigned int> aTriangles,aSegments;

        SGM::CreateTrianglesFromGrid(aValues,aValues,aPoints2D,aTriangles);

        std::vector<SGM::Point3D> aGridPoints3D;
        for(auto Pos : aPoints2D)
            {
            aGridPoints3D.push_back(SGM::Point3D(Pos.m_u,Pos.m_v,0.0));
            }
        
        // Point (1,1,0) is at index 5.
        
        std::vector<unsigned int> aRemovedOrChanged,aReplacedTriangles;
        SGM::RemovePointFromTriangles(rResult,5,aPoints2D,aTriangles,aRemovedOrChanged,aReplacedTriangles);
        new SGMInternal::complex(rResult,aGridPoints3D,aTriangles);

        return bAnswer;
        }

    if(nTestNumber==55)
        {
        // Test grid creation and polygon imprinting.

        bool bAnswer=true;

        std::vector<double> aValues;
        aValues.push_back(0);
        aValues.push_back(1);
        aValues.push_back(2);
        aValues.push_back(3);
        std::vector<SGM::Point2D> aPoints2D;
        std::vector<unsigned int> aTriangles,aSegments;

        SGM::CreateTrianglesFromGrid(aValues,aValues,aPoints2D,aTriangles);

        std::vector<SGM::Point2D> aPolyPoints2D;
        std::vector<SGM::Point3D> aPolyPoints3D;
        
        aPolyPoints2D.push_back(SGM::Point2D(0,0));
        aPolyPoints3D.push_back(SGM::Point3D(0,0,0));
        aPolyPoints2D.push_back(SGM::Point2D(3,0));
        aPolyPoints3D.push_back(SGM::Point3D(3,0,0));
        aPolyPoints2D.push_back(SGM::Point2D(3,3));
        aPolyPoints3D.push_back(SGM::Point3D(3,3,0));
        //aPolyPoints2D.push_back(SGM::Point2D(0,3));
        //aPolyPoints3D.push_back(SGM::Point3D(0,3,0));
        aSegments.push_back(0);
        aSegments.push_back(1);
        aSegments.push_back(1);
        aSegments.push_back(2);
        aSegments.push_back(2);
        //aSegments.push_back(3);
        //aSegments.push_back(3);
        aSegments.push_back(0);
        
        //new SGMInternal::complex(rResult,aSegments,aPolyPoints3D);

        std::vector<unsigned int> aPolygonIndices;
        SGM::InsertPolygon(rResult,aPolyPoints2D,aPoints2D,aTriangles,aPolygonIndices);
        std::vector<SGM::Point3D> aPoints3D;
        for(auto Pos : aPoints2D)
            {
            aPoints3D.push_back(SGM::Point3D(Pos.m_u,Pos.m_v,0.0));
            }
        SGMInternal::complex *pComplex=new SGMInternal::complex(rResult,aPoints3D,aTriangles);
        pComplex->ReduceToUsedPoints();

        return bAnswer;
        }

    if(nTestNumber==59)
        {
        // Testing polygon trianglulation.

        bool bAnswer=true;

        std::vector<SGM::Point2D> aPoints2D;
        aPoints2D.push_back(SGM::Point2D(2 , 0));
        aPoints2D.push_back(SGM::Point2D(1 , 0));
        aPoints2D.push_back(SGM::Point2D(0 ,-1));
        aPoints2D.push_back(SGM::Point2D(-1, 0));
        aPoints2D.push_back(SGM::Point2D(0 , 1));
        aPoints2D.push_back(SGM::Point2D(1 , 0));
        aPoints2D.push_back(SGM::Point2D(2 , 0));
        aPoints2D.push_back(SGM::Point2D(0 , 2));
        aPoints2D.push_back(SGM::Point2D(-2, 0));
        aPoints2D.push_back(SGM::Point2D(0 ,-2));

        std::vector<SGM::Point3D> aPoints3D;
        aPoints3D.push_back(SGM::Point3D(2 , 0, 0));
        aPoints3D.push_back(SGM::Point3D(1 , 0, 0));
        aPoints3D.push_back(SGM::Point3D(0 ,-1, 0));
        aPoints3D.push_back(SGM::Point3D(-1, 0, 0));
        aPoints3D.push_back(SGM::Point3D(0 , 1, 0));
        aPoints3D.push_back(SGM::Point3D(1 , 0, 0));
        aPoints3D.push_back(SGM::Point3D(2 , 0, 0));
        aPoints3D.push_back(SGM::Point3D(0 , 2, 0));
        aPoints3D.push_back(SGM::Point3D(-2, 0, 0));
        aPoints3D.push_back(SGM::Point3D(0 ,-2, 0));

        std::vector<unsigned int> aTriangles,aSegments;
        std::vector<unsigned int> aPolygon={0,1,2,3,4,5,6,7,8,9};
        SGM::TriangulatePolygon(rResult,aPoints2D,aPolygon,aTriangles);
        SGM::CreateComplex(rResult,aPoints3D,aSegments,aTriangles);

        return bAnswer;
        }

    if(nTestNumber==60)
        {
        std::vector<SGM::Point3D> aPoints3D;
        std::vector<unsigned int> aSegments,aTriangles;
        SGM::Point3D Center(0,0,0);
        SGM::UnitVector3D ZAxis(0,0,1),XAxis(1,0,0);
        SGM::CreateIcosahedron(1.0,Center,ZAxis,XAxis,aPoints3D,aTriangles);
        SGM::CreateComplex(rResult,aPoints3D,aSegments,aTriangles);
        return true;
        }

    if(nTestNumber==61)
        {
        std::vector<SGM::Point3D> aPoints3D;
        std::vector<unsigned int> aSegments,aTriangles;
        SGM::Point3D Center(0,0,0);
        SGM::UnitVector3D ZAxis(0,0,1),XAxis(1,0,0);
        SGM::CreateOctahedron(1.0,Center,ZAxis,XAxis,aPoints3D,aTriangles,1);
        SGM::CreateComplex(rResult,aPoints3D,aSegments,aTriangles);
        return true;
        }

    if(nTestNumber==62)
        {
        std::vector<Point2D> aPoints2D;
        aPoints2D.push_back(SGM::Point2D(0,0));
        aPoints2D.push_back(SGM::Point2D(1,0));
        aPoints2D.push_back(SGM::Point2D(0,1));
        aPoints2D.push_back(SGM::Point2D(0.25,0.25));
        std::vector<std::vector<unsigned int> > aaPolygons;
        std::vector<unsigned int> aPolygon1,aPolygon2;
        aPolygon1.push_back(0);
        aPolygon1.push_back(1);
        aPolygon1.push_back(2);
        aPolygon2.push_back(3);
        aaPolygons.push_back(aPolygon1);
        aaPolygons.push_back(aPolygon2);
        std::vector<unsigned int> aTriangles,aAdjacencies;
        SGM::TriangulatePolygonWithHoles(rResult,aPoints2D,aaPolygons,aTriangles,aAdjacencies);
        if(aTriangles.size()!=9)
            {
            return false;
            }
        return true;
        }

    return false;
    }

