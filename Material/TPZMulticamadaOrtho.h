#ifndef MULTICAMADAORTH
#define MULTICAMADAORTH

#include <iostream>
using namespace std;
#include "pzvec.h"


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
  TPZVec<TPZPlacaOrthotropic *>  fPlacaOrth;
  /**
   * fZ: altura m�xima (camada mais longe do plano XY)
   * fDx,fDy: dimens�es das placas (constantes para todas as placas)   
   */
  REAL fZ,fDx,fDy;

 public:
  /**construtor*/
  TPZMulticamadaOrthotropic(REAL z,REAL dx,REAL dy);
  /**Adiciona placas ao conjunto*/
  void AddPlacaOrtho();
  /**gera a malha computacional do conjunto de placas*/
  void GenerateMesh();
  /**cria o conjunto de placas multicamada*/
  static int main();
};
#endif
