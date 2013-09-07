/*
 * NetworkCore.cpp
 *
 *  Created on: 09 Aug 2013
 *      Author: Jan Gutter
 */

#include <string>
#include <iostream>
#include <stdlib.h>
#include <soap/soapChallengeServiceSoapBindingProxy.h>
#include <soap/nsmap.h>
#include <platformstl/performance/performance_counter.hpp>
#include "sleep.h"
#include "NetworkCore.h"
#include "PlayoutState.h"
#include <limits.h>
#include <StatCounter.hpp>
#include "MCTree.h"
#include <fstream>

#define DEBUG 0

#define NUMC 2
#define NUMPLAYERS 2
#define NUMTANKS 2
#define HALFLIMP 0
#define AREYOUNUTS 0
#define SAVESTARTMAP 0

const int fixed_commands[NUMPLAYERS][NUMTANKS][NUMC] = {
		{ //"Player One"
				{C_NONE,C_NONE},
				{C_NONE,C_NONE}
		},
		{ //"Player Two"
				{C_NONE,C_NONE},
				{C_NONE,C_NONE}
		}

};

using namespace std;

inline ns1__action hton_cmd(int cmd) {
	switch (cmd) {
	default:
	case C_NONE:
		return ns1__action__NONE;
	case C_UP:
		return ns1__action__UP;
	case C_DOWN:
		return ns1__action__DOWN;
	case C_LEFT:
		return ns1__action__LEFT;
	case C_RIGHT:
		return ns1__action__RIGHT;
	case C_FIRE:
		return ns1__action__FIRE;
	}
}

inline const char* action2str(ns1__action action) {
	switch (action) {
	default:
	case ns1__action__NONE:
		return "none";
	case ns1__action__UP:
		return "up";
	case ns1__action__DOWN:
		return "down";
	case ns1__action__LEFT:
		return "left";
	case ns1__action__RIGHT:
		return "right";
	case ns1__action__FIRE:
		return "fire";
	}
}

inline int ntoh_direction(enum ns1__direction *direction) {
	//TODO: Direction is probably static_cast<int>(*direction)-1;
	if (direction) {
		switch (*direction) {
		default:
		case ns1__direction__NONE:
			cerr << "Warning: Received direction NONE!" << endl;
		case ns1__direction__UP:
			return O_UP;
		case ns1__direction__DOWN:
			return O_DOWN;
		case ns1__direction__LEFT:
			return O_LEFT;
		case ns1__direction__RIGHT:
			return O_RIGHT;
		}
	}
	cerr << "Warning: Did not receive direction!" << endl;
	return O_UP;
}

NetworkCore::NetworkCore(const char* soap_endpoint)
{
	int i;
	state = new PlayoutState;
	state_synced = false;
	soaperr = SOAP_OK;
	s.soap_endpoint = soap_endpoint;
	state->max_x = 0;
	state->max_y = 0;
	state->tickno = 0;
	for (i = 0; i < 4;i++) {
		state->tank[i].active = 0;
		state->tank[i].id = INT_MAX;
		state->bullet[i].active = 0;
		state->bullet[i].id = INT_MAX;
	}
	policy = POLICY_RANDOM;
}

NetworkCore::~NetworkCore()
{
	delete state;
}

void NetworkCore::login() {
	//TODO: Need to loop to re-attempt login
	do {
		int square;
		state->min_x = 0;
		state->min_y = 0;
		state->max_x = 0;
		ns1__login login_req;
		ns1__loginResponse login_resp;
		soaperr = s.login(&login_req, &login_resp);
		if (soaperr == SOAP_OK) {
			state->endgame_tick = login_resp.return_->endGamePoint;
#if DEBUG
			cout << "Endgame starts at: " << state->endgame_tick << endl;
#endif
			for (vector<ns1__stateArray*>::iterator boardx = login_resp.return_->states.begin(); boardx != login_resp.return_->states.end(); ++boardx) {
				state->max_y = 0;
				for (vector<ns1__state>::iterator boardy = (*boardx)->item.begin(); boardy != (*boardx)->item.end() ; ++boardy) {
#if DEBUG
					cout << *boardy;
#endif
					switch (*boardy) {
					case ns1__state__FULL:
						square = B_WALL;
						break;
					case ns1__state__OUT_USCOREOF_USCOREBOUNDS:
						square = B_OOB;
						break;
					default:
					case ns1__state__NONE:
						cerr << "Warning, received NONE state on square!" << endl;
					case ns1__state__EMPTY:
						square = B_EMPTY;
					}
					state->board[state->max_x][state->max_y] = square;
					state->max_y++;
				}
#if DEBUG
				cout << endl;
#endif
				state->max_x++;
			}
		}
		if (soaperr != SOAP_OK) {
			s.soap_stream_fault(std::cerr);
			Sleep(250);
		}
		state_synced = false;
		/*PlayoutState p;
	memset(&p,0,sizeof(p));
	p.max_x = max_x;
	p.max_y = max_y;
	memcpy(p.board,board,sizeof(board));
	cout << p;*/
		//TODO: Need to know when endgame begins!
		state->gameover = false;
		state->stop_playout = false;

	} while (soaperr != SOAP_OK);
}

void NetworkCore::play()
{
	StatCounter stat_getstatus;
	StatCounter stat_setaction;
	platformstl::performance_counter soap_timer;
	platformstl::performance_counter comms_timer;
	platformstl::performance_counter ai_timer;
	platformstl::performance_counter loop_timer;
	bool its_me;
	int num_recv,player_offset,check;
	TankState received_tanks[2];
	BulletState received_bullets[2];
	int i,j;
	long long int nexttick;
	long int lasttick = 0;
	int safety_margin = 500; // send message 200ms before end of tick.
	int settle_time = 500; // only poll getStatus 200ms after the beginning of the tick.
	bool repeated_tick;
	bool skipped_tick;
	MCTree* mc_tree = new MCTree;
	PlayoutState* node_state = new PlayoutState;
#if SAVESTARTMAP
	bool firstrun = true;
#endif
	stat_getstatus.init();
	stat_setaction.init();

	while (soaperr == SOAP_OK) {
		comms_timer.restart();
		ns1__getStatus getStatus_req;
		ns1__getStatusResponse getStatus_resp;
		soap_timer.restart();
		soaperr = s.getStatus(&getStatus_req, &getStatus_resp);
		soap_timer.stop();
		stat_getstatus.push((double)soap_timer.get_milliseconds());
		skipped_tick = false;
		if (soaperr == SOAP_OK) {
#if DEBUG
			cout << "current tick: " << getStatus_resp.return_->currentTick << endl;
#endif
			soap_timer.stop();
			stat_getstatus.push((double)soap_timer.get_milliseconds());
			repeated_tick = true;
			if (lasttick != getStatus_resp.return_->currentTick) {
				//OK, we got a new tick.
				lasttick = state->tickno;
				state->tickno = getStatus_resp.return_->currentTick;
				repeated_tick = false;
				if (lasttick + 1 == state->tickno) {
					skipped_tick = false;
				} else {
					skipped_tick = true;
				}
			}
			if (!state_synced) {
				if (getStatus_resp.return_->playerName) {
					myname = (*getStatus_resp.return_->playerName);
#if DEBUG > 1
					cout << "my name: " << myname << endl;
#endif
				}
				//Re-init all the unit values
				for (i = 0; i < 4;i++) {
					state->tank[i].active = 0;
					state->bullet[i].active = 0;
				}
			}
			/* ignoring getStatus_resp.return_->nextTickTime hopefully */
			for (vector<ns1__player*>::iterator player = getStatus_resp.return_->players.begin(); player != getStatus_resp.return_->players.end(); ++player) {
				its_me = false;
#if DEBUG > 1
				cout << "Player: ";
#endif
				if ((*player)->name) {
#if DEBUG > 1
					cout << (*(*player)->name);
#endif
					if ( (*(*player)->name) == myname) {
						its_me = true;
					}
				} else {
					cerr << "Warning, received player without name!" << endl;
#if DEBUG > 1
					cout << "(anonymous)";
#endif
				}

				player_offset = its_me ? 0 : 2;

#if DEBUG > 1
				cout << endl;
#endif
				if (!state_synced && (*player)->base) {
#if DEBUG > 1
					cout << "-- base at (" << (*player)->base->x << "," << (*player)->base->y << ")" << endl;
#endif
					state->base[player_offset/2].x = (*player)->base->x;
					state->base[player_offset/2].y = (*player)->base->y;
				}

				num_recv = 0;

				for (vector<ns1__unit*>::iterator unit_iter = (*player)->units.begin(); unit_iter != (*player)->units.end(); ++unit_iter) {
					if ((*unit_iter)) {
						ns1__unit& unit = (*(*unit_iter));
#if DEBUG
						cout << "--" << "unit [" << unit.id << "] at (" << unit.x << "," << unit.y << ")";
						if (unit.direction) {
							cout << " o: " << (*unit.direction);
						}
						if (unit.action) {
							cout << " a: " << (*unit.action);
						}
						cout << endl;
#endif
						received_tanks[num_recv].id = unit.id;
						received_tanks[num_recv].x = unit.x;
						received_tanks[num_recv].y = unit.y;
						received_tanks[num_recv].active = 1;
						received_tanks[num_recv].tag = 0;
						received_tanks[num_recv].o = ntoh_direction(unit.direction);

						if (num_recv < 2) {
							num_recv++;
						} else {
							//assume that they might be sending more tanks; ignore them
							cerr << "Warning: Received more than two tanks for a player!" << endl;
							state_synced = false;
						}
					}
				}

				if (!state_synced) {
#if DEBUG > 1
					cout << "Syncing tanks" << endl;
#endif
					for (i = 0; i < num_recv; i++) {
						state->tank[i+player_offset] = received_tanks[i];
					}
				} else {
					// Clear active tanks
					for (i = 0; i < 2; i++) {
						state->tank[i+player_offset].active = 0;
					}
					check = 0;
					// Compare tank ID's, if they match, copy over new data
					for (i = 0; i < num_recv; i++) {
						for (j = 0; j < 2; j++) {
							if (state->tank[j+player_offset].id == received_tanks[i].id) {
								state->tank[j+player_offset] = received_tanks[i];
								check++;
							}
						}
					}
					if (check != num_recv) {
						cerr << "Warning, ID of tank suddenly changed!" << endl;
						for (i = 0; i < num_recv; i++) {
							state->tank[i+player_offset] = received_tanks[i];
						}
						state_synced = false;
					}
				}


				num_recv = 0;
				for (vector<ns1__bullet*>::iterator bullet_iter = (*player)->bullets.begin(); bullet_iter != (*player)->bullets.end(); ++bullet_iter) {
					ns1__bullet& bullet = (*(*bullet_iter));
#if DEBUG
					cout << "--" << " bullet [" << bullet.id << "] at (" << bullet.x << "," << bullet.y << ")";
					cout << " o: " << (*bullet.direction);
#endif
					received_bullets[num_recv].active = 1;
					received_bullets[num_recv].id = bullet.id;
					received_bullets[num_recv].x = bullet.x;
					received_bullets[num_recv].y = bullet.y;
					received_bullets[num_recv].o = ntoh_direction(bullet.direction);

					if (num_recv < 2) {
						num_recv++;
					} else {
						//assume that they might be sending more tanks; ignore them
						cerr << "Warning: Received more than two bullets for a player!" << endl;
						state_synced = false;
					}
#if DEBUG
					cout << endl;
#endif
				}

				if (!state_synced) {
#if DEBUG > 1
					cout << "Syncinc bullets" << endl;
#endif
					for (i = player_offset; i < 2+player_offset; i++) {
						//Unassociate bullets
						state->bullet[i].o = 0;
						state->bullet[i].x = 0;
						state->bullet[i].y = 0;
						state->bullet[i].id = INT_MAX;
					}
				}

				// Clear active bullets
				for (i = player_offset; i < 2+player_offset; i++) {
					state->bullet[i].active = 0;
				}

				check = 0;
				// Compare bullet ID's, if they match, copy over new data
				for (i = 0; i < num_recv; i++) {
					for (j = player_offset; j < 2+player_offset; j++) {
						if (state->bullet[j].id == received_bullets[i].id) {
							state->bullet[j] = received_bullets[i];
							received_bullets[i].active = 0;
							check++;
						}
					}
				}
				if (check < num_recv) {
					//There are new bullets on the field!
					if ((check == 1) && (num_recv == 2)) {
						//Special case: one bullet has already been tagged
						//We know who fired it then.
						if (received_bullets[0].active) {
							j = 0;
						} else {
							j = 1;
						}
						for (i = player_offset; i < 2+player_offset; i++) {
							if (!state->bullet[i].active) {
#if DEBUG > 1
								cout << "Associated bullet id(" << received_bullets[j].id << ") with tank id(" << state->tank[i].id <<") by elimination" << endl;
#endif
								state->bullet[i] = received_bullets[j];
							}
						}
					} else {
						//We have to backtrack
						for (i = 0; i < num_recv; i++) {
							int b_x,b_y,x,y,t;
							bool found = false;
							if (received_bullets[i].active) {
								b_x = received_bullets[i].x;
								b_y = received_bullets[i].y;
								switch (received_bullets[i].o) {
								case O_LEFT:
									for (x = (b_x+3); x < state->max_x-2 && !found; x++) {
										t = (x-(b_x+3))/2;
										for (y = max((b_y-t),2); y <= min((b_y+t),state->max_y-2) && !found;y++) {
											for (j = player_offset; j < player_offset+2 && !found; j++) {
												found = state->isTankAt(j,x,y);
											}
										}
									}
									break;
								case O_RIGHT:

									for (x = (b_x-3); x > 2 && !found; x--) {
										t = ((b_x-3)-x)/2;
										for (y = max((b_y-t),2); y <= min((b_y+t),state->max_y-2) && !found;y++) {
											for (j = player_offset; j < player_offset+2 && !found; j++) {
												found = state->isTankAt(j,x,y);
											}
										}
									}
									break;
								case O_DOWN:
									for (y = (b_y-3); y > 2 && !found; y--) {
										t = ((b_y-3)-y)/2;
										for (x = max((b_x-t),2); x <= min((b_x+t),state->max_x-2) && !found;x++) {
											for (j = player_offset; j < player_offset+2 && !found; j++) {
												found = state->isTankAt(j,x,y);
											}
										}
									}
									break;
								default:
								case O_UP:
									for (y = (b_y+3); y < state->max_y-2 && !found; y++) {
										t = (y-(b_y+3))/2;
										for (x = max((b_x-t),2); x <= min((b_x+t),state->max_x-2) && !found;x++) {
											for (j = player_offset; j < player_offset+2 && !found; j++) {
												found = state->isTankAt(j,x,y);
											}
										}
									}
									break;
								}
								if (found)
								{
									j--;
#if DEBUG > 1
									cout << "Associated bullet id(" << received_bullets[i].id << ") with tank id(" << state->tank[j].id <<") by backtracking" << endl;
#endif
									state->bullet[j] = received_bullets[i];
									received_bullets[i].active = 0;
								} else {
									//Orphaned bullet: look for an inactive tank
									for (j = player_offset; j < 2+player_offset; j++) {
										if ((!state->tank[j].active) && (!state->bullet[j].active)) {
#if DEBUG > 1
											cout << "Associated bullet id(" << received_bullets[i].id << ") with orphan" << endl;
#endif
											state->bullet[j] = received_bullets[i];
											received_bullets[i].active = 0;
											break;
										}
									}
								}
							}
						}
					}
				}

				for (i = 0; i < 2; i++) {
					if (!state->bullet[i].active) {
						state->bullet[i].id = INT_MAX;
					}
				}

#if DEBUG > 1
				cout << "-=end Player=-" << endl;
#endif
			}

			if (!state_synced) {
				pair<int,int> tank_and_id;
				vector< pair<int,int> > tanks_and_ids(4);
				for (i = 0; i < 4; i++) {
					tank_and_id.first = i;
					tank_and_id.second = state->tank[i].id;
					tanks_and_ids[i] = tank_and_id;
				}
				sort(tanks_and_ids.begin(), tanks_and_ids.end(), sort_pair_second<int, int>());
				for (i = 0; i < 4; i++) {
					state->tank_priority[i] = tanks_and_ids[i].first;
				}
			}

			if (getStatus_resp.return_->events) {
#if DEBUG > 1
				cout << "Events:" << endl;
#endif
				for (vector<ns1__blockEvent*>::iterator block_event_iter = getStatus_resp.return_->events->blockEvents.begin(); block_event_iter != getStatus_resp.return_->events->blockEvents.end(); ++block_event_iter) {
					ns1__blockEvent &block_event = *(*block_event_iter);
#if DEBUG > 1
					cout << "-- block";
					cout << " at (" << block_event.point->x << "," << block_event.point->y << ")";
					cout << " new state " << (*block_event.newState);
					cout << endl;
#endif
					if (block_event.point && block_event.newState) {
						switch (*block_event.newState) {
						case ns1__state__FULL:
							//WTF, walls suddenly appeared?!?!?
							state->board[block_event.point->x][block_event.point->y] |= B_WALL;
							break;
						case ns1__state__NONE:
						case ns1__state__EMPTY:
							//Walls got destroyed
							state->board[block_event.point->x][block_event.point->y] |= B_WALL;
							state->board[block_event.point->x][block_event.point->y] ^= B_WALL;
							break;
						case ns1__state__OUT_USCOREOF_USCOREBOUNDS:
							//Endgame
							state->board[block_event.point->x][block_event.point->y] |= B_OOB;
							break;
						}
					}
				}
#if 0
				//For now, ignore unit events that happened in the past.
				for (vector<ns1__unitEvent*>::iterator unit_event = getStatus_resp.return_->events->unitEvents.begin(); unit_event != getStatus_resp.return_->events->unitEvents.end(); ++unit_event) {
					cout << "-- event @(" << (*unit_event)->tickTime << ")";
					if ((*unit_event)->unit) {
						ns1__unit& unit = (*(*unit_event)->unit);
						cout << " {unit [" << unit.id << "] at (" << unit.x << "," << unit.y << ")";
						if (unit.direction) {
							cout << " o: " << (*unit.direction);
						}
						if (unit.action) {
							cout << " a: " << (*unit.action);
						}
						cout << "}";
					}
					if ((*unit_event)->bullet) {
						ns1__bullet& bullet = (*(*unit_event)->bullet);
						cout << " {bullet [" << bullet.id << "] at (" << bullet.x << "," << bullet.y << ")";
						if (bullet.direction) {
							cout << " o: " << (*bullet.direction);
						}
						cout << "}";
					}
					cout << endl;
				}
#endif
#if DEBUG > 1
				cout << "-=end Events=-" << endl;
#endif
			} //if events

			state->updateCanFire();
			state->updateSimpleUtilityScores();
			state->updateExpensiveUtilityScores();
			//TODO: check for cases when state is NOT synced!
			if (!state_synced) {
				mc_tree->init(state);
			} else {
				mc_tree->init(state);
			}
			state_synced = true;
#if SAVESTARTMAP
			if (firstrun) {
				firstrun = false;
				ofstream fout("btboard.map");
				fout << state;
			}
#endif

#if DEBUG > 1
			cout << "milliseconds to next tick: " << getStatus_resp.return_->millisecondsToNextTick << endl;
#endif
			nexttick = getStatus_resp.return_->millisecondsToNextTick;
			if (nexttick < 0) {
				skipped_tick = false;
				repeated_tick = true;
			} else {
				if (nexttick < safety_margin) {
					repeated_tick = false;
					skipped_tick = true;
				}
			}
		} else {
			repeated_tick = false;
			skipped_tick = false;
			s.soap_stream_fault(std::cerr);
			s.destroy();
			cout << "looping back" << endl;
			continue;
		}
		s.destroy();
		comms_timer.stop();

#if DEBUG > 1
		cout << "GetStatus [" << soap_timer.get_milliseconds() << " ms] [avg: " << stat_getstatus.mean() << " ms]" << endl;
		cout << "full processing time was [" << comms_timer.get_milliseconds() << " ms] " << endl;
#endif
		if (repeated_tick) {
			//Wait a little bit longer next time;
			settle_time += settle_time/4;
#if DEBUG > 1
			cout << "Repeated tick!" << endl;
			cout << "settle_time now: " << settle_time << endl;
#endif
			Sleep((uint32_t)settle_time/2);
			continue;
		}
		if (skipped_tick) {
			if (nexttick < safety_margin) {
				settle_time -= settle_time/4;
#if DEBUG > 1
				cout << "Missed a tick!" << endl;
				cout << "settle_time now: " << settle_time << endl;
#endif
				if (nexttick+settle_time > 0) {
					Sleep((uint32_t)nexttick+settle_time);
				}
				continue;
			}
		}

		//At this point, we should be synced, settled and have a window to do work
		int64_t window = nexttick-comms_timer.get_milliseconds()-safety_margin;
#if DEBUG > 1
		cout << "Expecting next tick at [" << nexttick << "-" << comms_timer.get_milliseconds() << "-" << safety_margin << " ms] = " << window  << " ms"<< endl;
#endif
		if (soaperr == SOAP_OK) {
			ai_timer.restart();
			loop_timer.restart();
			int64_t looptime;
			int64_t sleeptime;
			int tankid;
			ns1__setAction setAction_req;
			ns1__setActionResponse setAction_resp;
			ns1__setActions setActions_req;
			ns1__setActionsResponse setActions_resp;
			ns1__action action[2];
			tree_size_t node_id;
			vector<Move> path;
			vector<double> results;
			unsigned char width = 3;
			unsigned int alpha;
			int greedycmd[2];
#if DEBUG
			PlayoutState* paintme = new PlayoutState(state);
			paintme->drawBases();
			paintme->drawBullets();
			paintme->drawTanks();
			paintme->paint();
			delete paintme;
#endif
			sleeptime = safety_margin;

			switch (policy) {
			case POLICY_MCTS:
			case POLICY_GREEDY:
				//state->paint(*u);
				greedycmd[0] = C_NONE;
				greedycmd[1] = C_NONE;
				for (tankid = 0; tankid < 2; tankid++) {
					if (state->tank[tankid].active) {
#if DEBUG
						cout << "Costs [" << tankid << "]:";
#endif
						int bestcmd = C_FIRE;
						int bestcost = INT_MAX;
						int cost;
						for (int c = 0; c < 6; c++) {
							cost = state->cmdToExpensiveUtility(c,tankid);
#if DEBUG
							cout << " " << cmd2str(c) << ": " << cost;
#endif
							if (cost < bestcost) {
								bestcost = cost;
								bestcmd = c;
							}
						}
						action[tankid] = hton_cmd(bestcmd);
						if (tankid < 2) {
							greedycmd[tankid] = bestcmd;
						}
#if DEBUG
						cout << endl;
#endif
					}
				}
#if HALFLIMP
				if ((state->tank[2].active+state->tank[3].active) == 0 &&
						(state->tank[0].active+state->tank[1].active) == 2) {
#if DEBUG
					if (policy == POLICY_GREEDY) {
						cout << "GOING HALF-LIMP!" << endl;
					}
#endif
					action[1] = ns1__action__NONE;
				}
#endif

#if AREYOUNUTS
				if (state->tickno > 55) {
					cout << "GOING LIMP!" << endl;
					action[0] = ns1__action__NONE;
					action[1] = ns1__action__NONE;
				}
#endif

#if DEBUG
				cout << "Greedy chose";
				for (i = 0; i < 2; i++) {
					if (state->tank[i].active) {
						cout << " tank[" << i << "]: " << action2str(action[i]);
					}
				}
				cout << endl;
#endif
				if (policy == POLICY_GREEDY) {
					break;
				}

#if 0
				cout << "Root node ordering:" << endl;
				for (i = 0; i < 4; i++) {
					cout << "tank[" << i << "]{";
					for (j = 0; j < 6; j++) {
						cout << " " << cmd2str(mc_tree->tree[mc_tree->root_id].cmd_order[i][j]);
					}
					cout << " }" << endl;
				}
#endif
				loop_timer.stop();
				looptime = loop_timer.get_microseconds();
				loop_timer.restart();
				uint32_t linear;
				while (looptime < (window-50)*1000) {
					linear = sfmt_genrand_uint32(mc_tree->worker_sfmt[0]) % 10000;
					if (linear > 8500) {
						width = 2;
					} else if (linear > 100) {
						width = 3;
					} else if (linear > 10) {
						width = 4;
					} else {
						width = 5;
					}
					path.clear();
					results.clear();
					memcpy(node_state,mc_tree->root_state,sizeof(PlayoutState));
					node_id = mc_tree->root_id;
					mc_tree->select(width,path,node_id,node_state);
					mc_tree->expand_some(width,node_id,node_state,path,results);
					mc_tree->backprop(path,results);
					loop_timer.stop();
					looptime += loop_timer.get_microseconds();
					loop_timer.restart();
				}
				alpha = mc_tree->best_alpha(C_TO_ALPHA(greedycmd[0],greedycmd[1]));

				action[0] = hton_cmd(C_T0(alpha,0));
				action[1] = hton_cmd(C_T1(alpha,0));
#if DEBUG
				cout << "After MCTS: ";
				for (i = 0; i < 2; i++) {
					if (state->tank[i].active) {
						cout << " tank[" << i << "]: " << action2str(action[i]);
					}
				}
				cout << endl;
#endif
#if HALFLIMP
				if ((state->tank[2].active+state->tank[3].active) == 0 &&
						(state->tank[0].active+state->tank[1].active) == 2) {
#if DEBUG
					cout << "GOING HALF-LIMP!" << endl;
#endif
					action[1] = ns1__action__NONE;
				}
#endif
				break;
			default:
			case POLICY_RANDOM:
				for (tankid = 0; tankid < 2; tankid++) {
					action[tankid] = static_cast<ns1__action> (rand() % 6);
				}
				break;
			case POLICY_FIXED:
				for (tankid = 0; tankid < 2; tankid++) {
					if (state->tank[tankid].active) {
						int myid = (myname == "Player Two");
						if (state->tickno < NUMC) {
							action[tankid] = hton_cmd(fixed_commands[myid][tankid][state->tickno]);
						}
					}
				}
				break;
			}
			loop_timer.stop();
			ai_timer.stop();
			if (ai_timer.get_milliseconds() < window) {
				window -= ai_timer.get_milliseconds();
			} else {
				cerr << "OVERRAN WINDOW!" << endl;
			}
#if DEBUG > 1
			cout << "AI took " << ai_timer.get_milliseconds() << " ms, shortening the window to: " << window << endl;
			cout << "AI chose";
			for (i = 0; i < 2; i++) {
				if (state->tank[i].active) {
					cout << " tank[" << i << "]: " << action2str(action[i]);
				}
			}
			cout << endl;
#endif
			if (window > 0) {
				Sleep((uint32_t)(window));
			}

#if DEBUG > 1
			cout << "Finished delaying, next tick should be imminent!" << endl;
#endif
			for (tankid = 0; tankid < 2; tankid++) {
				if (state->tank[tankid].active) {
					setAction_req.arg0 = state->tank[tankid].id;
					setAction_req.arg1 = &action[tankid];
					soap_timer.restart();
					soaperr = s.setAction(&setAction_req,&setAction_resp);
					soap_timer.stop();
					stat_setaction.push((double)soap_timer.get_milliseconds());
					if (soaperr == SOAP_OK) {
						sleeptime = setAction_resp.return_->millisecondsToNextTick;
					} else {
						s.soap_stream_fault(std::cerr);
					}
					s.destroy();
				}
			}
#if DEBUG > 1
			cout << "Last SetAction [" << soap_timer.get_milliseconds() << " ms] [avg: " << stat_setaction.mean() << " ms]" << endl;
			cout << "Waiting for end of turn in [" << sleeptime << " ms]" << endl;
#endif
			//From here on it's pondering time!
			if (sleeptime+settle_time > 0 && (sleeptime < safety_margin)) {
#if DEBUG > 1
				cout << "setAction was sent in time, waiting for " <<sleeptime+settle_time <<" ms" << endl;
#endif
				Sleep((uint32_t)sleeptime+settle_time);
			} else {
				if (sleeptime > (2*safety_margin)) {
					safety_margin += safety_margin/4;
#if DEBUG > 1
					cout << "setAction was not sent in time!" << endl;
					cout << "Increasing safety_margin to: " << safety_margin << endl;
#endif
				} else {
#if DEBUG > 1
					cout << "setAction appears to be sent in time, waiting for " << safety_margin+settle_time << " ms" << endl;
#endif
					Sleep((uint32_t)safety_margin+settle_time);
				}
			}
		}
	}
	delete mc_tree;
	delete node_state;
}
