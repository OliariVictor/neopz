#include "pzintel.h"
#include "TPZPlacaOrthotropic.h"
#include "pzgnode.h"
#include "pzgeoel.h"

TPZPlacaOrthotropic::TPZPlacaOrthotropic(TPZInterpolatedElement *cel,REAL hight ){ 

  fIntel = cel;
  fH = hight;
  if ( hight == 0.){
    TPZGeoEl *geo = cel->Reference();
    TPZGeoNode *node0 = geo->NodePtr(0);
    TPZGeoNode *node4 = geo->NodePtr(4);
    REAL dist = (node0->Coord(0) -  node4->Coord(0))*(node0->Coord(0) -  node4->Coord(0));
    dist += (node0->Coord(1) -  node4->Coord(1))*(node0->Coord(1) -  node4->Coord(1));
    dist +=(node0->Coord(2) -  node4->Coord(2))*(node0->Coord(2) -  node4->Coord(2));
    dist = sqrt(dist);
    fH = dist;
  }
}

REAL TPZPlacaOrthotropic::Moment(TPZVec<REAL> n1, TPZVec<REAL> n2){
  return 0.;

}

REAL TPZPlacaOrthotropic::Force(TPZVec<REAL> n1, TPZVec<REAL> n2){
  return 0.;

}


// TPZPlacaOrthotropic::CriaNos(int num, TPZGeomesh &geomesh, double list[20][3]){
//   geomesh.NodeVec().Resize(num);
//   TPZVec<REAL> coord(3);
//   int i;
//   for (i=0; i<num; i++){
//     coord[0] = list[i][0];
//     coord[1] = list[i][1];
//     coord[2] = list[i][2];
//     geomesh.NodeVec()[i].Initialize(coord, geomesh);
//   }
// }

//  orto = new TPZMatOrthotropic(1, naxes, 69.e06, 69.e06, 69.e06, 0.33, 0.33, 0.33, 5.15378e06, 5.15378e06, 5.15378e06) ;
//  TPZMaterial *orto2 = new TPZMatOrthotropic(1, naxes, 69.e06, 69.e06, 69.e06, 0.33, 0.33, 0.33, 5.15378e06, 5.15378e06, 5.15378e06) ;

void TPZPlacaOrthotropic::Print(){

  fIntel->Print();
  cout << "Plate high : " ;
  cout << this->FH();//this � o objeto placa

}

//se fosse TPZMulticamadaOrthotropic::Print, o this seria multcam
//se a func�o fosse declarada como void Print(){...}, n�o haveria This.
//caso chamemos a func�o Print com o objeto placa2, ao inv�s de placa->Print, o This � o objeto placa2.
  
  
