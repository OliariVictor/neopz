/*
 *  TPZJIntegral.h
 *
 *  Created by Cesar Lucci on 25/04/12.
 *  Copyright 2010 LabMeC. All rights reserved.
 */

#ifndef PZ_TPZJIntegral_h
#define PZ_TPZJIntegral_h

#include <iostream>
#include "TPZGeoElement.h"
#include "pzcmesh.h"
#include "pzelasmat.h"
#include "pzelast3d.h"
#include "pzcompel.h"
#include "pzinterpolationspace.h"
#include "TPZPlaneFracture.h"


/**
 * @brief Plane of the fracture.
 * @author Cesar Lucci (Caju)
 * @since 09/08/2010
 */


class LinearPath3D
{
public:
    
    LinearPath3D();//It is not to be used
    LinearPath3D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &FinalPoint, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure);
    LinearPath3D(LinearPath3D * cp);
    ~LinearPath3D();
    
    void X(REAL t, TPZVec<REAL> & xt);
    //void dXdt(REAL t, TPZVec<REAL> & dxdt);
    void normalVec(REAL t, TPZVec<REAL> & n);
    
    REAL DETdxdt();
    
    REAL Radius();
    
    TPZVec<REAL> operator()(REAL t)
    {
        TPZVec<REAL> Vval = Func(t);
        return Vval;
    }
    
    virtual TPZVec<REAL> Func(REAL t);
    
    virtual TPZVec<REAL> Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt);
    
    void SetPressure(REAL p)
    {
        fcrackPressure = p;
    }
    
protected:
    
    /** Initial point of Line */
    TPZVec<REAL> fInitialPoint;
    
    /** Origin of coupled arc */
    TPZVec<REAL> fFinalPoint;
    
    /** This direction defines the coupled arc plane.
     *  (this direction is orthogonal to coupled arc plane and defines
     *   the right hand convention for the coupled arc direction)
     */
    TPZVec<REAL> fNormalDirection;
    
    /** Radius of coupled arc */
    REAL fradius;
    
    /** Determinant of dXdt(3x1) */
    REAL fDETdxdt;
    
    /** CMesh that constains data */
    TPZAutoPointer<TPZCompMesh> fcmesh;
    
    /** pressure applied inside fracture */
    REAL fcrackPressure;
    
    /** map that holds t and respective elId and qsi for ComputeXInverse optimization */
    std::map< REAL , std::pair< int , TPZVec<REAL> > > f_t_elIdqsi;
};


class LinearPath2D : public LinearPath3D
{
public:
    
    LinearPath2D();//It is not to be used
    LinearPath2D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &FinalPoint, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure);
    LinearPath2D(LinearPath2D * cp);
    ~LinearPath2D();
    
    virtual TPZVec<REAL> Func(REAL t);
    
    virtual TPZVec<REAL> Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt);
};



class ArcPath3D
{
public:
    
    ArcPath3D();//It is not to be used
    ArcPath3D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius);
    ArcPath3D(ArcPath3D * cp);
    ~ArcPath3D();
    
    void X(REAL t, TPZVec<REAL> & xt);
    //void dXdt(REAL t, TPZVec<REAL> & dxdt);
    void normalVec(REAL t, TPZVec<REAL> & n);
    
    REAL DETdxdt();
    
    REAL Radius();
    
    TPZVec<REAL> operator()(REAL t)
    {
        TPZVec<REAL> Vval = Func(t);
        return Vval;
    }
    
    virtual TPZVec<REAL> Func(REAL t);
    
    virtual TPZVec<REAL> Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt);
    
    void SetRadius(REAL radius);
    
protected:
    
    /** Origin of arc */
    TPZVec<REAL> fOrigin;
    
    /** This direction defines the arc plane.
     *  (this direction is orthogonal to arc plane and defines
     *   the right hand convention for the arc direction)
     */
    TPZVec<REAL> fNormalDirection;
    
    /** Radius of arc */
    REAL fradius;
    
    /** Determinant of dXdt(3x1) */
    REAL fDETdxdt;
    
    /** CMesh that constains data */
    TPZAutoPointer<TPZCompMesh> fcmesh;
    
    /** map that holds t and respective elId and qsi for ComputeXInverse optimization */
    std::map< REAL , std::pair< int , TPZVec<REAL> > > f_t_elIdqsi;
};


class ArcPath2D : public ArcPath3D
{
public:
    
    ArcPath2D();//It is not to be used
    ArcPath2D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius);
    ArcPath2D(ArcPath2D * cp);
    ~ArcPath2D();
    
    virtual TPZVec<REAL> Func(REAL t);
    
    virtual TPZVec<REAL> Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt);
};



class AreaPath3D
{
public:
    
    AreaPath3D();//It is not to be used
    AreaPath3D(LinearPath3D * givenLinearPath3D);
    ~AreaPath3D();
    
    REAL DETdxdt();
    
    TPZVec<REAL> operator()(REAL t)
    {
        TPZVec<REAL> Vval = Func(t);
        return Vval;
    }
    
    virtual TPZVec<REAL> Func(REAL t);
    
protected:
    
    struct LinearPath3D_2 : public LinearPath3D
    {
        public:
        LinearPath3D_2();
        LinearPath3D_2(LinearPath3D * cp);
        ~LinearPath3D_2();
        
        virtual TPZVec<REAL> Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt);
        
        struct ArcPath3D_2 : public ArcPath3D
        {
            public:
            ArcPath3D_2();
            ArcPath3D_2(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius);
            ~ArcPath3D_2();
            
            virtual TPZVec<REAL> Function(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & nt);
            
            TPZVec<REAL> ComputeFiniteDifference(REAL t, TPZVec<REAL> xt, REAL halfDelta = 0.1);
            
            TPZVec<REAL> FunctionAux(REAL t, TPZVec<REAL> & xt, TPZVec<REAL> & direction);
        };
        
        ArcPath3D_2 * fArcPath3D;
    };
    
    LinearPath3D_2 * fLinearPath3D;
    
    /** Determinant of dXdt(3x1) */
    REAL fDETdxdt;
};



/**
 *  ITS ALWAYS GOOD TO REMEMBER:
 *          THE CLASS Path3D CONSIDERS THAT THE NORMAL DIRECTION IS IN X,Z PLANE (JUST LIKE FRACTURE PLANE) AND
 *          THE ORIENTATION OF ARC and LINEAR stretch is:
 *              ARC : RIGHT HAND DIRECTION WITH RESPECT TO GIVEN NORMAL AXE (axe that defines the (orthogonal) arc plane).
 *              LINEAR: FROM THE END OF ARC (supposed to be inside crack plane) TO ORIGIN.
 * SO, ITS ALWAYS GOOD DEFINE NORMAL AXE TANGENT TO THE CRACK BOUNDARY, FOLLOWING RIGHT HAND FROM OUTSIDE CRACK TO INSIDE CRACK
 */
class Path3D
{
public:

    Path3D();

    /**
     * Given unidimensional element reffers to the cracktip element that will be used
     * to compute J-integral around it.
     * Obs.: normal direction must be in xz plane and the arcs (internal and external) will be in (y>0).
     */
    Path3D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure);
    ~Path3D();
    
    LinearPath3D * GetLinearPath3D()
    {
        return fLinearPath3D;
    }
    
    ArcPath3D * GetArcPath3D()
    {
        return fArcPath3D;
    }
    
    AreaPath3D * GetAreaPath3D()
    {
        return fAreaPath3D;
    }
    
    void SetPressure(REAL p)
    {
        fLinearPath3D->SetPressure(p);
    }
    
    
protected:
    
    LinearPath3D * fLinearPath3D;
    ArcPath3D * fArcPath3D;
    AreaPath3D * fAreaPath3D;
};


class Path2D
{
public:
    
    Path2D();
    
    /**
     * Given unidimensional element reffers to the cracktip element that will be used
     * to compute J-integral around it.
     * Obs.: normal direction must be in xz plane and the arcs (internal and external) will be in (y>0).
     */
    Path2D(TPZAutoPointer<TPZCompMesh> cmesh, TPZVec<REAL> &Origin, TPZVec<REAL> &normalDirection, REAL radius, REAL pressure);
    ~Path2D();
    
    LinearPath2D * GetLinearPath2D()
    {
        return fLinearPath2D;
    }
    
    ArcPath2D * GetArcPath2D()
    {
        return fArcPath2D;
    }
    
    void SetPressure(REAL p)
    {
        fLinearPath2D->SetPressure(p);
    }
    
    
protected:
    
    LinearPath2D * fLinearPath2D;
    ArcPath2D * fArcPath2D;
};



class JIntegral3D
{
public:
    
    JIntegral3D();
    ~JIntegral3D();
    
    virtual void PushBackPath3D(Path3D *Path3DElem);
    
    virtual TPZVec<REAL> IntegratePath3D(int p);
    
private:
    
    TPZVec<Path3D*> fPath3DVec;
};


class JIntegral2D
{
public:
    
    JIntegral2D();
    ~JIntegral2D();
    
    virtual void PushBackPath2D(Path2D *Path2DElem);
    
    virtual TPZVec<REAL> IntegratePath2D(int p);
    
    Path2D * Path(int p)
    {
        return fPath2DVec[p];
    }
    
private:
    
    TPZVec<Path2D*> fPath2DVec;
};



#endif
