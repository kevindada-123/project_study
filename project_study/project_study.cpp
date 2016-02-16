#include "yen_ksp.hpp"

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/utility.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <iostream>
#include <list>
#include <string>
#include <fstream>
#include <utility>
#include <map>
#include <tuple>

using namespace boost;


//vertex & edge properties
typedef property<vertex_name_t, std::string> vertex_properties;
typedef property < edge_weight_t, int, property<edge_capacity_t, int, 
	property<edge_residual_capacity_t, int> > > edge_properties;

//graph typedef
typedef adjacency_list< vecS, vecS, bidirectionalS,	vertex_properties, edge_properties >
	graph_t;

//vertex & edge descriptor
typedef graph_traits<graph_t>::vertex_descriptor vertex_t;
typedef graph_traits<graph_t>::edge_descriptor edge_t;

//property map typedef
typedef property_map<graph_t, vertex_name_t>::type vertex_name_map_t;
typedef property_map<graph_t, edge_weight_t>::type edge_weight_map_t;
typedef property_map<graph_t, edge_capacity_t>::type edge_capacity_map_t;
typedef property_map<graph_t, edge_residual_capacity_t>::type edge_residual_capacity_map_t;



template<typename graph_t>
void construct_graph(std::ifstream &file_in, graph_t &g)
{
	//property map
	vertex_name_map_t name_map = get(vertex_name, g);	
	edge_weight_map_t weight_map = get(edge_weight, g);	
	edge_capacity_map_t capacity_map = get(edge_capacity, g);
	edge_residual_capacity_map_t residual_capacity_map = get(edge_residual_capacity, g);
	
	typename property_traits<vertex_name_map_t>::value_type s, t;
	typename property_traits<edge_weight_map_t>::value_type weight;
	typename property_traits<edge_capacity_map_t>::value_type capacity;
	typename property_traits<edge_residual_capacity_map_t>::value_type residual_capacity;
	typedef std::map<std::string, vertex_t> name_map_t;
	name_map_t vertex_name_map;

	for (std::string line; std::getline(file_in, line);)
	{
		std::istringstream(line) >> s >> t >> weight >> capacity;

		name_map_t::iterator pos;
		bool inserted;
		vertex_t u, v;
		tie(pos, inserted) = vertex_name_map.insert(std::make_pair(s, vertex_t()));
		if (inserted)
		{
			u = add_vertex(g);
			name_map[u] = s;
			pos->second = u;
		}
		else
			u = pos->second;

		tie(pos, inserted) = vertex_name_map.insert(std::make_pair(t, vertex_t()));
		if (inserted)
		{
			v = add_vertex(g);
			name_map[v] = t;
			pos->second = v;
		}
		else
			v = pos->second;

		//add edge
		edge_t e;
		tie(e, inserted) = add_edge(u, v, g);
		if (inserted)
		{
			weight_map[e] = weight;
			capacity_map[e] = capacity;
			residual_capacity = capacity;
		}


	}


}


int main()
{
	std::ifstream file_in("graph_input.txt");

	if (!file_in)
	{
		std::cerr << "No graph_input.txt file!!" << std::endl;
		return EXIT_FAILURE;
	}

	graph_t g;

	//property map
	vertex_name_map_t name_map = get(vertex_name, g);
	edge_weight_map_t weight_map = get(edge_weight, g);
	edge_capacity_map_t capacity_map = get(edge_capacity, g);
	edge_residual_capacity_map_t residual_capacity_map = get(edge_residual_capacity, g);

	//construct the graph
	construct_graph(file_in, g);

	//k-shortest test
	typedef std::list<edge_t> Path;
	std::list<std::pair<int, Path>> r;
	vertex_t src = *(vertices(g).first);
	vertex_t dst = *(vertices(g).first + 10);
	r = yen_ksp(g, src, dst, 3);


	//k-shortest path output
	int k = 1;
	for (auto k_path : r)
	{
		std::cout << k << " path: " << " weight " << k_path.first << ", ";
		std::cout << get(name_map, src) << " ";
		for (auto v : k_path.second)
		{
			std::cout << get(name_map, target(v, g)) << " ";
		}
		++k;
		std::cout << std::endl;
	}


	return 0;
}

