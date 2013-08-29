/*
 * PlayoutState.cpp
 *
 *  Created on: 29 Jul 2013
 *      Author: Jan Gutter
 */

#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <queue>
#include "PlayoutState.h"
#include "SFMT.h"
#include "MCTree.h"
#include <string.h>
#include <limits.h>

using namespace std;

void PlayoutState::drawTanks()
{
	int t;
	for (t = 0; t < 4; t++) {
		if (tank[t].active) {
			drawTank(t, B_TANK);
		}
	}
}

void PlayoutState::drawBases()
{
	int i;
	for (i = 0; i < 2; i++) {
		board[base[i].x][base[i].y] = B_BASE;
	}
}

void PlayoutState::drawBullets()
{
	int i;
	for (i = 0; i < 4; i++) {
		if (bullet[i].active) {
			board[bullet[i].x][bullet[i].y] = B_LOOKUP(bullet[i].o);
		}
	}
}

bool PlayoutState::insideBounds(const int x, const int y)
{
	return (x > 0 && y > 0 && x < max_x && y < max_y);
}

bool PlayoutState::isTankInsideBounds(const int x, const int y)
{
	return (x > 1 && y > 1 && x < (max_x-2) && y < (max_y-2));
}

bool PlayoutState::insideTank(const int t, const int x, const int y)
{
	return (tank[t].active && (abs(tank[t].x - x) < 3) && (abs(tank[t].y - y) < 3));
}

bool PlayoutState::isTankAt(const int t, const int x, const int y)
{
	return (tank[t].active && tank[t].x == x && tank[t].y == y);
}

void PlayoutState::drawTank(const int t, const int block)
{
	int i,j;
	for (i = tank[t].x-2; i < tank[t].x+3; i++) {
		for (j = tank[t].y-2; j < tank[t].y+3; j++) {
			board[i][j] = block;
		}
	}
}

void PlayoutState::moveBullets()
{
	int i;
	for (i = 0; i < 4; i++) {
		if (bullet[i].active) {
			board[bullet[i].x][bullet[i].y] = B_EMPTY;
			bullet[i].x += O_LOOKUP(bullet[i].o,O_X);
			bullet[i].y += O_LOOKUP(bullet[i].o,O_Y);
			if (insideBounds(bullet[i].x,bullet[i].y)) {
				board[bullet[i].x][bullet[i].y] |= B_LOOKUP(bullet[i].o);
			} else {
				bullet[i].active = 0;
			}
		}
	}
	//NOTE: MUST RESOLVE COLLISIONS AFTER THIS!
}

void PlayoutState::moveTanks()
{
	int i,j,t,c,x,y,square;
	int newx,newy;
	bool clear;
	bool base;

	for (i = 0; i < 4; i++) {
		t = tank_priority[i];
		c = command[t];
		if (tank[t].active && C_ISMOVE(c)) {
			tank[t].o = C_TO_O(c);
			clear = true;
			base = false;
			x = tank[t].x;
			y = tank[t].y;
			newx = x + C_M_LOOKUP(c,C_X);
			newy = y + C_M_LOOKUP(c,C_Y);
			if (isTankInsideBounds(newx,newy)) {
				j = 0;
				while (j < 5) {
					square = board[x+BUMP_LOOKUP(c,j,C_X)][y+BUMP_LOOKUP(c,j,C_Y)];
					clear = clear && B_ISCLEAR(square);
					base = base || B_ISBASE(square);
					j++;
				}
				//TODO: Check for win by ramming? base
				if (clear && !base) {
					//Move tank
					tank[t].x = newx;
					tank[t].y = newy;
					//Set new points and unset old ones
					for (j = 0; j < 5; j++) {
						board[x+BUMP_LOOKUP(c,j,C_X)][y+BUMP_LOOKUP(c,j,C_Y)] |= B_TANK;
						board[newx+BUMP_OPPOSITE(c,j,C_X)][newy+BUMP_OPPOSITE(c,j,C_Y)] ^= B_TANK;
					}
				}
			}
		}
	}
	//NOTE: MUST RESOLVE COLLISIONS AFTER THIS!
}

void PlayoutState::fireTanks()
{
	int i;
	for (i = 0; i < 4; i++) {
		if ((command[i] == C_FIRE) && (tank[i].active) && (!bullet[i].active)) {
			bullet[i].x = tank[i].x + FIRE_LOOKUP(tank[i].o,O_X);
			bullet[i].y = tank[i].y + FIRE_LOOKUP(tank[i].o,O_Y);
			if (insideBounds(bullet[i].x,bullet[i].y)) {
				bullet[i].active = 1;
				bullet[i].o = tank[i].o;
				board[bullet[i].x][bullet[i].y] |= B_LOOKUP(tank[i].o);
			}
		}
	}
	//NOTE: MUST RESOLVE COLLISIONS AFTER THIS!
}

void PlayoutState::checkCollisions()
{
	int i,j,square,other_bullet,x,y,active_tanks,active_bullets;

	for (i = 0; i < 4; i++) {
		bullet[i].tag = 0;
	}
	for (i = 0; i < 4; i++) {
		tank[i].tag = 0;
	}

	//TODO: Bullets that pass each other can be whacked here.

	for (i = 0; i < 4; i++) {
		if (bullet[i].active) {
			//If the bullet passed another one in the opposite direction, it's tagged for removal;
			bullet[i].tag = board[bullet[i].x - O_LOOKUP(bullet[i].o,O_X)][bullet[i].y - O_LOOKUP(bullet[i].o,O_Y)] & B_OPPOSITE(bullet[i].o);
			other_bullet = ((board[bullet[i].x][bullet[i].y] & (B_BULLET)) != B_LOOKUP(bullet[i].o));
			square = board[bullet[i].x][bullet[i].y] & (B_OCCUPIED);
			if (square != B_EMPTY) {
				//There's something else here
				if (square == B_WALL) {
					//Apply splash damage to B_WALL squares ONLY
					for (j = 0; j < 4;j++) {
						x = bullet[i].x + B_SPLASH(bullet[i].o,j,B_X);
						y = bullet[i].y + B_SPLASH(bullet[i].o,j,B_Y);
						if (insideBounds(x,y)) {
							if (board[x][y] == B_WALL) {
								board[x][y] = B_EMPTY;
							}
						}
					}
					if (!other_bullet) {
						//Remove wall under bullet
						board[bullet[i].x][bullet[i].y] = B_EMPTY;
						bullet[i].active = 0;
					} else {
						//Remove the bullet from the board
						//Leave the wall so the other bullet can also detect a collision
						board[bullet[i].x][bullet[i].y] ^= B_LOOKUP(bullet[i].o);
					}
				} else if (square == B_TANK) {
					j = 0;
					while ((j < 3) && !insideTank(j,bullet[i].x,bullet[i].y)) {
						j++;
					}
					tank[j].tag = 1;
					bullet[i].active = 0;
				} else if (square == B_BASE) {
					bullet[i].active = 0;
					winner = W_DRAW; //It's at least a draw
					if ((base[PLAYER0].x == bullet[i].x) && (base[PLAYER0].y == bullet[i].y)) {
						//Player0's base has been destroyed
						if (!gameover) {
							winner = W_PLAYER1;
						}
					} else {
						//Player1's base has been destroyed
						if (!gameover) {
							winner = W_PLAYER0;
						}
					}
					gameover = 1;
				}
			} else if (other_bullet) {
				//It's an empty square with at least one other bullet
				bullet[i].tag = 1;
			} //else: NO COLLISION!
		}
	}

	//Erase tagged bullets
	for (i = 0; i < 4; i++) {
		if (bullet[i].active && bullet[i].tag) {
			board[bullet[i].x][bullet[i].y] = B_EMPTY;
			bullet[i].active = 0;
		}
	}
	//Erase tagged tanks
	for (i = 0; i < 4; i++) {
		if (tank[i].tag) {
			drawTank(i, B_EMPTY);
			tank[i].active = 0;
		}
	}
	//Check for draw (no tanks and no bullets)
	if (!gameover) {
		active_tanks = 0;
		active_bullets = 0;
		for (i = 0; i < 4; i++) {
			active_tanks += tank[i].active;
			active_bullets += bullet[i].active;
		}
		gameover = (active_tanks == 0) && (active_bullets == 0);
		winner = W_DRAW;
	}
}

void PlayoutState::simulateTick()
{
	tickno++;
	//TODO: check for endgame_tick here!

	moveBullets();
	checkCollisions();
	if (gameover) {
		return;
	}

	moveBullets();
	moveTanks();
	checkCollisions();
	if (gameover) {
		return;
	}

	fireTanks();
	checkCollisions();
}

void PlayoutState::move(Move& m)
{
	command[0] = C_T0(m.alpha,m.beta);
	command[1] = C_T1(m.alpha,m.beta);
	command[2] = C_T2(m.alpha,m.beta);
	command[3] = C_T3(m.alpha,m.beta);
	simulateTick();
}

double PlayoutState::playout(sfmt_t& sfmt)
{
	int move,i;
	int maxmove = endgame_tick+(max_x/2)-tickno;
	winner = W_DRAW;
	for (move = 0; move < maxmove; move++) {
		for (i = 0; i < 4; i++) {
			//command[i] = tank[i].active * (sfmt_genrand_uint32(&sfmt) % 6)
			command[i] = tank[i].active * ((1-bullet[i].active)*(sfmt_genrand_uint32(&sfmt) % 6)
					+ (bullet[i].active)*(sfmt_genrand_uint32(&sfmt) % 4));
		}
		simulateTick();
		if (gameover) {
			break;
		}
	}
	return winner;
}

bool PlayoutState::clearTrajectory(int x, int y, int o, int t_x, int t_y)
{
	//WARNING: DON'T CALL WITH OOB t_x and t_y
	bool clear = true;
	//We start from the center of tank: need to move 3 blocks to get where the bullet starts
	x += 3*O_LOOKUP(o,O_X);
	y += 3*O_LOOKUP(o,O_Y);
	while (clear && ((x != t_x) || (y != t_y))) {
		clear = insideBounds(x,y) && ((board[x][y] & B_WALL) == 0);
		x += O_LOOKUP(o,O_X);
		y += O_LOOKUP(o,O_Y);
	}
	return clear;
}

bool PlayoutState::clearPath(int x, int y, int o)
{
	//NOT A SUBSTITUTE FOR OOB x+move, y+move!
	bool clear = true;
	int i;
	for (i = 0; clear && (i < 5); i++) {
		clear = (board[x+MOVEPATH_LOOKUP(o,i,O_X)][y+MOVEPATH_LOOKUP(o,i,O_Y)] & B_WALL) == 0;
	}
	return clear;
}

bool PlayoutState::clearablePath(int x, int y, int o)
{
	//NOT A SUBSTITUTE FOR OOB x+move, y+move!
	return (board[x+FIRE_LOOKUP(o,O_X)][y+FIRE_LOOKUP(o,O_Y)] & B_WALL) == B_WALL;
}

void PlayoutState::populateUtilityScores(UtilityScores &u)
{
	int o,i,j;
	Tank t,g;
	int player;
	priority_queue<Tank> frontier;
	for (player = 0; player < 2; player++) {
		for (i = 0; i < max_x; i++) {
			for (j = 0; j < max_y; j++) {
				for (o = 0; o < 4; o++) {
					u.cost[player][i][j][o] = INT_MAX;
				}
			}
		}
	}
	//memset(&u,0,sizeof(u)); //Set all to be unexplored

	for (player = 0; player < 2; player++) {
		//Seed with possible final positions
		for (o = 0; o < 4; o++) {
			for (i = 0; i < max(max_x,max_y); i++) {
				t.cost = i/2 + 1;
				t.o = o;
				t.x = base[player^1].x - (3+i)*O_LOOKUP(o,O_X);
				t.y = base[player^1].y - (3+i)*O_LOOKUP(o,O_Y);
				if (isTankInsideBounds(t.x,t.y) && clearTrajectory(t.x,t.y,t.o,base[player^1].x,base[player^1].y)) {
					u.cost[player][t.x][t.y][t.o] = t.cost;
					frontier.push(t);
				} else {
					break;
				}
			}
		}

		//Flood-fill backward to determine shortest greedy path
		while (!frontier.empty()) {
			g = frontier.top();
			frontier.pop();
			t.x = g.x - O_LOOKUP(g.o,O_X);
			t.y = g.y - O_LOOKUP(g.o,O_Y);
			t.cost = g.cost+1;
			if (isTankInsideBounds(t.x,t.y)) {
				if (clearPath(t.x,t.y,g.o)) {
					for (o = 0; o < 4; o++) {
						t.o = o;
						if (t.cost < u.cost[player][t.x][t.y][t.o]) {
							u.cost[player][t.x][t.y][t.o] = t.cost;
							frontier.push(t);
						}
					}
				} else if (clearablePath(t.x,t.y,g.o)) {
					for (o = 0; o < 4; o++) {
						t.o = o;
						if (t.o == g.o) {
							t.cost = g.cost+2;
						} else {
							t.cost = g.cost+3;
						}
						if (t.cost < u.cost[player][t.x][t.y][t.o]) {
							u.cost[player][t.x][t.y][t.o] = t.cost;
							frontier.push(t);
						}
					}
				}
			}
		}
	}
	for (o = 0; o < 4; o++) {
		u.cost[0][base[1].x][base[1].y][o] = 0;
		u.cost[1][base[0].x][base[0].y][o] = 0;
	}

}

bool PlayoutState::onBase(const int b, const int x, const int y)
{
	return (x == base[b].x && y == base[b].y);
}

bool PlayoutState::onTarget(const int t, const int x, const int y)
{
	if (t < 2) {
		return (onBase(1,x,y) || insideTank(2,x,y) || insideTank(3,x,y));
	} else {
		return (onBase(0,x,y) || insideTank(0,x,y) || insideTank(1,x,y));
	}
}

int PlayoutState::cmdToUtility(int c, int tank_id, UtilityScores &u)
{
	int i;
	int wallcount;
	int hitcost;
	int x,y,o;
	TankState& t = tank[tank_id];
	BulletState& b = bullet[tank_id];
	switch (c) {
	case C_FIRE:
		if (b.active) {
			return INT_MAX;
		}
		x = t.x + FIRE_LOOKUP(t.o,O_X);
		y = t.y + FIRE_LOOKUP(t.o,O_Y);
		wallcount = 0;
		i = 0;
		while (insideBounds(x,y) && !onTarget(tank_id,x,y)) {
			wallcount += (board[x][y] & B_WALL)*((i/2) + 1);
			i++;
			x += O_LOOKUP(t.o,O_X);
			y += O_LOOKUP(t.o,O_Y);
		}
		hitcost = INT_MAX-1;
		if (onTarget(tank_id,x,y)) {
			//HIT!
			hitcost = ((i/2)+wallcount+1);
		}
		if (isTankInsideBounds(t.x + O_LOOKUP(t.o,O_X),t.y + O_LOOKUP(t.o,O_Y)) &&
				clearablePath(t.x,t.y,t.o)) {
			//Shoot to clear a space to move in
			return min(hitcost,u.cost[0][t.x + O_LOOKUP(t.o,O_X)][t.y + O_LOOKUP(t.o,O_Y)][t.o]);
		} else {
			//Just shoot
			return min(hitcost,INT_MAX-1);
		}
	case C_UP:
	case C_DOWN:
	case C_LEFT:
	case C_RIGHT:
		o = c-2;
		if (isTankInsideBounds(t.x + O_LOOKUP(o,O_X),t.y + O_LOOKUP(o,O_Y)) &&
				clearPath(t.x,t.y,o)) {
			return u.cost[0][t.x + O_LOOKUP(o,O_X)][t.y + O_LOOKUP(o,O_Y)][o];
		} else {
			return u.cost[0][t.x][t.y][o];
		}
	default:
	case C_NONE:
		return INT_MAX;
	}
}


ostream &operator<<(ostream &output, const PlayoutState &p)
{
	int i,j;
	output << p.tickno;
	for (i = 0; i < 4; i++) {
		output	<< p.tank[i].id << ' '
				<< p.tank[i].active << ' '
				<< p.tank[i].o << ' '
				<< p.tank[i].x << ' '
				<< p.tank[i].y << '\n';
	}
	for (i = 0; i < 4; i++) {
		output	<< p.bullet[i].id << ' '
				<< p.bullet[i].active << ' '
				<< p.bullet[i].o << ' '
				<< p.bullet[i].x << ' '
				<< p.bullet[i].y << '\n';
	}
	for (i = 0; i < 2; i++) {
		output	<< p.base[i].x << ' '
				<< p.base[i].y << '\n';
	}
	output << p.max_x << ' ' << p.max_y << '\n';
	for (i = 0; i < p.max_x; i++) {
		for (j = 0; j < p.max_y; j++) {
			output << setw(3) << p.board[i][j];
		}
		output << '\n';
		//output << P.board[i][P.dimy-1] << '\n';
	}

	return output;
}

istream &operator>>(istream &input, PlayoutState &p)
{
	int i,j;
	pair<int,int> tank;
	vector< pair<int,int> > tanks(4);
	input >> p.tickno;
	for (i = 0; i < 4; i++) {
		input >> p.tank[i].id
		>> p.tank[i].active
		>> p.tank[i].o
		>> p.tank[i].x
		>> p.tank[i].y;
		tank.first = i;
		tank.second = p.tank[i].id;
		tanks[i] = tank;
	}
	sort(tanks.begin(), tanks.end(), sort_pair_second<int, int>());
	for (i = 0; i < 4; i++) {
		p.tank_priority[i] = tanks[i].first;
	}
	for (i = 0; i < 4; i++) {
		input >> p.bullet[i].id
		>> p.bullet[i].active
		>> p.bullet[i].o
		>> p.bullet[i].x
		>> p.bullet[i].y;
	}
	for (i = 0; i < 2; i++) {
		input >> p.base[i].x
		>> p.base[i].y;
	}
	input >> p.max_x >> p.max_y;
	for (i = 0; i < p.max_x; i++) {
		for (j = 0; j < p.max_y; j++) {
			input >> p.board[i][j];
		}
	}
	return input;
}
