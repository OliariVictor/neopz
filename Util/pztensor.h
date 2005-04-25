#ifndef TPZTENSOR_H
#define TPZTENSOR_H
#include "TPZMetodos.h"
//#include "/home/pos/rgdamas/pzrepository/matrix/pzmatrix.h"


class TPZTensor: public TPZMetodos{
 public:
  
  /**Construtor da classe*/
  TPZTensor();
  /**Devolve o tensor*/
  void GetTensor(double tensor[3][3]);
  /**Armazena o tensor em fTensor.*/
  void SetTensor(double tensor[3][3]);
  /**Fornece o vetor de tens�o principal - r[3]. Sendo r[0]>>r[1]>>r[2].*/
  void GetStressP(double r[3]);
  /**Calcula a tens�o normal */
  void GetStressNormal();
  /**Fornece as invariantes do tensor - inv[3].*/
  void GetInvariants(double inv[3]);
  /**Fornece a matriz das dire��es principais - dirP[3][3].*/
  void GetDirP(double dirP[3][3]);
  /**Fornece a n-�sima dire��o principal - dirPNum[3].*/
  void GetDirPNum(double dirPNum[3], int n);
  /**Dada uma dire��o dirN[3], calcula seu respectivo vetor de tens�o stressN[3].*/
  void GetStressN(double dirN[3], double stressN[3]);
  /**Calcula a tens�o de VonMises*/
  void GetVonMises(double &vonM);
  /**Fornece o tensor de tens�o deviat�ria - devia[3][3].*/
  void GetDeviatorio(double devia[3][3]);
  /**Calcula o valor da tens�o Tresca.*/
  void GetTresca(double &tresca);
  /**Calcula a press�o hidrost�tica*/
  void GetHidroPressure(double &pressure);
  /**Calcula a tens�o de Mor Coulomb*/
  void GetMorC(double & morC, double ang);
  /**Fornece o tensor com ap�s aplicada uma rota��o(matrixRot[3][3]) - tensorRot[3][3].*/
  void Rotate(double matrixRot[3][3], double tensorRot[3][3]);
  
 private:
  
  /**Armazena as dire��es principais em fDirP.*/
  void SetDirP();
  /**Armazena as tens�es principais em fStressP.*/
  void SetStressP();
  /**Armazena as invariantes em fInvariants.*/
  void SetInvariants();
  /**Armazena a dire��o principal "n"  na vari�vel da classe fDirP.*/
  void SetDirP(int &n);

private:
  /**Invariantes do tensor.*/
  double fInvariants[3];
  /**Tens�es principais.*/
  double fStressP[3];
  /**Tensor.*/
  double fTensor[3][3];
  /**Dire��es principais.*/
  double fDirP[3][3];
  double fTolerance;
};
#endif
