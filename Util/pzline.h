/**
 ************************************************************************** TPZLine.h  -  description -------------------
 * begin                : Tue Apr 16 2002 copyright            : (C) 2002 by Renato Gomes Damas
 * email                : rgdamas@fec.unicamp.br
 */

/**
 **************************************************************************
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 */
#ifndef TPZLINE_H
#define TPZLINE_H
//#include "TPZMetodos.h"
#include "pzvec.h"
#include "pzreal.h"
#include <cmath>

/** @author Renato Gomes Damas */
class TPZLine {
public:
	TPZLine();
	~TPZLine();

	/** Armazena um ponto da reta e sua dire��o. */
	void SetLine(const TPZVec<REAL> &point, const TPZVec<REAL> &dir);

	/** Verifica se o ponto pertence a reta. */
	bool Belongs(const TPZVec<REAL> &point);

	/** Fornece a toler�ncia do c�lculo, armazenando em tol. */
	REAL GetTolerance();

	/** Especifica a toler�ncia para os c�lculos */
	void SetTolerance(const REAL &tol);

private:
	/** um ponto qualquer da reta. */
	TPZVec<REAL> fPoint;

	/** Dire��o da reta. */
	TPZVec<REAL> fDirection;

	/** Toler�ncia para os c�lculos */
	REAL fTolerance;
};
#endif
