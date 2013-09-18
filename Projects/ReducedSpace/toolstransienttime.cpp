//
//  toolstransienttime.cpp
//  PZ
//
//  Created by Agnaldo Farias on 9/5/12.
//  Copyright (c) 2012 LabMec-Unicamp. All rights reserved.
//

#include <iostream>

#include "toolstransienttime.h"


#include "pzmat1dlin.h"
#include "pzbuildmultiphysicsmesh.h"
#include "pzintel.h"
#include "pzskylstrmatrix.h"
#include "pzstepsolver.h"
#include "TPZSpStructMatrix.h"
#include "pzfstrmatrix.h"
#include "../HydraulicFracturePropagation/PlaneFracture/TPZJIntegral.h"
#include "pzreducedspace.h"
#include "pzbndcond.h"
#include "pzl2projection.h"
#include "tpzmathtools.cpp"

//Plasticidade
#include "pzelastoplasticanalysis.h"
#include "TPZSandlerDimaggio.h"
#include "TPZThermoForceA.h"
#include "TPZElasticResponse.h"
#include "TPZTensor.h"
#include "BrazilianTestGeoMesh.h"
#include "pzpostprocmat.h"
#include "pzpostprocanalysis.h"
//Plasticidade

#include "pzlog.h"
#ifdef LOG4CXX
static LoggerPtr logdata(Logger::getLogger("pz.toolstransienttime"));
#endif


std::set<int> locSetPressureMatIds;


ToolsTransient::ToolsTransient(){
    fMustStop = true;
    fCouplingMaterial = NULL;
    fgmesh = NULL;
    fmeshvec.Resize(0);
    fmphysics = NULL;
    DebugStop();//Nao deveria utilizar este construtor
}

ToolsTransient::ToolsTransient(int pOrder)
{
    fpOrder = pOrder;
    fMustStop = false;
    int NStripes = globFractInputData.NStripes();
    for(int stripe = 0; stripe < NStripes; stripe++)
    {
        locSetPressureMatIds.insert(globPressureMatId + stripe);
    }
    int dim = 2;
    fCouplingMaterial = new TPZNLFluidStructure2d(globMultiFisicMatId,dim);
    int planestrain = 0;
    fCouplingMaterial->SetfPlaneProblem(planestrain);
    fgmesh = NULL;
    fmeshvec.Resize(2);
    fmphysics = NULL;
}

ToolsTransient::~ToolsTransient(){
    
}


void ToolsTransient::RunPlasticity()
{
    //std::map<int,REAL> leakoffMap;
    
    const REAL Lmax_edge = 10.;
    
    this->Mesh2D(Lmax_edge);
    
    TPZCompMesh * cmesh = CMeshElastoPlastic(fgmesh, globFractInputData.SigN());
    TPZElastoPlasticAnalysis an(cmesh,std::cout);
    
    this->SolveInitialElastoPlasticity(an, cmesh);
    
    std::cout << "<<<<<<<<<<<<<<<<<<<<<<< 0\n";
    std::stringstream notUsedHere("none.txt");
    REAL KI = ComputeKIPlaneStrain();
    std::cout << "KI = " << KI << " >>>>>>>>>>>>>>>>>>>\n\n\n";
    
    std::string vtkFile = "pocoplastico.vtk";
    TPZPostProcAnalysis ppanalysis(cmesh);
    ppanalysis.SetStep(0);
    TPZFStructMatrix structmatrix(ppanalysis.Mesh());
    structmatrix.SetNumThreads(8);
    ppanalysis.SetStructuralMatrix(structmatrix);
    
    TPZVec<int> PostProcMatIds(1,1);
    TPZStack<std::string> PostProcVars, scalNames, vecNames;
    scalNames.Push("Alpha");
    scalNames.Push("PlasticSqJ2");
    scalNames.Push("PlasticSqJ2El");
    scalNames.Push("POrder");
    
    scalNames.Push("I1Stress");
    scalNames.Push("J2Stress");
    
    vecNames.Push("Displacement");
    vecNames.Push("YieldSurface");
    vecNames.Push("NormalStress");
    vecNames.Push("ShearStress");
    vecNames.Push("NormalStrain");
    vecNames.Push("ShearStrain");
    vecNames.Push("DisplacementMem");
    for (int i=0; i<scalNames.size(); i++)
    {
        PostProcVars.Push(scalNames[i]);
    }
    for (int i=0; i<vecNames.size(); i++)
    {
        PostProcVars.Push(vecNames[i]);
    }
    //
    ppanalysis.SetPostProcessVariables(PostProcMatIds, PostProcVars);
    //
    ppanalysis.DefineGraphMesh(2,scalNames,vecNames,vtkFile);
    //	
    ppanalysis.TransferSolution();
    ppanalysis.PostProcess(0);// pOrder    
}


void ToolsTransient::Run()
{
    const REAL Lmax_edge = 10.;
    TPZCompMesh * lastMPhysicsCMesh = NULL;
    TPZCompMesh * lastElastReferredCMesh = NULL;
    
    int anCount = 0;
    this->InitializeUncoupledMeshesAttributes(Lmax_edge);
    this->InitializeMultphysicsMeshAttribute();
    TPZAnalysis * an = new TPZAnalysis(fmphysics);
    globFractOutputData.PlotElasticVTK(an, anCount);
    PostprocessPressure();
    PostProcessAcumVolW();
    PostProcessVolLeakoff();
    REAL KI = ComputeKIPlaneStrain();
    globFractOutputData.InsertTKI(globFractInputData.actTime(), KI);//its for output data to txt (Mathematica format)
    
    bool initialElasticKickIsNeeded = true;
    while(fMustStop == false)
    {
        bool propagate = this->SolveSistTransient(an, initialElasticKickIsNeeded);
        initialElasticKickIsNeeded = false;
        anCount = an->GetStep();
        
        if(propagate == true)///Setting new Lfrac and tranferring solution
        {
            TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(fmeshvec, fmphysics);
            lastMPhysicsCMesh = fmphysics;
            lastElastReferredCMesh = fmeshvec[0];
        
            REAL newLfrac = globFractInputData.Lf() + Lmax_edge;
            globFractInputData.SetLf(newLfrac);
        
            this->InitializeUncoupledMeshesAttributes(Lmax_edge);
            this->InitializeMultphysicsMeshAttribute();
            this->TransferSolutions(lastMPhysicsCMesh, lastElastReferredCMesh);
            an = new TPZAnalysis(fmphysics);
            
            globFractOutputData.PlotElasticVTK(an, anCount);
            PostprocessPressure();
            PostProcessAcumVolW();
            PostProcessVolLeakoff();
        }
    }
    
    globFractOutputData.SetQinj1WingAndLfracmax(globFractInputData.Qinj(), globFractInputData.Lf());
    std::ofstream outMath("OutputMathematica.txt");
    globFractOutputData.PrintMathematica(outMath);
}


//------------------------------------------------------------------------------------


void ToolsTransient::InitializeUncoupledMeshesAttributes(REAL Lmax_edge)
{
    TPZCompMesh * elastReference = ElastCMeshReferenceProcessed(Lmax_edge);
    
    fmeshvec[0] = this->CMeshReduced(elastReference);
    fmeshvec[1] = this->CMeshPressure();
    fgmesh->ResetReference();
}

TPZCompMesh * ToolsTransient::ElastCMeshReferenceProcessed(REAL Lmax_edge)
{
    //Principal Geometric Mesh (Lf initial)
    this->Mesh2D(Lmax_edge);
    
    TPZCompMesh * cmesh_elast = this->CMeshElastic();
    TPZFMatrix<STATE> solutions(0,0);
    
    TPZAnalysis * an = new TPZAnalysis;
    int NStripes = globFractInputData.NStripes();
    for(int stripe = 0; stripe < NStripes; stripe++)
    {
        /** Resolvendo um problema modelo de elastica linear para utilizar a
         solucao como espaco de aproximacao do problema nao linear (acoplado) */
        SetSigmaNStripeNum(cmesh_elast,stripe);
        
        bool mustOptimizeBandwidth = (stripe == 0);
        an->SetCompMesh(cmesh_elast, mustOptimizeBandwidth);
        this->SolveInitialElasticity(*an, cmesh_elast);
        if(stripe == 0)
        {
            solutions.Resize(cmesh_elast->Solution().Rows(), NStripes);
        }
        for(int r = 0; r < cmesh_elast->Solution().Rows(); r++)
        {
            solutions(r,stripe) = cmesh_elast->Solution()(r,0);
        }
        
//        {
//            std::string plotfile = "StripeSolution.vtk";
//            
//            TPZManVector<std::string,10> scalnames(1), vecnames(1);
//            scalnames[0] = "SigmaY";
//            vecnames[0]  = "Displacement";
//            
//            const int dim = 2;
//            int div = 0;
//            an->DefineGraphMesh(dim,scalnames,vecnames,plotfile);
//            an->PostProcess(div);
//        }
    }
    cmesh_elast->LoadSolution(solutions);
    
    return cmesh_elast;
}

void ToolsTransient::Mesh2D(REAL Lmax_edge)
{
    fgmesh = new TPZGeoMesh;
    
    int ndivV = int(globFractInputData.Lx()/Lmax_edge + 0.5);
    int ndivH = int(globFractInputData.Ly()/Lmax_edge + 0.5);
    
    int ncols = ndivV + 1;
    int nrows = ndivH + 1;
    int nnodes = nrows*ncols;
    
    fgmesh->NodeVec().Resize(nnodes);
    
    REAL deltadivV = globFractInputData.Lx()/ndivV;
    REAL deltandivH = globFractInputData.Ly()/ndivH;
    
    int nid = 0;
    REAL cracktipDist = globFractInputData.Lf();
    int colCracktip = -1;
    for(int r = 0; r < nrows; r++)
    {
        for(int c = 0; c < ncols; c++)
        {
            REAL x = c*deltadivV;
            REAL y = r*deltandivH;
            REAL dist = fabs(globFractInputData.Lf()-x);
            if(r == 0 && dist < cracktipDist)
            {
                cracktipDist = dist;
                colCracktip = c;
            }
            
            TPZVec<REAL> coord(3,0.);
            coord[0] = x;
            coord[1] = y;
            fgmesh->NodeVec()[r*ncols + c].SetCoord(coord);
            fgmesh->NodeVec()[r*ncols + c].SetNodeId(nid);
            nid++;
        }
    }
    if(colCracktip == 0)
    {
        colCracktip = 1;//fratura minima corresponde aa distancia entre coluna 0 e coluna 1
    }
    
    TPZGeoEl * gel = NULL;
    TPZVec<int> topol(4);
    int indx = 0;
    for(int r = 0; r < nrows-1; r++)
    {
        for(int c = 0; c < ncols-1; c++)
        {
            topol[0] = r*(ncols) + c;
            topol[1] = r*(ncols) + c + 1;
            topol[2] = r*(ncols) + c + 1 + ncols;
            topol[3] = r*(ncols) + c + ncols;
            
            gel = fgmesh->CreateGeoElement(EQuadrilateral, topol, globReservMatId, indx);
            gel->SetId(indx);
            indx++;
        }
    }
    
    fgmesh->BuildConnectivity();
    
    REAL stripeWidth = globFractInputData.Lf() / globFractInputData.NStripes();
    int nelem = fgmesh->NElements();
    for(int el = 0; el < nelem; el++)
    {
        TPZGeoEl * gel = fgmesh->ElementVec()[el];
        
        //south BC
        TPZGeoElSide sideS(gel,4);
        TPZGeoElSide neighS(sideS.Neighbour());
        if(sideS == neighS)
        {
            if(el < colCracktip)
            {
                TPZGeoEl * bcFrac = gel->CreateBCGeoEl(4, globPressureMatId);
                
                //Increasing materialId number with respect with what stripe contains bcFrac
                TPZVec<REAL> centerQsi(bcFrac->Dimension(),0.);
                TPZVec<REAL> centerX(3,0.);
                bcFrac->CenterPoint(bcFrac->NSides()-1, centerQsi);
                bcFrac->X(centerQsi, centerX);
                REAL xCoord = centerX[0];
                int stripeNumber = (int)(xCoord/stripeWidth);
                bcFrac->SetMaterialId(globPressureMatId + stripeNumber);
                ////////
            }
            else
            {
                gel->CreateBCGeoEl(4, globDirichletElastMatId);
            }
        }
        
        //east BC
        TPZGeoElSide sideE(gel,5);
        TPZGeoElSide neighE(sideE.Neighbour());
        if(sideE == neighE)
        {
            gel->CreateBCGeoEl(5, globDirichletElastMatId);
        }
        
        //north BC
        TPZGeoElSide sideN(gel,6);
        TPZGeoElSide neighN(sideN.Neighbour());
        if(sideN == neighN)
        {
            gel->CreateBCGeoEl(6, globDirichletElastMatId);
        }
        
        //west BC
        TPZGeoElSide sideW(gel,7);
        TPZGeoElSide neighW(sideW.Neighbour());
        if(sideW == neighW)
        {
            gel->CreateBCGeoEl(7, globBlockedXElastMatId);
        }
    }
    
    topol.Resize(1);
    for(int p = 0; p < ncols; p++)
    {
        topol[0] = p;
        if(p == 0)
        {
            gel = fgmesh->CreateGeoElement(EPoint, topol, globBCfluxIn, indx);
        }
        else if(p == colCracktip)
        {
            gel = fgmesh->CreateGeoElement(EPoint, topol, globBCfluxOut, indx);
        }
        indx++;
    }
    fgmesh->BuildConnectivity();
    
    //#ifdef usingQPoints
    //    TPZGeoElSide pt(gel,0);
    //    TPZGeoElSide ptneigh(pt.Neighbour());
    //    while(pt != ptneigh)
    //    {
    //        if(ptneigh.Element()->HasSubElement() == false)
    //        {
    //            int neighSide = ptneigh.Side();
    //            TPZGeoEl * ptneighEl = TPZChangeEl::ChangeToQuarterPoint(fgmesh, ptneigh.Element()->Id(), neighSide);
    //            ptneigh = ptneighEl->Neighbour(neighSide);
    //        }
    //        else
    //        {
    //            ptneigh = ptneigh.Neighbour();
    //        }
    //    }
    //#endif
    
    int nrefUnif = 3;
    for(int ref = 0; ref < nrefUnif; ref++)
    {
        nelem = fgmesh->NElements();
        for(int el = 0; el < nelem; el++)
        {
            if(fgmesh->ElementVec()[el]->Dimension() < 1) continue;
            if(fgmesh->ElementVec()[el]->HasSubElement()) continue;
            if(locSetPressureMatIds.find(fgmesh->ElementVec()[el]->MaterialId()) != locSetPressureMatIds.end())
            {
                TPZVec<TPZGeoEl*> sons;
                fgmesh->ElementVec()[el]->Divide(sons);
                continue;
            }
            //else...
            for(int s = 0; s < fgmesh->ElementVec()[el]->NSides(); s++)
            {
                TPZGeoElSide gelside(fgmesh->ElementVec()[el],s);
                TPZGeoElSide neighside(gelside.Neighbour());
                bool refinedAlready = false;
                while(neighside != gelside)
                {
                    if(locSetPressureMatIds.find(neighside.Element()->MaterialId()) != locSetPressureMatIds.end())
                    {
                        TPZVec<TPZGeoEl*> sons;
                        fgmesh->ElementVec()[el]->Divide(sons);
                        refinedAlready = true;
                        break;
                    }
                    neighside = neighside.Neighbour();
                }
                if(refinedAlready == true)
                {
                    break;
                }
            }
        }
    }
    
    //#ifdef usingRefdir
    //    std::set<int> matDir;
    //    //matDir.insert(__2DfractureMat_inside);
    //    matDir.insert(bcfluxOut);
    //    int nrefDir = 1;
    //    for(int ref = 0; ref < nrefDir; ref++)
    //    {
    //        nelem = fgmesh->NElements();
    //        for(int el = 0; el < nelem; el++)
    //        {
    //            if(!gmesh->ElementVec()[el]) continue;
    //            if(gmesh->ElementVec()[el]->Dimension() < 1) continue;
    //            if(gmesh->ElementVec()[el]->HasSubElement()) continue;
    //            TPZRefPatternTools::RefineDirectional(gmesh->ElementVec()[el], matDir);
    //        }
    //    }
    //#endif
}

TPZCompMesh * ToolsTransient::CMeshElastic()
{
    /// criar materiais
	int dim = 2;
	
    TPZVec<REAL> force(dim,0.);

    //int planestress = 1;
    int planestrain = 0;
    
    TPZElasticityMaterial *material;
	material = new TPZElasticityMaterial(globReservMatId, globFractInputData.E(), globFractInputData.Poisson(), globFractInputData.Fx(), globFractInputData.Fy(), planestrain);
    
    TPZMaterial * mat(material);
    
    ///criar malha computacional
    TPZCompMesh * cmesh = new TPZCompMesh(fgmesh);
    cmesh->SetDefaultOrder(fpOrder);
	cmesh->SetDimModel(dim);
    cmesh->InsertMaterialObject(mat);
    
    ///Inserir condicao de contorno
    REAL big = material->gBigNumber;
    
    TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,0.);
    
    int NStripes = globFractInputData.NStripes();
    for(int stripe = 0; stripe < NStripes; stripe++)
    {
        TPZMaterial * BCond1 = material->CreateBC(mat, globPressureMatId + stripe, neumann, val1, val2);
        cmesh->InsertMaterialObject(BCond1);
    }
    val1.Redim(2,2);
    val2.Redim(2,1);
    TPZMaterial * BCond2 = material->CreateBC(mat, globDirichletElastMatId, dirichlet, val1, val2);
    
    val1(0,0) = big;
    TPZMaterial * BCond3 = material->CreateBC(mat, globBlockedXElastMatId, mixed, val1, val2);
    
    cmesh->SetAllCreateFunctionsContinuous();
	cmesh->InsertMaterialObject(BCond2);
	cmesh->InsertMaterialObject(BCond3);
	
	//Ajuste da estrutura de dados computacional
	cmesh->AutoBuild();
    cmesh->AdjustBoundaryElements();
	cmesh->CleanUpUnconnectedNodes();
    
	return cmesh;
}

void ToolsTransient::SetSigmaNStripeNum(TPZCompMesh * cmeshref, int actStripe)
{
    std::set<int>::iterator it;
    int stripe = 0;
    for(it = locSetPressureMatIds.begin(); it != locSetPressureMatIds.end(); it++)
    {
        int matId = *it;
        TPZMaterial * mat = cmeshref->MaterialVec().find(matId)->second;
        if(!mat)
        {
            DebugStop();
        }
        TPZBndCond * bcmat = dynamic_cast<TPZBndCond *>(mat);
        if(!bcmat)
        {
            DebugStop();
        }
        
        if(stripe < actStripe)
        {
            bcmat->Val2()(1,0) = 0.;
        }
        else if(stripe == actStripe)
        {
            bcmat->Val2()(1,0) = globFractInputData.SigN();
        }
        else if(stripe > actStripe)
        {
            return;
        }
        stripe++;
    }
}

TPZCompMeshReferred * ToolsTransient::CMeshReduced(TPZCompMesh *cmeshref){
    
    /// criar materiais
	int dim = 2;
    
    TPZVec<REAL> force(dim,0.);
    int planestrain = 0;
    
    TPZElasticityMaterial *material;
	material = new TPZElasticityMaterial(globReservMatId,
                                         globFractInputData.E(),
                                         globFractInputData.Poisson(),
                                         globFractInputData.Fx(),
                                         globFractInputData.Fy(),
                                         planestrain);
    
    material->SetPreStress(globFractInputData.PreStressXX(), globFractInputData.PreStressYY(), globFractInputData.PreStressXY(), 0.);
    
    TPZCompMeshReferred *cmeshreferred = new TPZCompMeshReferred(fgmesh);
    
    cmeshreferred->SetDimModel(dim);
    TPZMaterial * mat(material);
    cmeshreferred->InsertMaterialObject(mat);
    
    ///Inserir condicao de contorno
    REAL big = material->gBigNumber;
    
    TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,0.);
    int NStripes = globFractInputData.NStripes();
    for(int stripe = 0; stripe < NStripes; stripe++)
    {
        TPZMaterial * BCond1 = material->CreateBC(mat, globPressureMatId + stripe, neumann, val1, val2);
        cmeshreferred->InsertMaterialObject(BCond1);
    }
    
    val1.Redim(2,2);
    val2.Redim(2,1);
    TPZMaterial * BCond2 = material->CreateBC(mat, globDirichletElastMatId, dirichlet, val1, val2);
    
    val1(0,0) = big;
    TPZMaterial * BCond3 = material->CreateBC(mat, globBlockedXElastMatId, mixed, val1, val2);
    
    int numsol = cmeshref->Solution().Cols();
    cmeshreferred->AllocateNewConnect(numsol, 1, 1);
    
	TPZReducedSpace::SetAllCreateFunctionsReducedSpace(cmeshreferred);
    
    cmeshreferred->InsertMaterialObject(BCond2);
    cmeshreferred->InsertMaterialObject(BCond3);
    
	cmeshreferred->SetDefaultOrder(fpOrder);
    cmeshreferred->SetDimModel(dim);
	
    fgmesh->ResetReference();
	cmeshreferred->AutoBuild();
    cmeshref->AdjustBoundaryElements();
	cmeshref->CleanUpUnconnectedNodes();
    cmeshreferred->LoadReferred(cmeshref);
    
    return cmeshreferred;
}

TPZCompMesh * ToolsTransient::CMeshPressure(){
    
    ///criar malha computacional
    TPZCompMesh * cmesh = new TPZCompMesh(fgmesh);
    cmesh->SetDefaultOrder(fpOrder);
    int dim = 1;
	cmesh->SetDimModel(dim);
    
    /// criar materiais
    TPZFMatrix<REAL> xk(1,1,1.);
    TPZFMatrix<REAL> xc(1,1,0.);
    TPZFMatrix<REAL> xb(1,1,0.);
    TPZFMatrix<REAL> xf(1,1,-2.);
    int NStripes = globFractInputData.NStripes();
    for(int stripe = 0; stripe < NStripes; stripe++)
    {
        TPZMat1dLin *material;
        material = new TPZMat1dLin(globPressureMatId + stripe);
        material->SetMaterial(xk,xc,xb,xf);
        
        TPZMaterial * mat(material);
    
        cmesh->InsertMaterialObject(mat);
        
        if(stripe == 0)
        {
            ///Inserir condicao de contorno
            TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,0.);
            val2(0,0) = globFractInputData.Qinj();
            TPZMaterial * BCond1 = material->CreateBC(mat, globBCfluxIn, neumann, val1, val2);
            cmesh->InsertMaterialObject(BCond1);
        }
        if(stripe == NStripes-1)
        {
            TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,0.);
            TPZMaterial * BCond2 = material->CreateBC(mat, globBCfluxOut, dirichlet, val1, val2);
            cmesh->InsertMaterialObject(BCond2);
        }
        cmesh->InsertMaterialObject(mat);
    }
    cmesh->SetAllCreateFunctionsContinuous();
	
	//Ajuste da estrutura de dados computacional
	cmesh->AutoBuild();
    cmesh->AdjustBoundaryElements();
	cmesh->CleanUpUnconnectedNodes();
    
	return cmesh;
}

void ToolsTransient::SolveInitialElasticity(TPZAnalysis &an, TPZCompMesh *Cmesh)
{
	TPZSkylineStructMatrix full(Cmesh); //caso simetrico
	an.SetStructuralMatrix(full);
	TPZStepSolver<REAL> step;
	step.SetDirect(ELDLt); //caso simetrico
	an.SetSolver(step);
	an.Run();
}

void ToolsTransient::InitializeMultphysicsMeshAttribute()
{    
    //Creating computational mesh for multiphysic elements
	fgmesh->ResetReference();
	fmphysics = new TPZCompMesh(fgmesh);
    fmphysics->SetDefaultOrder(fpOrder);
    
    TPZMaterial *mat(fCouplingMaterial);
    fmphysics->InsertMaterialObject(mat);
    
    ///Inserir condicao de contorno
    REAL big = fCouplingMaterial->gBigNumber;
    
    TPZFMatrix<REAL> val1(3,2,0.), val2(3,1,0.);

    int NStripes = globFractInputData.NStripes();
    for(int stripe = 0; stripe < NStripes; stripe++)
    {
        TPZMaterial * BCond1 = fCouplingMaterial->CreateBC(mat, globPressureMatId + stripe, neumann, val1, val2);
        fmphysics->InsertMaterialObject(BCond1);
    }
    
    val2.Redim(3,1);
    val1.Redim(3,2);
    TPZMaterial * BCond2 = fCouplingMaterial->CreateBC(mat, globDirichletElastMatId, globDir_elast, val1, val2);
    
    val1(0,0) = big;
    TPZMaterial * BCond3 = fCouplingMaterial->CreateBC(mat, globBlockedXElastMatId, globMix_elast, val1, val2);
    
    val2.Redim(3,1);
    val1.Redim(3,2);
    val2(2,0) = globFractInputData.Qinj();
    TPZMaterial * BCond4 = fCouplingMaterial->CreateBC(mat, globBCfluxIn, globNeum_pressure, val1, val2);
    
    val2.Redim(3,1);
    val1.Redim(3,2);
    TPZMaterial * BCond5 = fCouplingMaterial->CreateBC(mat, globBCfluxOut, globNeum_pressure, val1, val2);
    
    fmphysics->SetAllCreateFunctionsMultiphysicElem();
    fmphysics->InsertMaterialObject(BCond2);
    fmphysics->InsertMaterialObject(BCond3);
    fmphysics->InsertMaterialObject(BCond4);
    fmphysics->InsertMaterialObject(BCond5);
    
    fmphysics->AutoBuild();
	fmphysics->AdjustBoundaryElements();
	fmphysics->CleanUpUnconnectedNodes();
    
    // Creating multiphysic elements into mphysics computational mesh
	TPZBuildMultiphysicsMesh::AddElements(fmeshvec, fmphysics);
	TPZBuildMultiphysicsMesh::AddConnects(fmeshvec, fmphysics);
	TPZBuildMultiphysicsMesh::TransferFromMeshes(fmeshvec, fmphysics);
}


//------------------------------------------------------------------------------------


void ToolsTransient::StiffMatrixLoadVec(TPZAnalysis *an, TPZAutoPointer< TPZMatrix<REAL> > & matK1, TPZFMatrix<REAL> &fvec)
{
	fCouplingMaterial->SetCurrentState();
    TPZFStructMatrix matsk(fmphysics);

	an->SetStructuralMatrix(matsk);
	TPZStepSolver<REAL> step;

	step.SetDirect(ELU);
	an->SetSolver(step);
    
    an->Assemble();
	
    matK1 = an->Solver().Matrix();

	fvec = an->Rhs();
}

void ToolsTransient::TransferSolutions(TPZCompMesh * lastMPhysicsCMesh, TPZCompMesh * lastElastReferredCMesh)
{
    TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(fmeshvec, fmphysics);
    TransferElasticSolution(lastElastReferredCMesh);
    TPZBuildMultiphysicsMesh::TransferFromMeshes(fmeshvec, fmphysics);

    std::map<int,REAL> leakoffMap = TransferLeakoff(lastMPhysicsCMesh);
    fCouplingMaterial->SetLeakoffData(leakoffMap);
}

void ToolsTransient::TransferElasticSolution(TPZCompMesh * cmeshFrom)
{
#ifdef DEBUG
    if(!cmeshFrom)
    {
        DebugStop();
    }
#endif
    
    std::cout << "\n******************** TRANSFERINDO ********************\n\n";
    
    REAL AreaFrom = IntegrateSolution(cmeshFrom, 0);
    
    TPZAutoPointer< TPZFunction<STATE> > func = new TElastSolFunction<STATE>(cmeshFrom);

    //Setting old solution as forcing function on new cmesh (fmeshvec[0])
    TPZMaterial * elastMat = NULL;
    std::map<int,TPZMaterial*>::iterator it = fmeshvec[0]->MaterialVec().find(globReservMatId);
    if(it != fmeshvec[0]->MaterialVec().end())
    {
        elastMat = it->second;
        elastMat->SetForcingFunction(func);
    }
    else
    {
        DebugStop();//cade o mardito material???
    }
    
    //////Solving
    TPZAnalysis anTo(fmeshvec[0]);
    TPZSkylineStructMatrix full(fmeshvec[0]);
    anTo.SetStructuralMatrix(full);
    TPZStepSolver<REAL> step;
    step.SetDirect(ELDLt);
    anTo.SetSolver(step);
    anTo.Run();    
    anTo.LoadSolution();

    //Restoring original state
    elastMat->SetForcingFunction(NULL);
    
    ///Integral correction
    REAL AreaTo = IntegrateSolution(fmeshvec[0], 0);
    
    if(fabs(AreaTo) > 1.E-18)
    {
        REAL alpha = AreaFrom/AreaTo;
        
        TPZFMatrix<REAL> solutionTo = fmeshvec[0]->Solution();
        for(int r = 0; r < solutionTo.Rows(); r++)
        {
            for(int c = 0; c < solutionTo.Cols(); c++)
            {
                solutionTo(r,c) *= alpha;
            }
        }
        
        fmeshvec[0]->LoadSolution(solutionTo);
    }
}

REAL ToolsTransient::IntegrateSolution(TPZCompMesh * cmesh, int variable)
{
    REAL integral = 0.;
    for(int c = 0; c < cmesh->NElements(); c++)
    {
        TPZCompEl * cel = cmesh->ElementVec()[c];
        if(!cel || locSetPressureMatIds.find(cel->Material()->Id()) == locSetPressureMatIds.end())
        {
            continue;
        }
        TPZInterpolationSpace * intel = dynamic_cast<TPZInterpolationSpace*>(cel);
        if(!intel)
        {
            DebugStop();
        }
        TPZVec<REAL> value;
        intel->Integrate(variable, value);
        
        if(value.NElements() == 1)//integrando Leakoff (vlAcum)
        {
            integral += value[0];
        }
        else if(value.NElements() == 2)//Integrando uy (w)
        {
            integral += value[1];
        }
    }
    
    return integral;
}


std::map<int,REAL> ToolsTransient::TransferLeakoff(TPZCompMesh * oldMphysicsCMesh)
{
    TPZCompMesh * cmeshTemp = new TPZCompMesh(fmeshvec[1]->Reference());
    cmeshTemp->SetAllCreateFunctionsDiscontinuous();
    cmeshTemp->AdjustBoundaryElements();
    
    //////L2Projection material
    int dim = 1;
    int pOrder = 0;
    int nsol = 1;
    TPZVec<REAL> solini(nsol,0.);
    
    TPZAutoPointer< TPZFunction<STATE> > func = new TLeakoffFunction<STATE>(oldMphysicsCMesh);
    
    int NStripes = globFractInputData.NStripes();
    for(int stripe = 0; stripe < NStripes; stripe++)
    {
        TPZL2Projection * materialL2 = new TPZL2Projection(globPressureMatId + stripe, dim, nsol, solini, pOrder);
        materialL2->SetForcingFunction(func);
    
        //////Inserindo na malha 1D
        cmeshTemp->InsertMaterialObject(materialL2);
    }
    
	cmeshTemp->AutoBuild();
    
    ///////Solving
    TPZAnalysis anTemp(cmeshTemp);
    TPZSkylineStructMatrix full(cmeshTemp);
    anTemp.SetStructuralMatrix(full);
    TPZStepSolver<REAL> step;
    step.SetDirect(ELDLt);
    anTemp.SetSolver(step);
    anTemp.Run();
    
    anTemp.LoadSolution();

    std::map<int,REAL> newLeakoff;
    
    for(int cel = 0; cel < cmeshTemp->NElements(); cel++)
    {
        TPZCompEl * compEl = cmeshTemp->ElementVec()[cel];
        TPZGeoEl * geoEl = compEl->Reference();
        
        int gelId = geoEl->Id();
        TPZVec<REAL> center(geoEl->Dimension());
        geoEl->CenterPoint(geoEl->NSides()-1, center);
        
        TPZInterpolationSpace * intpEl = dynamic_cast<TPZInterpolationSpace *>(compEl);
        TPZMaterialData data;
        intpEl->InitMaterialData(data);
        intpEl->ComputeShape(center, data);
        intpEl->ComputeSolution(center, data);
        TPZL2Projection * L2proj = dynamic_cast<TPZL2Projection *>(compEl->Material());
        TPZVec<REAL> Solout(1);
        int var = 1;//TPZL2Projection::ESolution
        L2proj->Solution(data, var, Solout);
        
        REAL vl = Solout[0];
        if(vl < 0.)
        {
            vl = 0.;
        }
        newLeakoff[gelId] = vl;
    }
    
    return newLeakoff;
}

void ToolsTransient::MassMatrix(TPZFMatrix<REAL> & Un)
{
    fCouplingMaterial->SetLastState();
	TPZSpStructMatrix matsp(fmphysics);
	TPZAutoPointer<TPZGuiInterface> guiInterface;
    matsp.CreateAssemble(Un,guiInterface);
}

bool ToolsTransient::SolveSistTransient(TPZAnalysis *an, bool initialElasticKickIsNeeded)
{
    //CheckConv(an->Solution(), an);
    
	int nrows;
	nrows = an->Solution().Rows();
	TPZFMatrix<REAL> res_total(nrows,1,0.);
    
    TPZFMatrix<REAL> SolIterK = fmphysics->Solution();
	TPZAutoPointer< TPZMatrix<REAL> > matK;
	TPZFMatrix<REAL> fres(fmphysics->NEquations(),1);
    TPZFMatrix<REAL> fmat(fmphysics->NEquations(),1);
    fres.Zero();
    fmat.Zero();
    
    TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(fmeshvec, fmphysics);
    
    MassMatrix(fmat);
    
    if(initialElasticKickIsNeeded)
    {
        TPZFMatrix<REAL> chutenewton(fmeshvec[0]->Solution().Rows(), fmeshvec[0]->Solution().Cols(), 1.);
        fmeshvec[0]->LoadSolution(chutenewton);
        TPZBuildMultiphysicsMesh::TransferFromMeshes(fmeshvec, fmphysics);
    }
    
    bool propagate = false;
	while( fMustStop == false && propagate == false )
	{
        fres.Zero();
        StiffMatrixLoadVec(an, matK, fres);
        
        res_total = fres + fmat;
        REAL res = Norm(res_total);
        REAL tol = 1.e-8;
        int maxit = 15;
        int nit = 0;
        
        while(res > tol && nit < maxit) //itercao de Newton
        {
            an->Rhs() = res_total;
            an->Solve();
            an->LoadSolution(SolIterK + an->Solution());
            
            TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(fmeshvec, fmphysics);
            
            SolIterK = an->Solution();
            
            fres.Zero();
            StiffMatrixLoadVec(an, matK, fres);
            res_total = fres + fmat;

            res = Norm(res_total);
            std::cout << "||res|| = " << res << std::endl;
            nit++;
        }
        
        if(res >= tol)
        {
            std::cout << "\nAtingido o numero maximo de iteracoes, nao convergindo portanto!!!\n";
            std::cout << "||Res|| = " << res << std::endl;
            DebugStop();
        }
        
        TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(fmeshvec, fmphysics);
        fCouplingMaterial->UpdateLeakoff(fmeshvec[1]);
        globFractInputData.UpdateActTime();
        
        REAL KI = ComputeKIPlaneStrain();
        if(KI > globFractInputData.KIc())
        {//propagou!!!
            globFractInputData.SetMinDeltaT();
            propagate = true;
        }
        else
        {//nao propagou!!!
            globFractInputData.SetNextDeltaT();
            fmat.Zero();
            MassMatrix(fmat);
            
            globFractOutputData.PlotElasticVTK(an);
            PostprocessPressure();
            PostProcessAcumVolW();
            PostProcessVolLeakoff();
        }
        globFractOutputData.InsertTKI(globFractInputData.actTime(), KI);//its for output data to txt (Mathematica format)
        REAL peteleco = 1.E-8;
        if( globFractInputData.actTime() > (globFractInputData.Ttot() - peteleco) )
        {
            fMustStop = true;
        }
    }
    
    return propagate;
}


REAL ToolsTransient::ComputeKIPlaneStrain()
{
    REAL radius = globFractInputData.Jradius();
    
    fmeshvec[0]->LoadReferences();
    
    TPZVec<REAL> computedJ(2,0.);
    REAL KI = -1.;
    
    REAL XcrackTip = -1.;
    TPZGeoMesh * gm = fmeshvec[0]->Reference();
    for(int ell = 0; ell < gm->NElements(); ell++)
    {
        if(gm->ElementVec()[ell] && gm->ElementVec()[ell]->MaterialId() == globBCfluxOut)
        {
            int nodeIndex = gm->ElementVec()[ell]->NodeIndex(0);
            XcrackTip = gm->NodeVec()[nodeIndex].Coord(0);
            break;
        }
    }
    if(XcrackTip < 1.E-3)
    {
        DebugStop();
    }
    TPZVec<REAL> Origin(3,0.);
    Origin[0] = XcrackTip;
    TPZVec<REAL> normalDirection(3,0.);
    normalDirection[2] = 1.;
    
    //////////// Computing pressure at middle point of J arc /////
    TPZVec<REAL> xx(3,0.), qsii(2,0.);
    xx[0] = XcrackTip - globFractInputData.Jradius()/2.;
    int initialEl = 0;
    TPZGeoEl * geoEl = fmeshvec[0]->Reference()->FindElement(xx, qsii, initialEl, 2);
    if(!geoEl)
    {
        DebugStop();
    }
    TPZCompEl * compEl = geoEl->Reference();
    if(!compEl)
    {
        DebugStop();
    }
    TPZInterpolationSpace * intpEl = dynamic_cast<TPZInterpolationSpace *>(compEl);
    TPZMaterialData data;
    intpEl->InitMaterialData(data);
    intpEl->ComputeShape(qsii, data);
    intpEl->ComputeSolution(qsii, data);
    TPZElasticityMaterial * elast2D = dynamic_cast<TPZElasticityMaterial *>(compEl->Material());
    TPZVec<REAL> Solout(3,0.);
    int var = 10;//Stress Tensor
    elast2D->Solution(data, var, Solout);
    REAL pressure = -Solout[1];
    /////////////////////////////////////////////////////////////////////
    
    Path2D * Jpath = new Path2D(fmeshvec[0], Origin, normalDirection, radius, pressure);
    JIntegral2D integralJ;
    integralJ.PushBackPath2D(Jpath);
    
    computedJ = integralJ.IntegratePath2D(0);
    REAL young = globFractInputData.E();
    REAL poisson = globFractInputData.Poisson();
    if(computedJ[0] < 0.)
    {
        //Estado compressivo!!!
        computedJ[0] = 0.;
    }
    KI = sqrt( (computedJ[0]*young)/(1. - poisson*poisson) );
    
    return KI;
}

void ToolsTransient::PostprocessPressure()
{
    std::map<REAL,REAL> pos_pressure;

    for(int i = 0;  i < fmeshvec[1]->ElementVec().NElements(); i++)
    {
        TPZCompEl * cel = fmeshvec[1]->ElementVec()[i];
        TPZInterpolatedElement * sp = dynamic_cast <TPZInterpolatedElement*>(cel);
        if(!sp)
        {
            continue;
        }
        TPZMaterialData data;
        sp->InitMaterialData(data);

        for(int qsiPt = -1; qsiPt <= +1; qsiPt++)
        {
            TPZVec<REAL> qsi(1,0.), Xqsi(3,0.);
            qsi[0] = qsiPt;
            cel->Reference()->X(qsi,Xqsi);
            
            sp->ComputeShape(qsi, data);
            sp->ComputeSolution(qsi, data);

            REAL pos = Xqsi[0];
            REAL press = data.sol[0][0];
            pos_pressure[pos] = press;
        }
    }
    
    globFractOutputData.InsertTposP(globFractInputData.actTime(), pos_pressure);
}

void ToolsTransient::PostProcessAcumVolW()
{
    REAL wAcum = 2. * IntegrateSolution(fmeshvec[0], 0);//aqui jah sao consideradas as 2 faces da fratura
    globFractOutputData.InsertTAcumVolW(globFractInputData.actTime(), wAcum);
}

void ToolsTransient::PostProcessVolLeakoff()
{
    //volume por trecho
    TPZAutoPointer< TPZFunction<STATE> > func = new TLeakoffFunction<STATE>(fmphysics);
    
    REAL vlAcum = 0.;

    for(int i = 0;  i < fmeshvec[1]->ElementVec().NElements(); i++)
    {
        TPZCompEl * cel = fmeshvec[1]->ElementVec()[i];
        if(locSetPressureMatIds.find(cel->Reference()->MaterialId()) == locSetPressureMatIds.end())
        {
            continue;
        }
        
#ifdef DEBUG
        if(!cel || cel->Reference()->Dimension() != 1)
        {
            DebugStop();
        }
#endif
        cel->Material()->SetForcingFunction(func);
        TPZGeoEl * fluidGel = cel->Reference();
        
#ifdef DEBUG
        if(!fluidGel)
        {
            DebugStop();
        }
#endif
        
        TPZVec<REAL> qsi(2,0.), xLeft(3,0.), xMiddle(3,0.), xRight(3,0.), leakoff(1,0);
        
        qsi[0] = -1.;
        fluidGel->X(qsi,xLeft);
        REAL posLeft = ((int)(xLeft[0]*1000.))/1000.;
        
        qsi[0] = +1.;
        fluidGel->X(qsi,xRight);
        REAL posRight = ((int)(xRight[0]*1000.))/1000.;
        
        REAL lengthElement = xRight[0] - xLeft[0];
        
        xMiddle[0] = (xLeft[0] + xRight[0])/2.;
        func->Execute(xMiddle, leakoff);
        
        //soh para nao coincidir pontos no mapa
        xLeft[0] = xMiddle[0] - 0.99*lengthElement/2.;
        xRight[0] = xMiddle[0] + 0.99*lengthElement/2.;

        REAL lengthLeakoff = leakoff[0];
        globFractOutputData.InsertTposVolLeakoff(globFractInputData.actTime(), posRight, lengthLeakoff);
        globFractOutputData.InsertTposVolLeakoff(globFractInputData.actTime(), posLeft, lengthLeakoff);
        
        cel->Material()->SetForcingFunction(NULL);
        
        //volume Acumulado
        vlAcum += 2. * (lengthLeakoff * lengthElement);//sao duas faces da fratura, lembra?
    }
    
    globFractOutputData.InsertTAcumVolLeakoff(globFractInputData.actTime(), vlAcum);
}

void ToolsTransient::CheckConv(TPZFMatrix<REAL> actQsi , TPZAnalysis *an)
{    
	int neq = fmphysics->NEquations();
    int nsteps = 7;
    
    TPZFMatrix<REAL> fxIni(neq,1);
    TPZFMatrix<REAL> fxAprox(neq,1);
    TPZFMatrix<REAL> fxExato(neq,1);
    
    TPZFMatrix<REAL> errorVec(neq,1,0.);
	TPZFMatrix<REAL> errorNorm(nsteps,1,0.);
    
    TPZAutoPointer< TPZMatrix<REAL> > fLIni;
    TPZAutoPointer< TPZMatrix<REAL> > fLtemp;
    
    TPZFMatrix<REAL> dFx(neq,1);
    
    for(int i = 0; i < actQsi.Rows(); i++)
    {
        REAL val = (double)(rand())*(1.e-10);
        actQsi(i,0) = val;
    }
    an->LoadSolution(actQsi);
    TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(fmeshvec, fmphysics);
    
    StiffMatrixLoadVec(an, fLIni, fxIni);
    
    TPZFMatrix<REAL> qsiIni = actQsi;
    
    if(fLIni->Rows() != neq || fLIni->Cols() != neq || fLIni->IsDecomposed())
    {
        DebugStop();
    }
    
    TPZVec<REAL> deltaQsi(neq,0.01), alphas(nsteps);
    double alpha;
    
    for(int i = 0; i < nsteps; i++)
    {
        alpha = (i+1)/10.;
        alphas[i] = alpha;
        
        ///Fx aproximado
        dFx.Zero();
        for(int r = 0; r < neq; r++)
        {
            for(int c = 0; c < neq; c++)
            {
                dFx(r,0) +=  (-1.) * fLIni->GetVal(r,c) * (alpha * deltaQsi[c]); // (-1) porque fLini = -D[res,sol]
            }
        }
        fxAprox = fxIni + dFx;
        //std::cout << "Aprox : " << fxAprox(3,0) << std::endl;
        
        ///Fx exato
        for(int r = 0; r < neq; r++)
        {
            actQsi(r,0) = qsiIni(r,0) + (alpha * deltaQsi[r]);
        }
        an->LoadSolution(actQsi);
        TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(fmeshvec, fmphysics);
        
        fxExato.Zero();
        if(fLtemp) fLtemp->Zero();
        StiffMatrixLoadVec(an, fLtemp, fxExato);
        //std::cout << "Exato : " << fxExato(3,0) << std::endl;
        
        ///Erro
        errorVec.Zero();
        for(int r = 0; r < neq; r++)
        {
            errorVec(r,0) = fxExato(r,0) - fxAprox(r,0);
        }
        
        ///Norma do erro
        double XDiffNorm = Norm(errorVec);
        errorNorm(i,0) = XDiffNorm;
    }
    
    std::cout << "Convergence Order:\n";
    for(int j = 1; j < nsteps; j++)
    {
        std::cout << ( log(errorNorm(j,0)) - log(errorNorm(j-1,0)) )/( log(alphas[j]) - log(alphas[j-1]) ) << "\n";
    }
}


//---------------------------------------------------------------


template<class TVar>
TElastSolFunction<TVar>::TElastSolFunction()
{
    fIniElIndex = 0;
    fcmesh = NULL;
}

template<class TVar>
TElastSolFunction<TVar>::TElastSolFunction(TPZCompMesh * cmesh)
{
    fIniElIndex = 0;
    this->fcmesh = cmesh;
}

template<class TVar>
TElastSolFunction<TVar>::~TElastSolFunction()
{
    fIniElIndex = 0;
    fcmesh = NULL;
}

template<class TVar>
void TElastSolFunction<TVar>::Execute(const TPZVec<REAL> &x, TPZVec<TVar> &f)
{
    TPZVec<REAL> xcp(x);
    TPZVec<REAL> qsi2D(2,0.);
    TPZGeoEl * gel = fcmesh->Reference()->FindElement(xcp, qsi2D, fIniElIndex, 2);

    if(!gel)
    {
        DebugStop();
    }
    
    TPZElasticityMaterial * elast = dynamic_cast<TPZElasticityMaterial *>(gel->Reference()->Material());
    if(!elast)
    {
        DebugStop();
    }
    
    TPZVec<REAL> Solout(3);
    
    TPZReducedSpace * intpEl = dynamic_cast<TPZReducedSpace *>(gel->Reference());
    if(!intpEl)
    {
        DebugStop();
    }
    TPZMaterialData data;
    intpEl->InitMaterialData(data);
    
    intpEl->ComputeShape(qsi2D, data);
    intpEl->ComputeSolution(qsi2D, data);
    
    int var = 0;
    elast->Solution(data, var, Solout);
    f = Solout;
}

template<class TVar>
void TElastSolFunction<TVar>::Execute(const TPZVec<REAL> &x, TPZVec<TVar> &f, TPZFMatrix<TVar> &df)
{
    DebugStop();
}

template<class TVar>
int TElastSolFunction<TVar>::NFunctions()
{
    return 2;
}

template<class TVar>
int TElastSolFunction<TVar>::PolynomialOrder()
{
    return fcmesh->GetDefaultOrder();
}


//---------------------------------------------------------------


template<class TVar>
TLeakoffFunction<TVar>::TLeakoffFunction()
{
    DebugStop();//use o outro construtor
}

template<class TVar>
TLeakoffFunction<TVar>::TLeakoffFunction(TPZCompMesh * cmesh)
{
    this->fIniElIndex = 0;
    this->fcmesh = cmesh;
    
    std::map<int,TPZMaterial*>::iterator it = cmesh->MaterialVec().find(globMultiFisicMatId);
    if(it != cmesh->MaterialVec().end())
    {
        TPZNLFluidStructure2d * fluidMat = dynamic_cast<TPZNLFluidStructure2d*>(it->second);
        if(fluidMat)
        {
            this->fleakoffMap = fluidMat->GetLeakoffData();
        }
        else
        {
            DebugStop();
        }
    }
    else
    {
        DebugStop();
    }
    
//    {
//        std::ofstream outAntes("LeakoffANTES.txt");
//        std::map<int,REAL>::iterator it;
//        for(it = fleakoffMap.begin(); it != fleakoffMap.end(); it++)
//        {
//            outAntes << it->second << "\n";
//        }
//    }
}

template<class TVar>
TLeakoffFunction<TVar>::~TLeakoffFunction()
{
    fIniElIndex = 0;
    fcmesh = NULL;
    fleakoffMap.clear();
}

template<class TVar>
void TLeakoffFunction<TVar>::Execute(const TPZVec<REAL> &x, TPZVec<TVar> &f)
{
    TPZVec<REAL> xcp(x);
    TPZVec<REAL> qsi1D(1,0.);
    TPZGeoEl * gel = fcmesh->Reference()->FindElement(xcp, qsi1D, fIniElIndex, 1);
    if(!gel)
    {
        DebugStop();
    }
    
    if(locSetPressureMatIds.find(gel->MaterialId()) == locSetPressureMatIds.end())
    {
        TPZGeoElSide gelSide(gel->NSides()-1);
        TPZGeoElSide neighSide(gelSide.Neighbour());
        while(neighSide != gelSide)
        {
            if(locSetPressureMatIds.find(neighSide.Element()->MaterialId()) != locSetPressureMatIds.end())
            {
                gel = neighSide.Element();
            }
            neighSide = neighSide.Neighbour();
        }
    }
    if(locSetPressureMatIds.find(gel->MaterialId()) == locSetPressureMatIds.end())
    {
        f[0] = 0.;
        return;
    }
    
    f.Resize(1);
    int elId = gel->Id();
    std::map<int,REAL>::iterator it = fleakoffMap.find(elId);
    if(it == fleakoffMap.end())
    {
        f[0] = 0.;
    }
    else
    {
        f[0] = it->second;
    }
}

template<class TVar>
void TLeakoffFunction<TVar>::Execute(const TPZVec<REAL> &x, TPZVec<TVar> &f, TPZFMatrix<TVar> &df)
{
    DebugStop();
}

template<class TVar>
int TLeakoffFunction<TVar>::NFunctions()
{
    return 1;
}

template<class TVar>
int TLeakoffFunction<TVar>::PolynomialOrder()
{
    return 0;
}

void ToolsTransient::SolveInitialElastoPlasticity(TPZElastoPlasticAnalysis &analysis, TPZCompMesh *Cmesh)
{
	TPZSkylineStructMatrix full(Cmesh);
    full.SetNumThreads(0);
	analysis.SetStructuralMatrix(full);
    
	TPZStepSolver<REAL> step;
    step.SetDirect(ELDLt);
	analysis.SetSolver(step);
    
    int NumIter = 2;
    bool linesearch = true;
    bool checkconv = false;
    
    analysis.IterativeProcess(cout, 1.e-6, NumIter, linesearch, checkconv);
    
    analysis.AcceptSolution();
}

TPZCompMesh * ToolsTransient::CMeshElastoPlastic(TPZGeoMesh *gmesh, REAL SigmaN)
{
    /// criar materiais
    int dim = 2;
    
    TPZVec<REAL> force(dim,0.);
    
    //int planestress = 1;
    int planestrain = 1;
    
    TPZSandlerDimaggio<SANDLERDIMAGGIOSTEP2> SD;
    REAL poisson = 0.203;
    REAL elast = 29269.;
    REAL A = 152.54;
    REAL B = 0.0015489;
    REAL C = 146.29;
    REAL R = 0.91969;
    REAL D = 0.018768;
    REAL W = 0.006605;
    SD.SetUp(poisson, elast, A, B, C, R, D, W);
    SD.SetResidualTolerance(1.e-10);
    SD.fIntegrTol = 10.;
    
    TPZMatElastoPlastic2D<TPZSandlerDimaggio<SANDLERDIMAGGIOSTEP2> > * PlasticSD = new TPZMatElastoPlastic2D<TPZSandlerDimaggio<SANDLERDIMAGGIOSTEP2> > (globReservMatId,planestrain);
    
    TPZMaterial * mat(PlasticSD);
    PlasticSD->SetPlasticity(SD);
    
    TPZMatWithMem<TPZElastoPlasticMem> * pMatWithMem = dynamic_cast<TPZMatWithMem<TPZElastoPlasticMem> *> (mat);
    
    ///criar malha computacional
    TPZCompMesh * cmesh = new TPZCompMesh(gmesh);
    cmesh->SetDefaultOrder(fpOrder);
    cmesh->SetDimModel(dim);
    cmesh->InsertMaterialObject(pMatWithMem);
    
    TPZElastoPlasticAnalysis::SetAllCreateFunctionsWithMem(cmesh);
    
    
    
    ///Inserir condicao de contorno
    REAL big = mat->gBigNumber;
    
    TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,0.);
    val2(1,0) = SigmaN;
    TPZMaterial * BCond1 = PlasticSD->CreateBC(pMatWithMem, globPressureMatId, neumann, val1, val2);
    
    val1.Redim(2,2);
    val2.Redim(2,1);
    TPZMaterial * BCond2 = PlasticSD->CreateBC(pMatWithMem, globDirichletElastMatId, dirichlet, val1, val2);
    
    val1(0,0) = big;
    TPZMaterial * BCond3 = PlasticSD->CreateBC(pMatWithMem, globBlockedXElastMatId, mixed, val1, val2);
    
    //cmesh->SetAllCreateFunctionsContinuous();
    cmesh->InsertMaterialObject(BCond1);
    cmesh->InsertMaterialObject(BCond2);
    cmesh->InsertMaterialObject(BCond3);
    
    //Ajuste da estrutura de dados computacional
    cmesh->AutoBuild();
    cmesh->AdjustBoundaryElements();
    cmesh->CleanUpUnconnectedNodes();
    
    return cmesh;
}
