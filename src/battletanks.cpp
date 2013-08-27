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
#define BENCHMARK 0

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
		int i,width;
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
#endif
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
#if BENCHMARK
		cout << "Populating utility scores..." << endl;
		utility_timer.restart();
#endif
		node_state.populateUtilityScores(*u);
#if BENCHMARK
		utility_timer.stop();
		utility_stat.push((double)utility_timer.get_milliseconds());
		cout << "Utility scores populated! [" << utility_timer.get_milliseconds() << " ms]"<<endl;
#endif
		mc_tree->init(node_state,*u);
		//cout << mc_tree->root_state;
		width = 2;
		cout << "width: " << width << endl;
		for (i = 0; i < 500; i++) {
			path.clear();
			results.clear();
			node_state = mc_tree->root_state;
			node_id = mc_tree->root_id;
#if BENCHMARK
			cout << ">Select ";
			select_timer.restart();
#endif
			mc_tree->select(width,path,node_id,node_state);
#if BENCHMARK
			select_timer.stop();
			select_stat.push((double)select_timer.get_milliseconds());
			cout << "mean: " << select_stat.mean() << " ms" << endl;
			cout << ">Populate ";
			utility_timer.restart();
#endif
			node_state.populateUtilityScores(*u);
#if BENCHMARK
			utility_timer.stop();
			utility_stat.push((double)utility_timer.get_milliseconds());
			cout << "mean: " << utility_stat.mean() << " ms" << endl;
			cout << ">Expand ";
			expand_timer.restart();
#endif
			mc_tree->expand_some(width,node_id,node_state,*u,path,results);
#if BENCHMARK
			expand_timer.stop();
			expand_stat.push((double)expand_timer.get_milliseconds());
			cout << "mean: " << expand_stat.mean() << " ms" << endl;
			cout << ">Backprop ";
			backprop_timer.restart();
#endif
			mc_tree->backprop(path,results);
#if BENCHMARK
			backprop_timer.stop();
			backprop_stat.push((double)backprop_timer.get_milliseconds());
			cout << "mean: " << backprop_stat.mean() << " ms" << endl;
#endif
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
