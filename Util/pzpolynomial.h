#ifndef TPZPOLYNOMIAL_H
#define TPZPOLYNOMIAL_H
#include <iostream>
#include <cmath>
#include "pzvec.h"
#include "pznumeric.h"

/**
 * @ingroup util
 * @brief Implements a polynomial
 */
class TPZPolynomial {
public:
    /** Construtor da classe. */
	/** @brief Default constructor */
    TPZPolynomial();
	
    /** Construtor da classe. */
	/** @brief Constructor based on coef as coefficients and tol as tolerance value */
    TPZPolynomial(const TPZVec<REAL> &coef, const REAL &tol);
	
    /** Construtor da classe.* */
	/** @brief Constructor based on coef as coefficients */
    TPZPolynomial(const TPZVec<REAL> &coef);
	
    /** Define a tolerancia dos calculos. */
	/** @brief Sets the tolerance value to computes */
    void SetTolerance(const REAL &tol);
	
    /** Fornece a tolerancia do calculo, armazenando em tol. */
	/** @brief Gets the tolerance value */
    void GetTolerance(REAL & tol);
	
    /** Armazena os coeficientes do polinomio em fCo. */
	/** @brief Sets up four coefficients to polynomial */
    void SetCoef(const REAL &c0, const REAL &c1, const REAL &c2, const REAL &c3);
	
    /** Armazena os coeficientes do polinomio em fCo. */
	/** @brief Sets the coefficients of the polynomial */
    void SetCoef(const TPZVec<REAL> &coef);
	
    /** Dados os coeficientes do polinomio, retorna as ra�zes do polinomio em r. */
	/** @brief Over given coefficients computes the roots of the polynomial in r */ 
    int GetRoots(const TPZVec<REAL> &coef, TPZVec<REAL> &r);
	
    /** Retorna as ra�zes do polinomio em r. */
	/** @brief Computes the roots of the polynomial in r */
    int GetRoots(TPZVec<REAL> &r);
	
	/** @brief Computes the roots of the cubic polynomial using Tartaglia method (until 3 degree) */
    int Tartaglia(const TPZVec<REAL> &coef, TPZVec<REAL> &real, REAL &imagem);
	
private:
    /** Calcula as ra�zes do polinomio e armazena em fReal. */
	/** @brief Computes the roots of the polynomial and stores into the fReal */
    int SetRoots();
	
    /** Tolerancia para o calculo. */
	/** @brief Tolerance value to computes */
    REAL fTolerance;
	
    /** Coeficientes do polin�mio. */
	/** @brief Polynomial coefficients */
    TPZVec <REAL> fCo;
	
    /** Ra�zes do polin�mio. */
	/** @brief Roots of the polynomial */
    TPZVec <REAL> fReal;
    REAL fImagem;
};

#endif
