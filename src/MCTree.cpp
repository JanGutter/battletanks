/*
 * MCTree.cpp
 *
 *  Created on: 17 Aug 2013
 *      Author: Jan Gutter
 */

#include "MCTree.h"
#include <tinythread.h>
#include <string.h>
#include <SFMT.h>
#include <time.h>
#include <stdlib.h>
#include <stack>
#include <utility>
#include <algorithm>
#include <functional>

#define DEBUG 0

inline double UCB1T_score_alpha(unsigned long int t_, double r_, double sigma_, double t)
{
	return r_ + sqrt(min(0.25,sigma_*sigma_+sqrt(2*log(t)/t_))*log(t)/t_);
}

inline double UCB1T_score_beta(unsigned long int t_, double r_, double sigma_, double t)
{
	return r_ - sqrt(min(0.25,sigma_*sigma_+sqrt(2*log(t)/t_))*log(t)/t_);
}

inline bool child_legalmove(tree_size_t child)
{
	return (child != THREADID_PRUNED);
}

inline bool child_unexplored(tree_size_t child)
{
	return (child == THREADID_UNEXPLORED);
}

inline bool child_explored(tree_size_t child)
{
	return (child > THREADID_PRUNED);
}

inline int Node::alpha(MCTree& tree)
{
	unsigned int t0,t1,t2,t3;
	int i,j;
	unsigned long int t_;
	double r_;
	double sigma_;
	double score;
	double maxscore = W_PLAYER1;
	int bestmove = -1;

	if (expanded_to < 2) {
		cout << "expanded_to < 2 !" << endl;
	}
	for (t1 = 0; t1 < expanded_to; t1++) {
		for (t0 = 0; t0 < expanded_to; t0++) {
			t_ = 0;
			r_ = 0.0;
			sigma_ = 0.0;
			i = t0+6*t1;
			for (t3 = 0; t3 < expanded_to; t3++) {
				for (t2 = 0; t2 < expanded_to; t2++) {
					j = t2+6*t3;
					//For all legal moves for P0 and P1 < width
					if (child_explored(child[i][j])) {
						Node& c = tree.tree[child[i][j]];
						t_ += c.r.count();
						if (r.mean() == 3.14) {
							cout << "Testing r.mean()" << endl;
						}
						r_ += c.r.count()*c.r.mean();
						sigma_ += c.r.count()*c.r.variance();
					}
				}
			}
			if (t_ > 0) {
				r_ /= t_;
				sigma_ /= t_;
				score = UCB1T_score_alpha(t_,r_,sigma_,r.count());
				if (score > maxscore) {
					maxscore = score;
					bestmove = i;
				}
			}
		}
	}
	return bestmove;
}

inline int Node::beta(MCTree& tree)
{
	unsigned int t0,t1,t2,t3;
	int i,j;
	unsigned long int t_;
	double r_;
	double sigma_;
	double score;
	double minscore = W_PLAYER0;
	int bestmove = -1;

	for (t3 = 0; t3 < expanded_to; t3++) {
		for (t2 = 0; t2 < expanded_to; t2++) {
			t_ = 0;
			r_ = 0.0;
			sigma_ = 0.0;
			j = t2+6*t3;
			for (t1 = 0; t1 < expanded_to; t1++) {
				for (t0 = 0; t0 < expanded_to; t0++) {
					//For all legal moves for P0 and P1 < width
					i = t0+6*t1;
					if (child_explored(child[i][j])) {
						Node& c = tree.tree[child[i][j]];
						t_ += c.r.count();
						r_ += c.r.count()*c.r.mean();
						sigma_ += c.r.count()*c.r.variance();
					}
				}
			}
			if (t_ > 0) {
				r_ /= t_;
				sigma_ /= t_;
				score = UCB1T_score_beta(t_,r_,sigma_,r.count());
				if (score < minscore) {
					minscore = score;
					bestmove = j;
				}
			}
		}
	}
	return bestmove;
}

void expand_subnodes(void* thread_param) {
	expand_thread_param_t* parameters = static_cast<expand_thread_param_t*>(thread_param);
	MCTree* mc_tree = parameters->mc_tree;
	unsigned int threadid = parameters->threadid;
	delete parameters;

	int command[4];
	int i;
#if DEBUG
	cout << "Thread (" << threadid << ") starting up" << endl;
#endif
	mc_tree->workqueue_mutex.lock();
	bool running = mc_tree->workers_keepalive;
	while (running) {
		if (!mc_tree->work_available[threadid]) {
#if DEBUG > 1
			cout << "Thread (" << threadid << ") going to sleep" << endl;
#endif
			mc_tree->start_workers.wait(mc_tree->workqueue_mutex);
#if DEBUG > 1
			cout << "Thread (" << threadid << ") waking up" << endl;
#endif
		} else {
#if DEBUG > 1
			cout << "Thread (" << threadid << ") work already in queue" << endl;
#endif
		}
		if (mc_tree->work_available[threadid]) {
			mc_tree->work_available[threadid] = false;
#if DEBUG > 1
			cout << "Thread (" << threadid << ") starting work" << endl;
#endif
			running = mc_tree->workers_keepalive;
			mc_tree->workqueue_mutex.unlock();
			if (!running) {
#if DEBUG > 1
				cout << "Thread (" << threadid << ") there's work, but no more keepalive" << endl;
#endif
				break;
			}
			expand_workqueue_t* workqueue = mc_tree->expand_workqueue[threadid];
			while (workqueue->child_ptr) {
				command[0] = C_T0(workqueue->alpha,workqueue->beta);
				command[1] = C_T1(workqueue->alpha,workqueue->beta);
				command[2] = C_T2(workqueue->alpha,workqueue->beta);
				command[3] = C_T3(workqueue->alpha,workqueue->beta);
#if DEBUG > 2
				cout << "Thread (" << threadid << ") getting work unit ["
						<< command[0] << "]["
						<< command[1] << "]["
						<< command[2] << "]["
						<< command[3] << "] ";
#endif
				for (i = 0; i < 4; i++) {
					//Prune illegal move
					if (!workqueue->parent_state->tank[i].active && command[i] != C_NONE) {
						(*workqueue->child_ptr) = THREADID_PRUNED;
#if DEBUG > 2
						cout << "illegal" << endl;
#endif
						break;
					}
					//Prune unnecessary move
					if (workqueue->parent_state->bullet[i].active && command[i] == C_FIRE) {
						(*workqueue->child_ptr) = THREADID_PRUNED;
#if DEBUG > 2
						cout << "redundant" << endl;
#endif
						break;
					}
				}
#if DEBUG > 2
				cout << endl;
#endif
#define ASSERT 1
#if ASSERT
				if (*workqueue->child_ptr == 0) {
					cout << "Something's wrong, child_ptr should never be 0!" << endl;
				}
#endif
				if (child_legalmove(*workqueue->child_ptr)) {
					memcpy(&mc_tree->child_state[threadid],workqueue->parent_state,sizeof(PlayoutState));
					memcpy(mc_tree->child_state[threadid].command,command,sizeof(command));
					mc_tree->child_state[threadid].simulateTick();
					if (mc_tree->child_state[threadid].gameover) {
						mc_tree->tree[*workqueue->child_ptr].terminal = true;
					} else {
						mc_tree->tree[*workqueue->child_ptr].terminal = false;
						mc_tree->child_state[threadid].playout(mc_tree->worker_sfmt[threadid]);
					}
					mc_tree->tree[*workqueue->child_ptr].expanded_to = 0;
					mc_tree->tree[*workqueue->child_ptr].r.init();
					mc_tree->tree[*workqueue->child_ptr].r.push(mc_tree->child_state[threadid].winner);
#if DEBUG > 2
					cout << "Thread (" << threadid << ") result: " << mc_tree->tree[*workqueue->child_ptr].r.mean() << " on node " << *workqueue->child_ptr << endl;
				} else {
					cout << "Thread (" << threadid << ") child_ptr == NULL" << endl;
#endif
				}
				workqueue++;
			}
			mc_tree->finished_mutex.lock();
			mc_tree->workers_busy--;
			if (!mc_tree->workers_busy) {
				mc_tree->finished_workers.notify_one();
			}
			mc_tree->finished_mutex.unlock();

			mc_tree->workqueue_mutex.lock();
			running = mc_tree->workers_keepalive;
#if DEBUG > 1
			cout << "Thread (" << threadid << ") work completed" << endl;
#endif
		} else {
#if DEBUG > 1
			cout << "Thread (" << threadid << ") woke up with no work!" << endl;
#endif
			running = mc_tree->workers_keepalive;
		}
	}
	mc_tree->workqueue_mutex.unlock();

	mc_tree->finished_mutex.lock();
	mc_tree->workers_busy--;
	if (!mc_tree->workers_busy) {
		mc_tree->finished_workers.notify_one();
	}
	mc_tree->finished_mutex.unlock();
#if DEBUG
	cout << "Thread (" << threadid << ") exiting" << endl;
#endif
}

void MCTree::expand_all(tree_size_t node_id, PlayoutState& node_state, UtilityScores& u, vector<Move>& path, vector<double>& results)
{
	expand_some(6,node_id,node_state,u,path,results);
}

void MCTree::expand_some(unsigned char width, tree_size_t node_id, PlayoutState& node_state, UtilityScores &u, vector<Move>& path, vector<double>& results)
{
	list<tree_size_t> new_children[36][36];

	unsigned int i,j,paramno,threadno;
	unsigned int t0,t1,t2,t3;

	if (unallocated_count < 36*36) {
		//TODO: Revert to playouts only
		cerr << "Ran out of tree!" << endl;
		return; //Silently fail
	}
#if DEBUG
	cout << "Expanding [" << node_id << "] to width: " << (int)width << endl;
#endif
	if (tree[node_id].r.count() == 1) {
		//This is the first time we're trying to expand this node. Set all children to UNEXPLORED;
		memset(&tree[node_id].child,0,sizeof(tree[node_id].child));
		//Calculate the move order.
		node_state.populateUtilityScores(u);
		pair<int,int> cmd_and_utility;
		vector< pair<int,int> > cmd_and_utilities(6);
		for (i = 0; i < 4; i++) {
			if (node_state.tank[i].active) {
				for (j = 0; j < 6; j++) {
					cmd_and_utility.first = j;
					cmd_and_utility.second = node_state.cmdToUtility(j,i,u);
					cmd_and_utilities[j] = cmd_and_utility;
				}
				sort(cmd_and_utilities.begin(), cmd_and_utilities.end(), sort_pair_second<int, int>());
				for (j = 0; j < 4; j++) {
					tree[node_id].cmd_order[i][j] = (unsigned char) cmd_and_utilities[j].first;
				}
			} else {
				for (j = 0; j < 6; j++) {
					tree[node_id].cmd_order[i][j] = j;
				}
			}
		}
	}

	threadno = 0;
	paramno = 0;
	for (t1 = 0; t1 < width; t1++) {
		for (t0 = 0; t0 < width; t0++) {
			for (t3 = 0; t3 < width; t3++) {
				for (t2 = 0; t2 < width; t2++) {
					i = t0+6*t1;
					j = t2+6*t3;
					if (child_unexplored(tree[node_id].child[i][j])) {
						new_children[i][j].splice(new_children[i][j].begin(),unallocated,unallocated.begin());
						tree[node_id].child[i][j] = new_children[i][j].front();
						expand_workqueue[threadno][paramno].child_ptr = &tree[node_id].child[i][j];
						expand_workqueue[threadno][paramno].alpha = i;
						expand_workqueue[threadno][paramno].beta = j;
						expand_workqueue[threadno][paramno].parent_state = &node_state;
						paramno += (threadno + 1) / num_workers;
						threadno = (threadno + 1) % num_workers;
					}
				}
			}
		}
	}

	for (i = 0; i < num_workers;i++) {
		expand_workqueue[threadno][paramno].child_ptr = NULL;
		paramno += (threadno + 1) / num_workers;
		threadno = (threadno + 1) % num_workers;
	}
#if DEBUG
	cout << "Waiting for workqueue mutex" << endl;
#endif
	workqueue_mutex.lock();
#if DEBUG
	cout << "Got workqueue mutex" << endl;
#endif
	for (i = 0; i < num_workers;i++) {
		work_available[i] = true;
	}
	workers_busy = num_workers;
	workqueue_mutex.unlock();
	start_workers.notify_all();

	finished_mutex.lock();
	while (workers_busy) {
		finished_workers.wait(finished_mutex);
	}
	finished_mutex.unlock();

	int& root_alpha = path[0].alpha;
	int& root_beta = path[0].beta;

	for (i = 0; i < num_workers; i++) {
		expand_workqueue_t* workqueue = expand_workqueue[i];
		while (workqueue->child_ptr) {
			if (child_legalmove(*(workqueue->child_ptr))) {
				//the worker returned a valid move and leads to a new leaf
				//the node is now allocated to the first alpha/beta move
				//in the chain.
				allocated[root_alpha][root_beta].splice(
						allocated[root_alpha][root_beta].begin(),
						new_children[workqueue->alpha][workqueue->beta],
						new_children[workqueue->alpha][workqueue->beta].begin());
				unallocated_count--;
				allocated_count[root_alpha][root_beta]++;
				//store results for backprop
				results.push_back(tree[*(workqueue->child_ptr)].r.mean());
			} else {
				//worker returned pruned move, send the node back to unallocated
				unallocated.splice(unallocated.begin(),
						new_children[workqueue->alpha][workqueue->beta],
						new_children[workqueue->alpha][workqueue->beta].begin());
			}
			workqueue++;
		}
	}

	tree[node_id].expanded_to = max(width,tree[node_id].expanded_to);
#if DEBUG
	cout << "[" << node_id << "] expanded to: " << (int)tree[node_id].expanded_to << endl;
#endif
}

void MCTree::select(unsigned char width, vector<Move>& path, tree_size_t& node_id, PlayoutState& node_state)
{
	Move m;
#if DEBUG
	cout << "Select at node: " << node_id << endl;
#endif
	while (tree[node_id].expanded_to >= width && !tree[node_id].terminal) {
		m.alpha = tree[node_id].alpha(*this);
		m.beta = tree[node_id].beta(*this);
#if DEBUG
		cout << "UCB1Tuned returned alpha:" << m.alpha << " beta:" << m.beta << endl;
#endif
		if (m.alpha != -1 && m.beta != -1 && child_explored(tree[node_id].child[m.alpha][m.beta])) {
			path.push_back(m);
			node_id = tree[node_id].child[m.alpha][m.beta];
			node_state.move(m);
		} else {
			cerr << "Oops, select alpha/beta is invalid! [" << node_id << "]" << endl;
			break;
		}
#if DEBUG
		cout << "Select at node [" << node_id << "] with c:" << tree[node_id].r.count() << endl;
#endif
	}
#if DEBUG
	cout << "[" << node_id << "] c:" << tree[node_id].r.count() << " terminal:" << tree[node_id].terminal << endl;
#endif
}

void MCTree::redistribute(vector<Move>& path)
{
	int i,j,root_alpha,root_beta;

	//int new_total = allocated_count[alpha][beta];
	//allocated_to_root.splice(allocated_to_root.begin(),allocated[alpha][beta]);
	allocated[path[0].alpha][path[0].beta].clear();
	//allocated_count[alpha][beta] = 0;

	for (i = 0; i < 36; i++) {
		for (j = 0; j < 36; j++) {
			unallocated.splice(unallocated.begin(),allocated[i][j]);
			unallocated_count += allocated_count[i][j];
			allocated_count[i][j] = 0;

		}
	}

	for (root_alpha = 0; root_alpha < 36; root_alpha++) {
		for (root_beta = 0; root_beta < 36; root_beta++) {
			stack<tree_size_t> frontier;
			tree_size_t current;
			if (tree[root_id].child[root_alpha][root_beta]) {
				frontier.push(tree[root_id].child[root_alpha][root_beta]);
			}
			while (frontier.size() > 0) {
				current = frontier.top();
				frontier.pop();
				allocated[root_alpha][root_beta].push_back(current);
				allocated_count[root_alpha][root_beta]++;
				if (tree[current].r.count() > 1 && !tree[current].terminal) {
					for (i = 0; i < 36; i++) {
						for (j = 0; j < 36; j++) {
							if (tree[current].child[i][j]) {
								frontier.push(tree[current].child[i][j]);
							}
						}
					}
				}
			}
		}
	}
}

void MCTree::backprop(vector<Move>& path,vector<double>& result)
{
	tree_size_t node = root_id;
	for (vector<double>::iterator result_iter = result.begin(); result_iter != result.end(); ++result_iter) {
		tree[node].r.push(*result_iter);
	}
	for (vector<Move>::iterator move_iter = path.begin(); move_iter != path.end(); ++move_iter) {
		node = tree[node].child[(*move_iter).alpha][(*move_iter).beta];
		for (vector<double>::iterator result_iter = result.begin(); result_iter != result.end(); ++result_iter) {
			tree[node].r.push(*result_iter);
		}
	}
}

void MCTree::init(PlayoutState& reference_state, UtilityScores& reference_u)
{
	vector<Move> path;
	vector<double> results;
	sfmt_t sfmt;
	Move zero;
	PlayoutState tmp_playout;
	unsigned int i,j;

	unallocated.clear();
	unallocated_count = tree_size-2; //0 is reserved and 1 belongs to root
	for (i = 2; i < tree_size; i++) {
		unallocated.push_back(i);
	}
	memset(allocated_count,0,sizeof(allocated_count));
	for (i = 0; i < 36; i++) {
		for (j = 0; j < 36; j++) {
			allocated[i][j].clear();
		}
	}

	root_id = 1;
	allocated_to_root.push_back(root_id);
	memcpy(&root_state,&reference_state,sizeof(root_state));

	root_state = reference_state;
	root_state.drawBases();
	root_state.drawTanks();
	root_state.drawBullets();
	memcpy(&tmp_playout,&root_state,sizeof(tmp_playout));
	sfmt_init_gen_rand(&sfmt, (uint32_t)(time(NULL)));
	tree[root_id].r.init();
	tree[root_id].r.push(tmp_playout.playout(sfmt));
	tree[root_id].terminal = false;
	zero.alpha = 0;
	zero.beta = 0;
	path.push_back(zero);
	tree[root_id].expanded_to = 0;
	//no need to select when priming root
	expand_all(root_id,root_state,reference_u,path,results);
	//only necessary to redistribute when priming root or promoting a node to root
	redistribute(path);
	//Only backprop to root!
	path.clear();
	backprop(path,results);

}

MCTree::MCTree()
{
	tree_size_t i;

	//TODO: figure out a good value for tree_size
	//tree_size = 100000l;
	tree_size = 100000l;
	tree = new Node[tree_size];
	unallocated_count = tree_size-2; //0 is reserved and 1 belongs to root
	for (i = 2; i < tree_size; i++) {
		unallocated.push_back(i);
	}
	memset(allocated_count,0,sizeof(allocated_count));

	root_id = 1;
	allocated_to_root.push_back(root_id);
	num_workers = min(tthread::thread::hardware_concurrency(),MAXTHREADS);
	workers_keepalive = true;
	srand((unsigned int)(time(NULL)));
	//workqueue_mutex.lock();
	child_state.resize(num_workers);
	workers_busy = 0;
	work_available.resize(num_workers,false);
	//workqueue_mutex.unlock();
	for (i = 0; i < num_workers; i++) {
		expand_thread_param_t* expand_param = new expand_thread_param_t;
		expand_param->threadid = i;
		expand_param->mc_tree = this;
		sfmt_t sfmt;
		sfmt_init_gen_rand(&sfmt, rand());
		worker_sfmt.push_back(sfmt);
		expand_workqueue.push_back(new expand_workqueue_t[(SUBNODE_COUNT-1)/num_workers + 2]);
		expand_worker[i] = new tthread::thread(expand_subnodes,expand_param);
	}
}

MCTree::~MCTree()
{
	tree_size_t i;
	delete[] tree;
	workqueue_mutex.lock();
	for (i = 0; i < num_workers;i++) {
		work_available[i] = true;
	}

	workers_busy = num_workers;
	workers_keepalive = false;
	workqueue_mutex.unlock();
	start_workers.notify_all();

	finished_mutex.lock();
	while (workers_busy) {
		finished_workers.wait(finished_mutex);
	}
	finished_mutex.unlock();

	for (i = 0; i < num_workers; i++) {
		expand_worker[i]->join();
		delete expand_worker[i];
		delete[] expand_workqueue[i];
	}
}
