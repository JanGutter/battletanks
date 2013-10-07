//============================================================================
// Name        : CalcEquilibrium.h
// Author      : Jan Gutter
// Copyright   : To the extent possible under law, Jan Gutter has waived all
//             : copyright and related or neighboring rights to this work.
//             : For more information, go to:
//             : http://creativecommons.org/publicdomain/zero/1.0/
//             : or consult the README and COPYING files
// Description : Nash Equilibrium Calculator (vestigial)
//============================================================================

#ifndef CALCEQUILIBRIUM_H_
#define CALCEQUILIBRIUM_H_

#include "consts.h"

typedef double player_strategy_t[NUMMOVES];
typedef double payoff_matrix_t[NUMMOVES][NUMMOVES];
// The labelrow initially contains P2's moves. 0xff00 | move tags 'em
typedef move_t labelrow_t[NUMMOVES];
// The labelcolumn initially contains P1's moves. The most significant byte is zero
typedef move_t labelcolumn_t[NUMMOVES];
typedef double tableau_t[NUMMOVES+1][NUMMOVES+1];

class CalcEquilibrium {
public:

	labelrow_t labelrow;
	labelcolumn_t labelcolumn;
	payoff_matrix_t payoff_matrix;// = { {2.0,-1.0,6.0}, {0.0,1.0,-1.0}, {-2.0, 2.0, 1.0} };
	tableau_t tableau;
	tableau_t newtableau;
	CalcEquilibrium();
	void printtableau();
	void do_calc();
	virtual ~CalcEquilibrium();
};

#endif /* CALCEQUILIBRIUM_H_ */
