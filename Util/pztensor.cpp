#include <math.h>
#include "TPZTensor.h"
#include "TPZPolinomio.h"
#include <iostream>


/** Inicializador da classe* */
TPZTensor::TPZTensor() : TPZMetodos() {
	int i, j;
	for (i = 0; i < 3; i++) {
		fStressP[i] = -99.99999;
		fInvariants[i] = -99.99999;
		for (j = 0; j < 3; j++) {
			fTensor[i] [j] = -99.99999;
			fDirP[i] [j] = -99.99999;
		}
	}
}

/** Armazena a dire��o principal "n"  na vari�vel da classe fDirP.* */
void TPZTensor::SetDirP(int &n) {
	vector<double> vetor(3);
	int resp[3], i, j, test;
    double sist[3][3];

	/**
	 *Verificando quais equa��es s�o linearmente independentes. sist[i][0] verifica linhas 0 e 1,sist[i][1] linhas 1 e 2, e
	 * sist[i][2] 0 e 2.*
	 */
    for(i=0; i<3; i++){
        for(j=0; j<3; j++){
        sist[i][j] =0.0;
        }
    }
    test = 0;

	for (i = 0; i < 3; i++) {

		sist[0][i] = (fTensor[i][i] - fStressP[n])*(fTensor[(i+1)%3][(i+1)%3] - fStressP[n])
            - fTensor[i][(i+ 1)%3]*fTensor[(i+1)%3][i];
        sist[1][i] = fTensor[i][(i+1)%3]*fTensor[(i+1)%3][(i+2)%3]
            - fTensor[i][(i+2)%3]*(fTensor[(i+1)%3][(i+1)%3]-fStressP[n]);
        sist[2][i] = (fTensor[i][i] - fStressP[n])*fTensor[(i+1)%3][(i+2)%3]
            - fTensor[i][(i+2)%3]*fTensor[(i+1)%3][i];
/*
        cout << "Qual� mano?\n";
        cout << fabs(sist[0][i]) <<  "\n"<< fabs(sist[1][i]) <<  "\n" << fTolerance << endl;
*/
		if (fabs(sist[0][i]) < fTolerance && fabs(sist[1][i])<fTolerance && fabs(sist[1][i])<fTolerance)
            test ++;
	}

    //cout << "test = " << test << endl;
    int pos[2];
    vector<double> aux(3);
    int aux2[3];
    for( i = 0; i < 3; i++ ){
        for(j=0; j<3 ; j++){
            vetor[j]=sist[i][j];
        }
        /** Pegando o maior valor dos determinantes das submatrizes, de cada conjunto de 2 duas */
        SortArray3(vetor, resp);
        /** aux[i] = 0 corresponde a sist[0][i], e assim por diante. */
        aux[i] = vetor[resp[0]];
        /** aux2[i] = 0 corresponde a sist[i][0], e assim por diante. */
        aux2[i] = resp[0];
    }
    SortArray3(aux, resp);
    //Posi��o horizontal
    pos[0]=resp[0];
    //Posi��o vertical
    pos[1]=aux2[resp[0]];
    if (test<3){
/*
        cout << "pos[0] = " << pos[0] << ", pos[1] = " << pos[1] << endl;
*/

        sist[0][(pos[0]+pos[1])%3]= fTensor[(pos[0]+pos[1])%3][(pos[0]+pos[1])%3] - fStressP[n];
	    sist[0][(pos[0]+pos[1]+1)%3] = fTensor[(pos[0]+pos[1])%3] [(pos[0]+pos[1]+1)%3];
        sist[0][(pos[0]+pos[1]+2)%3] = fTensor[(pos[0]+pos[1])%3] [(pos[0]+pos[1]+2)%3];

        sist[1][(pos[0]+pos[1])%3] = sist[0][(pos[0]+pos[1]+1)%3];
	    sist[1][(pos[0]+pos[1]+1)%3] = fTensor[(pos[0]+pos[1]+1)%3][(pos[0]+pos[1]+1)%3] - fStressP[n];
	    sist[1][(pos[0]+pos[1]+2)%3] = fTensor[(pos[0]+pos[1]+1)%3][(pos[0]+pos[1]+2)%3];
/*
        for(i=0; i<2; i++){
            for(j=0; j<3; j++){
                cout << sist[i][j]<<"\t";
            }
            cout << "\n";
        }
*/


        /** M�todo Direto de Cramer vetor[(pos[0]+pos[1]+2)%3] = 1.0;
        *   vetor[((pos[0]+pos[1]+1)%3] e vetor[(pos[0]+pos[1])%3] em fun��o de vetor[(pos[0]+pos[1]+2)%3].
        *   sist[0][(pos[0]+pos[1])%3]*vetor[(pos[0]+pos[1])%3]
        *   + sist[0][(pos[0]+pos[1]+1)%3]*vetor[(pos[0]+pos[1]+1)%3] == - sist[0][(pos+2)%3]*1.0 ;
        *   sist[1][(pos[0]+pos[1])%3]*vetor[(pos[0]+pos[1])%3]
        *   + sist[1][(pos[0]+pos[1]+1)%3]*vetor[(pos[0]+pos[1]+1)%3] == - sist[1][(pos+2)%3]*1.0;
        **/
        double det;
        det= (sist[0][(pos[0]+pos[1])%3] * sist[1][(pos[0]+pos[1]+1)%3]
                - sist[0][(pos[0]+pos[1]+1)%3]*sist[1][(pos[0]+pos[1])%3]);
        aux[0] = det;
/*
        cout << "det = " << (pos[0]+pos[1])%3 << "\t" << (pos[0]+pos[1]+1)%3 << endl;
        cout << "aux[0] = " << aux[0] << endl<< endl;
*/
        vetor[(pos[0]+pos[1])%3] = (-sist[0][(pos[0]+pos[1]+2)%3] * sist[1][(pos[0]+pos[1]+1)%3]
            + sist[1][(pos[0]+pos[1]+2)%3]*sist[0][(pos[0]+pos[1]+1)%3])/aux[0];

        vetor[(pos[0]+pos[1]+1)%3] = (-sist[0][(pos[0]+pos[1])%3] * sist[1][(pos[0]+pos[1]+2)%3]
            + sist[1][(pos[0]+pos[1])%3]*sist[0][(pos[0]+pos[1]+2)%3])
            / aux[0];

        vetor[(pos[0]+pos[1]+2)%3] = 1.0;

        NormalizeVetor3(vetor);
        for(i=0; i<3; i++){
            fDirP[n][i] = vetor[i];
        }
        //cout << "Caminho A" << endl;
    }
    else{
        /*  Neste caso teremos apenas uma linha do sistema */
        /*aux[0] guardar� o valor m�ximo e pos[0] e pos[1] a posi��o no tensor*/
//        cout << "N�vel zero" << endl;
        aux[0] = 0.;
        resp[2] = 0;
        for(i=0; i<3; i++)
            for(j=0; j<3; j++){
                if (fabs(fTensor[i][j])>aux[0]) {
                    aux[0]=fTensor[i][j];
                    pos[0] = i;
                    pos[1] = j;
                }
        }
        cout << "M�ximo = " << aux[0] << endl;
        if (fabs(aux[0])<=fTolerance){
            for( pos[1] = 0; pos[1] < 3; pos[1]++ ){
                vetor[(pos[1]+2)%3] = 0.0;
                vetor[(pos[1]+1)%3] = 0.0;
                vetor[pos[1]] =0.0;
                for(i=0; i<3; i++) fDirP[n][i] = vetor[i];
            }
        }
        else{
        /* C�lculando a dire�ao n*/
        vetor[(pos[1]+2)%3] = 0.0;
        vetor[(pos[1]+1)%3] = 1;
        vetor[pos[1]] = -fTensor[pos[0]][(pos[1]+1)%3]/fTensor[pos[0]][pos[1]];
        NormalizeVetor3(vetor);
        for(i=0; i<3; i++){
            fDirP[n][i] = vetor[i];
        }

        /* C�lculando a dire�ao n+1*/
        n= n+1;
        vetor[(pos[1]+2)%3] = 1.0;
        vetor[(pos[1]+1)%3] = 0.0;
        vetor[pos[1]] = -fTensor[pos[0]][(pos[1]+2)%3]/fTensor[pos[0]][pos[1]];

        /* Ortogonalizando as duas dire��es */
        aux[0] =  0.0;
        /* V1.V0 */
        for(i=0; i<3; i++) aux[0] = aux[0] + fDirP[n-1][i]*(vetor[i]);
        /* V1& = V1 - (V1.V0)V0 */
        for(i=0; i<3; i++) vetor[i] = vetor[i] -aux[0]*(fDirP[n-1][i]);
        NormalizeVetor3(vetor);
        for(i=0; i<3; i++){
           fDirP[n][i] = vetor[i];
        }
//        cout << "Caminho B" << endl;
        }
    }
}

/** Armazena as dire��es principais em fDirP.* */
void TPZTensor::SetDirP() {
	int i;
	for (i = 0; i < 3; i++) SetDirP(i);
}

/** Calcula a tens�o normal * */
void TPZTensor::GetStressNormal() {
}

/** Armazena as tens�es principais em fStressP.* */
void TPZTensor::SetStressP() {
    TPZPolinomio stressP;
    vector<double> coef(4), aux(3);
    double imagem;
    int resp,i;
    coef[3] = 1.0;
    coef[2] = -fInvariants[0];
    coef[1] = fInvariants[1];
    coef[0] = -fInvariants[2];
    resp = stressP.Tartaglia(coef, aux, imagem);
    copy(aux.begin(), aux.end(), fStressP);
    if(resp==-1){
        fStressP[1] = 0.0;
        fStressP[2] = 0.0;
        i = 0;
        SetDirP(i);
        for(i=0;i<3; i++){
            fDirP[1][i]= 0.;
            fDirP[2][i]= 0.;
        }
    }
    else{
        SetDirP();
    }
}

/** Armazena as invariantes em fInvariants.* */
void TPZTensor::SetInvariants() {
	int i;
/*	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			cout << "fTensor[" << i << "][" << j << "] = " << fTensor[i] [j] << "\n";
		}
	}*/
	for (i = 0; i < 3; i++) fInvariants[i] = 0.0;
	for (i = 0; i < 3; i++) {
		int ip1 = (i + 1) % 3;
		int ip2 = (i + 2) % 3;
		fInvariants[0] += fTensor[i] [i];
		fInvariants[1] += fTensor[i] [i] * fTensor[ip1] [ip1] - 1.0 * fTensor[i] [ip1] * fTensor[ip1] [i];
		fInvariants[2] += fTensor[0] [i] * fTensor[1] [ip1] * fTensor[2] [ip2] - fTensor[i] [i] * fTensor[ip1] [ip2] *
		   fTensor[ip2] [ip1];
	}
//	for (i = 0; i < 3; i++) cout << "fInvariants[" << i << "]= " << fInvariants[i] << "\n";
}

/** Devolve o tensor* */
void TPZTensor::GetTensor(double tensor[3] [3]) {
	int i, j;
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++) tensor[i] [j] = fTensor[i] [j];
}

/** Armazena o tensor em fTensor.* */
void TPZTensor::SetTensor(double tensor[3] [3]) {
	int i, j;
	double max = 0.;
    /*Armazenando tensor e verificando o m�ximo valor  */
	for (i = 0; i < 3; i++){
		for (j = 0; j < 3; j++) {
			fTensor[i][j] = tensor[i][j];
			if (fabs(fTensor[i][j])> max){
                max = fabs(fTensor[i][j]);
		    }
        }
    }
    /* Especificando a toler�ncia dos c�lculos em fun��o do valor m�ximo */
	if (max>1||max==0)fTolerance =1e-9;
    else fTolerance = fabs(max * 1e-9);

    //cout << "fTolerance = " << fTolerance << endl;
	/*cout<<"TPZTensor::SetTensor - SetInvariants\n";*/
    SetInvariants();
	SetStressP();
}

/** Calcula a press�o hidrost�tica* */
void TPZTensor::GetHidroPressure(double & pressure) {
	pressure = (1.0 / 3.0) * fInvariants[0];
}

/** Calcula a tens�o de Mor Coulomb* */
void TPZTensor::GetMorC(double & morC, double ang) {
	double fi = (3.141516 / 360.0) * ang;
	morC = (fStressP[0] - fStressP[2]) * 0.5 * cos(fi) +
	   ((fStressP[0] + fStressP[2]) * 0.5 + (fStressP[0] - fStressP[2]) * 0.5 * sin(fi)) * tan(fi);
}

/** Calcula o valor da tens�o Tresca.* */
void TPZTensor::GetTresca(double & tresca) {
	tresca = (fStressP[0] - fStressP[2]) * 0.5;
}

/** Fornece o tensor com ap�s aplicada uma rota��o(matrixRot[3][3]) - tensorRot[3][3].* */
void TPZTensor::Rotate(double matrixRot[3] [3], double tensorRot[3] [3]) {
	int i, j;
	double aux;
	for (i = 0; i < 3; i++) {
		aux = 0.0;
		for (j = 0; j < 3; j++) aux = aux + (fTensor[i] [j]) * matrixRot[j] [i];
		tensorRot[i] [j] = aux;
	}
}

/** Fornece o tensor de tens�o deviat�ria - devP[3][3].* */
void TPZTensor::GetDeviatorio(double devia[3] [3]) {
	int i, j;
	i = 0;
	j = 0;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			if (i != j) devia[i] [j] = fTensor[i] [j];
			else devia[i] [j] = (fTensor[i] [j]) - (1.0 / 3.0) * fInvariants[0];
		}
	}
}

/** Calcula a tens�o de VonMises* */
void TPZTensor::GetVonMises(double & vonM) {
	vonM = (1.00 / 3.00) * (fInvariants[0] * fInvariants[0] - 3.0 * fInvariants[1]);
}

/** Dada uma dire��o dirN[3], calcula seu respectivo vetor de tens�o stressN[3].* */
void TPZTensor::GetStressN(double dirN[3], double stressN[3]) {
	int i, j;
	for (i = 0; i < 3; i++) {
		double aux = 0.0;
		for (j = 0; j < 3; j++) aux = aux + (fTensor[i] [j]) * dirN[i];
		stressN[i] = aux;
	}
}

/** Fornece a matriz das dire��es principais - dirP[3][3].* */
void TPZTensor::GetDirP(double dirP[3] [3]) {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++) dirP[i] [j] = fDirP[i] [j];
}

/** Fornece as invariantes do tensor - inv[3].* */
void TPZTensor::GetInvariants(double inv[3]) {
	for (int i = 0; i < 3; i++) inv[i] = fInvariants[i];
}

/** Fornece o vetor de tens�o principal - r[3].Sendo r[0] >> r[1] >> r[2].* */
void TPZTensor::GetStressP(double r[3]) {
	for (int i = 0; i < 3; i++) r[i] = fStressP[i];
}

/** Fornece a n-�sima dire��o principal - dirPNum[3].* */
void TPZTensor::GetDirPNum(double dirPNum[3], int n) {
	for (int i = 0; i < 3; i++) dirPNum[i] = fDirP[n] [i];
}
