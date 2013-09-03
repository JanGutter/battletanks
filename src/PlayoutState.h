/*
 * PlayoutState.h
 *
 *  Created on: 29 Jul 2013
 *      Author: Jan Gutter
 */

#ifndef PLAYOUTSTATE_H_
#define PLAYOUTSTATE_H_

#include <iostream>
#include "consts.h"
#include <SFMT.h>
using namespace std;

struct TankState {
	int id;
	int active;
	int canfire;
	int x,y; //Position
	int o; //Orientation
	int a; //Action (probably going to get ignored)
	int tag; //Tagged to be destroyed
};

struct BulletState {
	int id;
	int active;
	int x,y; //Position
	int o; //Orientation
	int tag; //Tagged to be destroyed
};

struct BaseState {
	int x,y;
};

class UtilityScores {
public:
	//(player)(x)(y)(o)
	int cost[2][MAX_BATTLEFIELD_DIM][MAX_BATTLEFIELD_DIM][4];
};

struct Tank {
	int x,y,o,cost;
};

inline bool operator<(const Tank& a,const Tank& b)
//The priority is HIGHER if the cost is LOWER
{
  return a.cost > b.cost;
}

//POD structure
class PlayoutState {
public:
	int tickno;
	int command[4]; //commands given to the tanks (index is the same as tank[4])
	int tank_priority[4]; //order in which tanks should be moved
	TankState tank[4]; //Tank 0-1 belongs to PLAYER0, Tank 2-3 belongs to PLAYER1
	BulletState bullet[4]; //(index is the same as tank[4])
	BaseState base[2]; //Base 0 belongs to PLAYER0, Base 1 belongs to PLAYER1
	unsigned char board[MAX_BATTLEFIELD_DIM][MAX_BATTLEFIELD_DIM];
	int min_x,min_y;
	int max_x,max_y;
	bool gameover;
	bool stop_playout;
	double state_score;
	double winner;
	int endgame_tick;
	void drawTanks();
	void drawTinyTanks();
	void drawBases();
	void drawBullets();
	void moveBullets();
	void moveTanks();
	void fireTanks();
	void checkCollisions();
	void checkDestroyedBullets();
	void move(Move& m);
	void simulateTick();
	double playout(sfmt_t& sfmt, UtilityScores& u);
	void updateCanFire(PlayoutState& p);
	bool insideBounds(const int x, const int y);
	bool isTankInsideBounds(const int x, const int y);
	bool clearFireTrajectory(int x, int y, int o, int t_x, int t_y);
	bool clearBallisticTrajectory(int x, int y, int o, int t_x, int t_y);
	bool clearPath(int x, int y, int o);
	bool clearablePath(int x, int y, int o);
	bool insideTank(const unsigned int t, const int x, const int y);
	bool insideAnyTank(const int x, const int y);
	bool insideTinyTank(const int t, const int x, const int y);
	bool onBase(const int b, const int x, const int y);
	bool onTarget(const int t, const int x, const int y);
	bool incomingBullet(const int x, const int y, const int o);
	bool isTankAt(const int t, const int x, const int y);
	void drawTank(const int t, const int block);
	void drawTinyTank(const int t, const int block);
	void populateUtilityScores(UtilityScores &u);
	int cmdToUtility(int c, int t, UtilityScores &u);
	//friend ostream &operator<<(ostream &output, const PlayoutState &p);
	//friend istream &operator>>(istream  &input, PlayoutState &p);
	void paint(UtilityScores& u);
	void paint();
};

ostream &operator<<(ostream &output, const PlayoutState &p);
istream &operator>>(istream  &input, PlayoutState &p);
#endif /* PLAYOUTSTATE_H_ */
