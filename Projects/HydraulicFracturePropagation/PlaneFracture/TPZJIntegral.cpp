//
//  TPZJIntegral.cpp
//  PZ
//
//  Created by Labmec on 25/04/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>

#include "TPZJIntegral.h"

#include "adapt.h"
#include "TPZVTKGeoMesh.h"
#include "TPZTimer.h"

const REAL gIntegrPrecision = 1.e-5;
const REAL gScaleFactor = 1.e5;


//--------------------------------------------------------class LinearPath3D


LinearPath3D::LinearPath3D()
{
    DebugStop();//Nao eh para usar construtor vazio
}


LinearPath3D::LinearPath3D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &FinalPoint, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure)
{    
    fFinalPoint = FinalPoint;
    fNormalDirection = normalDirection;
    fradius = radius;
    
    fDETdxdt = fradius/2.;
    
    fcmesh = cmesh;
    fcrackPressure = fabs(pressure);
    
    fInitialPoint.Resize(3, 0.);
    fInitialPoint[0] = (fFinalPoint[0] - fradius*sin((M_PI)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])));
    fInitialPoint[1] = fradius*cos((M_PI)/2.); 
    fInitialPoint[2] = (fFinalPoint[2] + fradius*cos(atan2(fNormalDirection[2],fNormalDirection[0]))*sin((M_PI)/2.));
    
    f_t_elIdqsi.clear();
}

LinearPath3D::LinearPath3D(LinearPath3D * cp)
{
    fInitialPoint = cp->fInitialPoint;
    fFinalPoint = cp->fFinalPoint;
    fNormalDirection = cp->fNormalDirection;
    fradius = cp->fradius;
    
    fDETdxdt = cp->fDETdxdt;
    
    fcmesh = cp->fcmesh;
    fcrackPressure = cp->fcrackPressure;
    
    f_t_elIdqsi.clear();
}

LinearPath3D::~LinearPath3D()
{
    fcmesh = NULL;
}


void LinearPath3D::X(REAL t, TPZVec<REAL> & xt)
{
    xt.Resize(3,0.);
    
    for(int c = 0; c < 3; c++)
    {
        xt[c] = (1.-t)/2.*fInitialPoint[c] + (1.+t)/2.*fFinalPoint[c];
    }
}


//void LinearPath3D::dXdt(REAL t, TPZVec<REAL> & dxdt)
//{
//    dxdt.Resize(3,0.);
//
//    TPZVec<REAL> xarc(3);
//    t = 1;
//    xarc[0] = (fOrigin[0] - fradius*sin(M_PI/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])));
//    xarc[1] = fradius*cos((M_PI*t)/2.);
//    xarc[2] = (fOrigin[2] + fradius*cos(atan2(fNormalDirection[2],fNormalDirection[0]))*sin((M_PI*t)/2.));
//
//    for(int c = 0; c < 3; c++)
//    {
//        dxdt[c] = (fOrigin[c] - xarc[c])/2.;
//    }
//}


void LinearPath3D::normalVec(REAL t, TPZVec<REAL> & n)
{
    n[0] =  0.;
    n[1] = -1.;
    n[2] =  0.;
}


REAL LinearPath3D::DETdxdt()
{
    return fDETdxdt;
}

REAL LinearPath3D::Radius()
{
    return fradius;
}


#ifdef print_Jx_Linear
std::map<REAL,REAL> functionLinJx;
#endif

TPZVec<REAL> LinearPath3D::Func(REAL t)
{
    TPZVec<REAL> xt(3), nt(3);
    
    X(t,xt);
    this->normalVec(t, nt);
    
    TPZVec<REAL> linContribution(3,0.);
    linContribution = Function(t, xt, nt);
    
    linContribution[0] = linContribution[0] * gScaleFactor;
    linContribution[1] = 0.;
    linContribution[2] = linContribution[2] * gScaleFactor;
    
    #ifdef print_Jx_Linear
    functionLinJx[xt[0]] = linContribution[0];
    #endif
    
    return linContribution;
}


TPZVec<REAL> LinearPath3D::Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt)
{
    TPZVec<REAL> qsi(0);
    
    int InitialElementId = 0;
    std::map< REAL , std::pair< int , TPZVec<REAL> > >::iterator it = f_t_elIdqsi.lower_bound(t);
    if(it != f_t_elIdqsi.end())
    {
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else if(f_t_elIdqsi.size() > 0)
    {
        it--;
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else
    {
        qsi.Resize(fcmesh->Reference()->ElementVec()[InitialElementId]->Dimension(),0.);
    }
    TPZGeoEl * geoEl = fcmesh->Reference()->FindElement(xt, qsi, InitialElementId, 3);
    
    if(!geoEl)
    {
        std::cout << "geoEl not found! See " << __PRETTY_FUNCTION__ << " !!!\n";
        DebugStop();
    }
    
    f_t_elIdqsi[t] = std::make_pair(geoEl->Id(), qsi);
    
    TPZVec<REAL> minusGradUt_Sigma__n(3,0.);
    
    TPZCompEl * compEl = geoEl->Reference();
    
#ifdef DEBUG
    if(!compEl)
    {
        std::cout << "Null compEl!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
#endif
    
    TPZInterpolationSpace * intpEl = dynamic_cast<TPZInterpolationSpace *>(compEl);
    TPZMaterialData data;
    intpEl->InitMaterialData(data);
    
    intpEl->ComputeShape(qsi, data);
    intpEl->ComputeSolution(qsi, data);
    
    TPZFMatrix<REAL> GradUtxy(3,3);
    GradUtxy.Zero();
    
    //remember: Dont need to multiply with axes once this
    //          last is IDENTITY in PZ (because is 3D element).
    GradUtxy = data.dsol[0];
    
    TPZVec<REAL> Sigma_n(3,0.);
    Sigma_n[1] = fcrackPressure;
    for(int r = 0; r < 3; r++)
    {
        for(int c = 0; c < 3; c++)
        {
            minusGradUt_Sigma__n[r] += -(GradUtxy(r,c)*Sigma_n[c]);
        }
    }
    
    return minusGradUt_Sigma__n;
}

//-------------------------class LinearPath2D

LinearPath2D::LinearPath2D() : LinearPath3D()
{
    
}

LinearPath2D::LinearPath2D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &FinalPoint, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure) :
              LinearPath3D(cmesh,FinalPoint,normalDirection,radius,pressure)
{
    
}

LinearPath2D::LinearPath2D(LinearPath2D * cp) : LinearPath3D(cp)
{
    
}

LinearPath2D::~LinearPath2D()
{
    fcmesh = NULL;
}
    
TPZVec<REAL> LinearPath2D::Func(REAL t)
{
    TPZVec<REAL> xt(3), nt(3);
    
    X(t,xt);
    this->normalVec(t, nt);
    
    TPZVec<REAL> linContribution(2,0.);
    linContribution = Function(t, xt, nt);
    
    linContribution[0] = linContribution[0] * gScaleFactor;
    linContribution[1] = 0.;
    
#ifdef print_Jx_Linear
    functionLinJx[xt[0]] = linContribution[0];
#endif
    
    return linContribution;
}

TPZVec<REAL> LinearPath2D::Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt)
{
    TPZVec<REAL> qsi(0);
    
    int InitialElementId = 0;
    std::map< REAL , std::pair< int , TPZVec<REAL> > >::iterator it = f_t_elIdqsi.lower_bound(t);
    if(it != f_t_elIdqsi.end())
    {
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else if(f_t_elIdqsi.size() > 0)
    {
        it--;
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else
    {
        qsi.Resize(fcmesh->Reference()->ElementVec()[InitialElementId]->Dimension(),0.);
    }
    TPZGeoEl * geoEl = fcmesh->Reference()->FindElement(xt, qsi, InitialElementId, 2);
    
    if(!geoEl)
    {
        std::cout << "geoEl not found! See " << __PRETTY_FUNCTION__ << " !!!\n";
        DebugStop();
    }
    
    f_t_elIdqsi[t] = std::make_pair(geoEl->Id(), qsi);
    
    TPZVec<REAL> minusGradUt_Sigma__n(2,0.);
    
    TPZCompEl * compEl = geoEl->Reference();
    
#ifdef DEBUG
    if(!compEl)
    {
        std::cout << "Null compEl!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
#endif
    
    TPZInterpolationSpace * intpEl = dynamic_cast<TPZInterpolationSpace *>(compEl);
    TPZMaterialData data;
    intpEl->InitMaterialData(data);
    
    intpEl->ComputeShape(qsi, data);
    intpEl->ComputeSolution(qsi, data);
    
    TPZFMatrix<REAL> GradUtxy(2,2);
    GradUtxy.Zero();

    TPZFMatrix<REAL> GradUtax(2,2);
    GradUtax = data.dsol[0];
    GradUtxy(0,0) = GradUtax(0,0)*data.axes(0,0) + GradUtax(1,0)*data.axes(1,0);
    GradUtxy(1,0) = GradUtax(0,0)*data.axes(0,1) + GradUtax(1,0)*data.axes(1,1);
    GradUtxy(0,1) = GradUtax(0,1)*data.axes(0,0) + GradUtax(1,1)*data.axes(1,0);
    GradUtxy(1,1) = GradUtax(0,1)*data.axes(0,1) + GradUtax(1,1)*data.axes(1,1);
    
    TPZVec<REAL> Sigma_n(2,0.);
    
    Sigma_n[1] = fcrackPressure;
    for(int r = 0; r < 2; r++)
    {
        for(int c = 0; c < 2; c++)
        {
            minusGradUt_Sigma__n[r] += -(GradUtxy(r,c)*Sigma_n[c]);
        }
    }
    
    return minusGradUt_Sigma__n;
}


//--------------------------------------------------------class ArcPath3D


ArcPath3D::ArcPath3D()
{
    DebugStop();//Nao eh para usar construtor vazio
}


ArcPath3D::ArcPath3D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius)
{
    fOrigin = Origin;
    fNormalDirection = normalDirection;
    fradius = radius;
    
    fDETdxdt = M_PI*fradius/2.;
    
    fcmesh = cmesh;
    
    f_t_elIdqsi.clear();
}


ArcPath3D::ArcPath3D(ArcPath3D * cp)
{
    fOrigin = cp->fOrigin;
    fNormalDirection = cp->fNormalDirection;
    fradius = cp->fradius;
    
    fDETdxdt = cp->fDETdxdt;
    
    fcmesh = cp->fcmesh;
    
    f_t_elIdqsi.clear();
}


ArcPath3D::~ArcPath3D()
{
    fcmesh = NULL;
}


void ArcPath3D::X(REAL t, TPZVec<REAL> & xt)
{
    xt.Resize(3,0.);
    
    xt[0] = (fOrigin[0] - fradius*sin((M_PI*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])));
    xt[1] = fradius*cos((M_PI*t)/2.);
    xt[2] = (fOrigin[2] + fradius*cos(atan2(fNormalDirection[2],fNormalDirection[0]))*sin((M_PI*t)/2.));
}


//void ArcPath3D::dXdt(REAL t, TPZVec<REAL> & dxdt)
//{
//    dxdt.Resize(3,0.);
//
//    dxdt[0] = -(M_PI*fradius*cos((M_PI*t)/2.)*sin(atan2(fNormalDirection[2],fNormalDirection[0])))/2.;
//    dxdt[1] = -(M_PI*fradius*sin((M_PI*t)/2.))/2.;
//    dxdt[2] = +(M_PI*fradius*cos((M_PI*t)/2.)*cos(atan2(fNormalDirection[0],fNormalDirection[2])))/2.;
//}


void ArcPath3D::normalVec(REAL t, TPZVec<REAL> & n)
{
    TPZVec<REAL> xt(3);
    X(t, xt);
    for(int i = 0; i < 3; i++)
    {
        n[i] = (xt[i] - fOrigin[i])/fradius;
    }
}


REAL ArcPath3D::DETdxdt()
{
    return fDETdxdt;
}


TPZVec<REAL> ArcPath3D::Func(REAL t)
{
    TPZVec<REAL> xt(3), nt(3);
    
    X(t,xt);
    this->normalVec(t, nt);
    
    TPZVec<REAL> arcContribution(3,0.);
    arcContribution = Function(t, xt, nt);
    
    arcContribution[0] = arcContribution[0] * gScaleFactor;
    arcContribution[1] = 0.;
    arcContribution[2] = arcContribution[2] * gScaleFactor;
    
    return arcContribution;
}


TPZVec<REAL> ArcPath3D::Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt)
{
    TPZVec<REAL> qsi(0);
    
    int InitialElementId = 0;
    std::map< REAL , std::pair< int , TPZVec<REAL> > >::iterator it = f_t_elIdqsi.lower_bound(t);
    if(it != f_t_elIdqsi.end())
    {
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else if(f_t_elIdqsi.size() > 0)
    {
        it--;
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else
    {
        qsi.Resize(fcmesh->Reference()->ElementVec()[InitialElementId]->Dimension(),0.);
    }
    TPZGeoEl * geoEl = fcmesh->Reference()->FindElement(xt, qsi, InitialElementId, 3);
    
    if(!geoEl)
    {
        std::cout << "geoEl not found! See " << __PRETTY_FUNCTION__ << " !!!\n";
        DebugStop();
    }
    
    f_t_elIdqsi[t] = std::make_pair(geoEl->Id(), qsi);
    
    TPZCompEl * compEl = geoEl->Reference();
    
#ifdef DEBUG
    if(!compEl)
    {
        std::cout << "Null compEl!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
#endif
    
    TPZInterpolationSpace * intpEl = dynamic_cast<TPZInterpolationSpace *>(compEl);
    TPZMaterialData data;
    intpEl->InitMaterialData(data);
    
    intpEl->ComputeShape(qsi, data);
    intpEl->ComputeSolution(qsi, data);
    
    TPZFMatrix<REAL> Sigma(3,3), strain(3,3), GradUtxy(3,3);
    Sigma.Zero();
    strain.Zero();
    GradUtxy.Zero();

    GradUtxy = data.dsol[0];
    
    TPZElasticity3D * elast3D = dynamic_cast<TPZElasticity3D *>(compEl->Material());
    
#ifdef DEBUG
    if(!elast3D)
    {
        std::cout << "This material might be TPZElastMat3D type!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
#endif
    
    elast3D->ComputeStressTensor(Sigma, data);
    elast3D->ComputeStrainTensor(strain, GradUtxy);
    
    TPZFMatrix<REAL> GradUt_Sigma(3,3,0.);
    GradUtxy.Multiply(Sigma, GradUt_Sigma);
    
    REAL W = 0.;
    for(int r = 0; r < 3; r++)
    {
        for(int c = 0; c < 3; c++)
        {
            W += 0.5*Sigma(r,c)*strain(r,c);
        }
    }
    
    TPZFMatrix<REAL> W_I(3,3,0.);
    for(int d = 0; d < 3; d++)
    {
        W_I(d,d) = W;
    }
    
    TPZFMatrix<REAL> W_I_minus_GradUt_Sigma(3,3,0.);
    W_I_minus_GradUt_Sigma = W_I - GradUt_Sigma;
    
    TPZVec<REAL> W_I_minus_GradUt_Sigma__n(3,0.);
    for(int r = 0; r < 3; r++)
    {
        for(int c = 0; c < 3; c++)
        {
            W_I_minus_GradUt_Sigma__n[r] += (W_I_minus_GradUt_Sigma(r,c)*nt[c]);
        }
    }
    
    return W_I_minus_GradUt_Sigma__n;
}


void ArcPath3D::SetRadius(REAL radius)
{
    fradius = radius;
    fDETdxdt = M_PI*radius/2.;
    
    f_t_elIdqsi.clear();
}


REAL ArcPath3D::Radius()
{
    return fradius;
}

//-------------------------class ArcPath2D

ArcPath2D::ArcPath2D() : ArcPath3D()
{
    
}

ArcPath2D::ArcPath2D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius) :
           ArcPath3D(cmesh,Origin,normalDirection,radius)
{
    
}

ArcPath2D::ArcPath2D(ArcPath2D * cp) : ArcPath3D(cp)
{
    
}

ArcPath2D::~ArcPath2D()
{
    fcmesh = NULL;
}

TPZVec<REAL> ArcPath2D::Func(REAL t)
{
    TPZVec<REAL> xt(3), nt(3);
    
    X(t,xt);
    this->normalVec(t, nt);
    
    TPZVec<REAL> arcContribution(2,0.);
    arcContribution = Function(t, xt, nt);
    
    arcContribution[0] = arcContribution[0] * gScaleFactor;
    arcContribution[1] = 0.;
    
    return arcContribution;
}

TPZVec<REAL> ArcPath2D::Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt)
{
    TPZVec<REAL> qsi(0);
    
    int InitialElementId = 0;
    std::map< REAL , std::pair< int , TPZVec<REAL> > >::iterator it = f_t_elIdqsi.lower_bound(t);
    if(it != f_t_elIdqsi.end())
    {
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else if(f_t_elIdqsi.size() > 0)
    {
        it--;
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else
    {
        qsi.Resize(fcmesh->Reference()->ElementVec()[InitialElementId]->Dimension(),0.);
    }
    TPZGeoEl * geoEl = fcmesh->Reference()->FindElement(xt, qsi, InitialElementId, 2);
    
    if(!geoEl)
    {
        std::cout << "geoEl not found! See " << __PRETTY_FUNCTION__ << " !!!\n";
        DebugStop();
    }
    
    f_t_elIdqsi[t] = std::make_pair(geoEl->Id(), qsi);
    
    TPZCompEl * compEl = geoEl->Reference();
    
#ifdef DEBUG
    if(!compEl)
    {
        std::cout << "Null compEl!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
#endif
    
    TPZInterpolationSpace * intpEl = dynamic_cast<TPZInterpolationSpace *>(compEl);
    TPZMaterialData data;
    intpEl->InitMaterialData(data);
    
    intpEl->ComputeShape(qsi, data);
    intpEl->ComputeSolution(qsi, data);
    
    TPZFMatrix<REAL> Sigma(2,2), strain(2,2), GradUtxy(2,2);
    Sigma.Zero();
    strain.Zero();
    GradUtxy.Zero();
    
    TPZFMatrix<REAL> GradUtax(2,2);
    GradUtax = data.dsol[0];
    GradUtxy(0,0) = GradUtax(0,0)*data.axes(0,0) + GradUtax(1,0)*data.axes(1,0);
    GradUtxy(1,0) = GradUtax(0,0)*data.axes(0,1) + GradUtax(1,0)*data.axes(1,1);
    GradUtxy(0,1) = GradUtax(0,1)*data.axes(0,0) + GradUtax(1,1)*data.axes(1,0);
    GradUtxy(1,1) = GradUtax(0,1)*data.axes(0,1) + GradUtax(1,1)*data.axes(1,1);
    
    TPZElasticityMaterial * elast2D = dynamic_cast<TPZElasticityMaterial *>(compEl->Material());
    
#ifdef DEBUG
    if(!elast2D)
    {
        std::cout << "This material might be TPZElasticityMaterial type!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
#endif
    
    TPZVec<REAL> Solout(3);
    int var;
    
    var = 10;//Stress Tensor
    elast2D->Solution(data, var, Solout);
    Sigma(0,0) = Solout[0];
    Sigma(1,1) = Solout[1];
    Sigma(0,1) = Solout[2];
    Sigma(1,0) = Solout[2];
    
    var = 11;//Strain Tensor
    elast2D->Solution(data, var, Solout);
    strain(0,0) = Solout[0];
    strain(1,1) = Solout[1];
    strain(0,1) = Solout[2];
    strain(1,0) = Solout[2];
    
    TPZFMatrix<REAL> GradUt_Sigma(2,2,0.);
    GradUtxy.Multiply(Sigma, GradUt_Sigma);
    
    REAL W = 0.;
    for(int r = 0; r < 2; r++)
    {
        for(int c = 0; c < 2; c++)
        {
            W += 0.5*Sigma(r,c)*strain(r,c);
        }
    }
    
    TPZFMatrix<REAL> W_I(2,2,0.);
    for(int d = 0; d < 2; d++)
    {
        W_I(d,d) = W;
    }
    
    TPZFMatrix<REAL> W_I_minus_GradUt_Sigma(2,2,0.);
    W_I_minus_GradUt_Sigma = W_I - GradUt_Sigma;
    
    TPZVec<REAL> W_I_minus_GradUt_Sigma__n(2,0.);
    for(int r = 0; r < 2; r++)
    {
        for(int c = 0; c < 2; c++)
        {
            W_I_minus_GradUt_Sigma__n[r] += (W_I_minus_GradUt_Sigma(r,c)*nt[c]);
        }
    }
    
    return W_I_minus_GradUt_Sigma__n;
}


//--------------------------------------------------------class AreaPath3D


AreaPath3D::LinearPath3D_2::LinearPath3D_2()
{
    DebugStop();//Nao eh para usar construtor vazio
}

AreaPath3D::LinearPath3D_2::LinearPath3D_2(LinearPath3D * cp) : LinearPath3D(cp)
{
    fArcPath3D = new ArcPath3D_2(this->fcmesh, this->fFinalPoint, this->fNormalDirection, this->fradius);
    
#ifdef DEBUG
    if(!fArcPath3D)
    {
        DebugStop();
    }
#endif
}

AreaPath3D::LinearPath3D_2::~LinearPath3D_2()
{
    fcmesh = NULL;
}

TPZVec<REAL> AreaPath3D::LinearPath3D_2::Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt)
{
    TPZVec<REAL> arcIntegral(3,0.);
    REAL arcRadius = 0.;
    for(int c = 0; c < 3; c++)
    {
        arcRadius += (fFinalPoint[c] - xt[c])*(fFinalPoint[c] - xt[c]);
    }
    arcRadius = sqrt(arcRadius);
    fArcPath3D->SetRadius(arcRadius);
    
    Adapt intRule(1.e-1);
    arcIntegral = intRule.Vintegrate(*(fArcPath3D),3,-1.,+1.);
    
    for(int i = 0; i < 3; i++)
    {
        arcIntegral[i] = arcIntegral[i] * fArcPath3D->DETdxdt();
    }
    
    return arcIntegral;
}

//------------

AreaPath3D::LinearPath3D_2::ArcPath3D_2::ArcPath3D_2()
{
    DebugStop();//Nao eh para usar construtor vazio
}

AreaPath3D::LinearPath3D_2::ArcPath3D_2::ArcPath3D_2(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius) :
ArcPath3D(cmesh,Origin,normalDirection,radius)
{
    
}

AreaPath3D::LinearPath3D_2::ArcPath3D_2::~ArcPath3D_2()
{
    fcmesh = NULL;
}

TPZVec<REAL> AreaPath3D::LinearPath3D_2::ArcPath3D_2::Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt)
{
    TPZVec<REAL> answ(3,0.);
    answ = ComputeFiniteDifference(t, xt, 0.001);
    
    return answ;
}

TPZVec<REAL> AreaPath3D::LinearPath3D_2::ArcPath3D_2::ComputeFiniteDifference(REAL t, TPZVec<REAL> xt, REAL halfDelta)
{
    TPZVec<REAL> direction = this->fNormalDirection;
    REAL norm = 0;
    for(int i = 0; i < 3; i++)
    {
        norm += direction[i]*direction[i];
    }
    norm = sqrt(norm);
    
#ifdef DEBUG
    if(norm < 1.8-12)
    {
        DebugStop();
    }
#endif
    
    TPZVec<REAL> upPos(3,0.), downPos(3,0.);
    for (int i = 0; i < 3; i++)
    {
        upPos[i]    = xt[i] + halfDelta*direction[i]/norm;
        downPos[i]  = xt[i] - halfDelta*direction[i]/norm;
    }
    
    TPZVec<REAL> upSol(3,0.), downSol(3,0.);
    upSol = FunctionAux(t,upPos,direction);
    downSol = FunctionAux(t,downPos,direction);
    
    TPZVec<REAL> answ(3,0.);
    for(int i = 0; i < 3; i++)
    {
        answ[i] = (upSol[i] - downSol[i])/(2.*halfDelta);
    }
    
    return answ;
}

TPZVec<REAL> AreaPath3D::LinearPath3D_2::ArcPath3D_2::FunctionAux(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & direction)
{
//    double x = xt[0];
//    double y = xt[1];
//    double z = xt[2];
//    TPZVec<REAL> answTeste(3,0.);
//    answTeste[0] = -sin(2.*x*y*z);
//    return answTeste; //Para Z = -5 e r = 0.6, a integral final (multiplicada por 2) = 0.0669331
    
    TPZVec<REAL> qsi(0);
    
    int InitialElementId = 0;
    std::map< REAL , std::pair< int , TPZVec<REAL> > >::iterator it = f_t_elIdqsi.lower_bound(t);
    if(it != f_t_elIdqsi.end())
    {
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else if(f_t_elIdqsi.size() > 0)
    {
        it--;
        InitialElementId = it->second.first;
        qsi = it->second.second;
    }
    else
    {
        qsi.Resize(fcmesh->Reference()->ElementVec()[InitialElementId]->Dimension(),0.);
    }
    TPZGeoEl * geoEl = fcmesh->Reference()->FindElement(xt, qsi, InitialElementId, 3);
    
    if(!geoEl)
    {
        std::cout << "geoEl not found! See " << __PRETTY_FUNCTION__ << " !!!\n";
        DebugStop();
    }
    
    f_t_elIdqsi[t] = std::make_pair(geoEl->Id(), qsi);
    
    TPZCompEl * compEl = geoEl->Reference();
    
#ifdef DEBUG
    if(!compEl)
    {
        std::cout << "Null compEl!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
#endif
    
    TPZInterpolationSpace * intpEl = dynamic_cast<TPZInterpolationSpace *>(compEl);
    TPZMaterialData data;
    intpEl->InitMaterialData(data);
    
    intpEl->ComputeShape(qsi, data);
    intpEl->ComputeSolution(qsi, data);
    
    TPZFMatrix<REAL> Sigma(3,3), strain(3,3), GradUtxy(3,3);
    Sigma.Zero();
    strain.Zero();
    GradUtxy.Zero();
    
    GradUtxy = data.dsol[0];
    
    TPZElasticity3D * elast3D = dynamic_cast<TPZElasticity3D *>(compEl->Material());
    
#ifdef DEBUG
    if(!elast3D)
    {
        std::cout << "This material might be TPZElastMat3D type!\nSee " << __PRETTY_FUNCTION__ << std::endl;
        DebugStop();
    }
#endif
    
    elast3D->ComputeStressTensor(Sigma, data);
    elast3D->ComputeStrainTensor(strain, GradUtxy);
    
    TPZFMatrix<REAL> GradUt_Sigma(3,3,0.);
    GradUtxy.Multiply(Sigma, GradUt_Sigma);
    
    REAL W = 0.;
    for(int r = 0; r < 3; r++)
    {
        for(int c = 0; c < 3; c++)
        {
            W += 0.5*Sigma(r,c)*strain(r,c);
        }
    }
    
    TPZFMatrix<REAL> W_I(3,3,0.);
    for(int d = 0; d < 3; d++)
    {
        W_I(d,d) = W;
    }
    
    TPZFMatrix<REAL> W_I_minus_GradUt_Sigma(3,3,0.);
    W_I_minus_GradUt_Sigma = W_I - GradUt_Sigma;
    
    TPZVec<REAL> W_I_minus_GradUt_Sigma__n(3,0.);
    for(int r = 0; r < 3; r++)
    {
        for(int c = 0; c < 3; c++)
        {
            W_I_minus_GradUt_Sigma__n[r] += (W_I_minus_GradUt_Sigma(r,c)*direction[c]);
        }
    }
    
    return W_I_minus_GradUt_Sigma__n;
}

//------------

AreaPath3D::AreaPath3D()
{
    DebugStop();//Nao eh para usar construtor vazio
}


AreaPath3D::AreaPath3D(LinearPath3D * givenLinearPath3D)
{
    fLinearPath3D = new LinearPath3D_2(givenLinearPath3D);
    
    fDETdxdt = fLinearPath3D->DETdxdt();//O ArcPath3D_2 jah incluirah seu Detdxdt(), soh faltarah do LinearPath3D_2::Detdxdt()
    
    #ifdef DEBUG
    if(!fLinearPath3D)
    {
        DebugStop();
    }
    #endif
}


AreaPath3D::~AreaPath3D()
{
    fLinearPath3D = NULL;
}


REAL AreaPath3D::DETdxdt()
{
    return fDETdxdt;
}


TPZVec<REAL> AreaPath3D::Func(REAL t)
{
    TPZVec<REAL> areaContribution(3,0.);
    areaContribution = fLinearPath3D->Func(t);
    
    return areaContribution;
}


//--------------------------------------------------------class JPath3D


Path3D::Path3D()
{
    fLinearPath3D = NULL;
    fArcPath3D = NULL;
    fAreaPath3D = NULL;
}


Path3D::Path3D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure)
{
    fLinearPath3D = new LinearPath3D(cmesh,Origin,normalDirection,radius,pressure);
    fArcPath3D = new ArcPath3D(cmesh,Origin,normalDirection,radius);
    fAreaPath3D = new AreaPath3D(fLinearPath3D);
    
#ifdef DEBUG
    if(fabs(normalDirection[1]) > 1.E-8)
    {
        std::cout << "\nThe normal direction of J integral arc must be in XZ plane!!!\n";
        DebugStop();
    }
    if(radius < 1.e-3)
    {
        std::cout << "\nRadius too small!!!\n";
        DebugStop();
    }
#endif
}


Path3D::~Path3D()
{
    fLinearPath3D = NULL;
    fArcPath3D = NULL;
    fAreaPath3D = NULL;
}


Path2D::Path2D()
{
    fLinearPath2D = NULL;
    fArcPath2D = NULL;
}


Path2D::Path2D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure)
{
    fLinearPath2D = new LinearPath2D(cmesh,Origin,normalDirection,radius,pressure);
    fArcPath2D = new ArcPath2D(cmesh,Origin,normalDirection,radius);
    
#ifdef DEBUG
    if(fabs(normalDirection[1]) > 1.E-8)
    {
        std::cout << "\nThe normal direction of J integral arc must be in XZ plane!!!\n";
        DebugStop();
    }
    if(radius < 1.e-3)
    {
        std::cout << "\nRadius too small!!!\n";
        DebugStop();
    }
#endif
}


Path2D::~Path2D()
{
    fLinearPath2D = NULL;
    fArcPath2D = NULL;
}


//--------------------------------------------------------class JIntegral


JIntegral3D::JIntegral3D()
{
    fPath3DVec.Resize(0);
}

JIntegral3D::~JIntegral3D()
{
    fPath3DVec.Resize(0);
}

void JIntegral3D::PushBackPath3D(Path3D * Path3DElem)
{
    int oldSize = fPath3DVec.NElements();
    fPath3DVec.Resize(oldSize+1);
    fPath3DVec[oldSize] = Path3DElem;
}

TPZVec<REAL> JIntegral3D::IntegratePath3D(int p)
{
    Path3D * jPath3DElem = fPath3DVec[p];
    
    
    Adapt intRule(gIntegrPrecision);
    
    TPZTimer linInt("LinearIntegration"); linInt.start();
    TPZVec<REAL> linJintegral(3,0.);
    linJintegral = intRule.Vintegrate(*(jPath3DElem->GetLinearPath3D()),3,-1.,+1.);

    //Simetry in xz plane
    linJintegral[0] = 2. * linJintegral[0] * jPath3DElem->GetLinearPath3D()->DETdxdt() / gScaleFactor;
    linJintegral[1] = 0.;
    linJintegral[2] = 2. * linJintegral[2] * jPath3DElem->GetLinearPath3D()->DETdxdt() / gScaleFactor;

    linInt.stop();
    
    //4debug Jlin_x
    {
        #ifdef print_Jx_Linear
        std::map<REAL,REAL>::iterator ii;
        std::ofstream plotLinJx("PlotLinJx.txt");
        plotLinJx.precision(10);
        plotLinJx << "Jx = {";
        for(ii = functionLinJx.begin(); ii != functionLinJx.end(); ii++)
        {
            plotLinJx << "{" << ii->first << "," << ii->second << "},";
        }
        plotLinJx << "};\n";
        plotLinJx << "Jxplot = ListPlot[Jx,AxisOrigin->{0,0}]\n";
        plotLinJx.close();
        functionLinJx.clear();
        #endif
    }
    
    TPZTimer arcInt("ArcIntegration"); arcInt.start();
    TPZVec<REAL> arcJintegral(3,0.);
    arcJintegral = intRule.Vintegrate(*(jPath3DElem->GetArcPath3D()),3,-1.,+1.);

    //Simetry in xz plane
    arcJintegral[0] = 2. * arcJintegral[0] * jPath3DElem->GetArcPath3D()->DETdxdt() / gScaleFactor;
    arcJintegral[1] = 0.;
    arcJintegral[2] = 2. * arcJintegral[2] * jPath3DElem->GetArcPath3D()->DETdxdt() / gScaleFactor;

    arcInt.stop();
    //
    TPZTimer areaInt("AreaIntegration"); areaInt.start();
    TPZVec<REAL> areaJIntegral(3,0.);
    intRule.SetPrecision(1.e-3);
    areaJIntegral = intRule.Vintegrate(*(jPath3DElem->GetAreaPath3D()),3,-1.,+1.);

    //Simetry in xz plane
    areaJIntegral[0] = 2. * areaJIntegral[0] * jPath3DElem->GetAreaPath3D()->DETdxdt() / (gScaleFactor * gScaleFactor);
    areaJIntegral[1] = 0.;
    areaJIntegral[2] = 2. * areaJIntegral[2] * jPath3DElem->GetAreaPath3D()->DETdxdt() / (gScaleFactor * gScaleFactor);

    areaInt.stop();
    
    TPZVec<REAL> answ(3);
    for(int i = 0; i < 3; i++)
    {
        answ[i] = linJintegral[i] + arcJintegral[i] + areaJIntegral[i];
    }
    
//    std::cout << "DeltaT integracao linha = " << linInt.seconds() << " s" << std::endl;
//    std::cout << "DeltaT integracao arco = " << arcInt.seconds() << " s" << std::endl;
//    std::cout << "DeltaT integracao area = " << areaInt.seconds() << " s" << std::endl;
//    std::cout << "Jx AREA = " << areaJIntegral[0] << "\n";
//    std::cout << "J = " << answ[0] << "\n";
    
    return answ;
}

/*
 Franc2D : somente pressão de 5 no interior da fratura resulta em J = 0.0027
 */

JIntegral2D::JIntegral2D()
{
    fPath2DVec.Resize(0);
}

JIntegral2D::~JIntegral2D()
{
    fPath2DVec.Resize(0);
}

void JIntegral2D::PushBackPath2D(Path2D * Path2DElem)
{
    int oldSize = fPath2DVec.NElements();
    fPath2DVec.Resize(oldSize+1);
    fPath2DVec[oldSize] = Path2DElem;
}

TPZVec<REAL> JIntegral2D::IntegratePath2D(int p)
{
    Path2D * jPath2DElem = fPath2DVec[p];
    
    
    Adapt intRule(gIntegrPrecision);
    
    TPZTimer linInt("LinearIntegration"); linInt.start();
    TPZVec<REAL> linJintegral(2,0.);
    linJintegral = intRule.Vintegrate(*(jPath2DElem->GetLinearPath2D()),2,-1.,+1.);
    
    //Simetry in xz plane
    linJintegral[0] = 2. * linJintegral[0] * jPath2DElem->GetLinearPath2D()->DETdxdt() / gScaleFactor;
    linJintegral[1] = 0.;
    
    linInt.stop();
    
    //4debug Jlin_x
    {
#ifdef print_Jx_Linear
        std::map<REAL,REAL>::iterator ii;
        std::ofstream plotLinJx("PlotLinJx.txt");
        plotLinJx.precision(10);
        plotLinJx << "Jx = {";
        for(ii = functionLinJx.begin(); ii != functionLinJx.end(); ii++)
        {
            plotLinJx << "{" << ii->first << "," << ii->second << "},";
        }
        plotLinJx << "};\n";
        plotLinJx << "Jxplot = ListPlot[Jx,AxisOrigin->{0,0}]\n";
        plotLinJx.close();
        functionLinJx.clear();
#endif
    }
    
    TPZTimer arcInt("ArcIntegration"); arcInt.start();
    TPZVec<REAL> arcJintegral(2,0.);
    arcJintegral = intRule.Vintegrate(*(jPath2DElem->GetArcPath2D()),2,-1.,+1.);
    
    //Simetry in xz plane
    arcJintegral[0] = 2. * arcJintegral[0] * jPath2DElem->GetArcPath2D()->DETdxdt() / gScaleFactor;
    arcJintegral[1] = 0.;
    
    arcInt.stop();
    //
    
    TPZVec<REAL> answ(2);
    for(int i = 0; i < 2; i++)
    {
        answ[i] = linJintegral[i] + arcJintegral[i];
    }
    
//    std::cout << "DeltaT integracao linha = " << linInt.seconds() << " s" << std::endl;
//    std::cout << "DeltaT integracao arco = " << arcInt.seconds() << " s" << std::endl;
//    std::cout << "J = " << answ[0] << "\n";
    
    return answ;
}



