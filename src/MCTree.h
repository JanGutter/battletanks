/*
 * MCTree.h
 *
 *  Created on: 17 Aug 2013
 *      Author: Jan Gutter
 */

#ifndef MCTREE_H_
#define MCTREE_H_

#include "consts.h"
#include <StatCounter.hpp>
#include "PlayoutState.h"
#include <list>
#include <vector>
#include <math.h>
#include <tinythread.h>
#include <SFMT.h>

using namespace std;

typedef unsigned long int tree_size_t;

class Node;

const unsigned int MAXTHREADS = 8;
#define SUBNODE_COUNT (36*36)
#define TASK_RING_SIZE (2048)
#define RESULT_RING_SIZE (2048)
#define THREADID_UNEXPLORED 0
#define THREADID_PRUNED 1

class MCTree;

struct expand_thread_param_t {
	MCTree* mc_tree;
	unsigned int threadid;
};

struct expand_task_t {
	tree_size_t* child_ptr;
	PlayoutState* parent_state;
	int alpha;
	int beta;
};

struct expand_result_t {
	int alpha;
	int beta;
};

class MCTree {
public:
	PlayoutState root_state;
	UtilityScores root_u;
	tree_size_t root_id;
	tree_size_t tree_size;
	Node* tree;

	unsigned int num_workers;

	tthread::mutex task_mutex;
	tthread::condition_variable tasks_available;
	expand_task_t tasks[TASK_RING_SIZE];
	unsigned int task_first;
	unsigned int task_last;

	bool workers_keepalive;
	int workers_running;
	tthread::condition_variable workers_finished;
	int workers_awake;
	tthread::condition_variable workers_quit;

	tthread::mutex task_result_mutex;
	tthread::condition_variable results_available;
	expand_result_t task_results[RESULT_RING_SIZE];
	unsigned int task_result_first;
	unsigned int task_result_last;

	vector<PlayoutState> child_state;
	vector<sfmt_t> worker_sfmt;
	tthread::thread* expand_worker[MAXTHREADS];

	list <tree_size_t> unallocated;
	tree_size_t unallocated_count; //Workaround for O(n) complexity list.size()
	list <tree_size_t> allocated[36][36];
	tree_size_t allocated_count[36][36]; //Workaround for O(n) complexity list.size()
	list <tree_size_t> allocated_to_root;
	unsigned int num_results();
	unsigned int best_alpha();
	void handle_task(int taskid, int threadid);
	void post_result(int alpha, int beta);
	bool tasks_empty();
	void init(PlayoutState& reference_state,UtilityScores& reference_u);
	void reset(PlayoutState& reference_state,UtilityScores& reference_u);
	void select(unsigned char width,vector<Move>& path, tree_size_t& node_id, PlayoutState& node_state);
	void redistribute(vector<Move>& path);
	void backprop(vector<Move>& path, vector<double>& result);
	void expand_all(tree_size_t node_id, PlayoutState& node_state, UtilityScores& u, vector<Move>& path, vector<double>& results);
	void expand_some(unsigned char width, tree_size_t node_id, PlayoutState& node_state, UtilityScores& u, vector<Move>& path, vector<double>& results);
	MCTree();
	virtual ~MCTree();
};

class Node {
public:
	StatCounter r;
	bool terminal;
	//THE FOLLOWING ELEMENTS ARE INVALID IF r.count() == 0
	tree_size_t child[36][36];
	unsigned char cmd_order[4][6];
	unsigned char expanded_to;
	int alpha(MCTree& tree);
	int beta(MCTree& tree);
	void print(MCTree& tree);
};


#endif /* MCTREE_H_ */
