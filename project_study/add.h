#ifndef ADD_H
#define ADD_H

#include "online_path_computation.hpp"
#include <vector>
#include <algorithm>

namespace boost
{
	template<typename KPath, typename BitMaskMap>
	std::vector<std::pair<int, typename KPath::value_type>> resort_with_max_block(KPath k_path, BitMaskMap bit_mask_map)
	{
		//建立紀錄每條path的max_block的vector, max_block預設為0
		std::vector<std::pair<int,typename KPath::value_type>> max_block_and_path_vector;
		for (const auto& path : k_path)
			max_block_and_path_vector.push_back(std::make_pair(0, path));


		//算出每條path中所有edge的max_block最小值, 代表此path的max_block
		for (auto& max_block_and_path : max_block_and_path_vector)
		{
			auto path = max_block_and_path.second;


			std::vector<int> bit_mask_a(B, 0); //大小=300的陣列(ai),內容預設為0
			std::vector<int> bit_mask_b;
			for (const auto& edge : path)
			{
				bit_mask_b = get(bit_mask_map, edge); //取得邊的bit mask map

				for (int i = 0; i < B; ++i)
					bit_mask_a[i] = bit_mask_b[i] || bit_mask_a[i]; //做OR

			}
			std::pair<int, int> max_block_a = max_block(bit_mask_a); //得到aimax的 起始位置,大小

			max_block_and_path.first = max_block_a.second;
		}


		//對k_path以每條path的max_blcok做排序

		//max_block_and_path_vector的比較原則, max_blcok由大到小排序
		using MaxBlock_Kpath_Detail = std::pair<int, typename KPath::value_type>;
		auto cmp = [](const MaxBlock_Kpath_Detail& lhs, const MaxBlock_Kpath_Detail& rhs)
		{return lhs.first > rhs.first; };

		std::sort(max_block_and_path_vector.begin(), max_block_and_path_vector.end(), cmp);


		return max_block_and_path_vector;
	}
	
	
	template<typename Graph, typename Request, typename BitMaskMap>
	bool add(Graph& graph, Request& request, BitMaskMap& bit_mask_map)
	{
		//對演算法1進行微調

		//G(V, E, B, D) → G'(V, E, D')
		d_prime_convert(graph, get(edge_weight, graph), bit_mask_map);
		//k-shortest path with D'
		auto k_path_and_weight = k_shortest_path(graph, request, get(edge_weight2, graph), 5);

		//將k-path(weight, path)的path單獨取出, 放在vector中
		std::vector<std::list<typename Graph::edge_descriptor>> k_path;
		for (auto weight_and_path : k_path_and_weight)
			k_path.push_back(weight_and_path.second);
		//將k-path以max_blcok為基準進行排序
		auto max_block_and_path_vector = resort_with_max_block(k_path, bit_mask_map);

		/////////////////////測試用, 印出 k_path 以 max_block 為基準排序的後的結果
		//print_k_path_with_something(graph, max_block_and_path_vector);
		/////////////////////




		//使用演算法1配置需求
		bool result = algorithm_detail(graph, request, max_block_and_path_vector, bit_mask_map);

		return result;
	}
}//boost

#endif //ADD_H
