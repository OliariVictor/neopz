//Classes utilit�rias
#include "pzvec.h"
#include "pzstack.h"
#include "TPZTimer.h"

//Classes Geom�tricas
#include "pzgmesh.h"
#include "pzgeoel.h"

//Classes Computacionais
#include "pzcmesh.h"
#include "pzcompel.h"

//Materiais
#include "pzelasmat.h"
#include "pzmat2dlin.h"
#include "pzbndcond.h"

//Matrizes de armazenamento e estruturais
#include "pzfmatrix.h"
//#include "pzskylmat.h"
#include "pzysmp.h"
#include "pzskylstrmatrix.h"
#include "TPZParSkylineStructMatrix.h"
#include "TPZParFrontStructMatrix.h"
#include "TPZFrontSym.h"

//Solver
#include "pzstepsolver.h"

//An�lise
#include "pzanalysis.h"

//Reordenamento de equa��es grafos
#include "pzmetis.h"
#include "tpznodesetcompute.h"

//P�s-processamento
#include "pzvisualmatrix.h"

//Bibliotecas std, math etc
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "fast.h"
//#include "pzelct2d.h"
//template<class T>
//class TPZVec;
//#define NOTDEBUG

//M�todo para inser��o de materiais
void InicializarMaterial(TPZCompMesh &cmesh);

//M�todo para a leitura da malha
void LerMalha(char *arquivo,TPZGeoMesh &mesh);

//M�todo para a cria��o de um elemento
void UmElemento(TPZGeoMesh &malha);

//Inser��o de uma condi��o de contorno dada por uma fun��o
void forcingfunction(TPZVec<REAL> &ponto, TPZVec<REAL> &force);

int main() {
  //Arquivo de impress�o de log (n�o essencial)
  std::ofstream file("graphtest.txt");
   
  //malha geometrica
  TPZGeoMesh *malha = new TPZGeoMesh;
  malha->SetName("Malha geometrica Curso Floripa");
   
  //Cria e insere um elemento na malha geom�trica criada acima
  UmElemento(*malha);
  
  // Os elementos da malha original podem ser refinados
  TPZVec<int> csub(0);
  TPZManVector<TPZGeoEl *> pv(4);  //vetor auxiliar que receber� os subelementos 
  int n1=1,level=0;   
  cout << "\nDividir ate nivel ? ";
  int resp = 3;
  cin >> resp;      
  
  int nelc = malha->ElementVec().NElements(); //n�mero de elementos da malha
  int el;  //iterador

  TPZGeoEl *cpel;  //ponteiro auxiliar para um elemento geom�trico
  for(el=0;el<malha->ElementVec().NElements();el++) { //loop sobre todos os elementos da malha
    cpel = malha->ElementVec()[el];    // obt�m o ponteiro para o elemento da malha de 
                                       // posi��o igual ao do iterador atual
    if(cpel && cpel->Level() < resp){  // verifica se o ponteiro n�o � nulo e se
                                       // se o n�vel de refinamento � menor que o n�vel desejado
      cpel->Divide(pv);                // Executa a divis�o do elemento
    }
  }
  
  //solicita ao usu�rio uma ordem p de interpolacao padr�o para a malha
  int ord = 4;
  cout << "Entre ordem 1,2,3,4,5 : ";
  cin >> ord;
  
  //Define a ordem p de cria��o de todo elemento computacional como sendo ord
  TPZCompEl::gOrder = ord;
  
  // Constru��o da malha computacional. Aqui a malha ter� apenas uma refer�ncia
  // para a malha geom�trica...
  TPZCompMesh *malhacomp = new TPZCompMesh(malha);
  
  //Pode-se dar um nome a malha para efeito de impress�o de resultados...
  malhacomp->SetName("Malha Computacional :  Connects e Elementos");
  
  // Define-se o problema a resolver em cada parte do dom�nio / definem-se os materiais
  InicializarMaterial(*malhacomp);
  
  //Define-se o espa�o de aproxima��o
  malhacomp->AutoBuild();
  
  //Ajusta o espa�o levando em conta as condi��es de contorno
  malhacomp->AdjustBoundaryElements();
  
  //Inicializa-se a estrutura de dados
  malhacomp->InitializeBlock();
  
   
  //Verifica��o e melhoria da banda da matriz de rigidez (Opcional)
  TPZFMatrix before, after;
  malhacomp->ComputeFillIn(100,before);

  // Cria-se um objeto an�lise. O �nico par�metro � a malha computacional
  // com toda a sua estrutura de dados j� preparada. O PZ tentar� otimizar
  // a banda aqui.
  TPZAnalysis an(malhacomp);
  
  // c�lculo da banda ap�s o ajuste
  malhacomp->ComputeFillIn(100,after);
  
  //P�s processamento da matriz de rigidez antes e depois da otimiza��o da banda 
  VisualMatrix(before,"before.dx");
  VisualMatrix(after,"after.dx");
   
  //Escolha do padr�o de armazenamento. O par�metro de entrada � a malha computacional
  //TPZSkylineStructMatrix strmat(malhacomp);
  TPZParSkylineStructMatrix strmat(malhacomp);       //skyline em paralelo (multthread)
  //TPZParFrontStructMatrix<TPZFrontSym> strmat(malhacomp);
  
  //Define-se o padr�o de armazenamento para a an�lise
  an.SetStructuralMatrix(strmat);
  
  //Cria��o e defini��o do solver 
  TPZStepSolver step;
  step.SetDirect(ECholesky);   
  
  //Define-se o solver para a an�lise
  an.SetSolver(step);
  /*
  TPZMatrixSolver *precond = an.BuildPreconditioner(TPZAnalysis::EElement,false);
  step.SetCG(100,*precond,1.e-10,0);
  an.SetSolver(step);
  delete precond;
  */
  //Processamento
  cout << "Number of equations " << malhacomp->NEquations() << endl;
  an.Run();

  // Posprocessamento
  TPZVec<char *> scalnames(1);
  scalnames[0] = "state";    //nome das vari�veis que se quer p�s-processar
  TPZVec<char *> vecnames(0);
  char plotfile[] =  "termica.dx"; //nome do arquivo de resultados
  //Define-se para a an�lise as vari�veis a p�s-processar
  an.DefineGraphMesh(2, scalnames, vecnames, plotfile);
  //Executa os c�lculos para gera��o dos resultados de p�s-processamento
  an.PostProcess(2);
  //   an.DefineGraphMesh(2, scalnames, vecnames, pltfile);
  //   an.PostProcess(2);
  
  //limpar a mem�ria alocada dinamicamente.
  delete malhacomp;
  delete malha;
  return 0;
}

void UmElemento(TPZGeoMesh &malha) {
  //Para efeito de teste ser� criado um elemento quadrilateral de
  //comprimento 1, com as seguintes coordenadas (x,y,z)
  // - canto inferior esquerdo: (0,0,0);
  // - canto inferior direito : (0,1,0);
  // - canto superior direito : (1,1,0);
  // - canto superior esquerdo: (0,1,0);  
  double coordstore[4][3] = {{0.,0.,0.},{1.,0.,0.},{1.,1.,0.},{0.,1.,0.}};

  // criar os quatro n�s geom�tricos
  int i,j;                              //iteradores
  TPZVec<REAL> coord(3,0.);             //vetor auxiliar para armazenar uma coordenada
  
  for(i=0; i<4; i++) {                  //loop sobre o n�mero de n�s da malha
    
    // inicializar as coordenadas do no em um vetor do tipo pz
    for (j=0; j<3; j++) coord[j] = coordstore[i][j]; 
    // identificar um espa�o no vetor da malha geom�trica 
    // onde podemos armazenar o objeto n� a criar
    int nodeindex = malha.NodeVec ().AllocateNewElement ();

    // criar um n� geom�trico e inser�-lo na posi��o 
    // alocada no vetor de n�s da malha geom�trica
    malha.NodeVec ()[i].Initialize (i,coord,malha);
  
  }

  // Cria��o de um elemento geom�trico
  // inicializar os �ndices dos n�s do elemento
  TPZVec<int> indices(4);
  for(i=0; i<4; i++) indices[i] = i; //loop sobre o n�mero de n�s do elemento
  //no caso s� h� quatro n�s e eles foram criados na ordem correta para o
  //elemento em quest�o. A ordem dos n�s deve seguir um padr�o pr�-estabelecido
  
  // O pr�prio construtor vai inserir o elemento na malha
  // os par�metros de cria��o do elemento s�o os seguintes:
  // 1) Tipo geom�trico do elemento
  // - EPoint            element 0D - type point        -  associated index 0
  // - EOned             element 1D - type oned         -  associated index 1
  // - ETriangle         element 2D - type triangle     -  associated index 2
  // - EQuadrilateral    element 2D - type quad         -  associated index 3
  // - ETetraedro        element 3D - type tetraedro    -  associated index 4
  // - EPiramide         element 3D - type piramide     -  associated index 5
  // - EPrisma           element 3D - type prisma       -  associated index 6
  // - ECube             element 3D - type cube         -  associated index 7
  // 2) Vetor de conectividades dos elementos (o n�mero de n�s deve ser 
  //    compat�vel com o n�mero de n�s do tipo do elemento
  // 3) �ndice do material que ser� associado ao elemento
  // 4) Vari�vel onde ser� retornado o �ndice do elemento criado no vetor
  //    de elementos da malha geom�trica
  // TPZGeoEl *gel = new TPZGeoElQ2d(0,indices,1,malha);  //forma antiga.
  int index;
  TPZGeoEl *gel = malha.CreateGeoElement(EQuadrilateral,indices,1,index);

  //Gerar as estruturas de dados de conectividade e vizinhan�a
  malha.BuildConnectivity ();

  // Identificar onde ser�o inseridas condi��es de contorno.
  // Uma condi��o de contorno � aplicada a um lado (parte do contorno) 
  // de um elemento. Este objeto ir� inserir-se automaticamente na malha
  // Os par�metros s�o os seguintes:
  // 1) elemento onde ser� aplicada a condi��o de contorno
  // 2) lado do elemento onde ser� inserida a condi��o de contorno
  // 3) identificador da condi��o de contorno
  // 4) refer�ncia para a malha geom�trica.
  TPZGeoElBC(gel,4,-4,malha);
}


void forcingfunction(TPZVec<REAL> &p, TPZVec<REAL> &f) {
  REAL x0=0.5,y0=0.2,r=0.005,eps=0.0001;

  REAL dist2 = (p[0]-x0)*(p[0]-x0)+(p[1]-y0)*(p[1]-y0);
  REAL A = exp(-dist2/eps);
  REAL Bx2 = 4.*(p[0]-x0)*(p[0]-x0)/(eps*eps);
  REAL By2 = 4.*(p[1]-y0)*(p[1]-y0)/(eps*eps);
  REAL Bx = 2.*(p[0]-x0);
  REAL By = 2.*(p[1]-y0);
  if(dist2 < r*r) {
    f[0] = A*Bx2-2.*A/eps+A*By2-2.*A/eps;
  } else f[0] = 0.;
}

// Exemplo de leitura de malha a partir de um arquivo
// os par�metros utilizados s�o o nome do arquivo e uma
// refer�ncia para a malha geom�trica que receber� os dados
void LerMalha(char *nome, TPZGeoMesh &grid) {
  //Cria��o de um objeto arquivo de entrada de dados;
  ifstream infile(nome);

	int linestoskip;
	char buf[256];
	infile >> linestoskip;
	int i,j;
	for(i=0; i<linestoskip;i++) infile.getline(buf,255);
	infile.getline (buf,255);
	infile.getline (buf,255);
	int ntri,npoin,nbouf,nquad,nsidif;
	infile >> ntri >> npoin >> nbouf >> nquad >> nsidif;
	infile.getline (buf,255);
	infile.getline(buf,255);

	grid.NodeVec ().Resize(npoin+1);
	TPZVec<int> nodeindices(4);
	int mat, elid;
	for(i=0;i<nquad;i++) {
		infile >> elid;
		for(j=0; j<4;j++) infile >> nodeindices[j];
		infile >> mat;
                int index;
                grid.CreateGeoElement(EQuadrilateral,nodeindices,mat,index);
//		new TPZGeoElQ2d(elid,nodeindices,mat,grid);
	}
	infile.getline(buf,255);
	infile.getline(buf,255);

	int nodeid,dum;
	char c;
	TPZVec<REAL> coord(3,0.);
	for(i=0; i<npoin; i++) {
		infile >> nodeid >> coord[0] >> coord[1] >> c >> dum;
		grid.NodeVec ()[nodeid].Initialize (nodeid,coord,grid);
	}
	infile.getline (buf,255);
	infile.getline (buf,255);

	TPZVec<int> sideid(2,0);
	for(i=0; i<nbouf; i++) {
		infile >> sideid[0] >> sideid[1] >> elid >> dum >> mat;
		TPZGeoEl *el = grid.ElementVec ()[elid-1];
		int side = el->WhichSide (sideid);
		TPZGeoElBC(el,side,-mat,grid);
	}
	grid.BuildConnectivity();

	return;
}

void InicializarMaterial(TPZCompMesh &cmesh) {
  //O par�metro b�sico para a cria��o de um material � o 
  // identificador. Esse deve corresponder aos �ndices utilizados
  // na cria��o dos elementos geom�tricos.
  // No restante, o material define a equa��o diferencial a ser
  // resolvida...
  TPZMat2dLin *meumat = new TPZMat2dLin(1);
  
  //Cada material tem par�metros de inicializa��o pr�prios, assim
  //deve-se consultar a documenta��o para verificar como definir os
  //par�metros. No caso em quest�o o material requer tr�s matrizes
  //e uma fun��o de c�lculo tamb�m � fornecida
  TPZFMatrix xk(1,1,1.),xc(1,2,0.),xf(1,1,1.);
  meumat->SetMaterial (xk,xc,xf);
  meumat->SetForcingFunction(forcingfunction);
  
  //Ap�s a cria��o do material este dever ser inserido na estrutura
  //de dados da malha computacional
  cmesh.InsertMaterialObject(meumat);

  // inserir as condi��es de contorno
  // Uma condi��o de contorno pode ser dada por duas matrizes
  // relacionadas a rigidez e ao vetor de carga. As dimens�es dessas
  // matrizes dependem da dimens�o e do n�mero de vari�veis de estado
  // do problema. O tipo de condi��o de contorno � definido por um 
  // identificador, onde:
  //  - 0 = Dirichlet
  //  - 1 = Neumann
  //  - 2 = Mista
  TPZFMatrix val1(1,1,0.),val2(1,1,0.);
  
  //Os par�metros necess�rios � cria��o de uma condi��o de contorno s�o:
  // 1) Identificador da condi��o de contorno (lembre-se que uma BC � como um material)
  // 2) O Tipo da BC : Dirichlet, Neumann ou Mista 
  // 3) Os valores da BC
  TPZMaterial *bnd = meumat->CreateBC (-4,0,val1,val2);
  
  //Da mesma forma que para os materiais, ap�s sua cria��o � necess�rio
  //a sua inser��o na estrutura de dados da malha computacional 
  cmesh.InsertMaterialObject(bnd);
  
  //cria��o e inser��o de outras BC's
  bnd = meumat->CreateBC (-3,0,val1,val2);
  cmesh.InsertMaterialObject(bnd);
  bnd = meumat->CreateBC (-2,0,val1,val2);
  cmesh.InsertMaterialObject(bnd);
  bnd = meumat->CreateBC (-1,0,val1,val2);
  cmesh.InsertMaterialObject(bnd);
}

