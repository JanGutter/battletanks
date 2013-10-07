//============================================================================
// Name        : battletanks.cpp
// Author      : Jan Gutter
// Copyright   : To the extent possible under law, Jan Gutter has waived all
//             : copyright and related or neighboring rights to this work.
//             : For more information, go to:
//             : http://creativecommons.org/publicdomain/zero/1.0/
//             : or consult the README and COPYING files
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
#define BENCHMARK 1
#define DEBUG 0
#define MODE_SOAP 1
#define MODE_BENCHMARK 2
#define MODE_SELFPLAY 3
#define MODE_SHOWPATH 4

int main(int argc, char** argv) {
	int mode = MODE_SOAP;
	const char* soap_endpoint = "http://localhost:9090/ChallengePort";
#if DEBUG
	cerr << "Hardware concurrency: " << tthread::thread::hardware_concurrency() << endl;
#endif
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
		if (strcmp(argv[1],"selfplay") == 0) {
			mode = MODE_SELFPLAY;
		}
		if (strcmp(argv[1],"showpath") == 0) {
			mode = MODE_SHOWPATH;
		}
	}
	if (mode == MODE_SOAP) {
		cout << "Network Play using SOAP: [" << soap_endpoint << "]" << endl;
		NetworkCore* netcore = new NetworkCore(soap_endpoint);
		netcore->policy = POLICY_MCTS;
		netcore->login();
		netcore->play();
		delete netcore;
	} else if (mode == MODE_BENCHMARK) {
		MCTree *mc_tree = new MCTree;
		PlayoutState* node_state = new PlayoutState;
		vector<Move> path;
		int i,width;
		uint32_t linear;
		platformstl::performance_counter overall_timer;
#if BENCHMARK
		platformstl::performance_counter utility_timer;
		platformstl::performance_counter select_timer;
		platformstl::performance_counter backprop_timer;
		platformstl::performance_counter expand_timer;
		StatCounter select_stat;
		StatCounter utility_stat;
		StatCounter backprop_stat;
		StatCounter expand_stat;
		select_stat.init();
		utility_stat.init();
		backprop_stat.init();
		expand_stat.init();
		obstacles_t obstacles;
#endif
		overall_timer.restart();
		tree_size_t node_id;
		vector<double> results;
		int64_t looptime = 0;

		ifstream fin("board1.map");
		fin >> *node_state;
		node_state->endgame_tick = 200;
		node_state->gameover = false;
		node_state->stop_playout = false;
		fin.close();
#if BENCHMARK
		UtilityScores* u = new UtilityScores;
		cout << "Populating utility scores..." << endl;
		utility_timer.restart();
		node_state->updateCanFire();
		node_state->updateSimpleUtilityScores(*u,obstacles);
		node_state->updateExpensiveUtilityScores(*u,obstacles);
		utility_timer.stop();
		utility_stat.push((double)utility_timer.get_milliseconds());
		cout << "Utility scores populated! [" << utility_timer.get_milliseconds() << " ms]"<<endl;
		delete u;
#endif
		mc_tree->init(node_state);
		//cout << mc_tree->root_state;

		overall_timer.stop();
		looptime += overall_timer.get_microseconds();
		overall_timer.restart();
		for (i = 0; i < 10 || looptime < 2000000; i++) {
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
			//cout << "width: " << width << endl;
			path.clear();
			results.clear();
			memcpy(node_state,mc_tree->root_state,sizeof(PlayoutState));
			node_id = mc_tree->root_id;
#if BENCHMARK
			select_timer.restart();
#endif
			mc_tree->select(width,path,node_id,node_state);
#if BENCHMARK
			select_timer.stop();
			select_stat.push((double)select_timer.get_milliseconds());
			/*
			utility_timer.restart();
#endif
			node_state.populateUtilityScores(*u);
#if BENCHMARK
			utility_timer.stop();
			utility_stat.push((double)utility_timer.get_milliseconds());
			 */
			expand_timer.restart();
#endif
			mc_tree->expand_some(width,node_id,node_state,path,results);
#if BENCHMARK
			expand_timer.stop();
			expand_stat.push((double)expand_timer.get_milliseconds());
			backprop_timer.restart();
#endif
			mc_tree->backprop(path,results);
#if BENCHMARK
			backprop_timer.stop();
			backprop_stat.push((double)backprop_timer.get_milliseconds());
#endif

			overall_timer.stop();
			looptime += overall_timer.get_microseconds();
			overall_timer.restart();


		}
#if BENCHMARK
		cout << "Select mean: " << select_stat.mean() << " ms count: " << select_stat.count() << endl;
		//cout << "Populate mean: " << utility_stat.mean() << " ms count: " utility_stat.count() << endl;
		cout << "Expand mean: " << expand_stat.mean() << " ms count: " << expand_stat.count() << endl;
		cout << "Backprop mean: " << backprop_stat.mean() << " ms count: " << backprop_stat.count() << endl;
#endif
#if 0
		for (i = 0; i < 36; i++) {
			for (j = 0; j < 36; j++) {
				cout << "(" << i << "," << j<< ") " <<  mc_tree->allocated_count[i][j] << "/" << mc_tree->allocated[i][j].size() << " [" << *(mc_tree->allocated[i][j].begin()) << "]" << endl;
			}
		}
#endif
		cout << "Root " << mc_tree->tree[mc_tree->root_id].r.mean() << "/" << mc_tree->tree[mc_tree->root_id].r.variance() << "/" << mc_tree->tree[mc_tree->root_id].r.count() << endl;

		mc_tree->tree[mc_tree->root_id].print(*mc_tree);
		delete node_state;
		delete mc_tree;
	} else if (mode == MODE_SELFPLAY) {
		MCTree* mc_tree = new MCTree;
		PlayoutState* node_state = new PlayoutState;
		PlayoutState* tmp_state = new PlayoutState;
		vector<Move> path;
		int i;
		ifstream fin("board1.map");
		fin >> *node_state;
		node_state->endgame_tick = 200;
		node_state->gameover = false;
		node_state->stop_playout = false;
		fin.close();
		StatCounter playouts;
		playouts.init();
		mc_tree->init(node_state);
		mc_tree->root_state->paintUtilityScores(*mc_tree->root_u);
		for (i = 0; i < 50000; i++) {
			memcpy(tmp_state,mc_tree->root_state,sizeof(PlayoutState));
			double result = tmp_state->playout(mc_tree->worker_sfmt[0],*mc_tree->root_u);
			playouts.push(result);
			//cout << result << endl;
		}
		cerr << "Mean: " << playouts.mean() << " variance: " << playouts.variance() << endl;
		delete node_state;
		delete tmp_state;
		delete mc_tree;
	} else if (mode == MODE_SHOWPATH) {
		MCTree* mc_tree = new MCTree;
		PlayoutState* node_state = new PlayoutState;
		vector<Move> path;
		ifstream fin("board1.map");
		fin >> *node_state;
		node_state->endgame_tick = 200;
		node_state->gameover = false;
		node_state->stop_playout = false;
		fin.close();
		StatCounter playouts;
		playouts.init();
		mc_tree->init(node_state);
		mc_tree->root_state->paintUtilityScores(*mc_tree->root_u);
		double result = mc_tree->root_state->playout(mc_tree->worker_sfmt[0],*mc_tree->root_u);
		playouts.push(result);
		//cout << result << endl;
		delete node_state;
		delete mc_tree;
	}


	cerr << "Done!" << endl;
	return 0;
}
