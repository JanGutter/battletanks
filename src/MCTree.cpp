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

#define DEBUG 0

inline double UCB1T_score_alpha(unsigned long int t_, double r_, double sigma_, double t)
{
	return r_ + sqrt(min(0.25,sigma_*sigma_+sqrt(2*log(t)/t_))*log(t)/t_);
}

inline double UCB1T_score_beta(unsigned long int t_, double r_, double sigma_, double t)
{
	return r_ - sqrt(min(0.25,sigma_*sigma_+sqrt(2*log(t)/t_))*log(t)/t_);
}
inline int Node::alpha(MCTree& tree)
{
	int i,j;
	unsigned long int t_;
	double r_;
	double sigma_;
	double score;
	double maxscore = W_PLAYER1;
	int bestmove = -1;

	for (i = 0; i < 36; i++) {
		t_ = 0;
		r_ = 0.0;
		sigma_ = 0.0;
		for (j = 0; j < 36; j++) {
			//For all legal moves for P0 and P1
			if (child[i][j]) {
				Node& c = tree.tree[child[i][j]];
				t_ += c.r.count();
				r_ += c.r.count()*c.r.mean();
				sigma_ += c.r.count()*c.r.variance();
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
	return bestmove;
}

inline int Node::beta(MCTree& tree)
{
	int i,j;
	unsigned long int t_;
	double r_;
	double sigma_;
	double score;
	double minscore = W_PLAYER0;
	int bestmove = -1;

	for (j = 0; j < 36; j++) {
		t_ = 0;
		r_ = 0.0;
		sigma_ = 0.0;
		for (i = 0; i < 36; i++) {
			//For all legal moves for P0 and P1
			if (child[i][j]) {
				Node& c = tree.tree[child[i][j]];
				t_ += c.r.count();
				r_ += c.r.count()*c.r.mean();
				sigma_ += c.r.count()*c.r.variance();
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
		} else {
#if DEBUG > 1
			cout << "Thread (" << threadid << ") work already in queue" << endl;
#endif
		}
#if DEBUG > 1
		cout << "Thread (" << threadid << ") waking up" << endl;
#endif
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
						(*workqueue->child_ptr) = 0;
#if DEBUG > 2
						cout << "illegal" << endl;
#endif
						break;
					}
					//Prune unnecessary move
					if (workqueue->parent_state->bullet[i].active && command[i] == C_FIRE) {
						(*workqueue->child_ptr) = 0;
#if DEBUG > 2
						cout << "redundant" << endl;
#endif
						break;
					}
				}
#if DEBUG > 2
				cout << endl;
#endif
				if (*workqueue->child_ptr) {
					memcpy(&mc_tree->child_state[threadid],workqueue->parent_state,sizeof(PlayoutState));
					memcpy(mc_tree->child_state[threadid].command,command,sizeof(command));
					mc_tree->child_state[threadid].simulateTick();
					if (mc_tree->child_state[threadid].gameover) {
						mc_tree->tree[*workqueue->child_ptr].terminal = true;
					} else {
						mc_tree->tree[*workqueue->child_ptr].terminal = false;
						mc_tree->child_state[threadid].playout(mc_tree->worker_sfmt[threadid]);
					}
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
			mc_tree->work_completed[threadid] = true;
			mc_tree->finished_mutex.unlock();
			mc_tree->finished_work.notify_one();
			mc_tree->workqueue_mutex.lock();
			running = mc_tree->workers_keepalive;
#if DEBUG > 1
			cout << "Thread (" << threadid << ") work completed" << endl;
#endif
		} else {
			running = mc_tree->workers_keepalive;
		}
	}
	mc_tree->workqueue_mutex.unlock();

	mc_tree->finished_mutex.lock();
	mc_tree->work_completed[threadid] = true;
	mc_tree->finished_mutex.unlock();
	mc_tree->finished_work.notify_one();
#if DEBUG
	cout << "Thread (" << threadid << ") exiting" << endl;
#endif
}

void MCTree::expand(tree_size_t node_id, PlayoutState& node_state, vector<Move>& path, vector<double>& results)
{
	list<tree_size_t> new_children[36][36];

	unsigned int i,j,paramno,threadno;

	if (unallocated_count < 36*36) {
		//TODO: Revert to playouts only
		cout << "Ran out of tree!" << endl;
		return; //Silently fail
	}

	threadno = 0;
	paramno = 0;
	for (i = 0; i < 36; i++) {
		for (j = 0; j < 36; j++) {
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

	for (i = 0; i < num_workers;i++) {
		expand_workqueue[threadno][paramno].child_ptr = NULL;
		paramno += (threadno + 1) / num_workers;
		threadno = (threadno + 1) % num_workers;
	}

	workqueue_mutex.lock();
	for (i = 0; i < num_workers;i++) {
		work_completed[i] = false;
		work_available[i] = true;
	}
	workqueue_mutex.unlock();
	start_workers.notify_all();

	finished_mutex.lock();
	bool done = true;
	for (i = 0; i < num_workers;i++) {
		done = done && work_completed[i];
	}
	while (!done) {
		finished_work.wait(finished_mutex);
		done = true;
		for (i = 0; i < num_workers;i++) {
			done = done && work_completed[i];
		}
	}
	finished_mutex.unlock();

	for (i = 0; i < 36; i++) {
		for (j = 0; j < 36; j++) {
			if (tree[node_id].child[i][j]) {
				//[i][j] is a valid move and leads to a new leaf
				//the node is now allocated to the first alpha/beta move
				//in the chain.
				allocated[path[0].alpha][path[0].beta].splice(
						allocated[path[0].alpha][path[0].beta].begin(),
						new_children[i][j],
						new_children[i][j].begin());
				unallocated_count--;
				allocated_count[path[0].alpha][path[0].beta]++;
				//store results for backprop
				results.push_back(tree[tree[node_id].child[i][j]].r.mean());
			} else {
				//[i][j] is invalid, send the node back to unallocated
				unallocated.splice(unallocated.begin(),new_children[i][j],new_children[i][j].begin());
			}
		}
	}
}

void MCTree::select(vector<Move>& path, tree_size_t& node_id, PlayoutState& node_state)
{
	Move m;
	cout << "Select at node: " << node_id << endl;
	while (tree[node_id].r.count() > 1 && !tree[node_id].terminal) {
		m.alpha = tree[node_id].alpha(*this);
		m.beta = tree[node_id].beta(*this);
		cout << "UCB1Tuned returned alpha:" << m.alpha << " beta:" << m.beta << endl;
		if (m.alpha != -1 && m.beta != -1 && tree[node_id].child[m.alpha][m.beta]) {
			path.push_back(m);
			node_id = tree[node_id].child[m.alpha][m.beta];
			node_state.move(m);
		} else {
			cout << "Oops, select alpha/beta is invalid!" << endl;
			break;
		}
		cout << "Select at node [" << node_id << "] with c:" << tree[node_id].r.count() << endl;
	}
	cout << "[" << node_id << "] c:" << tree[node_id].r.count() << " t" << tree[node_id].terminal << endl;
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

MCTree::MCTree()
{
	tree_size_t i;

	//TODO: figure out a good value for tree_size
	//tree_size = 100000l;
	tree_size = 200000l;
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
	work_completed.resize(4,false);
	work_available.resize(4,false);
	child_state.resize(4);
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
		work_completed[i] = false;
	}
	workers_keepalive = false;
	workqueue_mutex.unlock();
	start_workers.notify_all();

	finished_mutex.lock();
	bool done = true;
	for (i = 0; i < num_workers;i++) {
		done = done && work_completed[i];
	}
	while (!done) {
		finished_work.wait(finished_mutex);
		done = true;
		for (i = 0; i < num_workers;i++) {
			done = done && work_completed[i];
		}
	}
	finished_mutex.unlock();

	for (i = 0; i < num_workers; i++) {
		expand_worker[i]->join();
		delete expand_worker[i];
		delete[] expand_workqueue[i];
	}
}
