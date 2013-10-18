//
//  pztracerflow.cpp
//  PZ
//
//  Created by Agnaldo Farias on 16/10/13.
//
//

#include "pztracerflow.h"
#include "pzmaterialdata.h"
#include "pzbndcond.h"
#include "pzaxestools.h"
#include "pzlog.h"


using namespace std;

TPZTracerFlow::TPZTracerFlow():TPZDiscontinuousGalerkin(){
	
    fDim = 1;
    fConvDir.resize(fDim);
    fConvDir[0]=0.;
    fPoros = 0.;
    fVisc = 0.;
    fk = 0.;
    fxfPQ = 0.;
    fxfS = 0.;
    fTimeStep = 0.;
    fTimeValue = 0.;
    fmatId = 0;
}

TPZTracerFlow::TPZTracerFlow(int matid, int dim):TPZDiscontinuousGalerkin(matid){
	
    if(dim<0 || dim >2){
        DebugStop();
    }
    
    fDim = dim;
    fConvDir.Resize(fDim, 0.);
    fPoros = 1.;
    fVisc = 1.;
    fk = 1.;
    fxfPQ = 0.;
    fxfS = 0.;
    fTimeStep = 0.;
    fTimeValue = 0.;
    fmatId = matid;
}

TPZTracerFlow::~TPZTracerFlow(){
}

TPZTracerFlow::TPZTracerFlow(const TPZTracerFlow &copy):TPZDiscontinuousGalerkin(copy){
    
    this->operator=(copy);
}

TPZTracerFlow & TPZTracerFlow::operator=(const TPZTracerFlow &copy){
    
    TPZDiscontinuousGalerkin::operator = (copy);
	fxfPQ  = copy.fxfPQ;
    fxfS = copy.fxfS;
	fDim = copy.fDim;
	fConvDir  = copy.fConvDir;
    fPoros  = copy.fPoros;
    fk = copy.fk;
    fVisc = copy.fVisc;
    fTimeStep = copy.fTimeStep ;
    fTimeValue = copy.fTimeValue;
    fmatId = copy.fmatId;
    
	return *this;
}

int TPZTracerFlow::NStateVariables() {
	return 1;
}

void TPZTracerFlow::SetForcesPressure(REAL fx){
    fxfPQ = fx;
}

void TPZTracerFlow::SetForcesSaturation(REAL gx){
    fxfS = gx;
}

void TPZTracerFlow::SetPermeability(REAL perm) {
    fk = perm;
}

void TPZTracerFlow::SetViscosity(REAL visc) {
    fVisc = visc;
}

void TPZTracerFlow::SetPorosity(REAL poros){
    fPoros = poros;
}

void TPZTracerFlow::GetPermeability(REAL &perm) {
    perm = fk;
}

void TPZTracerFlow::SetConvectionDirection(TPZVec<REAL> convdir){
    
    for(int d=0; d<fDim; d++) fConvDir[d] = convdir[d];
}

void TPZTracerFlow::GetConvectionDirection(TPZVec<REAL> &convdir){
    
    for(int d=0; d<fDim; d++) convdir[d] = fConvDir[d];
}

void TPZTracerFlow::Print(std::ostream &out) {
	out << "name of material : " << Name() << "\n";
    out << "Dimesion of problem " << fDim << endl;
	out << "Porosity  "<< fPoros << endl;
    out << "Viscosity  "<< fVisc << endl;
    out << "Permeability  "<< fk << endl;
	out << "Term of convection direction "<< fConvDir << endl;
	out << "Forcing vector pressure: fx " << fxfPQ << endl;
    out << "Forcing vector saturation: gx " << fxfS << endl;
    out << "Time step " << fTimeStep << endl;
	out << "Base Class properties :";
	TPZMaterial::Print(out);
	out << "\n";
}

void TPZTracerFlow::Contribute(TPZVec<TPZMaterialData> &datavec, REAL weight, TPZFMatrix<STATE> &ek, TPZFMatrix<STATE> &ef){
    
#ifdef DEBUG
	int nref =  datavec.size();
	if (nref != 3 ) {
        std::cout << " Erro. The size of the datavec is different from 3 \n";
		DebugStop();
	}
#endif
    
    // Setting the phis
    TPZFMatrix<REAL>  &phiQ =  datavec[0].phi;
    TPZFMatrix<REAL> &dphiQ = datavec[0].dphix;
    TPZFMatrix<REAL>  &phiP =  datavec[1].phi;
    TPZFMatrix<REAL>  &phiS =  datavec[2].phi;
    TPZFMatrix<REAL>  &dphiS =  datavec[2].dphix;
    
	
	TPZFMatrix<REAL> &axesS = datavec[2].axes;
	int phrQ = phiQ.Rows();
    int phrP = phiP.Rows();
    int phrS = phiS.Rows();
    int rQP = phrQ+phrP;
    
    //current state n+1: stiffness matrix
    if(gState == ECurrentState)
    {
        //Calculate the matrix contribution for flux. Matrix A
        REAL ratiok = fVisc/fk;
        for(int iq=0; iq<phrQ; iq++)
        {
            ef(iq, 0) += 0.;
            
            int ivecind = datavec[0].fVecShapeIndex[iq].first;
            int ishapeind = datavec[0].fVecShapeIndex[iq].second;
            for (int jq=0; jq<phrQ; jq++)
            {
                int jvecind = datavec[0].fVecShapeIndex[jq].first;
                int jshapeind = datavec[0].fVecShapeIndex[jq].second;
                REAL prod = datavec[0].fNormalVec(0,ivecind)*datavec[0].fNormalVec(0,jvecind)+
                datavec[0].fNormalVec(1,ivecind)*datavec[0].fNormalVec(1,jvecind)+
                datavec[0].fNormalVec(2,ivecind)*datavec[0].fNormalVec(2,jvecind);//dot product between u and v
                ek(iq,jq) += ratiok*weight*phiQ(ishapeind,0)*phiQ(jshapeind,0)*prod;
            }
        }
        
        // Coupling terms between flux and pressure. Matrix B
        for(int iq=0; iq<phrQ; iq++)
        {
            int ivecind = datavec[0].fVecShapeIndex[iq].first;
            int ishapeind = datavec[0].fVecShapeIndex[iq].second;
            
            TPZFNMatrix<3> ivec(3,1);
            ivec(0,0) = datavec[0].fNormalVec(0,ivecind);
            ivec(1,0) = datavec[0].fNormalVec(1,ivecind);
            ivec(2,0) = datavec[0].fNormalVec(2,ivecind);
            TPZFNMatrix<3> axesvec(3,1);
            datavec[0].axes.Multiply(ivec,axesvec);
            
            REAL divwq = 0.;
            for(int iloc=0; iloc<fDim; iloc++)
            {
                divwq += axesvec(iloc,0)*dphiQ(iloc,ishapeind);
            }
            for (int jp=0; jp<phrP; jp++) {
                
                REAL fact = (-1.)*weight*phiP(jp,0)*divwq;
                // Matrix B
                ek(iq, phrQ+jp) += fact;
                
                // Matrix B^T
                ek(phrQ+jp,iq) += fact;
            }
        }
        
        //Right side of the pressure equation
        REAL fXfLocP = fxfPQ;
        if(fForcingFunction) {
            TPZManVector<STATE> res(1);
            fForcingFunction->Execute(datavec[1].x,res);
            fXfLocP = res[0];
        }
        for(int ip=0; ip<phrP; ip++){
            ef(phrQ+ip,0) += (-1.)*weight*fXfLocP*phiP(ip,0);
        }
        
        //Calculate the matrix contribution for saturation.
        REAL fXfLocS = fxfS;
        if(fForcingFunction) {
            TPZManVector<STATE> res(1);
            fForcingFunction->Execute(datavec[2].x,res);
            fXfLocS = res[0];
        }
        for(int in = 0; in < phrS; in++ ) {
            
            ef(in+rQP, 0) += fTimeStep*weight*fXfLocS*phiS(in,0);
            
            for(int jn = 0; jn < phrS; jn++)
            {
                ek(in+rQP,jn+rQP) += weight*fPoros*phiS(in,0)*phiS(jn,0);
            }
        }
    }//end stiffness matrix at ECurrentState
	
    
    //Last state (n): mass matrix
	if(gState == ELastState)
    {
        TPZVec<STATE> ConvDirAx;
        ConvDirAx.Resize(fDim, 0.);
        
        int di,dj;
        for(di=0; di<fDim; di++){
            for(dj=0; dj<fDim; dj++){
                ConvDirAx[di] += axesS(di,dj)*fConvDir[dj];
            }
        }

        int kd;
        for(int in = 0; in < phrS; in++) {
            
            for(int jn = 0; jn < phrS; jn++)
            {
                ek(in+rQP,jn+rQP) += weight*fPoros*phiS(in,0)*phiS(jn,0);
                
                for(kd=0; kd<fDim; kd++)
                {
                    ek(in+rQP,jn+rQP) += weight*(fTimeStep*ConvDirAx[kd]*dphiS(kd,in)*phiS(jn,0));
                }
            }
        }
    }//end stiffness matrix at ELastState
    
}

void TPZTracerFlow::ContributeBC(TPZVec<TPZMaterialData> &datavec,REAL weight, TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef,TPZBndCond &bc){
    
    
#ifdef DEBUG
    int nref =  datavec.size();
	if (nref != 3 ) {
        std::cout << " Erro.!! datavec tem que ser de tamanho 2 \n";
		DebugStop();
	}
#endif
	
	TPZFMatrix<REAL>  &phiQ = datavec[0].phi;
	int phrQ = datavec[0].fVecShapeIndex.NElements();
    
	REAL v2;
	v2 = bc.Val2()(0,0);
	
	switch (bc.Type()) {
		case 0 :		// Dirichlet condition
			//primeira equacao
			for(int iq=0; iq<phrQ; iq++)
            {
                //the contribution of the Dirichlet boundary condition appears in the flow equation
                ef(iq,0) += (-1.)*v2*phiQ(iq,0)*weight;
            }
            break;
			
		case 1 :			// Neumann condition
			//primeira equacao
			for(int iq=0; iq<phrQ; iq++)
            {
                ef(iq,0)+= gBigNumber*v2*phiQ(iq,0)*weight;
                for (int jq=0; jq<phrQ; jq++) {
                    
                    ek(iq,jq)+= gBigNumber*phiQ(iq,0)*phiQ(jq,0)*weight;
                }
            }
			break;
            
        case 2 :			// mixed condition
            for(int iq = 0; iq < phrQ; iq++) {
                
				ef(iq,0) += v2*phiQ(iq,0)*weight;
				for (int jq = 0; jq < phrQ; jq++) {
					ek(iq,jq) += weight*bc.Val1()(0,0)*phiQ(iq,0)*phiQ(jq,0);
				}
			}
            
            break;
            
        case 5 :        // Neumann(pressure)-Inflow(saturation)
			//primeira equacao
			for(int iq=0; iq<phrQ; iq++)
            {
                ef(iq,0)+= gBigNumber*v2*phiQ(iq,0)*weight;
                for (int jq=0; jq<phrQ; jq++) {
                    
                    ek(iq,jq)+= gBigNumber*phiQ(iq,0)*phiQ(jq,0)*weight;
                }
            }
			break;
            
        case 6 :		// Dirichlet(pressure)-Outflow(saturation)
			//primeira equacao
			for(int iq=0; iq<phrQ; iq++)
            {
                //the contribution of the Dirichlet boundary condition appears in the flow equation
                ef(iq,0) += (-1.)*v2*phiQ(iq,0)*weight;
            }
            break;
	}
}

//Only to saturation equation
void TPZTracerFlow::ContributeInterface(TPZMaterialData &data, TPZVec<TPZMaterialData> &dataleft, TPZVec<TPZMaterialData> &dataright, REAL weight, TPZFMatrix<STATE> &ek, TPZFMatrix<STATE> &ef){
    
    if(gState == ECurrentState){
		return;
	}

	TPZFMatrix<REAL> &dphiLdAxes = dataleft[2].dphix;
	TPZFMatrix<REAL> &dphiRdAxes = dataright[2].dphix;
	TPZFMatrix<REAL> &phiL = dataleft[2].phi;
	TPZFMatrix<REAL> &phiR = dataright[2].phi;
	TPZManVector<REAL,3> &normal = data.normal;
	
	TPZFNMatrix<660> dphiL, dphiR;
	TPZAxesTools<REAL>::Axes2XYZ(dphiLdAxes, dphiL, dataleft[2].axes);
	TPZAxesTools<REAL>::Axes2XYZ(dphiRdAxes, dphiR, dataright[2].axes);
	
	
	int nrowl = phiL.Rows();
	int nrowr = phiR.Rows();
    int rQPleft = dataleft[0].phi.Rows() + dataleft[1].phi.Rows();
    int rQPright = dataright[0].phi.Rows() + dataright[1].phi.Rows();
	int il,jl,ir,jr,id;
	
	//Convection term
	REAL ConvNormal = 0.;
	for(id=0; id<fDim; id++) ConvNormal += fConvDir[id]*normal[id];
	if(ConvNormal > 0.)
    {
        //phi_I_left, phi_J_left
		for(il=0; il<nrowl; il++)
        {
			for(jl=0; jl<nrowl; jl++)
            {
				ek(il+rQPleft,jl+rQPleft) += weight*(-fTimeStep*ConvNormal*phiL(il,0)*phiL(jl,0));
			}
		}
        
        // phi_I_right, phi_J_left
		for(ir=0; ir<nrowr; ir++)
        {
			for(jl=0; jl<nrowl; jl++)
            {
				ek(ir+rQPright+nrowl,jl+rQPleft) -= weight*(-fTimeStep*ConvNormal*phiR(ir,0)*phiL(jl,0));
			}
		}
	} else
    {
        //phi_I_right, phi_J_right
		for(ir=0; ir<nrowr; ir++)
        {
			for(jr=0; jr<nrowr; jr++)
            {
				ek(ir+rQPright+nrowl,jr+rQPright+nrowl) -= weight*(-fTimeStep*ConvNormal*phiR(ir,0)*phiR(jr,0));
			}
		}
        
        //phi_I_left, phi_J_right
		for(il=0; il<nrowl; il++)
        {
			for(jr=0; jr<nrowr; jr++)
            {
				ek(il+rQPleft,jr+rQPright+nrowl) += weight*(-fTimeStep*ConvNormal*phiL(il,0)*phiR(jr,0));
			}
		}
	}
}

void TPZTracerFlow::ContributeBCInterface(TPZMaterialData &data, TPZVec<TPZMaterialData> &dataleft, REAL weight, TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef,TPZBndCond &bc){
    
    if(gState == ELastState){
		return;
	}
    
    
    TPZFMatrix<REAL> &phiL = dataleft[2].phi;
	TPZManVector<REAL,3> &normal = data.normal;
	
	int il,jl,nrowl,id;
	nrowl = phiL.Rows();
    int rQPleft = dataleft[0].phi.Rows() + dataleft[1].phi.Rows();
	REAL ConvNormal = 0.;
    
	for(id=0; id<fDim; id++) ConvNormal += fConvDir[id]*normal[id];
    
	switch(bc.Type()) {
		case 3: // Inflow or Dirichlet
			
            //convection
			if(ConvNormal > 0.)
            {
				for(il=0; il<nrowl; il++){
					for(jl=0; jl<nrowl; jl++)
                    {
						ek(il+rQPleft,jl+rQPleft) += weight*fTimeStep*ConvNormal*phiL(il)*phiL(jl);
					}
				}
			}
            else{
				for(il=0; il<nrowl; il++)
                {
					ef(il+rQPleft,0) -= weight*fTimeStep*ConvNormal*bc.Val2()(1,0)*phiL(il);
				}
			}
			
			break;
            
			
        case 1: // Neumann
			for(il=0; il<nrowl; il++) {
				ef(il+rQPleft,0) += 0.;//ainda nao temos condicao de Neumann
			}
            break;
            
		case 4: // outflow condition
			if(ConvNormal > 0.) {
				for(il=0; il<nrowl; il++) {
					for(jl=0; jl<nrowl; jl++) {
						ek(il+rQPleft,jl+rQPleft) += weight*fTimeStep*ConvNormal*phiL(il)*phiL(jl);
					}
				}
			}
			else {
				if (ConvNormal < 0.) std::cout << "Boundary condition error: inflow detected in outflow boundary condition: ConvNormal = " << ConvNormal << "\n";
			}
			break;
            
        case 5: // // Neumann(pressure)-Inflow(saturation)
			
            //convection
			if(ConvNormal > 0.)
            {
				for(il=0; il<nrowl; il++){
					for(jl=0; jl<nrowl; jl++)
                    {
						ek(il+rQPleft,jl+rQPleft) += weight*fTimeStep*ConvNormal*phiL(il)*phiL(jl);
					}
				}
			}
            else{
				for(il=0; il<nrowl; il++)
                {
					ef(il+rQPleft,0) -= weight*fTimeStep*ConvNormal*bc.Val2()(1,0)*phiL(il);
				}
			}
			break;
            
        case 6: // Dirichlet(pressure)-Outflow(saturation)
			if(ConvNormal > 0.) {
				for(il=0; il<nrowl; il++) {
					for(jl=0; jl<nrowl; jl++) {
						ek(il+rQPleft,jl+rQPleft) += weight*fTimeStep*ConvNormal*phiL(il)*phiL(jl);
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
int TPZTracerFlow::VariableIndex(const std::string &name)
{
    if(!strcmp("Flux",name.c_str()))                return  1;
    if(!strcmp("Pressure",name.c_str()))            return  2;
	if(!strcmp("Saturation",name.c_str()))          return  3;
	if(!strcmp("SaturationFlux",name.c_str()))      return  4;
	
	return TPZMaterial::VariableIndex(name);
}

int TPZTracerFlow::NSolutionVariables(int var){
	if((var == 2) || (var == 3)) return 1;
	if((var == 1) || (var == 4)) return fDim;
	return TPZMaterial::NSolutionVariables(var);
}

void TPZTracerFlow::Solution(TPZVec<TPZMaterialData> &datavec, int var, TPZVec<STATE> &Solout){
	
	Solout.Resize(this->NSolutionVariables(var));
    
    if(var == 1){ //function (state variable Q)
		Solout[0] = datavec[0].sol[0][0];
        Solout[1] = datavec[0].sol[0][1];
		return;
	}
    
    if(var == 2){
		Solout[0] = datavec[1].sol[0][0];//function (state variable p)
		return;
	}
    
	if(var == 3){
		Solout[0] = datavec[2].sol[0][0];//function (state variable S)
		return;
	}
    
	if(var == 4) {
		int id;
		for(id=0 ; id<fDim; id++) {
			Solout[id] =fConvDir[id]*datavec[2].sol[0][0];//function (state variable Q*S)
		}
		return;
	}
}

void TPZTracerFlow::ContributeInterface(TPZMaterialData &data, TPZMaterialData &dataleft, TPZMaterialData &dataright, REAL weight, TPZFMatrix<STATE> &ek, TPZFMatrix<STATE> &ef){
	DebugStop();
}

void TPZTracerFlow::ContributeBCInterface(TPZMaterialData &data, TPZMaterialData &dataleft, REAL weight, TPZFMatrix<STATE> &ek,TPZFMatrix<STATE> &ef,TPZBndCond &bc){
	DebugStop();
}

