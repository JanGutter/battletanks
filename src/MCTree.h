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

class MCTree;

struct expand_thread_param_t {
	MCTree* mc_tree;
	unsigned int threadid;
};

struct expand_workqueue_t {
	tree_size_t* child_ptr;
	PlayoutState* parent_state;
	int alpha;
	int beta;
};

class MCTree {
public:
	PlayoutState root_state;
	tree_size_t root_id;
	tree_size_t tree_size;
	Node* tree;

	unsigned int num_workers;

	tthread::mutex workqueue_mutex;
	tthread::condition_variable start_workers;
	vector<bool> work_available;
	bool workers_keepalive;
	vector<expand_workqueue_t*> expand_workqueue;

	tthread::mutex finished_mutex;
	tthread::condition_variable finished_work;
	vector<bool> work_completed;

	vector<PlayoutState> child_state;
	vector<sfmt_t> worker_sfmt;
	tthread::thread* expand_worker[MAXTHREADS];

	list <tree_size_t> unallocated;
	tree_size_t unallocated_count; //Workaround for O(n) complexity list.size()
	list <tree_size_t> allocated[36][36];
	tree_size_t allocated_count[36][36]; //Workaround for O(n) complexity list.size()
	list <tree_size_t> allocated_to_root;
	void init(PlayoutState& reference_state);
	void select(vector<Move>& path, tree_size_t& node_id, PlayoutState& node_state);
	void redistribute(vector<Move>& path);
	void backprop(vector<Move>& path, vector<double>& result);
	void expand_all(tree_size_t node_id, PlayoutState& node_state, vector<Move>& path, vector<double>& results);
	MCTree();
	virtual ~MCTree();
};

class Node {
public:
	StatCounter r;
	bool terminal;
	tree_size_t child[36][36];
	int alpha(MCTree& tree);
	int beta(MCTree& tree);
};


#endif /* MCTREE_H_ */
