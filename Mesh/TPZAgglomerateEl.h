//$Id: TPZAgglomerateEl.h,v 1.16 2003-12-02 14:08:43 tiago Exp $
#ifndef AGGLOMERATEELEMHPP
#define AGGLOMERATEELEMHPP

#include "pzreal.h"
#include "pzstack.h"
#include "pzeltype.h"
#include "TPZCompElDisc.h"
#include <iostream>

class TPZGeoEl;
class TPZCompEl;
class TPZCompMesh;
class TPZGeoElSide;
class TPZInterfaceElement;
struct TPZElementMatrix;
using namespace std;

/**
 * TPZAgglomerateElement clase that it manages the generation of elements 
 * from the agglomeration of some geometric elements
 */

class TPZAgglomerateElement : public TPZCompElDisc { 

private:

  /**
   * indexes na malha fina dos sub-elementos computacionais aglomerados pelo atual
   */
  TPZStack<int> fIndexes;

  /**
   * malha � qual os elementos aglomerados fazem parte e 
   * do qual o atual foi obtido
   */
  TPZCompMesh *fMotherMesh;

  REAL fInnerRadius;

public:

  /** Constructor: caso o elemento � pass�vel de ser agrupado retorna-se 
   * um novo index caso contr�rio index = -1
   */
  TPZAgglomerateElement(int nummat,int &index,TPZCompMesh &aggcmesh,TPZCompMesh *finemesh);

  /** inicializa os dados caracteristicos do elemento aglomerado */
  void InitializeElement();

  /**
   * Set the inner radius value.
   */
  void SetInnerRadius(REAL InnerRadius) { fInnerRadius = InnerRadius;}

  /**
   * Returns the inner radius value.
   */
  REAL InnerRadius() {return fInnerRadius;}

  /** adiciona index do sub-elemento*/
  static void AddSubElementIndex(TPZCompMesh *aggcmesh,int subel,int destind);

  /**Destructor do objeto*/
  ~TPZAgglomerateElement(){};

  /**
   * Type of the element 
   */  
  MElementType Type(){return EAgglomerate;}

  /** retorna malha mae */
  TPZCompMesh *MotherMesh(){return fMotherMesh;}

  /** acumula a lista de regras de integra��o no elemento deformado*/
  virtual void AccumulateIntegrationRule(int degree, TPZStack<REAL> &point, TPZStack<REAL> &weight);

  /**calcula o ponto centro de massa dos elementos aglomerados */
  void CenterPoint();

  /** retorna o volume do elemento geom�trico referenciado */
  REAL VolumeOfEl();

  /**
   * Calcula a restri��o da solu��o ou res�duo dos elementos aglomerados para o
   * obtido por aglomera��o - este �ltimo chamado de elemento pai
   */
  void CalcResidual(TPZFMatrix &Rhs,TPZCompElDisc *el);

  /**
   * Monta a equa��o diferencial do modelo sobre o elemento definido por 
   * aglomera��o de elementos da malha fina
   */
  void CalcStiff(TPZElementMatrix &ek, TPZElementMatrix &ef);

  /** retorna o n�mero de sub-elementos aglomerados */
  int NIndexes(){return fIndexes.NElements();}

  /**
   * retorna o elemento computacional da malha fina de �ndice index
   */
  TPZCompEl *FineElement(int index);

  /**return the geometric element to which this element references*/
  TPZGeoEl *CalculateReference();

  /**os geom�tricos agrupados apontam para o computacional*/
  void SetReference();
  void SetReference2(int sub);

   /** 
    * retorna o sub-elemento n�mero sub 
    */
   TPZCompEl *SubElement(int sub);

   REAL NormalizeConst();

  /**
   * it creates new conect that it associates the degrees of freedom of the
   * element and returns its index 
   */
  virtual int CreateMidSideConnect();

  /**
   * it returns dimension from the elements
   */
  int Dimension() const;

  /**
   * it prints the features of the element 
   */
  virtual void Print(ostream & out = cout);

  /**
   * create copy of the materials tree
   */
  void CreateMaterialCopy(TPZCompMesh &aggcmesh);

  void ListOfDiscEl(TPZStack<TPZCompEl *> &elvec);

  void IndexesDiscSubEls(TPZStack<int> &elvec);

  int NSides();

  void CreateGraphicalElement(TPZGraphMesh &grmesh, int dimension);

  void Solution(TPZVec<REAL> &qsi,int var,TPZManVector<REAL> &sol);

  TPZGeoEl *FatherN(TPZGeoEl *sub,int n);

  int NSubsOfLevels(TPZGeoEl *father,int nlevels);

  int NSubCompEl(TPZGeoEl *father);

  static void ListOfGroupings(TPZCompMesh *finemesh,TPZVec<int> &accumlist,int nivel,int &numaggl,int dim);

  void FineSolution(TPZVec<REAL> &x,TPZCompElDisc *disc,TPZVec<REAL> &uh);
  void FineSolution(TPZVec<REAL> &x,TPZVec<REAL> &uh);

  void Print(TPZStack<int> &listindex);

  void ProjectSolution(TPZFMatrix &projectsol);
  void ProjectSolution2(TPZFMatrix &projectsol);
};
#endif
