#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include <unordered_set>
#include <math.h>

using namespace std;

typedef long long LL;

class Vertex
{
private:
	int *neighbors;
	int *adj_edges;
	int degree;
	int weight;
public:
	Vertex()
	{
		neighbors = NULL;
		adj_edges = NULL;
		degree = 0;
		weight = 0;
	}
	~Vertex()
	{
		free_neighborhood_space();
	}
	void allocate_neighborhood_space(int dgr)
	{
		neighbors = new int[dgr];
		adj_edges = new int[dgr];
	}
	void free_neighborhood_space()
	{
		delete[] neighbors;
		delete[] adj_edges;
	}
	void add_neighbor(int name, int index)
	{
		neighbors[index] = name;
	}
	void add_adj_edge(int name, int index)
	{
		adj_edges[index] = name;
	}
	int *get_neighbors()
	{
		return neighbors;
	}
	int *get_adj_edges()
	{
		return adj_edges;
	}
	void set_degree(int d)
	{
		degree = d;
	}
	int get_degree()
	{
		return degree;
	}
	int get_weight()
	{
		return weight;
	}
	void set_weight(int w)
	{
		weight = w;
	}
	void show_neighbors()
	{
		cout << "neighbors: ";
		for(int i = 0; i < degree; i++)
		{
			cout << neighbors[i] << '\t';
		}
		cout << endl;
	}
};

class Edge
{
private:
	int v1, v2;
public:
	Edge(){}
	void set_vertices(int u1, int u2)
	{
		v1 = u1;
		v2 = u2;
	}
	void get_vertices(int &u1, int &u2)
	{
		u1 = v1;
		u2 = v2;
	}
	~Edge(){}
};

class Graph
{
protected:
	Vertex *vertices;
	Edge	*edges;
	int v_num;
	int e_num;
	int max_degree;
	int max_vertex_weight;
	int min_vertex_weight;
	unordered_set<LL> edge_hash_id_set;
private:
	void encode_pairID(LL &pairID, LL n1, LL n2)
	{
		pairID = ((n1 + n2 + 1) * (n1 + n2) >> 1) + n2;
	}
	void decode_pairID(LL pairID, LL &n1, LL &n2)
	{
		LL w = LL((sqrt(double((pairID << 3) + 1)) - 1) / 2);
		LL t = (w * w + w) >> 1;
		n2 = pairID - t;
		n1 = w - n2; 
	}
	void encode_unordered_pairID(LL &pairID, LL n1, LL n2)
	{
		LL u, v;
		if(n1 < n2)
		{
			u = n1; v = n2;
		}
		else
		{
			u = n2; v = n1;
		}
		encode_pairID(pairID, u, v);		
	}
	void insertEdgeHashIDToSet(int n1, int n2)
	{
		LL edge_hash_id;
		encode_unordered_pairID(edge_hash_id, LL(n1), LL(n2));
		edge_hash_id_set.insert(edge_hash_id);
	}
public:
	Graph(char *filename)
	{

		ifstream infile(filename);
		if(infile == NULL)
		{
			cout << "File " << filename << " cannot be opened" << endl;
			exit(1);
		}

		char line[1024];
		infile.getline(line, 1024);

		while(line[0] != 'p')
			infile.getline(line, 1024);

		char tempstr1[1024], tempstr2[1024];
		sscanf(line, "%s %s %d %d", tempstr1, tempstr2, &v_num, &e_num);

		if(strcmp(tempstr1, "p") != 0 || strcmp(tempstr2, "edge") != 0)
		{
			cout << "format error occurs in reading p lines" << endl;
			exit(1);
		}

		vertices = new Vertex[v_num + 2];
		edges = new Edge[e_num + 2];

		char ch_tmp;

		int v;
		for(v = 1; v <= v_num; v++)
		{
			int v_index;
			int v_weight;
			infile >> ch_tmp >> v_index >> v_weight;
			if(ch_tmp != 'v' || v_index != v)
			{
				cout << "format error occurs in reading v lines" << endl;
				exit(1);
			}
			vertices[v].set_weight(v_weight);
		}

		int v1, v2;
		int *v_degree_tmp = new int[v_num + 2];
		memset(v_degree_tmp, 0, sizeof(int) * (v_num + 2));

		int e;
		for(e = 0; e < e_num; e++)
		{
			infile >> ch_tmp >> v1 >> v2;
			edges[e].set_vertices(v1, v2);
			v_degree_tmp[v1]++;
			v_degree_tmp[v2]++;
			insertEdgeHashIDToSet(v1, v2);
		}

		for(v = 1; v <= v_num; v++)
		{
			vertices[v].allocate_neighborhood_space(v_degree_tmp[v]);
			vertices[v].set_degree(v_degree_tmp[v]);
		}
		max_degree = v_degree_tmp[1];
		for(int i = 2; i <= v_num; i++)
		{
			if(v_degree_tmp[i] > max_degree)
				max_degree = v_degree_tmp[i];
		}

		max_vertex_weight = vertices[1].get_weight();
		for(int i = 2; i <= v_num; i++)
		{
			if(vertices[i].get_weight() > max_vertex_weight)
				max_vertex_weight = vertices[i].get_weight();
		}
		min_vertex_weight = vertices[1].get_weight();
		for(int i = 2; i <= v_num; i++)
		{
			if(vertices[i].get_weight() < min_vertex_weight)
				min_vertex_weight = vertices[i].get_weight();
		}

		memset(v_degree_tmp, 0, sizeof(int) * (v_num + 2));
		for(e = 0; e < e_num; e++)
		{
			edges[e].get_vertices(v1, v2);
			vertices[v1].add_neighbor(v2, v_degree_tmp[v1]);
			vertices[v2].add_neighbor(v1, v_degree_tmp[v2]);
			vertices[v1].add_adj_edge(e, v_degree_tmp[v1]);
			vertices[v2].add_adj_edge(e, v_degree_tmp[v2]);
			v_degree_tmp[v1]++;
			v_degree_tmp[v2]++;
		}

		delete[] v_degree_tmp;
		infile.close();
	}
	Vertex* get_vertices()
	{
		return vertices;
	}
	int get_max_degree()
	{
		return max_degree;
	}
	int isConnected(int n1, int n2)
	{
		LL edge_hash_id;
		encode_unordered_pairID(edge_hash_id, LL(n1), LL(n2));
		return edge_hash_id_set.count(edge_hash_id);
	}
	~Graph()
	{
		delete[] vertices;
		delete[] edges;
	}
};
#endif
