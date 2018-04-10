#include "SGMChecker.h"
#include "SGMPrimatives.h"
#include "SGMComplex.h"
#include "SGMMathematics.h"
#include "SGMTranslators.h"
#include "SGMDataClasses.h"
#include "SGMGeometry.h"
#include "SGMQuery.h"
#include "SGMTree.h"
#include "FileFunctions.h"
#include "EntityClasses.h"
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

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
        if(c==-109 || c==-108)  // String
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
        bFound=ReadFileLine(pTestFile,sFileLine);
        if(bFound)
            {
            if(RunFileLine(rResult,mFunctionMap,sTestDirectory,mVariableMap,sFileLine,pOutputFile)==false)
                {
                bPassed=false;
                }
            }
        }
    //if(rResult.GetResult()!=SGM::ResultTypeOK)
    //    {
    //    rResult.SetResult(SGM::ResultTypeOK);
    //    bPassed=false;
    //    }
    if(bPassed)
        {
        fprintf(pOutputFile,"Passed  \"%s\"\n",sFileName.c_str());
        }
    else
        {
        fprintf(pOutputFile,"Failed  \"%s\"\n",sFileName.c_str());
        }
    return bPassed;
    }

bool SGM::RunTestFile(SGM::Result       &rResult,
                      std::string const &sTestDirectory,
                      std::string const &sTestFileName,
                      std::string const &sOutputFileName)
    {
    std::map<std::string,SGMFunction> mFunctionMap;
    CreateFunctionMap(mFunctionMap);
    FILE *pOutputFile = fopen(sOutputFileName.c_str(),"w");
    if(pOutputFile==NULL)
        {
        rResult.SetResult(SGM::ResultType::ResultTypeFileOpen);
        rResult.SetMessage(sOutputFileName);
        return false;
        }

    std::string sFullPathName=sTestDirectory+"/"+sTestFileName;
    FILE *pTestFile = fopen(sFullPathName.c_str(),"rt");
    if(pTestFile==NULL)
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
    ReadDirectory(sTestDirectory,aFileNames);

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
            FindFileExtension(aFileNames[Index1],sExtension);
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
    fprintf(pOutputFile,"\n%ld Passed %ld Failed\n",nPassed,nFailed);
    fclose(pOutputFile);
    }

bool SGM::CompareFiles(SGM::Result       &rResult,
                       std::string const &sFile1,
                       std::string const &sFile2)
    {
    // Find the file types.

    std::string Ext1,Ext2;
    FindFileExtension(sFile1,Ext1);
    FindFileExtension(sFile2,Ext2);
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
        thing *pThing=rResult.GetThing();
        std::vector<double> aAreas1,aAreas2;
        aAreas1.reserve(nEntities1);
        aAreas2.reserve(nEntities2);
        size_t Index1;
        for(Index1=0;Index1<nEntities1;++Index1)
            {
            complex *pComplex=(complex *)(pThing->FindEntity(aEntities1[Index1].m_ID));
            aAreas1.push_back(pComplex->Area());
            delete pComplex;
            }
        for(Index1=0;Index1<nEntities1;++Index1)
            {
            complex *pComplex=(complex *)(pThing->FindEntity(aEntities2[Index1].m_ID));
            aAreas2.push_back(pComplex->Area());
            delete pComplex;
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
        ReadToString(pFile1,"Data;");
        ReadToString(pFile2,"Data;");
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

bool TestSurface(surface      const *pSurface,
                 SGM::Point2D const &uv1)
    {
    bool bAnswer=true;

    // Test to see if evalaute and inverse match.

    SGM::Point3D Pos,CPos;
    SGM::UnitVector3D Norm;
    SGM::Vector3D dU,dV,dUU,dUV,dVV;
    pSurface->Evaluate(uv1,&Pos,&dU,&dV,&Norm,&dUU,&dUV,&dVV);
    SGM::Point2D uv2=pSurface->Inverse(Pos,&CPos);

    if(SGM::NearEqual(uv1,uv2,SGM_ZERO)==false)
        {
        bAnswer=false;
        }
    if(SGM::NearEqual(Pos,CPos,SGM_ZERO)==false)
        {
        bAnswer=false;
        }

    // Test all the derivatives.

    double dx=1E-3,dy=1E-3;
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

    if(SGM::NearEqual(dU,dNU,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }
    if(SGM::NearEqual(dV,dNV,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }
    if(SGM::NearEqual(dUU,dNUU,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }
    if(SGM::NearEqual(dUV,dNUV,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }
    if(SGM::NearEqual(dVV,dNVV,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }

    return bAnswer;
    }

bool TestCurve(curve *pCurve,
               double t1)
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

    double h=1E-3;
    SGM::Point3D Pos0,Pos1,Pos2,Pos3;
    pCurve->Evaluate(t1-2*h,&Pos0);
    pCurve->Evaluate(t1-h,&Pos1);
    pCurve->Evaluate(t1+h,&Pos2);
    pCurve->Evaluate(t1+2*h,&Pos3);

    SGM::Vector3D VecN1=SGM::FirstDerivative<SGM::Point3D,SGM::Vector3D>(Pos0,Pos1,Pos2,Pos3,h);
    SGM::Vector3D VecN2=SGM::SecondDerivative<SGM::Point3D,SGM::Vector3D>(Pos0,Pos1,Pos,Pos2,Pos3,h);

    if(SGM::NearEqual(Vec1,VecN1,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }
    if(SGM::NearEqual(Vec2,VecN2,SGM_MIN_TOL)==false)
        {
        bAnswer=false;
        }

    return bAnswer;
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
        size_t nRoots=SGM::Quartic(2,-20,70,-100,48,aRoots);
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
        nRoots=SGM::Quartic(1,-9,29,-39,18,aRoots);
        if( nRoots!=3 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2) ||
            SGM_ZERO<fabs(aRoots[2]-3))
            {
            bAnswer=false;
            }

        // (x-1)(x-2)(x-2)(x-2) -> x^4-7*x^3+18*x^2-20*x+8 Two roots, one triple

        aRoots.clear();
        nRoots=SGM::Quartic(1,-7,18,-20,8,aRoots);
        if( nRoots!=2 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2))
            {
            bAnswer=false;
            }

        // (x-1)(x-1)(x-2)(x-2) -> x^4-6*x^3+13*x^2-12*x+4 Two double roots

        aRoots.clear();
        nRoots=SGM::Quartic(1,-6,13,-12,4,aRoots);
        if( nRoots!=2 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2))
            {
            bAnswer=false;
            }

        // (x-1)(x-2)(x^2+1) -> x^4-3*x^3+3*x^2-3*x+2 Two roots

        aRoots.clear();
        nRoots=SGM::Quartic(1,-3,3,-3,2,aRoots);
        if( nRoots!=2 || 
            SGM_ZERO<fabs(aRoots[0]-1) || 
            SGM_ZERO<fabs(aRoots[1]-2))
            {
            bAnswer=false;
            }

        // (x-1)(x-1)(x^2+1) -> x^4-2*x^3+2*x^2-2*x+1 One double root.

        aRoots.clear();
        nRoots=SGM::Quartic(1,-2,2,-2,1,aRoots);
        if( nRoots!=1 || 
            SGM_ZERO<fabs(aRoots[0]-1))
            {
            bAnswer=false;
            }

        // (x-1)(x-1)(x-1)(x-1) -> x^4-4*x^3+6*x^2-4*x+1 One quadruple root.

        aRoots.clear();
        nRoots=SGM::Quartic(1,-4,6,-4,1,aRoots);
        if( nRoots!=1 || 
            SGM_ZERO<fabs(aRoots[0]-1))
            {
            bAnswer=false;
            }

        // (x^2+1)(x^2+1) -> x^4+2*x^2+1 No roots.

        aRoots.clear();
        nRoots=SGM::Quartic(1,0,2,0,1,aRoots);
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
        double dScale=2.5;
        plane *pPlane=new plane(rResult,Origin,XAxis,YAxis,ZAxis,dScale);

        bool bAnswer=TestSurface(pPlane,SGM::Point2D(0.5,0.2));
        pPlane->Remove(rResult);

        return bAnswer;
        }

    if(nTestNumber==4)
        {
        // Test sphere inverse.

        SGM::Point3D Origin(10,11,12);
        SGM::UnitVector3D XAxis(1,2,3);
        SGM::UnitVector3D YAxis=XAxis.Orthogonal();
        double dRadius=2.5;
        sphere *pSphere=new sphere(rResult,Origin,dRadius,&XAxis,&YAxis);

        bool bAnswer=TestSurface(pSphere,SGM::Point2D(0.5,0.2));
        pSphere->Remove(rResult);

        return bAnswer;
        }

    if(nTestNumber==5)
        {
        // Test cylinder inverse.

        SGM::Point3D Bottom(10,11,12),Top(13,14,15);
        double dRadius=2.5;
        cylinder *pCylinder=new cylinder(rResult,Bottom,Top,dRadius);

        bool bAnswer=TestSurface(pCylinder,SGM::Point2D(0.5,0.2));
        pCylinder->Remove(rResult);

        return bAnswer;
        }

    if(nTestNumber==6)
        {
        // Test torus inverse.

        SGM::Point3D Origin(10,11,12);
        SGM::UnitVector3D ZAxis(1,2,3);
        torus *pTorus=new torus(rResult,Origin,ZAxis,2,5,true);

        bool bAnswer=TestSurface(pTorus,SGM::Point2D(0.5,0.2));
        pTorus->Remove(rResult);

        return bAnswer;
        }

    if(nTestNumber==7)
        {
        // Test cone inverse.

        SGM::Point3D Origin(10,11,12);
        SGM::UnitVector3D ZAxis(1,2,3);
        cone *pCone=new cone(rResult,Origin,ZAxis,2,0.4);

        bool bAnswer=TestSurface(pCone,SGM::Point2D(0.5,0.2));
        pCone->Remove(rResult);

        return bAnswer;
        }

    if(nTestNumber==8)
        {
        std::vector<double> aKnots={0,0,0,0,0.5,1,1,1,1};
        std::vector<SGM::Point3D> aControlPoints;
        aControlPoints.push_back(SGM::Point3D(1,1,0));
        aControlPoints.push_back(SGM::Point3D(1.166666666666666,1.166666666666666,0));
        aControlPoints.push_back(SGM::Point3D(2,2.8333333333333333,0));
        aControlPoints.push_back(SGM::Point3D(2.8333333333333333,1.166666666666666,0));
        aControlPoints.push_back(SGM::Point3D(3,1,0));

        NUBcurve *pNUB=new NUBcurve(rResult,aControlPoints,aKnots);

        bool bAnswer=TestCurve(pNUB,0.45);
        pNUB->Remove(rResult);

        return bAnswer;
        }

    if(nTestNumber==9)
        {
        SGM::Point3D Pos0(1,2,3),Pos1(4,5,6);
        SGM::UnitVector3D Axis(7,8,9);
        double dScale=10;

        line *pLine1=new line(rResult,Pos0,Pos1);
        bool bAnswer=TestCurve(pLine1,0.5);
        pLine1->Remove(rResult);

        line *pLine2=new line(rResult,Pos0,Axis,dScale);
        if(TestCurve(pLine2,0.5)==false)
            {
            bAnswer=false;
            }

        line *pLine3=new line(rResult,pLine2);
        if(TestCurve(pLine3,0.5)==false)
            {
            bAnswer=false;
            }
        pLine2->Remove(rResult);
        pLine3->Remove(rResult);

        return bAnswer;
        }

    if(nTestNumber==10)
        {
        SGM::Point3D Center(1,2,3);
        SGM::UnitVector3D Normal(4,5,6);
        double dRadius=2.1;
        SGM::Interval1D Domain(-1,1);
        SGM::UnitVector3D XAxis=Normal.Orthogonal();
   
        circle *pCircle1=new circle(rResult,Center,Normal,dRadius,&XAxis,&Domain);
        bool bAnswer=TestCurve(pCircle1,0.5);
        pCircle1->Remove(rResult);

        circle *pCircle2=new circle(rResult,Center,Normal,dRadius,&XAxis);
        if(TestCurve(pCircle2,0.5)==false)
            {
            bAnswer=false;
            }
        pCircle2->Remove(rResult);

        circle *pCircle3=new circle(rResult,Center,Normal,dRadius);
        if(TestCurve(pCircle3,0.5)==false)
            {
            bAnswer=false;
            }

        circle *pCircle4=new circle(rResult,pCircle3);
        if(TestCurve(pCircle4,0.5)==false)
            {
            bAnswer=false;
            }
        pCircle3->Remove(rResult);
        pCircle4->Remove(rResult);

        return bAnswer;
        }

    if(nTestNumber==11)
        {
        bool bAnswer=true;

        // if x=1,y=2,z=3,w=4, then
        //
        // 1x+2y+0z+0w= 5
        // 2x+2y+2z+0w=12
        // 0x+2y-1z+3w=13
        // 0x+0y+2z-1w= 2

        std::vector<std::vector<double> > aaMatrix;
        aaMatrix.reserve(4);
        std::vector<double> aRow;
        aRow.reserve(4);
        aRow.push_back(0);
        aRow.push_back(1);
        aRow.push_back(2);
        aRow.push_back(5);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(2);
        aRow.push_back(2);
        aRow.push_back(2);
        aRow.push_back(12);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(2);
        aRow.push_back(-1);
        aRow.push_back(3);
        aRow.push_back(13);
        aaMatrix.push_back(aRow);
        aRow.clear();
        aRow.push_back(2);
        aRow.push_back(-1);
        aRow.push_back(0);
        aRow.push_back(2);
        aaMatrix.push_back(aRow);

        if(SGM::TridiagonalSolve(aaMatrix)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[0][3],1,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[1][3],2,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[2][3],3,SGM_ZERO,false)==false)
            {
            bAnswer=false;
            }
        if(SGM::NearEqual(aaMatrix[3][3],4,SGM_ZERO,false)==false)
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
        aPoints.push_back(SGM::Point3D(0,0,0));
        aPoints.push_back(SGM::Point3D(3,4,0));
        aPoints.push_back(SGM::Point3D(-1,4,0));
        aPoints.push_back(SGM::Point3D(-4,0,0));
        aPoints.push_back(SGM::Point3D(-4,-3,0));

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
        aPoints.push_back(SGM::Point3D(1,1,0));
        aPoints.push_back(SGM::Point3D(2,2,0));
        aPoints.push_back(SGM::Point3D(3,1,0));

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
        SGM::EvaluateCurve(rResult,NUBID,0.0,NULL,&Vec0);
        SGM::EvaluateCurve(rResult,NUBID,1.0,NULL,&Vec1);
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
        aPoints.push_back(SGM::Point3D(1,1,0));
        aPoints.push_back(SGM::Point3D(2,2,0));
        aPoints.push_back(SGM::Point3D(3,1,0));
        aPoints.push_back(SGM::Point3D(5,0,0));

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
        SGM::EvaluateCurve(rResult,NUBID,0.0,NULL,&Vec0);
        SGM::EvaluateCurve(rResult,NUBID,1.0,NULL,&Vec1);
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
        aPoints.push_back(SGM::Point3D(1,1,0));
        aPoints.push_back(SGM::Point3D(3,1,0));

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
        SGM::EvaluateCurve(rResult,NUBID,0.0,NULL,&Vec0);
        SGM::EvaluateCurve(rResult,NUBID,1.0,NULL,&Vec1);
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
        aPoints.push_back(SGM::Point3D(1,1,0));
        aPoints.push_back(SGM::Point3D(2,2,0));
        aPoints.push_back(SGM::Point3D(3,1,0));

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
        aPoints.push_back(SGM::Point3D(1,1,0));
        aPoints.push_back(SGM::Point3D(3,1,0));

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
        // Test cylinder inverse.

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
        aaPoints[0][0]=SGM::Point3D(0,0,1);
        aaPoints[0][1]=SGM::Point3D(0,1,0);
        aaPoints[0][2]=SGM::Point3D(0,2,-1);
        aaPoints[1][0]=SGM::Point3D(1,0,0);
        aaPoints[1][1]=SGM::Point3D(1,1,0);
        aaPoints[1][2]=SGM::Point3D(1,2,0);
        aaPoints[2][0]=SGM::Point3D(2,0,-1);
        aaPoints[2][1]=SGM::Point3D(2,1,0);
        aaPoints[2][2]=SGM::Point3D(2,2,1);
        NUBsurface *pNUB=new NUBsurface(rResult,aaPoints,aUKnots,aVKnots);

        bool bAnswer=TestSurface(pNUB,SGM::Point2D(0.3,0.2));
        pNUB->Remove(rResult);

        return bAnswer;
        }


    return false;
    }

bool SGM::CompareSizes(size_t nSize1,size_t nSize2)
    {
    return nSize1==nSize2;
    }
