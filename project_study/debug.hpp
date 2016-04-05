#ifndef DEEUG_H
#define DEBUG_H

#include <iomanip>
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

	template<typename Graph, typename BitMaskMap>
	void bit_mask_print(Graph graph, const std::vector<std::vector<int>>& bit_mask, BitMaskMap& bit_mask_map)
	{
		using EdgeIter = typename graph_traits<Graph>::edge_iterator;
		EdgeIter edge_iter, edge_end;
		tie(edge_iter, edge_end) = edges(graph);//edges() 得到 graph 對於所有 edge 的 iterator
		auto VertexNameMap = get(vertex_name, graph);
		
		
		static int r_num = 1;
		std::ofstream file_out("bit_mask_print.txt", std::ios_base::app);

		////第一次進行 bit mask print 時, 將檔案清空
		if (r_num == 1)		
			std::ofstream file_out("bit_mask_print.txt", std::ios_base::trunc);

		


		file_out << "request " << r_num << std::endl;

		//輸出按照邊的順序
		for (; edge_iter != edge_end; ++edge_iter)
		{
			auto edge_bit_mask = get(bit_mask_map, *edge_iter);
			Vertex source_v = source(*edge_iter, graph);
			Vertex target_v = target(*edge_iter, graph);
			file_out << std::setw(2) << resetiosflags(std::ios::left) << VertexNameMap[source_v] << " - "
				<< std::setw(2) << setiosflags(std::ios::left) << VertexNameMap[target_v] << " ";

			for (auto bit : edge_bit_mask)
				file_out << bit;

			file_out << std::endl;
		}

		file_out << std::endl;
		++r_num;

		file_out.close();
	}









}//boost

#endif //DEBUG_H