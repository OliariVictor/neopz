//
//  main_mixed.cpp
//  PZ
//
//  Created by Agnaldo Farias on 5/28/12.
//  Copyright (c) 2012 LabMec-Unicamp. All rights reserved.
//

#include "pzgmesh.h"
#include "pzcmesh.h"
#include "pzcompel.h"
#include "pzbndcond.h"
#include "TPZInterfaceEl.h"
#include "pzbuildmultiphysicsmesh.h"
#include "pzinterpolationspace.h"
#include "TPZCompElDisc.h"
#include "pzpoisson3d.h"
#include "mixedpoisson.h"

#include "tpzgeoelrefpattern.h"
#include "TPZGeoLinear.h"
#include "tpztriangle.h"
#include "pzgeoquad.h"

#include "pzanalysis.h"
#include "pzskylstrmatrix.h"
#include "pzstrmatrix.h"
#include "pzstepsolver.h"

#include "pzlog.h"

#include <iostream>
#include <math.h>
using namespace std;

int const matId =1;
int const bc0=-1;
int const bc1=-2;
int const bc2=-3;
int const bc3=-4;

int const dirichlet =0;
int const neumann = 1;

REAL const Pi = 4.*atan(1.);

TPZGeoMesh *GMesh(bool triang_elements);
TPZCompMesh *CMeshFlux(TPZGeoMesh *gmesh, int pOrder);
TPZCompMesh *CMeshPressure(TPZGeoMesh *gmesh, int pOrder);
TPZCompMesh *MalhaCompMultphysics(TPZGeoMesh * gmesh, TPZVec<TPZCompMesh *> meshvec, TPZMixedPoisson* &mymaterial);

void Forcing1(const TPZVec<REAL> &pt, TPZVec<REAL> &disp);
void SolExata1(const TPZVec<REAL> &pt, TPZVec<REAL> &disp, TPZFMatrix<REAL> &flux);
void ForcingBC1(const TPZVec<REAL> &pt, TPZVec<REAL> &disp);

void SolExata2(const TPZVec<REAL> &pt, TPZVec<REAL> &disp, TPZFMatrix<REAL> &flux);
void ForcingBC2(const TPZVec<REAL> &pt, TPZVec<REAL> &disp);

void SolveSyst(TPZAnalysis &an, TPZCompMesh *fCmesh);
void PosProcessMultphysics(TPZVec<TPZCompMesh *> meshvec, TPZCompMesh* mphysics, TPZAnalysis &an, std::string plotfile);
TPZCompMesh *CMeshHDivPressure(TPZGeoMesh *gmesh, int pOrder);
void PosProcessHDiv(TPZAnalysis &an, std::string plotfile);

void UniformRefine(TPZGeoMesh* gmesh, int nDiv);
#ifdef LOG4CXX
static LoggerPtr logdata(Logger::getLogger("pz.mixedpoisson.data"));
#endif

const bool triang = true;
const int teste = 2;
int main(int argc, char *argv[])
{
#ifdef LOG4CXX
//	std::string logs("../logmixedproblem.cfg");
//	InitializePZLOG("../logmixedproblem.cfg");
    InitializePZLOG();
#endif
    
    ofstream arg12("Erro.txt");
    for (int p =3; p<4; p++)
    {
        arg12<<"\n Ordem = " << p <<endl;
        for(int h=2; h<3;h++)
        {
            arg12<<" Refinamento h  = " << h <<endl;
            // int p = 5;
            //primeira malha

            // geometric mesh (initial)
            TPZGeoMesh * gmesh = GMesh(triang);
            //UniformRefine(gmesh,h);
            // ofstream arg1("gmesh_inicial.txt");
            // gmesh->Print(arg1);

            /*
            // First computational mesh
            TPZCompMesh * cmesh1= CMeshFlux(gmesh, p);
            //ofstream arg2("cmesh1_inicial.txt");
            //cmesh1->Print(arg2);


            // Second computational mesh
            TPZCompMesh * cmesh2;
            if(triang==true){
               cmesh2 = CMeshPressure(gmesh, p-1);
            }
            else cmesh2 = CMeshPressure(gmesh, p);
            // ofstream arg3("cmesh2_inicial.txt");
            // cmesh2->Print(arg3);


            // Cleaning reference of the geometric mesh to cmesh1
            gmesh->ResetReference();
            cmesh1->LoadReferences();
            TPZBuildMultiphysicsMesh::UniformRefineCompMesh(cmesh1,h,false);
            //ofstream arg4("cmesh1_final.txt");
            //cmesh1->Print(arg4);
            

            // Cleaning reference to cmesh2
            gmesh->ResetReference();
            cmesh2->LoadReferences();
            TPZBuildMultiphysicsMesh::UniformRefineCompMesh(cmesh2,h,true);
            //ofstream arg5("cmesh2_final.txt");
            //cmesh2->Print(arg5);

            //malha multifisica
            TPZVec<TPZCompMesh *> meshvec(2);
            meshvec[0] = cmesh1;
            meshvec[1] = cmesh2;
            TPZMixedPoisson * mymaterial;
            TPZCompMesh * mphysics = MalhaCompMultphysics(gmesh,meshvec,mymaterial);
//            ofstream arg6("mphysic.txt");
//            mphysics->Print(arg6);

            ofstream arg7("gmesh_Final.txt");
            gmesh->Print(arg7);

            TPZAnalysis an(mphysics);
            
            ofstream arg10("mphysic_posAnalysis.txt");
            mphysics->Print(arg10);
            
            SolveSyst(an, mphysics);
            
            ofstream arg6("mphysic.txt");
            mphysics->Print(arg6);
            
            TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(meshvec, mphysics);
            
            ofstream arg9("mphysic_apos_Transferfrom.txt");
            mphysics->Print(arg9);
/*
            TPZVec<REAL> erros;
            arg12<<" Erro da simulacao multifisica  para o flux" <<endl;

            TPZAnalysis an1(cmesh1);
            if (teste==1) an1.SetExact(*SolExata1);
            else an1.SetExact(*SolExata2);
            an1.PostProcessError(erros, arg12);

            arg12<<" \nErro da simulacao multifisica  para a pressao" <<endl;
             
            TPZAnalysis an2(cmesh2);
            if (teste==1) an2.SetExact(*SolExata1);
            else an2.SetExact(*SolExata2);
            an2.PostProcessError(erros, arg12);

            string plotfile("Solution_mphysics.vtk");
            PosProcessMultphysics(meshvec,  mphysics, an, plotfile);
*/

            //solucao HDivPressure
            TPZCompMesh * cmesh3= CMeshHDivPressure(gmesh, p);
//            ofstream arg8("cmesh_HdivInicial.txt");
//            cmesh3->Print(arg8);

//            ofstream arg10("gmesh_final.txt");
//            gmesh->Print(arg10);

            TPZAnalysis an3(cmesh3);
            SolveSyst(an3, cmesh3);
            ofstream arg8("cmesh_HdivInicial.txt");
            cmesh3->Print(arg8);

//            arg12<<" \nErro da HdivPressure  " <<endl;
//            an3.SetExact(*SolExata1);
//            an3.PostProcess(erros, arg12);


            string plotile2("Solution_HDiv.vtk");
            PosProcessHDiv(an3, plotile2);

        }
    }
    return 0;
}

TPZGeoMesh *GMesh(bool triang_elements){
    
    int Qnodes = 4;
	
	TPZGeoMesh * gmesh = new TPZGeoMesh;
	gmesh->SetMaxNodeId(Qnodes-1);
	gmesh->NodeVec().Resize(Qnodes);
	TPZVec<TPZGeoNode> Node(Qnodes);
	
	TPZVec <int> TopolQuad(4);
    TPZVec <int> TopolTriang(3);
	TPZVec <int> TopolLine(2);
	
	//indice dos nos
	int id = 0;
	REAL valx, dx=1.;
	for(int xi = 0; xi < Qnodes/2; xi++)
	{
		valx = xi*dx;
		Node[id].SetNodeId(id);
		Node[id].SetCoord(0 ,valx );//coord X
		Node[id].SetCoord(1 ,0. );//coord Y
		gmesh->NodeVec()[id] = Node[id];
		id++;
	}
	
	for(int xi = 0; xi < Qnodes/2; xi++)
	{
		valx = 1. - xi*dx;
		Node[id].SetNodeId(id);
		Node[id].SetCoord(0 ,valx );//coord X
		Node[id].SetCoord(1 ,1. );//coord Y
		gmesh->NodeVec()[id] = Node[id];
		id++;
	}
	
	//indice dos elementos
	id = 0;
    
    if(triang_elements==true)
    {
        TopolTriang[0] = 0;
        TopolTriang[1] = 1;
        TopolTriang[2] = 3;
        new TPZGeoElRefPattern< pzgeom::TPZGeoTriangle> (id,TopolTriang,matId,*gmesh);
        id++;
        
        TopolTriang[0] = 2;
        TopolTriang[1] = 1;
        TopolTriang[2] = 3;
        new TPZGeoElRefPattern< pzgeom::TPZGeoTriangle> (id,TopolTriang,matId,*gmesh);
        id++;
        
        TopolLine[0] = 0;
        TopolLine[1] = 1;
        new TPZGeoElRefPattern< pzgeom::TPZGeoLinear > (id,TopolLine,bc0,*gmesh);
        id++;
        
        TopolLine[0] = 2;
        TopolLine[1] = 1;
        new TPZGeoElRefPattern< pzgeom::TPZGeoLinear > (id,TopolLine,bc1,*gmesh);
        id++;
        
        TopolLine[0] = 3;
        TopolLine[1] = 2;
        new TPZGeoElRefPattern< pzgeom::TPZGeoLinear > (id,TopolLine,bc2,*gmesh);
        id++;
        
        TopolLine[0] = 3;
        TopolLine[1] = 0;
        new TPZGeoElRefPattern< pzgeom::TPZGeoLinear > (id,TopolLine,bc3,*gmesh);
    }
    else{
        TopolQuad[0] = 0;
        TopolQuad[1] = 1;
        TopolQuad[2] = 2;
        TopolQuad[3] = 3;
        new TPZGeoElRefPattern< pzgeom::TPZGeoQuad> (id,TopolQuad,matId,*gmesh);
        id++;
        
        TopolLine[0] = 0;
        TopolLine[1] = 1;
        new TPZGeoElRefPattern< pzgeom::TPZGeoLinear > (id,TopolLine,bc0,*gmesh);
        id++;
        
        TopolLine[0] = 1;
        TopolLine[1] = 2;
        new TPZGeoElRefPattern< pzgeom::TPZGeoLinear > (id,TopolLine,bc1,*gmesh);
        id++;
        
        TopolLine[0] = 3;
        TopolLine[1] = 2;
        new TPZGeoElRefPattern< pzgeom::TPZGeoLinear > (id,TopolLine,bc2,*gmesh);
        id++;
        
        TopolLine[0] = 0;
        TopolLine[1] = 3;
        new TPZGeoElRefPattern< pzgeom::TPZGeoLinear > (id,TopolLine,bc3,*gmesh);
    }
    
	gmesh->BuildConnectivity();
    
//#ifdef LOG4CXX
//	if(logdata->isDebugEnabled())
//	{
//        std::stringstream sout;
//        sout<<"\n\n Malha Geometrica Inicial\n ";
//        gmesh->Print(sout);
//        LOGPZ_DEBUG(logdata,sout.str())
//	}
//#endif
   
	return gmesh;
}

void UniformRefine(TPZGeoMesh* gmesh, int nDiv)
{
    for(int D = 0; D < nDiv; D++)
    {
        int nels = gmesh->NElements();
        for(int elem = 0; elem < nels; elem++)
        {
            TPZVec< TPZGeoEl * > filhos;
            TPZGeoEl * gel = gmesh->ElementVec()[elem];
            gel->Divide(filhos);
        }
    }
    //	gmesh->BuildConnectivity();
}


TPZCompMesh *CMeshFlux(TPZGeoMesh *gmesh, int pOrder)
{
    /// criar materiais
	int dim = 2;
	TPZMatPoisson3d *material;
	material = new TPZMatPoisson3d(matId,dim); 
	TPZMaterial * mat(material);
	material->NStateVariables();
    
//    TPZAutoPointer<TPZFunction<STATE> > force1 = new TPZDummyFunction<STATE>(Forcing1);
//	material->SetForcingFunction(force1);
	
	TPZCompMesh * cmesh = new TPZCompMesh(gmesh);
	
    
    ///Inserir condicao de contorno
	TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,0.);
	TPZMaterial * BCond0 = material->CreateBC(mat, bc0,dirichlet, val1, val2);
    TPZMaterial * BCond1 = material->CreateBC(mat, bc1,dirichlet, val1, val2);
    TPZMaterial * BCond2 = material->CreateBC(mat, bc2,dirichlet, val1, val2);
    TPZMaterial * BCond3 = material->CreateBC(mat, bc3,dirichlet, val1, val2);
    
    cmesh->InsertMaterialObject(mat);
	cmesh->SetAllCreateFunctionsHDiv();
    cmesh->InsertMaterialObject(BCond0);
    cmesh->InsertMaterialObject(BCond1);
    cmesh->InsertMaterialObject(BCond2);
    cmesh->InsertMaterialObject(BCond3);
    
	cmesh->SetDefaultOrder(pOrder);
    cmesh->SetDimModel(dim);
	
	//Ajuste da estrutura de dados computacional
	cmesh->AutoBuild();
    
//#ifdef LOG4CXX
//	if(logdata->isDebugEnabled())
//	{
//        std::stringstream sout;
//        sout<<"\n\n Malha Computacional_1 Fluxo\n ";
//        cmesh->Print(sout);
//        LOGPZ_DEBUG(logdata,sout.str())
//	}
//#endif
	
	return cmesh;
}

TPZCompMesh *CMeshPressure(TPZGeoMesh *gmesh, int pOrder)
{
    /// criar materiais
	int dim = 2;
	TPZMatPoisson3d *material;
	material = new TPZMatPoisson3d(matId,dim);
	material->NStateVariables();
    
    //    TPZAutoPointer<TPZFunction<STATE> > force1 = new TPZDummyFunction<STATE>(Forcing1);
    //	material->SetForcingFunction(force1);
	
    TPZCompMesh * cmesh = new TPZCompMesh(gmesh);
    cmesh->SetDimModel(dim);
    TPZMaterial * mat(material);
    cmesh->InsertMaterialObject(mat);
    
    ///Inserir condicao de contorno
	TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,0.);
	TPZMaterial * BCond0 = material->CreateBC(mat, bc0,dirichlet, val1, val2);
    TPZMaterial * BCond1 = material->CreateBC(mat, bc1,dirichlet, val1, val2);
    TPZMaterial * BCond2 = material->CreateBC(mat, bc2,dirichlet, val1, val2);
    TPZMaterial * BCond3 = material->CreateBC(mat, bc3,dirichlet, val1, val2);
    
    
	cmesh->SetAllCreateFunctionsDiscontinuous();
    
    
    cmesh->InsertMaterialObject(BCond0);
    cmesh->InsertMaterialObject(BCond1);
    cmesh->InsertMaterialObject(BCond2);
    cmesh->InsertMaterialObject(BCond3);
    
	cmesh->SetDefaultOrder(pOrder);
    cmesh->SetDimModel(dim);
	
	//Ajuste da estrutura de dados computacional
	cmesh->AutoBuild();
    
    int ncon = cmesh->NConnects();
    for(int i=0; i<ncon; i++)
    {
        TPZConnect &newnod = cmesh->ConnectVec()[i];
        newnod.SetPressure(true);
    }
    
    int nel = cmesh->NElements();
    for(int i=0; i<nel; i++){
        TPZCompEl *cel = cmesh->ElementVec()[i];
        TPZCompElDisc *celdisc = dynamic_cast<TPZCompElDisc *>(cel);
        celdisc->SetConstC(1.);
        celdisc->SetCenterPoint(0, 0.);
        celdisc->SetCenterPoint(1, 0.);
        celdisc->SetCenterPoint(2, 0.);
        celdisc->SetTrueUseQsiEta();
        if(celdisc && celdisc->Reference()->Dimension() == cmesh->Dimension())
        {
            if(triang==true) celdisc->SetTotalOrderShape();
            else celdisc->SetTensorialShape();
        }
        
    }
    
    
#ifdef DEBUG
    int ncel = cmesh->NElements();
    for(int i =0; i<ncel; i++){
        TPZCompEl * compEl = cmesh->ElementVec()[i];
        if(!compEl) continue;
        TPZInterfaceElement * facel = dynamic_cast<TPZInterfaceElement *>(compEl);
        if(facel)DebugStop();
        
    }
#endif
    
    //#ifdef LOG4CXX
    //	if(logdata->isDebugEnabled())
    //	{
    //        std::stringstream sout;
    //        sout<<"\n\n Malha Computacional_2 pressure\n ";
    //        cmesh->Print(sout);
    //        LOGPZ_DEBUG(logdata,sout.str());
    //	}
    //#endif
	return cmesh;
}



TPZCompMesh *MalhaCompMultphysics(TPZGeoMesh * gmesh, TPZVec<TPZCompMesh *> meshvec, TPZMixedPoisson * &mymaterial){
    
    //Creating computational mesh for multiphysic elements
	gmesh->ResetReference();
	TPZCompMesh *mphysics = new TPZCompMesh(gmesh);
    
    int MatId=1;
    int dim =2;
    
    REAL coefk=1.;
    mymaterial = new TPZMixedPoisson(MatId,dim);
    mymaterial->SetParameters(coefk);
    
    
    TPZMaterial *mat(mymaterial);
    mphysics->InsertMaterialObject(mat);
    
    
    ///Inserir condicao de contorno
    TPZAutoPointer<TPZFunction<STATE> > fCC0;
    TPZAutoPointer<TPZFunction<REAL> > force1;
    TPZAutoPointer<TPZFunction<STATE> > fCC23;
    
    TPZMaterial * BCond0;
    TPZMaterial * BCond1;
    TPZMaterial * BCond2;
    TPZMaterial * BCond3;
    
    if(teste==1){
        force1 = new TPZDummyFunction<REAL>(Forcing1);
        mymaterial->SetForcingFunction(force1);
        fCC0 = new TPZDummyFunction<STATE>(ForcingBC1);
    }
    else{
        REAL fxy=8.;
        mymaterial->SetInternalFlux(fxy);
        fCC23 = new TPZDummyFunction<STATE>(ForcingBC2);
    }
    
    
    if(teste==1){
        TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,0.);
        BCond0 = mymaterial->CreateBC(mat, bc0,dirichlet, val1, val2);
        BCond2 = mymaterial->CreateBC(mat, bc2,dirichlet, val1, val2);
        
        
        TPZFMatrix<REAL> val11(2,2,0.), val21(2,1,0.);
        BCond1 = mymaterial->CreateBC(mat, bc1,dirichlet, val11, val21);
        BCond3 = mymaterial->CreateBC(mat, bc3,dirichlet, val11, val21);
        
        //BCond0->SetForcingFunction(fCC0);
    }else{
        TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,1.);
        BCond0 = mymaterial->CreateBC(mat, bc0,dirichlet, val1, val2);
        BCond2 = mymaterial->CreateBC(mat, bc2,dirichlet, val1, val2);
        
        
        TPZFMatrix<REAL> val11(2,2,0.), val21(2,1,0.);
        BCond1 = mymaterial->CreateBC(mat, bc1,neumann, val11, val21);
        BCond3 = mymaterial->CreateBC(mat, bc3,neumann, val11, val21);
        
        //BCond1->SetForcingFunction(fCC23);
        //BCond3->SetForcingFunction(fCC23);
    }
    
    mphysics->SetAllCreateFunctionsMultiphysicElem();
    mphysics->InsertMaterialObject(BCond0);
    mphysics->InsertMaterialObject(BCond1);
    mphysics->InsertMaterialObject(BCond2);
    mphysics->InsertMaterialObject(BCond3);
    
    mphysics->AutoBuild();
	mphysics->AdjustBoundaryElements();
	mphysics->CleanUpUnconnectedNodes();
    
    // Creating multiphysic elements into mphysics computational mesh
	TPZBuildMultiphysicsMesh::AddElements(meshvec, mphysics);
	TPZBuildMultiphysicsMesh::AddConnects(meshvec,mphysics);
	TPZBuildMultiphysicsMesh::TransferFromMeshes(meshvec, mphysics);
    
    return mphysics;
}

void Forcing1(const TPZVec<REAL> &pt, TPZVec<REAL> &disp){
	double x = pt[0];
    double y = pt[1];
    disp[0]= 2.*Pi*Pi*sin(Pi*x)*sin(Pi*y);
}

void ForcingBC2(const TPZVec<REAL> &pt, TPZVec<REAL> &disp){
	//double x = pt[0];
    double y = pt[1];
    disp[0]= 4.*y*(1. - y) + 1.;
}

void SolExata2(const TPZVec<REAL> &pt, TPZVec<REAL> &disp, TPZFMatrix<REAL> &flux){
   // double x = pt[0];
    double y = pt[1];
    flux.Resize(3, 1);
    //disp[0] = 0.;
    disp[0]= 4.*y*(1. - y) + 1;
    flux(0,0)=0.;
    flux(1,0)=-(4.-8.*y);
    flux(2,0)=8;
}

void SolExata1(const TPZVec<REAL> &pt, TPZVec<REAL> &disp, TPZFMatrix<REAL> &flux){
    double x = pt[0];
    double y = pt[1];
    flux.Resize(3, 1);
    disp[0]= sin(Pi*x)*sin(Pi*y);
    flux(0,0)=-Pi*cos(Pi*x)*sin(Pi*y);
    flux(1,0)=-Pi*cos(Pi*y)*sin(Pi*x);
    flux(2,0)=2.*Pi*Pi*sin(Pi*x)*sin(Pi*y);
}


void ForcingBC1(const TPZVec<REAL> &pt, TPZVec<REAL> &disp){
    double x = pt[0];
    //double y = 0.;
    disp[0]= Pi*sin(Pi*x);
    //disp[1]= 0.;
}

#include "pzbstrmatrix.h"
void SolveSyst(TPZAnalysis &an, TPZCompMesh *fCmesh)
{			
	//TPZBandStructMatrix full(fCmesh);
	TPZSkylineStructMatrix full(fCmesh); //caso simetrico
	an.SetStructuralMatrix(full);
	TPZStepSolver<REAL> step;
	step.SetDirect(ELDLt); //caso simetrico
	//step.SetDirect(ELU);
	an.SetSolver(step);
	an.Run();
	
	//Saida de Dados: solucao e  grafico no VT
	ofstream file("Solutout");
	an.Solution().Print("solution", file);    //Solution visualization on Paraview (VTK)
}

void PosProcessMultphysics(TPZVec<TPZCompMesh *> meshvec, TPZCompMesh* mphysics, TPZAnalysis &an, std::string plotfile){
    
    TPZBuildMultiphysicsMesh::TransferFromMultiPhysics(meshvec, mphysics);
	TPZManVector<std::string,10> scalnames(1), vecnames(2);
	vecnames[0]  = "Flux";
    vecnames[1]  = "GradFluxX";
    scalnames[0] = "Pressure";
			
	const int dim = 2;
	int div =0;
	an.DefineGraphMesh(dim,scalnames,vecnames,plotfile);
	an.PostProcess(div,dim);
	std::ofstream out("malha.txt");
	an.Print("nothing",out);
    
}

void PosProcessHDiv(TPZAnalysis &an, std::string plotfile){
	TPZManVector<std::string,10> scalnames(1), vecnames(1);
	scalnames[0] = "Pressure";
	vecnames[0]= "Flux";
	
	const int dim = 2;
	int div = 0;
	an.DefineGraphMesh(dim,scalnames,vecnames,plotfile);
	an.PostProcess(div,dim);
	std::ofstream out("malha.txt");
	an.Print("nothing",out);
}


TPZCompMesh *CMeshHDivPressure(TPZGeoMesh *gmesh, int pOrder)
{
    /// criar materiais
    int MatId=1;
    int dim =2;
	TPZMatPoisson3d *material = new TPZMatPoisson3d(MatId,dim);
	material->NStateVariables();
	
    TPZCompMesh * cmesh = new TPZCompMesh(gmesh);
    TPZMaterial * mat(material);
    cmesh->InsertMaterialObject(mat);
    
    
    
    REAL diff = 1.;
	REAL conv = 0.;
	TPZVec<REAL> convdir(3,0.);
    material->SetParameters(diff, conv, convdir);
    
    TPZAutoPointer<TPZFunction<STATE> > fCC0;
    TPZAutoPointer<TPZFunction<REAL> > force1;
    TPZAutoPointer<TPZFunction<STATE> > fCC23;
    
    TPZMaterial * BCond0;
    TPZMaterial * BCond1;
    TPZMaterial * BCond2;
    TPZMaterial * BCond3;
    
    if(teste==1){
        force1 = new TPZDummyFunction<REAL>(Forcing1);
        material->SetForcingFunction(force1);
        // fCC0 = new TPZDummyFunction<STATE>(ForcingBC);
    }
    else{
        REAL fxy=8.;
        material->SetInternalFlux(fxy);
        fCC23 = new TPZDummyFunction<STATE>(ForcingBC2);
    }
    
    
    ///Inserir condicao de contorno
    
    if(teste==1){
        TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,0.);
        BCond0 = material->CreateBC(mat, bc0,dirichlet, val1, val2);
        BCond2 = material->CreateBC(mat, bc2,dirichlet, val1, val2);
        
        
        TPZFMatrix<REAL> val11(2,2,0.), val21(2,1,0.);
        BCond1 = material->CreateBC(mat, bc1,dirichlet, val11, val21);
        BCond3 = material->CreateBC(mat, bc3,dirichlet, val11, val21);
        
        //BCond0->SetForcingFunction(fCC0);
    }else{
        TPZFMatrix<REAL> val1(2,2,0.), val2(2,1,1.);
        BCond0 = material->CreateBC(mat, bc0,dirichlet, val1, val2);
        BCond2 = material->CreateBC(mat, bc2,dirichlet, val1, val2);
        
        
        TPZFMatrix<REAL> val11(2,2,0.), val21(2,1,0.);
        BCond1 = material->CreateBC(mat, bc1,neumann, val11, val21);
        BCond3 = material->CreateBC(mat, bc3,neumann, val11, val21);
        
        //BCond1->SetForcingFunction(fCC23);
        //BCond3->SetForcingFunction(fCC23);
    }
    
    cmesh->SetAllCreateFunctionsHDivPressure();
    cmesh->InsertMaterialObject(BCond0);
    cmesh->InsertMaterialObject(BCond1);
    cmesh->InsertMaterialObject(BCond2);
    cmesh->InsertMaterialObject(BCond3);
    
    cmesh->SetDefaultOrder(pOrder);
    cmesh->SetDimModel(dim);
    
    cmesh->AutoBuild();
	cmesh->AdjustBoundaryElements();
	cmesh->CleanUpUnconnectedNodes();
    
	return cmesh;
}
