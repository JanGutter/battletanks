/*
 * consts.h
 *
 *  Created on: 29 Jul 2013
 *      Author: Jan Gutter
 */

#ifndef CONSTS_H_
#define CONSTS_H_

#include <stdint.h>
#include <utility>
#include <algorithm>
#include <functional>

using namespace std;

template <class T1, class T2> struct sort_pair_second {
	bool operator()(const pair<T1,T2>&left, const pair<T1,T2>&right) {
		return left.second < right.second;
	}
};

struct Move {
	int alpha;
	int beta;
};

//15% of moves in the random playout uses the greedy algorithm
#define EPSILON_GREEDY 15

#define PLAYER0 0
#define PLAYER1 1

#define W_DRAW 0.5
#define W_PLAYER0 1.0
#define W_PLAYER1 0.0

// A tank's command fits in a nibble (hex digit)
// A single player's command consists of two nibbles: (T1 << 4) | T0
// Two players use (Py << 8) | Px
// If the most significant byte is 0xff, then it's Py's command label
typedef uint16_t move_t;

// Given alpha/beta moves, what's the commands for T0-T3
#define C_T0(alpha,beta) ((alpha) % 6)
#define C_T1(alpha,beta) ((alpha) / 6)
#define C_T2(alpha,beta) ((beta) % 6)
#define C_T3(alpha,beta) ((beta) / 6)
// Given commands for T0 and T1, what's alpha?
#define C_TO_ALPHA(t0,t1) ((t1)*6+(t0))
// Given commands for T2 and T3, what's beta?
#define C_TO_BETA(t2,t3) ((t3)*6+(t2))

#define C_X 0
#define C_Y 1

#define C_NONE 0
#define C_FIRE 1
#define C_UP 2
#define C_DOWN 3
#define C_LEFT 4
#define C_RIGHT 5

//C_M_LOOKUP_TABLE looks up the movement from a command
const int C_M_LOOKUP_TABLE[6][2] = {
		{ 0, 0},/*none*/
		{ 0, 0},/*fire*/
		{ 0,-1},/*up*/
		{ 0, 1},/*down*/
		{-1, 0},/*left*/
		{ 1, 0}/*right*/
};

#define C_M_LOOKUP(c,axis) C_M_LOOKUP_TABLE[(c)][(axis)]
#define C_ISMOVE(c) ((c) > 1)
#define C_TO_O(c) ((c) - 2)

const int BUMP_LOOKUP_TABLE[4][5][2] = {
		{{-2,-3},{-1,-3},{ 0,-3},{ 1,-3},{ 2,-3}},/*up*/
		{{-2, 3},{-1, 3},{ 0, 3},{ 1, 3},{ 2, 3}},/*down*/
		{{-3,-2},{-3,-1},{-3, 0},{-3, 1},{-3, 2}},/*left*/
		{{ 3,-2},{ 3,-1},{ 3, 0},{ 3, 1},{ 3, 2}},/*right*/
		};
// c == command, i == 1-5, axis == X or Y
#define BUMP_LOOKUP(c,i,axis) BUMP_LOOKUP_TABLE[(c)-2][(i)][(axis)]
#define BUMP_OPPOSITE(c,i,axis) BUMP_LOOKUP_TABLE[((c)-2)^1][(i)][(axis)]

#define O_X 0
#define O_Y 1

#define O_UP 0
#define O_DOWN 1
#define O_LEFT 2
#define O_RIGHT 3

const int O_LOOKUP_TABLE[4][2] = {
		{0,-1},/*up*/
		{0,1},/*down*/
		{-1,0},/*left*/
		{1,0}/*right*/};

#define O_LOOKUP(o,axis) O_LOOKUP_TABLE[(o)][(axis)]
#define O_OPPOSITE(o) ((o)^1)

const int FIRE_LOOKUP_TABLE[4][2] = {
		{ 0,-3},/*up*/
		{ 0, 3},/*down*/
		{-3, 0},/*left*/
		{ 3, 0},/*right*/
		};
#define FIRE_LOOKUP(o,axis) FIRE_LOOKUP_TABLE[(o)][(axis)]

#define NUMMOVES 36 //Number of possible moves

#define MAX_BATTLEFIELD_DIM 128 // 128x128 battlefield

#define B_X 0
#define B_Y 1

#define B_EMPTY 0
#define B_WALL 1
#define B_BASE 2
#define B_TANK 4
#define B_BULLET (8+16+32+64)
#define B_OOB 128
#define B_OCCUPIED (B_WALL+B_BASE+B_TANK)
#define B_LOOKUP(o) (8 << (o))
#define B_OPPOSITE(o) (8 << (o^1))
#define B_ISCLEAR(b) (((b) & B_OCCUPIED) == B_EMPTY)
#define B_ISBASE(b) (((b) & B_BASE) == B_BASE)
#define B_ISOOB(b) (((b) & B_OOB) == B_OOB)

//B_SPLASH[o][i][x/y] is a lookup for the 4 "splash damage" coordinates of an explosion.
const int B_SPLASH_LOOKUP_TABLE[4][4][2] = {
		{{-2,0}, {-1,0}, {1,0}, {2,0}},
		{{-2,0}, {-1,0}, {1,0}, {2,0}},
		{{0,-2}, {0,-1}, {0,1}, {0,2}},
		{{0,-2}, {0,-1}, {0,1}, {0,2}}};

#define B_SPLASH(o,i,axis) B_SPLASH_LOOKUP_TABLE[(o)][(i)][(axis)]


// o == orientation, i == 1-5, axis == X or Y
#define MOVEPATH_LOOKUP(o,i,axis) BUMP_LOOKUP_TABLE[(o)][(i)][(axis)]

#endif /* CONSTS_H_ */
