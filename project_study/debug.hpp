#ifndef DEEUG_H
#define DEBUG_H


#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/property_map.hpp>
#include "online_path_computation.hpp"

namespace boost
{

	//k-shortest path output
	template<typename Graph, typename WeightMap, typename Request>
	void k_path_print(Graph& graph, Request request, WeightMap& weight_map, int path_num)
	{

		auto name_map = get(vertex_name, graph);

		auto r = k_shortest_path(graph, request, weight_map, path_num);
		int k = 1;
		for (auto k_path : r)
		{
			std::cout << k << " path: " << " weight " << k_path.first << ", ";
			std::cout << get(name_map, request.src) << " ";
			for (auto v : k_path.second)
			{
				std::cout << get(name_map, target(v, graph)) << " ";
			}
			++k;
			std::cout << std::endl;
		}

	}







}//boost

#endif //DEBUG_H