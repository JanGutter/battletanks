//============================================================================
// Name        : CalcEquilibrium.cpp
// Author      : Jan Gutter
// Copyright   : To the extent possible under law, Jan Gutter has waived all
//             : copyright and related or neighboring rights to this work.
//             : For more information, go to:
//             : http://creativecommons.org/publicdomain/zero/1.0/
//             : or consult the README and COPYING files
// Description : Nash Equilibrium calculator (vestigial)
//============================================================================

#include "CalcEquilibrium.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <iostream>
#include <limits>

using namespace std;

CalcEquilibrium::CalcEquilibrium() {
	int i,j;

	srand((unsigned)time(NULL));
	for (i = 0;i < NUMMOVES;i++) {
		for (j = 0;j < NUMMOVES;j++) {
			payoff_matrix[i][j] = ((double)rand()/(double)RAND_MAX);
		}
	}

}

void CalcEquilibrium::printtableau() {
	int i,j;

	cout << "\t";
	for (j = 0; j < NUMMOVES; j++) {
		if ( (labelrow[j] & 0xff00) == 0xff00) {
			cout << "y";
		} else {
			cout << "x";
		}
		cout << (int)(labelrow[j] & 0x00ff) << '\t';
	}
	cout << endl;
	for (i = 0; i < NUMMOVES; i++) {
		if ( (labelcolumn[i] & 0xff00) == 0xff00) {
			cout << "y";
		} else {
			cout << "x";
		}
		cout << (int)(labelcolumn[i] & 0x00ff) << '\t';
		for (j = 0; j < NUMMOVES+1; j++) {
			cout << tableau[i][j] << '\t';
		}
		cout << endl;
	}
	cout << '\t';
	for (j = 0; j < NUMMOVES+1; j++) {
		cout << tableau[NUMMOVES][j] << '\t';
	}
	cout << endl;
}

void CalcEquilibrium::do_calc() {
	int i,j;

	move_t tmpmove;
	double adjust = 0.0;
	double pivot=0.0, ratio, testratio, value;
	int pivotcolumn=0, pivotrow=0;
	bool foundpivot;
	player_strategy_t sx,sy;



	for (i = 0;i < NUMMOVES; i++) {
		labelcolumn[i] = i;
		for (j = 0;j < NUMMOVES; j++) {
			adjust = min(adjust,payoff_matrix[i][j]);
			tableau[i][j] = payoff_matrix[i][j];
		}
		tableau[i][NUMMOVES] = 1.0;
	}
	if (adjust < 0.5) {
		adjust -= 0.5;
		//cout << "Need to adjust the payoff matrix with " << -adjust << endl;
		for (j = 0; j < NUMMOVES; j++) {
			labelrow[j] = j | 0xff00;
			for (i = 0; i < NUMMOVES; i++) {
				tableau[i][j] -= adjust;
			}
			tableau[NUMMOVES][j] = -1.0;
		}
		tableau[NUMMOVES][NUMMOVES] = 0.0;
	}
	while (true) {

		//printtableau(&tableau,&labelrow,&labelcolumn);

		j = 0;
		foundpivot = false;
		while (j < NUMMOVES && !foundpivot) {
			if (tableau[NUMMOVES][j] < 0.0) {
				// We're looking for a candidate pivot in column j
				pivotcolumn = j;
				pivotrow = 0;
				i = 0;
				pivot = 0.0;
				ratio = std::numeric_limits<double>::infinity();
				while (i < NUMMOVES) {
					if (tableau[i][j] > 0.0) {
						testratio = (tableau[i][NUMMOVES]/tableau[i][j]);
						if (testratio < ratio) {
							foundpivot = true;
							ratio = testratio;
							pivotrow = i;
							pivot = tableau[i][j];
						}
					}
					i++;
				}
			}
			j++;
		}

		if (!foundpivot) {
			break;
		}

		//cout << "Pivot: " << pivot << " at row: " << pivotrow << " column: " << pivotcolumn << endl;

		for (i = 0;i <= NUMMOVES; i++) {
			for (j = 0; j <= NUMMOVES; j++) {
				newtableau[i][j] = tableau[i][j] - tableau[pivotrow][j] * tableau[i][pivotcolumn] / pivot;
			}
		}

		for (j = 0; j <= NUMMOVES; j++) {
			newtableau[pivotrow][j] = tableau[pivotrow][j]/pivot;
		}

		for (i = 0; i <= NUMMOVES; i++) {
			newtableau[i][pivotcolumn] = -tableau[i][pivotcolumn]/pivot;
		}

		newtableau[pivotrow][pivotcolumn] = 1.0/pivot;
		memcpy(tableau,newtableau,sizeof(tableau));

		tmpmove = labelrow[pivotrow];
		labelrow[pivotrow] = labelcolumn[pivotcolumn];
		labelcolumn[pivotcolumn] = tmpmove;

	}
	value = 1.0/tableau[NUMMOVES][NUMMOVES] + adjust;
	for (i = 0; i < NUMMOVES;i++) {
		sx[i] = 0.0;
		sy[i] = 0.0;
	}

	for (j = 0; j < NUMMOVES;j++) {
		//If px is in the labelrow
		if ((labelrow[j] & 0x00ff) == labelrow[j]) {
			sx[labelrow[j]] = tableau[NUMMOVES][j]/tableau[NUMMOVES][NUMMOVES];
		}
	}

	for (i = 0; i < NUMMOVES;i++) {
		//If py is in the labelcolumn
		if ((labelcolumn[i] & 0x00ff) != labelcolumn[i]) {
			sy[labelcolumn[i] & 0x00ff] = tableau[i][NUMMOVES]/tableau[NUMMOVES][NUMMOVES];
		}
	}

	cout << "Value: " << value << endl;
	cout << "Strategy for x = ";
	for (i = 0;i < NUMMOVES;i++) {
		cout << "[" << sx[i] << "]";
	}
	cout << endl;
	cout << "Strategy for y = ";
	for (i = 0;i < NUMMOVES;i++) {
		cout << "[" << sy[i] << "]";
	}
	cout << endl;
}

CalcEquilibrium::~CalcEquilibrium() {
	// TODO Auto-generated destructor stub
}
