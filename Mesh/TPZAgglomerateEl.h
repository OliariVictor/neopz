//$Id: TPZAgglomerateEl.h,v 1.7 2003-11-04 18:42:48 cedric Exp $

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

public:

  /** Constructor: caso o elemento � pass�vel de ser agrupado retorna-se 
   * um novo index caso contr�rio index = -1
   */
  TPZAgglomerateElement(int nummat,int &index,TPZCompMesh &aggcmesh,TPZCompMesh *finemesh);

  /** inicializa os dados caracteristicos do elemento aglomerado */
  void InitializeElement();

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
   * Calcula o res�duo do elemento aglomerado
   */
  void CalcResidual(TPZElementMatrix &ef);

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
  int Dimension() const {return (gInterfaceDimension + 1);}

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

  /**
   * efetua agrupamentos de indexes de elementos computacionais sobre a malha fina 
   * numaggl retorna o n�mero de elementos obtidos por agrupamento 
   * accumlist � a lista contendo os indexes dos elementos que dever�o ser agrupados 
   * o index indica a posic�o dentro da lista de elementos computacionais
   * accumlist[5] = 7 indica que o elemento computacional de index = 5 
   * ser� agrupado para formar o elemento de index 7 
   * finemesh � a malha fina cujos elementos ser�o agrupados 
   * level indica o n�mero de n�veis que o agrupamento conter� 
   * por exemplo, caso level = 2 elementos geom�tricos ser�o agrupados no elemento av�
   * dim � a dimens�o da malha de elementos de volume
   */
  static void ListOfGroupings(TPZCompMesh *finemesh,TPZVec<int> &accumlist,int level,int &numaggl,int dim);
};
#endif
