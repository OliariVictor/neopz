#ifndef MULTICAMADAORTH
#define MULTICAMADAORTH

#include <iostream>
using namespace std;
#include "pzvec.h"
#include "pzstack.h"

class TPZGeoMesh;
class TPZCompMesh;
class TPZPlacaOrthotropic;


/**
 * Gerencia um conjunto de placas
 * dispostas em forma multicamada
 */

class TPZMulticamadaOrthotropic {

  /**malha geom�trica onde s�o inseridas as placas geom�tricas*/
  TPZGeoMesh             *fGeoMesh;
  /**malha computacional: elementos computacionais correspondentes*/
   TPZCompMesh            *fCompMesh;
  /**Vetor de placas*/
  //TPZVec<TPZPlacaOrthotropic *>  fPlacaOrth;
   TPZStack<TPZPlacaOrthotropic *> fPlacaOrth;
  /**
   * fZ: altura m�xima (camada mais longe do plano XY)
   * fDx,fDy: dimens�es das placas (constantes para todas as placas)   
   */
  REAL fDx,fDy;
  REAL fZ;
  double fQuantPlacas;

 public:
  /**construtor*/
  TPZMulticamadaOrthotropic(REAL z,REAL dx,REAL dy, TPZGeoMesh *gmesh, TPZCompMesh *cmesh);
  /*destrutor*/
  ~TPZMulticamadaOrthotropic(){}

  /**Adiciona placas ao conjunto*/
  void AddPlacaOrtho(TPZPlacaOrthotropic *placa);
  /**gera a malha computacional do conjunto de placas*/
  void GenerateMesh();
  /**cria o conjunto de placas multicamada*/
  //  static int main();
  void Print(ostream &out = cout);
  /*criando m�todo para retornar a altura da multicamada*/
  REAL ZHight(TPZPlacaOrthotropic *placa);
  /*criando m�todo para retornar fPlacaOrth*/
  TPZVec<TPZPlacaOrthotropic *> &RPlacaOrtho(){return fPlacaOrth;}
  /*criando m�todo para contar quant de placas*/
  int RQPlacas();

};
#endif
