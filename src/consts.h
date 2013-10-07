//============================================================================
// Name        : consts.h
// Author      : Jan Gutter
// Copyright   : To the extent possible under law, Jan Gutter has waived all
//             : copyright and related or neighboring rights to this work.
//             : For more information, go to:
//             : http://creativecommons.org/publicdomain/zero/1.0/
//             : or consult the README and COPYING files
// Description : Used to be constants, now all common stuff got dumped here
//============================================================================

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

//A terminal score should dominate the normal scores
#define TERMINAL_BONUS 36.0

#define MCTS_CONF_MARGIN 4000.0

#define TANK_PROXIMITY_WARNING 30

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
#define C_ISMOVE(c) ((c) > 1)
#define C_TO_O(c) ((c) - 2)

const int BUMP_LOOKUP_TABLE[4][5][2] = {
		{{-2,-3},{-1,-3},{ 0,-3},{ 1,-3},{ 2,-3}},/*up*/
		{{-2, 3},{-1, 3},{ 0, 3},{ 1, 3},{ 2, 3}},/*down*/
		{{-3,-2},{-3,-1},{-3, 0},{-3, 1},{-3, 2}},/*left*/
		{{ 3,-2},{ 3,-1},{ 3, 0},{ 3, 1},{ 3, 2}},/*right*/
};

#define O_X 0
#define O_Y 1

// o == orientation, i == 1-5, axis == X or Y
#define BUMP_LOOKUP(o,i,axis) BUMP_LOOKUP_TABLE[(o)][(i)][(axis)]


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
#define B_DESTRUCTABLES B_WALL+B_BASE+B_TANK
#define B_LOOKUP(o) (8 << (o))
#define B_OPPOSITE(o) (8 << (o^1))
#define B_ISCLEAR(b) (((b) & (B_WALL+B_BASE+B_TANK)) == B_EMPTY)
#define B_ISBASE(b) (((b) & B_BASE) == B_BASE)
#define B_ISOOB(b) (((b) & B_OOB) == B_OOB)

//B_SPLASH[o][i][x/y] is a lookup for the 4 "splash damage" coordinates of an explosion.
const int B_SPLASH_LOOKUP_TABLE[4][4][2] = {
		{{-2, 0}, {-1, 0}, { 1, 0}, { 2, 0}},
		{{ 2, 0}, { 1, 0}, {-1, 0}, {-2, 0}},
		{{ 0,-2}, { 0,-1}, { 0, 1}, { 0, 2}},
		{{ 0, 2}, { 0, 1}, { 0,-1}, { 0,-2}}};

#define B_SPLASH(o,i,axis) B_SPLASH_LOOKUP_TABLE[(o)][(i)][(axis)]
#define AVOID_LOOKUP(o,i,axis) B_SPLASH_LOOKUP_TABLE[(o)][(i)][(axis)]

// o == orientation, i == 1-5, axis == X or Y
#define MOVEPATH_LOOKUP(o,i,axis) BUMP_LOOKUP_TABLE[(o)][(i)][(axis)]

//AVOID_WIDE_LOOKUP_TABLE[o][i][x/y] is a lookup for the wide avoidance coordinates
const int AVOID_WIDE_LOOKUP_TABLE[4][2][2] = {
		{{-2, 0}, { 2, 0}},
		{{ 2, 0}, {-2, 0}},
		{{ 0,-2}, { 0, 2}},
		{{ 0, 2}, { 0,-2}}};
//AVOID_WIDE_O_LOOKUP_TABLE[o][i] is a lookup to see which direction to dodge
const int AVOID_WIDE_O_LOOKUP_TABLE[4][2] = {
		{O_LEFT, O_RIGHT},
		{O_RIGHT, O_LEFT},
		{O_UP,O_DOWN},
		{O_DOWN,O_UP}};

#define AVOID_WIDE_LOOKUP(o,i,axis) AVOID_WIDE_LOOKUP_TABLE[(o)][(i)][(axis)]
#define AVOID_WIDE_O_LOOKUP(o,i) AVOID_WIDE_O_LOOKUP_TABLE[(o)][(i)/2]

const int WEAKSPOT_LOOKUP_TABLE[5] = {
		5,2,5,2,5
};

#define WEAKSPOT_LOOKUP(i) WEAKSPOT_LOOKUP_TABLE[(i)]

const int AVOIDANCE_CANFIRE_MATRIX[5][5] = {
	{1,2,0,2,1},
	{2,3,0,3,2},
	{0,0,0,0,0},
	{2,3,0,3,2},
	{1,2,0,1,2}
};

const int AVOIDANCE_CANTFIRE_MATRIX[5][5] = {
	{1,2,3,2,1},
	{2,3,4,3,2},
	{3,4,5,4,3},
	{2,3,4,3,2},
	{1,2,3,1,2}
};

inline const char* o2str(int o) {
	switch (o) {
	default:
	case O_UP:
		return "up";
	case O_DOWN:
		return "down";
	case O_LEFT:
		return "left";
	case O_RIGHT:
		return "right";
	}
}

inline const char* cmd2str(int cmd) {
	switch (cmd) {
	default:
	case C_NONE:
		return "none";
	case C_UP:
		return "up";
	case C_DOWN:
		return "down";
	case C_LEFT:
		return "left";
	case C_RIGHT:
		return "right";
	case C_FIRE:
		return "fire";
	}
}

#endif /* CONSTS_H_ */
