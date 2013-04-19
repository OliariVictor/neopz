//
//  pzconvectionproblem.cpp
//  PZ
//
//  Created by Agnaldo Farias on 4/2/13.
//
//

#include "pzconvectionproblem.h"
#include "pzmaterialdata.h"
#include "pzbndcond.h"
#include "pzaxestools.h"
#include "pzlog.h"


using namespace std;

TPZMatConvectionProblem::TPZMatConvectionProblem():TPZDiscontinuousGalerkin(){
	
    fDim = 1;
    fConvDir.resize(fDim);
    fConvDir[0]=0.;
    fRho = 0.;
    fXf = 0.;
    fTimeStep = 0.;
    fTimeValue = 0.;
    fmatId = 0;
}

TPZMatConvectionProblem::TPZMatConvectionProblem(int matid, int dim):TPZDiscontinuousGalerkin(matid){
	
    if(dim<0 || dim >2){
        DebugStop();
    }
    
    fDim = dim;
    fConvDir.Resize(fDim, 0.);
    fRho = 0.;
    fXf = 0.;
    fTimeStep = 0.;
    fTimeValue = 0.;
    fmatId = matid;
}

TPZMatConvectionProblem::~TPZMatConvectionProblem(){
}

TPZMatConvectionProblem::TPZMatConvectionProblem(const TPZMatConvectionProblem &copy):TPZDiscontinuousGalerkin(copy){
    
    this->operator=(copy);
}

TPZMatConvectionProblem & TPZMatConvectionProblem::operator=(const TPZMatConvectionProblem &copy){
    
    TPZDiscontinuousGalerkin::operator = (copy);
	fXf  = copy.fXf;
	fDim = copy.fDim;
	fConvDir  = copy.fConvDir;
    fRho  = copy.fRho;
    fTimeStep = copy.fTimeStep ;
    fTimeValue = copy.fTimeValue;
    fmatId = copy.fmatId;

	return *this;
}

int TPZMatConvectionProblem::NStateVariables() {
	return 1;
}

void TPZMatConvectionProblem::SetInternalFlux(REAL flux){
    fXf = flux;
}

void TPZMatConvectionProblem::SetParameters(REAL rho, TPZVec<REAL> &convdir){
    
    fRho = rho;
    for(int d=0; d<fDim; d++) fConvDir[d] = convdir[d];
}

void TPZMatConvectionProblem::GetParameters(REAL &rho, TPZVec<REAL> &convdir){
    
    rho = fRho;
    for(int d=0; d<fDim; d++) convdir[d] = fConvDir[d];
}


void TPZMatConvectionProblem::Print(std::ostream &out) {
	out << "name of material : " << Name() << "\n";
    out << "Dimesion of problem " << fDim << endl;
	out << "Coeficient that multiply temporal derivative  "<< fRho << endl;
	out << "Term of convection direction "<< fConvDir << endl;
	out << "Forcing vector f " << fXf << endl;
    out << "Time step " << fTimeStep << endl;
	out << "Base Class properties :";
	TPZMaterial::Print(out);
	out << "\n";
}

void TPZMatConvectionProblem::Contribute(TPZMaterialData &data, REAL weight, TPZFMatrix<REAL> &ek, TPZFMatrix<REAL> &ef){
    
	
    TPZFMatrix<REAL>  &phi = data.phi;
	TPZFMatrix<REAL> &dphi = data.dphix;
	TPZVec<REAL>  &x = data.x;
	TPZFMatrix<REAL> &axes = data.axes;
	int phr = phi.Rows();
    
    //current state n+1: stiffness matrix
    if(gState == ECurrentState)
    {
        REAL fXfLoc = fXf;
        if(fForcingFunction) {            
            TPZManVector<STATE> res(1);
            fForcingFunction->Execute(x,res);
            fXfLoc = res[0];
        }
        
        TPZVec<STATE> ConvDirAx;
        ConvDirAx.Resize(fDim, 0.);

        int di,dj;
        for(di=0; di<fDim; di++){
            for(dj=0; dj<fDim; dj++){
                 ConvDirAx[di] += axes(di,dj)*fConvDir[dj];
            }
        }
                
        //Equacao de Poisson
        for(int in = 0; in < phr; in++ ) {
            
            int kd;
            ef(in, 0) += fTimeStep*weight*fXfLoc*phi(in,0);
            
            for(int jn = 0; jn < phr; jn++ )
            {
                ek(in,jn) += weight*fRho*phi(in,0)*phi(jn,0);
                
                for(kd=0; kd<fDim; kd++)
                {
                    ek(in,jn) += weight*(-fTimeStep*ConvDirAx[kd]*dphi(kd,in)*phi(jn,0));
                }
            }
        }
    }//end stiffness matrix
	
    
    //Last state (n): mass matrix
	if(gState == ELastState)
    {
        for(int in = 0; in < phr; in++) {
            
            for(int jn = 0; jn < phr; jn++)
            {
                ek(in,jn) += weight*fRho*phi(in,0)*phi(jn,0);
            }
        }
    }
    
}

void TPZMatConvectionProblem::ContributeBC(TPZMaterialData &datavec,REAL weight, TPZFMatrix<REAL> &ek,TPZFMatrix<REAL> &ef,TPZBndCond &bc){
                                              
   
    std::cout<<" This class uses only discontinuous functions"<<std::endl;
	DebugStop();
}


void TPZMatConvectionProblem::ContributeInterface(TPZMaterialData &data, TPZMaterialData &dataleft, TPZMaterialData &dataright, REAL weight, TPZFMatrix<REAL> &ek, TPZFMatrix<REAL> &ef){
    
    if(gState == ELastState){
		return;
	}

	TPZFMatrix<REAL> &dphiLdAxes = dataleft.dphix;
	TPZFMatrix<REAL> &dphiRdAxes = dataright.dphix;
	TPZFMatrix<REAL> &phiL = dataleft.phi;
	TPZFMatrix<REAL> &phiR = dataright.phi;
	TPZManVector<REAL,3> &normal = data.normal;
	
	TPZFNMatrix<660> dphiL, dphiR;
	TPZAxesTools<REAL>::Axes2XYZ(dphiLdAxes, dphiL, dataleft.axes);
	TPZAxesTools<REAL>::Axes2XYZ(dphiRdAxes, dphiR, dataright.axes);
	
	
	int nrowl = phiL.Rows();
	int nrowr = phiR.Rows();
	int il,jl,ir,jr,id;
	
	//Convection term
	REAL ConvNormal = 0.;
	for(id=0; id<fDim; id++) ConvNormal += fConvDir[id]*normal[id];
	if(ConvNormal > 0.)
    {
		for(il=0; il<nrowl; il++)
        {
			for(jl=0; jl<nrowl; jl++)
            {
				ek(il,jl) += weight*fTimeStep*ConvNormal*phiL(il,0)*phiL(jl,0);
			}
		}
		for(ir=0; ir<nrowr; ir++)
        {
			for(jl=0; jl<nrowl; jl++)
            {
				ek(ir+nrowl,jl) -= weight*fTimeStep*ConvNormal*phiR(ir,0)*phiL(jl,0);
			}
		}
	} else{
		for(ir=0; ir<nrowr; ir++)
        {
			for(jr=0; jr<nrowr; jr++)
            {
				ek(ir+nrowl,jr+nrowl) -= weight*fTimeStep*ConvNormal*phiR(ir,0)*phiR(jr,0);
			}
		}
		for(il=0; il<nrowl; il++)
        {
			for(jr=0; jr<nrowr; jr++)
            {
				ek(il,jr+nrowl) += weight*fTimeStep*ConvNormal*phiL(il,0)*phiR(jr,0);
			}
		}
	}
}

void TPZMatConvectionProblem::ContributeBCInterface(TPZMaterialData &data, TPZMaterialData &dataleft, REAL weight, TPZFMatrix<REAL> &ek,TPZFMatrix<REAL> &ef,TPZBndCond &bc){
    
    if(gState == ELastState){
		return;
	}

    
    TPZFMatrix<REAL> &phiL = dataleft.phi;
	TPZManVector<REAL,3> &normal = data.normal;
	
	int il,jl,nrowl,id;
	nrowl = phiL.Rows();
	REAL ConvNormal = 0.;
    
	for(id=0; id<fDim; id++) ConvNormal += fConvDir[id]*normal[id];
    
	switch(bc.Type()) {
		case 0: // DIRICHLET
			
            //convection
			if(ConvNormal > 0.)
            {
				for(il=0; il<nrowl; il++){
					for(jl=0; jl<nrowl; jl++)
                    {
						ek(il,jl) += weight*fTimeStep*ConvNormal*phiL(il)*phiL(jl);
					}
				}
			}
            else{
				for(il=0; il<nrowl; il++)
                {
					ef(il,0) -= weight*fTimeStep*ConvNormal*bc.Val2()(0,0)*phiL(il);
				}
			}
			
			break;

			
        case 1: // Neumann
			for(il=0; il<nrowl; il++) {
				ef(il,0) += 0.;//ainda nao temos condicao de Neumann
			}
            break;

		case 3: // outflow condition
			if(ConvNormal > 0.) {
				for(il=0; il<nrowl; il++) {
					for(jl=0; jl<nrowl; jl++) {
						ek(il,jl) += weight*fTimeStep*ConvNormal*phiL(il)*phiL(jl);
					}
				}
			}
			else {
				if (ConvNormal < 0.) std::cout << "Boundary condition error: inflow detected in outflow boundary condition: ConvNormal = " << ConvNormal << "\n";
			}
			break;
			
		default:
			PZError << __PRETTY_FUNCTION__ << " - Wrong boundary condition type\n";
			break;
	}
}

/** Returns the variable index associated with the name */
int TPZMatConvectionProblem::VariableIndex(const std::string &name){
	if(!strcmp("Solution",name.c_str()))        return  1;
    if(!strcmp("ConvDirGradU",name.c_str()))        return  2;
	if(!strcmp("Derivative",name.c_str()))        return  3;
	if(!strcmp("Flux",name.c_str()))        return  4;
	
	return TPZMaterial::VariableIndex(name);
}

int TPZMatConvectionProblem::NSolutionVariables(int var){
	if((var == 1) || (var == 2)) return 1;
	if((var == 3) || (var == 4)) return fDim;
	return TPZMaterial::NSolutionVariables(var);
}

void TPZMatConvectionProblem::Solution(TPZMaterialData &data, int var, TPZVec<REAL> &Solout){
	
	Solout.Resize(this->NSolutionVariables(var));
	
	TPZVec<REAL> Sol_u;
    TPZFMatrix<REAL> DSol_u;
    TPZFMatrix<REAL> axes_u;
    
	Sol_u=data.sol[0];
    DSol_u = data.dsol[0];
    axes_u=data.axes;
    
	if(var == 1){
		Solout[0] = Sol_u[0];//function (state variable u)
		return;
	}
    
    if(var == 2){
		int id;
		for(id=0 ; id<fDim; id++) {
			TPZFNMatrix<9> dsoldx;
			TPZAxesTools<REAL>::Axes2XYZ(DSol_u, dsoldx, axes_u);
			Solout[0] += fConvDir[id]*dsoldx(id,0);//derivate of u
		}
		return;
	}
    
	if(var == 3) {
		int id;
		for(id=0 ; id<fDim; id++) {
			TPZFNMatrix<9> dsoldx;
			TPZAxesTools<REAL>::Axes2XYZ(DSol_u, dsoldx, axes_u);
			Solout[id] = dsoldx(id,0);//derivate of u
		}
		return;
	}//var == 2
	
	if(var == 4) {
		int id;
		for(id=0 ; id<fDim; id++) {
			Solout[id] =fConvDir[id]*Sol_u[0];
		}
		return;
	}//var == 3
}
