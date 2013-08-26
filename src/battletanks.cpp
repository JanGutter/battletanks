//============================================================================
// Name        : battletanks.cpp
// Author      : Jan Gutter
// Version     :
// Copyright   : Copyright (C) 2013 Jan Gutter, All Rights Reserved.
// Description : Battletanks
//============================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <string.h>
#include <stdlib.h>
#include "consts.h"
#include "CalcEquilibrium.h"
#include "PlayoutState.h"
#include "NetworkCore.h"
#include <tinythread.h>
#include "MCTree.h"
#include <time.h>
#include <SFMT.h>
#include <iomanip>
#include <platformstl/performance/performance_counter.hpp>

using namespace std;
#define MODE_SOAP 1
#define MODE_BENCHMARK 2

int main(int argc, char** argv) {
	int mode = MODE_SOAP;
	const char* soap_endpoint = "http://localhost:9090/ChallengePort";

	cout << "Hardware concurrency: " << tthread::thread::hardware_concurrency() << endl;

	if (argc == 2) {
		soap_endpoint = argv[1]; //Use 1st argument as default;
		if (strcmp(argv[1],"0") == 0) {
			soap_endpoint = "http://localhost:7070/Challenge/ChallengeService";
		}
		if (strcmp(argv[1],"1") == 0) {
			soap_endpoint = "http://localhost:7071/Challenge/ChallengeService";
		}
		if (strcmp(argv[1],"benchmark") == 0) {
			mode = MODE_BENCHMARK;
		}
	}
	MCTree *mc_tree = new MCTree;
	if (mode == MODE_SOAP) {

		cout << "Network Play using SOAP: [" << soap_endpoint << "]" << endl;
		NetworkCore* netcore = new NetworkCore(soap_endpoint);
		netcore->policy = POLICY_GREEDY;
		netcore->login();
		netcore->play();
		delete netcore;

	} else if (mode == MODE_BENCHMARK) {
		int i;
		platformstl::performance_counter utility_timer;
		UtilityScores* u = new UtilityScores;
		tree_size_t node_id;
		PlayoutState node_state;
		vector<Move> path;
		vector<double> results;

		ifstream fin("board1.map");
		fin >> node_state;
		node_state.endgame_tick = 200;
		node_state.gameover = 0;
		fin.close();
		mc_tree->init(node_state);

		//cout << mc_tree->root_state;
		cout << "Populating utility scores..." << endl;
		utility_timer.restart();
		mc_tree->root_state.populateUtilityScores(*u);
		utility_timer.stop();
		cout << "Utility scores populated! [" << utility_timer.get_milliseconds() << " ms]"<<endl;
#if 0
		int j;
		int maxcost = 0;
		cout << "Down:" << endl;
		for (j = 0; j < mc_tree->root_state.max_y; j++) {
			for (i = 0; i < mc_tree->root_state.max_x; i++) {
				if (u->cost[0][i][j][O_DOWN] < INT_MAX) {
					maxcost = max(maxcost,u->cost[0][i][j][O_DOWN]);
				}
			}
		}

		for (j = 0; j < mc_tree->root_state.max_y; j++) {
			for (i = 0; i < mc_tree->root_state.max_x; i++) {
				if (u->cost[0][i][j][O_DOWN] < INT_MAX) {
					if (u->cost[0][i][j][O_DOWN] == 0) {
						cout << "*";
					} else {
						cout << ((u->cost[0][i][j][O_DOWN]-1)*10/maxcost);
					}
				} else {
					if (mc_tree->root_state.board[i][j] & B_WALL) {
						cout << "#";
					} else {
						cout << " ";
					}
				}
			}
			cout << endl;
		}
		cout << endl;

		cout << "Left:" <<endl;
		for (j = 0; j < mc_tree->root_state.max_y; j++) {
			for (i = 0; i < mc_tree->root_state.max_x; i++) {
				if (u->cost[0][i][j][O_LEFT] < INT_MAX) {
					maxcost = max(maxcost,u->cost[0][i][j][O_LEFT]);
				}
			}
		}

		for (j = 0; j < mc_tree->root_state.max_y; j++) {
			for (i = 0; i < mc_tree->root_state.max_x; i++) {
				if (u->cost[0][i][j][O_LEFT] < INT_MAX) {
					if (u->cost[0][i][j][O_LEFT] == 0) {
						cout << "*";
					} else {
						cout << ((u->cost[0][i][j][O_LEFT]-1)*10/maxcost);
					}
				} else {
					if (mc_tree->root_state.board[i][j] & B_WALL) {
						cout << "#";
					} else {
						cout << " ";
					}
				}
			}
			cout << endl;
		}
		cout << endl;
#endif
		for (i = 0; i < 50; i++) {
			path.clear();
			results.clear();
			node_state = mc_tree->root_state;
			node_id = mc_tree->root_id;
			mc_tree->select(path,node_id,node_state);
			mc_tree->expand_all(node_id,node_state,path,results);
			mc_tree->backprop(path,results);
		}
#if 0
		for (i = 0; i < 36; i++) {
			for (j = 0; j < 36; j++) {
				cout << "(" << i << "," << j<< ") " <<  mc_tree->allocated_count[i][j] << "/" << mc_tree->allocated[i][j].size() << " [" << *(mc_tree->allocated[i][j].begin()) << "]" << endl;
			}
		}
#endif
		cout << "Root " << mc_tree->tree[mc_tree->root_id].r.mean() << "/" << mc_tree->tree[mc_tree->root_id].r.variance() << "/" << mc_tree->tree[mc_tree->root_id].r.count() << endl;

		delete u;
	}

	delete mc_tree;
	cout << "Done!" << endl;
	return 0;
}
