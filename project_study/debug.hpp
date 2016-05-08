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
	void k_path_print(const Graph& graph,const Request& request,const WeightMap& weight_map, int path_num)
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

	//print k-path with something, ex: weight, max_block, fragmentation rate(ry
	template<typename Graph, typename Sth_with_KPath>
	void print_k_path_with_something(const Graph& graph,const Sth_with_KPath& sth_with_k_path_container)
	{
		auto name_map = get(vertex_name, graph);

		for (const auto& sth_with_k_path : sth_with_k_path_container)
		{
			std::cout << sth_with_k_path.first << " ";

			auto path = sth_with_k_path.second;

			//印出path中第一個edge的src
			auto first_edge = *(path.begin());
			std::cout << get(name_map, source(first_edge, graph)) << " ";

			for (const auto& edge : path)
				std::cout << get(name_map, target(edge, graph)) << " ";

			std::cout << std::endl;
		}

	}


	//print path with something, ex: weight, max_block, fragmentation rate(ry
	template<typename Graph, typename SthWithPath>
	void print_sth_with_path(const Graph& graph,const SthWithPath& sth_with_path)
	{
		std::ofstream file_result("result.txt", std::ios_base::app);
			
		auto name_map = get(vertex_name, graph);
		auto path = sth_with_path.second;

		//印出path中第一個edge的src
		auto first_edge = *(path.begin());
		file_result << get(name_map, source(first_edge, graph)) << " ";

		for (const auto& edge : path)
			file_result << get(name_map, target(edge, graph)) << " ";

		//印出(sth) 
		file_result << "(" << sth_with_path.first << ")" << std::endl;

		file_result.close();
	}

	void print_slot_begin_and_num(int slot_begin, int slot_num)
	{
		std::ofstream file_result("result.txt", std::ios_base::app);

		//印出(slot_begin, slot_num)
		file_result << "(" << slot_begin << "," << slot_num << ") ";


		file_result.close();
	}

	//print path
	template<typename Graph, typename Path>
	void print_path(const Graph& graph, const Path& path)
	{
		std::ofstream file_result("result.txt", std::ios_base::app);

		auto name_map = get(vertex_name, graph);

		//印出path中第一個edge的src
		auto first_edge = *(path.begin());
		file_result << get(name_map, source(first_edge, graph)) << " ";

		for (const auto& edge : path)
			file_result << get(name_map, target(edge, graph)) << " ";

		file_result << std::endl;

		file_result.close();
	}


	//print bitmask to file
	template<typename Graph, typename BitMaskMap>
	void bit_mask_print(const Graph& graph, const std::vector<std::vector<int>>& bit_mask,const BitMaskMap& bit_mask_map)
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


	//print g_UsingPaths to file
	template <typename Graph, typename UsingPaths>
	void print_usingPaths(const Graph& graph,const UsingPaths& usingPaths)
	{
		std::ofstream file_out("UsingPaths.txt", std::ios_base::trunc);

		for (auto v_and_path : usingPaths)
		{
			auto vertexNameMap = get(vertex_name, graph);
			
			//印出 (Vertex, Vertex)
			auto vertex_pair = v_and_path.first;
			Vertex src = vertex_pair.first;
			Vertex dst = vertex_pair.second;
			std::string src_name = vertexNameMap[src];
			std::string dst_name = vertexNameMap[dst];

			file_out << "(" << src_name << "," << dst_name << ")" << std::endl;


			//印出 set 裡的 path slot起始位置 slot數量

			const auto& set = v_and_path.second;
			for (const auto& detail : set)
			{
				//印出起點
				file_out << src_name << " ";

				//印出path
				for (const auto& edge : detail.edge_list)
				{
					Vertex tar = target(edge, graph);
					file_out << vertexNameMap[tar] << " ";
				}

				
				//印出 slot 起始位置, slot數量
				file_out << "(" << detail.slot_begin << "," << detail.slot_num << ")";
				file_out << std::endl;
			}

			//印完一組 map 後加上分隔線
			file_out << "------------------------------" << std::endl;

		}

	}







}//boost

#endif //DEBUG_H