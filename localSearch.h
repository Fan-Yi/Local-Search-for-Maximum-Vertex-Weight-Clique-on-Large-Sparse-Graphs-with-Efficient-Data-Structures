#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include <cstdlib>
#include <sys/times.h>
#include  <stdio.h>

#include "graph.h"
#include "myBijection.h"
#include "vertexLinkedQueue.h"
#include "hugeInt.h"
#include "operandSets.h"
#include "constants.h"

//#define strategy_analysis_mode

#define age_init_i_mode

//#define vertex_stamp_queue_mode
#define random_construction_mode

class StochasticLocalSearch : private Graph{
private:
	ScoreAgePickSet *ptr_to_weighted_clique;
	int clique_weight;

	Bijection *ptr_to_clique_neighbors;
	int *connect_clique_degree;

	int *score;
	HugeInt *time_stamp;
	int *confChange;

	HugeInt step;
	HugeInt step_bound;
	int time_limit;
	tms start, finish;
	int start_time;

	int *best_weighted_clique;
	int best_weighted_clique_size;
	int best_clique_weight;
	double best_cmp_time;
	HugeInt best_solve_step;

	ConfchangedScoreAgePickSet *ptr_to_add_set;
	ConfchangedScoreAgePickSet *ptr_to_swap_set;
	int *swap_partner;
#ifdef vertex_stamp_queue_mode
	VertexLinkedQueue *ptr_to_vertex_stamp_queue;
#endif


#ifdef strategy_analysis_mode
int empty_clique_occur_num;
int ans_update_times;
#endif

	int check_solution()
	{
		int i, j;
		int verified_weight_sum = 0;
		for(i = 1; i <= best_weighted_clique_size; i++)
		{
			verified_weight_sum += vertices[best_weighted_clique[i]].get_weight();
			for(j = i + 1; j <= best_weighted_clique_size; j++)
			{
				if(!isConnected(best_weighted_clique[i], best_weighted_clique[j]))
				{
					cout << best_weighted_clique[i] << " and " << best_weighted_clique[j] << " is not connected" << endl;
					cout << "false clique" << endl;
					return 0;
				}
			}
		}
		if(verified_weight_sum != best_clique_weight)
		{
			cout << "the total weight is computed incorrectly" << endl;
			return 0;
		}
		return 1;
	}

	void unlock_all_neighbors(int v)
	{
		int i;
		int* nbs = vertices[v].get_neighbors();
		int dgr = vertices[v].get_degree();
		for(i = 0; i < dgr; i++)
			confChange[nbs[i]] = 1;
	}

	void initGreedyConstructionRandomly()
	{
		int v;
#ifdef vertex_stamp_queue_mode
		v = ptr_to_vertex_stamp_queue->get_head_v();
#else
		v = rand() % v_num + 1;
#endif
		add(v);	
		unlock_all_neighbors(v);
#ifdef random_construction_mode
		while(ptr_to_add_set->size())
		{
			int rand_v = ptr_to_add_set->rand_element();
			add(rand_v);
			unlock_all_neighbors(rand_v);
			step++;
		}
		if(clique_weight > best_clique_weight)// update best solution
		{
			update_best_weighted_clique();
			times(&finish);
			best_cmp_time = double((finish.tms_utime + finish.tms_stime - start_time)) / sysconf(_SC_CLK_TCK);
			best_solve_step = step;
		}
#endif
	}

	void update_best_weighted_clique()
	{
		for(int i = 1; i <= ptr_to_weighted_clique->size(); i++)
		{
			best_weighted_clique[i] = ptr_to_weighted_clique->at(i);
		}
		best_weighted_clique_size = ptr_to_weighted_clique->size();
		best_clique_weight = clique_weight;
#ifdef strategy_analysis_mode
ans_update_times++;
#endif
	}

	void drop(int u)
	{
//cout << "drop " << u << endl;
		ptr_to_weighted_clique->delete_element(u);
		clique_weight -= vertices[u].get_weight();
		int *nbs = vertices[u].get_neighbors();
		int dgr = vertices[u].get_degree();
		int i, j;
		for(i = 0; i < dgr; i++)
		{
			int n = nbs[i];
			if(ptr_to_weighted_clique->element_in(n)) continue;
			connect_clique_degree[n]--;
			if(connect_clique_degree[n] == 0)
			{ 
				ptr_to_clique_neighbors->delete_element(n);//deleting it from the set of clique neighbors may not be enough
				if(ptr_to_weighted_clique->size() == 1)
				{
					ptr_to_swap_set->delete_element(n);
				}
				else if(ptr_to_weighted_clique->size() == 0)
				{
					ptr_to_add_set->delete_element(n);//before n must be in the add-set; equivalent to clear all the elements
				}
			}
		}
		if(ptr_to_weighted_clique->size() != 0)//when drop(u) does not lead to an empty clique
		{
			ptr_to_add_set->insert_element(u);
			ptr_to_clique_neighbors->insert_element(u);
		}
		connect_clique_degree[u] = ptr_to_weighted_clique->size();
		for(i = 1; i <= ptr_to_clique_neighbors->size(); i++)
		{
			int v = ptr_to_clique_neighbors->at(i);		
			if(isConnected(v, u) || v == u) continue;// if v and u are neighbors, the state of v should be unchanged; 
																						//before in add-set, later as well; before in swap-set, later as well; exceptions are handled above
			// weighted clique size is decreased by 1
			if(connect_clique_degree[v] == ptr_to_weighted_clique->size())//for any vertex with connect-clique-degree == clique.size, move it from swap-set to add-set
			{
					ptr_to_add_set->insert_element(v);// add to add-set
					ptr_to_swap_set->delete_element(v);
			}
			else if(connect_clique_degree[v] == ptr_to_weighted_clique->size() - 1)//for any vertex with connect-clique-degree == clique.size - 1, put it into swap-set
			{
				// add to swap-set
				for(j = 1; j <= ptr_to_weighted_clique->size(); j++)
				{
					if(!isConnected(v, ptr_to_weighted_clique->at(j)))
						break;
				}
				int w = ptr_to_weighted_clique->at(j);
				score[v] = vertices[v].get_weight() - vertices[w].get_weight();
				ptr_to_swap_set->insert_element(v);
				swap_partner[v] = w;
			}
		}
#ifdef vertex_stamp_queue_mode
		ptr_to_vertex_stamp_queue->insertVertexNodeToQueue(u);
#endif
		time_stamp[u] = step;
	}

	void add(int v)
	{
//cout << "add " << v << endl;
		ptr_to_weighted_clique->insert_element(v);
		clique_weight += vertices[v].get_weight();
		int *nbs = vertices[v].get_neighbors();
		int dgr = vertices[v].get_degree();
		int i;
		for(i = 0; i < dgr; i++)
		{
			int n = nbs[i];
			if(ptr_to_weighted_clique->element_in(n)) continue;
			connect_clique_degree[n]++;
			if(connect_clique_degree[n] == 1)	//the first time identified as a clique neighbor
			{
				ptr_to_clique_neighbors->insert_element(n);
				if(ptr_to_weighted_clique->size() == 1)// the neighbors of v should be used to establish add-set
				{
					ptr_to_add_set->insert_element(n);// equivalent to put all neighbors into add-set
				}
				else if(ptr_to_weighted_clique->size() == 2)// the neighbors of v should be put into the swap-set
				{
					int z;
					if(ptr_to_weighted_clique->at(1) == v)
					{
						z = ptr_to_weighted_clique->at(2);
					}
					else
					{
						z = ptr_to_weighted_clique->at(1);
					}
					swap_partner[n] = z;
					score[n] = vertices[n].get_weight() - vertices[z].get_weight();
					ptr_to_swap_set->insert_element(n);
				}
			}
		}
		//the first vertex in C comes neither from swap-set nor add-set
		if(ptr_to_weighted_clique->size() != 1)// not adding the first vertex
		{
			ptr_to_clique_neighbors->delete_element(v);
		}
		if(ptr_to_add_set->element_in(v))//if it comes from add-set
			ptr_to_add_set->delete_element(v);
		else if(ptr_to_swap_set->element_in(v))
			ptr_to_swap_set->delete_element(v);
		for(i = 1; i <= ptr_to_clique_neighbors->size(); i++)
		{
			int w = ptr_to_clique_neighbors->at(i);
			if(isConnected(w, v) || w == v) continue;// if v and u are neighbors, the state of v should be unchanged; 
																						//before in add-set, later as well; before in swap-set, later as well; exceptions are handled above
			if(connect_clique_degree[w] == ptr_to_weighted_clique->size() - 1)
			{			
				ptr_to_add_set->delete_element(w);//delete from add-set	
				score[w] = vertices[w].get_weight() - vertices[v].get_weight();
				ptr_to_swap_set->insert_element(w);
				swap_partner[w] = v;
			}
			else if(connect_clique_degree[w] == ptr_to_weighted_clique->size() - 2)
			{
				ptr_to_swap_set->delete_element(w);
			}
		}
#ifdef vertex_stamp_queue_mode
		ptr_to_vertex_stamp_queue->deleteVertexNode(v);
#endif
		time_stamp[v] = step;
	}

	void swap(int u, int v)// assuming that u is in Clique, but v is not in Clique
	{
		drop(u);
		add(v);
	}

	void local_move()
	{
		if(!ptr_to_weighted_clique->size())// adding the first vertex
		{
#ifdef strategy_analysis_mode
empty_clique_occur_num++;
#endif
			initGreedyConstructionRandomly();
		}
/////////////////////////////////////////////////////////////////////////
			int i, j;
			int best_add_v = 0;
			best_add_v = ptr_to_add_set->best_element(confChange, vertices, time_stamp);
			int best_swap_v = 0;
			best_swap_v = ptr_to_swap_set->best_element(confChange, score, time_stamp);

			if(best_add_v != 0)// succeeds in obtaining such a vertex
			{
				if(best_swap_v == 0 || (vertices[best_add_v].get_weight() > score[best_swap_v]))
				{
					add(best_add_v);
					unlock_all_neighbors(best_add_v);
				}
				else
				{
					swap(swap_partner[best_swap_v], best_swap_v);
					confChange[swap_partner[best_swap_v]] = 0;
				}
			}
			else// fail to obtain a suitable vertex in the add-set
			{
				int best_remove_v;
				best_remove_v = ptr_to_weighted_clique->best_element(vertices, time_stamp);	
				if(best_swap_v == 0 || -vertices[best_remove_v].get_weight() > score[best_swap_v])
				{
					drop(best_remove_v);
					confChange[best_remove_v] = 0;
				}
				else
				{
					swap(swap_partner[best_swap_v], best_swap_v);
					confChange[swap_partner[best_swap_v]] = 0;
				}
			}
		step++;
		if(clique_weight > best_clique_weight)// update best solution
		{
			update_best_weighted_clique();
			times(&finish);
			best_cmp_time = double((finish.tms_utime + finish.tms_stime - start_time)) / sysconf(_SC_CLK_TCK);
			best_solve_step = step;
		}
	}

	void clear()
	{
		while(ptr_to_weighted_clique->size())
			drop(ptr_to_weighted_clique->at(ptr_to_weighted_clique->begin()));
	}

public:
	StochasticLocalSearch(char *filename, int cut_off) : Graph(filename)
	{
		step = 0;
		step_bound = 0;
		time_limit = cut_off;
		times(&start);
		start_time = start.tms_utime + start.tms_stime;

		ptr_to_weighted_clique = new ScoreAgePickSet(v_num);
		clique_weight = 0;

		ptr_to_clique_neighbors = new Bijection(v_num);

		connect_clique_degree = new int[v_num + 2];
		memset(connect_clique_degree, 0, sizeof(int) * (v_num + 2));

		score = new int[v_num + 2];
		memset(score, 0, sizeof(int) * (v_num + 2));

		time_stamp = new HugeInt[v_num + 2];

		for(int v = 1; v <= v_num; v++)
			time_stamp[v] = -v_num - 1 + v;

		confChange = new int[v_num + 2];
		for(int v = 1; v <= v_num; v++)
		{
			confChange[v] = 1;
		}

		best_weighted_clique = new int[max_degree + 1 + 2];
		best_weighted_clique_size = 0;
		best_clique_weight = 0;

		ptr_to_add_set = new ConfchangedScoreAgePickSet(v_num);
		ptr_to_swap_set = new ConfchangedScoreAgePickSet(v_num);
		swap_partner = new int[v_num + 2];
		memset(swap_partner, 0, sizeof(int) * (v_num + 2));
#ifdef vertex_stamp_queue_mode
		ptr_to_vertex_stamp_queue = new VertexLinkedQueue(v_num);
		for(int v = 1; v <= v_num; v++)
		{
			ptr_to_vertex_stamp_queue->insertVertexNodeToQueue(v);
		}
#endif

#ifdef strategy_analysis_mode
empty_clique_occur_num = 0;
ans_update_times = 0;
#endif
	}

	~StochasticLocalSearch()
	{
		delete ptr_to_weighted_clique;
		delete ptr_to_clique_neighbors;
		delete[] connect_clique_degree;
		delete[] time_stamp;
		delete[] score;
		delete[] confChange;
		delete[] best_weighted_clique;
		delete ptr_to_add_set;
		delete ptr_to_swap_set;
		delete[] swap_partner;
#ifdef vertex_stamp_queue_mode
		delete ptr_to_vertex_stamp_queue;
#endif
	}

	void clique_sls()
	{
		while(1)
		{
				if(step % TRY_STEP == 0)
				{
					times(&finish);
					double elap_time = double(finish.tms_utime + finish.tms_stime - start_time) / sysconf(_SC_CLK_TCK);
					if(elap_time >= time_limit) return;
				}
				local_move();
//show_state();
//getchar();
		}
	}

	void show_results()
	{
		times(&finish);
		double elap_time = double(finish.tms_utime + finish.tms_stime - start_time) / sysconf(_SC_CLK_TCK);
		if(check_solution())
		{
			cout << "o " << best_clique_weight << endl;
			cout << "c size " << best_weighted_clique_size << endl;
			cout << "c solveTime " << best_cmp_time << endl;
			cout << "c searchSteps " << best_solve_step << endl;
			cout << "c stepSpeed(/ms) " << step / 1000.0 / elap_time << endl;
#ifdef strategy_analysis_mode
cout << "the total number of steps: " << step << endl;
cout << "the number of try is: " << (long long)(step / double(SEARCH_DEPTH) )+ 1 << endl;
cout << "empty clique occur num: " << empty_clique_occur_num << endl;
cout << "answer update times: " << ans_update_times << endl;
#endif
		}
		else
		{
			cout << "sorry, something is wrong" << endl;
		}
	}	

	void show_state()
	{
		int v, e;
/*
		for(e = 0; e < e_num; e++)
		{
			cout << edges[e].get_v1() << " " << edges[e].get_v2() << endl;
		}

		for(v = 1; v <= v_num; v++)
		{
			cout << "vertex " << v << ": " << endl;
			vertices[v].show_neighbors();
		}
*/
		int i, j;
		cout << "step: " << step << endl;
		cout << "the clique: " << endl;
		for(i = 1; i <= ptr_to_weighted_clique->size(); i++)
		{
			cout << ptr_to_weighted_clique->at(i) << '\t';
		}
		cout << endl;
		
		for(i = 1; i <= ptr_to_weighted_clique->size(); i++)
		{
			for(j = i + 1; j <= ptr_to_weighted_clique->size(); j++)
			{
				if(!isConnected(ptr_to_weighted_clique->at(i), ptr_to_weighted_clique->at(j)))
				{
					cout << ptr_to_weighted_clique->at(i) << " and " << ptr_to_weighted_clique->at(j) << " is not connected" << endl;
					cout << "false clique" << endl;
					exit(1);
				}
			}
		}
		cout << "clique weight: " << clique_weight << endl;
		cout << "weighted clique size: " << ptr_to_weighted_clique->size() << endl;
		cout << "swap pairs:" << endl;
		for(v = 1; v <= v_num; v++)
		{
			if(ptr_to_swap_set->element_in(v))
				cout << "outside: " << v << " and inside: " << swap_partner[v] << endl;
		}
		cout << "clique neighbors: " << endl;
		for(i = 1; i <= ptr_to_clique_neighbors->size(); i++)
		{
			cout << ptr_to_clique_neighbors->at(i) << '\t';
		}
		cout << endl;
		cout << "connect clique degrees: " << endl;
		for(i = 1; i <= ptr_to_clique_neighbors->size(); i++)
		{
			cout << connect_clique_degree[ptr_to_clique_neighbors->at(i)] << '\t';
		}
		cout << endl;
		cout << "add-set: " << endl;
		for(i = 1; i <= ptr_to_add_set->size(); i++)
		{
			cout << ptr_to_add_set->at(i) << '\t';
		}
		cout << endl;

cout << "**************************************" << endl;
	}
};
#endif
