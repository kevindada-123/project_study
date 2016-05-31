#ifndef EXPAND_H
#define EXPAND_H

#include <string>
#include <algorithm>
#include <vector>
#include <limits>
#include <numeric>
#include <iostream>
#include <list>
#include <map>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/tuple/tuple.hpp>

#include "yen_ksp.hpp"
#include "add.hpp"
#include "fr.hpp"

namespace boost
{

	//計算mi
	template<typename Graph, typename Request, typename Path>
	int calculate_mi(const Graph& graph, const Request& request, const Path& path)
	{
		//先計算出調變等級mi
		auto weight_map = get(edge_weight, graph);
		double path_weight = 0;
		for (const auto& edge : path)
			path_weight += get(weight_map, edge);
		int mi = mlvl(path_weight);

		return mi;
	}

	//針對path計算max_block
	template<typename Path, typename BitMaskMap>
	int calculate_path_max_block(const Path& path, const BitMaskMap& bit_mask_map)
	{
	
		std::vector<int> bit_mask_a(B, 0); //大小=300的陣列(ai),內容預設為0
		std::vector<int> bit_mask_b;
		for (const auto& edge : path)
		{
			bit_mask_b = get(bit_mask_map, edge); //取得邊的bit mask map

			for (int i = 0; i < B; ++i)
				bit_mask_a[i] = bit_mask_b[i] || bit_mask_a[i]; //做OR

		}
		std::pair<int, int> max_block_a = max_block(bit_mask_a); //得到aimax的 起始位置,大小

		int result = max_block_a.second;

		return result;
	}

	//對using_path_detail_vector以max_block大小進行排序
	template<typename UsingPathDetailVector, typename BitMaskMap>
	void resort_by_max_block(UsingPathDetailVector& using_path_detail_vector,const BitMaskMap& bit_mask_map)
	{
		auto cmp = [&](const typename UsingPathDetailVector::value_type& lhs,const typename UsingPathDetailVector::value_type& rhs)
		{	
			int lhs_result = calculate_path_max_block(lhs.edge_list, bit_mask_map);
			int rhs_result = calculate_path_max_block(rhs.edge_list, bit_mask_map);
			return lhs_result > rhs_result; 
		};
		

		std::sort(using_path_detail_vector.begin(), using_path_detail_vector.end(), cmp);
	}

	//計算path的d'
	template<typename Path, typename Graph, typename BitMaskMap>
	double calculate_path_d_prime(Path path, Graph graph, BitMaskMap bit_mask_map)
	{
		//先更新d' (graph的weight2屬性)
		//呼叫 online_path_computation裡的d_prime_convert
		auto weight_map = get(edge_weight, graph);
		d_prime_convert(graph, weight_map, bit_mask_map);

		//宣告weight_2的map
		auto weight_2_map = get(edge_weight2, graph);

		//計算path的weight(用d')
		double sum = 0;
		for (const auto& edge : path)
		{
			double weight = get(weight_2_map, edge);
			sum += weight;
		}

		return sum;

	}

	//對using_path_detail_vector以d_prime大小進行排序
	template<typename UsingPathDetailVector,typename Graph ,typename BitMaskMap>
	void resort_by_d_prime(UsingPathDetailVector& using_path_detail_vector, const Graph& graph, const BitMaskMap& bit_mask_map)
	{
		auto cmp = [&](const typename UsingPathDetailVector::value_type& lhs, const typename UsingPathDetailVector::value_type& rhs)
		{
			double lhs_result = calculate_path_d_prime(lhs.edge_list, graph, bit_mask_map);
			double rhs_result = calculate_path_d_prime(rhs.edge_list, graph, bit_mask_map);

			return lhs_result < rhs_result;
		};

		std::sort(using_path_detail_vector.begin(), using_path_detail_vector.end(), cmp);
	}


	template <typename UsingPathDetailSet, typename UsingPathDetailVectorIterator>
	typename UsingPathDetailSet::iterator find_iterator_in_set
	(const UsingPathDetailSet& _multiset, const UsingPathDetailVectorIterator& _vector_iterator)
	{
		typename UsingPathDetailSet::iterator result{};
		
		//先取出vector裡的元素
		auto vector_element = *_vector_iterator;

		//找到在multiset裡對應的元素的範圍
		typename UsingPathDetailSet::iterator equal_range_begin, equal_range_end;
		tie(equal_range_begin, equal_range_end) = _multiset.equal_range(vector_element);

		//在找到的範圍裡找出slot_begin, slot_num一樣的元素
		for (; equal_range_begin != equal_range_end; ++equal_range_begin)
		{
			auto set_element = *equal_range_begin;

			if (vector_element == set_element)
			{
				result = equal_range_begin;
				break;
			}

		}

		return result;
	}


	int checkleft(int bitmask_left, const std::vector<int>& edge_bit_mask)//檢測可以最多擴充多少
	{
		int range = 0;//代表可以擴充的slot數量

		//假如bitmask_left已經等於bitmask的最左邊 代表左邊沒有可擴充空間
		if (bitmask_left == -1)
			return 0;

		for (int i = bitmask_left; i!= -1 ; --i)//檢測可以最多擴充多少
		{
			if (edge_bit_mask[i] == 1 || edge_bit_mask[i] == 7)
				break;
			range++;
		}

		return range;
	}


	int checkright(int bitmask_right, std::vector<int> edge_bit_mask)//檢測可以最多擴充多少
	{

		int range = 0;//代表可以擴充的slot數量

		//假如bitmask_right已經等於bitmask的最右邊 代表右邊沒有可擴充空間
		if (bitmask_right == B)
			return 0;

		for (int i = bitmask_right; i < B; ++i)//檢測可以最多擴充多少
		{
			if (edge_bit_mask[i] == 1 || edge_bit_mask[i] == 7)
				break;
			range++;
		}

		return range;
	}


	
	//expand 主程式
	template<typename Graph, typename Request, typename BitMaskMap>
	bool expand(const Graph& graph, Request& request, BitMaskMap& bit_mask_map)
	{
		bool req_state = false; //表示需求是否分配完成
		auto sourece = request.src;
		auto destination = request.dst;
	
		int ni;//剩餘未分配的slot數
	
		int bitmask_left;
		int bitmask_right;
		
		auto& usedPaths = g_usingPaths.find(std::make_pair(sourece, destination))->second;//這裡找到UsingPathDetail的set

		//因為無法直接對multiset進行排序, 所以將multiset內容copy到vector, 再進行排序
		std::vector<UsingPathDetail> usedPaths_vector{ usedPaths.begin(), usedPaths.end() };

		//根據 g_sort_priority 決定使用何種排序方式
		switch(g_expand_priority)
		{
			case Priority::d_prime: 
				resort_by_d_prime(usedPaths_vector, graph, bit_mask_map);
				break;
			case Priority::max_block: 
				resort_by_max_block(usedPaths_vector, bit_mask_map);
				break;
			case Priority::fr: 
				resort_by_fr(usedPaths_vector, bit_mask_map);
				break;
		}

		


		//根據在vector每條path去檢查和分配, 會將更改的usingPathDetail內容寫到multiset(就是usedPaths)
		for(auto iter = usedPaths_vector.begin(); iter != usedPaths_vector.end();)
		{
			auto path = iter->edge_list;
			
			//當left為bitmask最左邊 bitmask_left設為 -1
			if (iter->slot_begin == 0)
				bitmask_left = -1;
			else
				bitmask_left = iter->slot_begin - 1;
			//當right為bitmask最右邊 bitmask_right設為B(300)
			if (iter->slot_begin + iter->slot_num >= B)
				bitmask_right = B;
			else
				bitmask_right = iter->slot_begin + iter->slot_num;
		
			std::vector<int> left_range;
			std::vector<int> right_range;
			int max_left_range;
			int max_right_range;
		
			/*if (bitmask_left == -1 && bitmask_right == B)
				break;*/


			for(auto iter2=path.begin(); iter2 !=path.end();iter2++)//內迴圈是檢查左邊和右邊能擴充多少
			{
				auto edge_bit_mask = get(bit_mask_map, *iter2);
				
				if(bitmask_left != -1)//向左邊檢查 如果slot正在被使用(1)或是當gb(7) 代表左邊無法擴充 反之則可以擴充
				{
					left_range.push_back(checkleft(bitmask_left, edge_bit_mask));
				}

			    if(bitmask_right != B)//向右邊檢查 如果slot正在被使用(1)或是當gb(7) 代表左邊無法擴充 反之則可以擴充
			    {
				    right_range.push_back(checkright(bitmask_right, edge_bit_mask));
			    }
			}

			//找出左邊和右邊能擴充的範圍
			if (left_range.size() != 0)
				max_left_range = *min_element(left_range.begin(), left_range.end());
			else
				max_left_range = 0;

			if (right_range.size() != 0)
				max_right_range = *min_element(right_range.begin(), right_range.end());
			else
				max_right_range = 0;
			


			int mi = calculate_mi(graph, request, path);
			if (max_left_range != 0)
			{
				//先以左邊去擴充
				tie(req_state, ni) = countNi(max_left_range, mi, request);

				/////輸出分配結果測試////
				int path_max_block = calculate_path_max_block(path, bit_mask_map);
				////////////////////////

				//先對左邊擴充
				for (const auto& edge : path)
				{
					auto& edge_bit_mask = get(bit_mask_map, edge);
					for (int i = bitmask_left; i != bitmask_left - ni + 1; --i)
						edge_bit_mask[i] = 1;
				}

				//更改g_usingPaths裡的multiset內容
				int old_slot_begin = iter->slot_begin;
				int old_slot_num = iter->slot_num;
				//找到目前在vector裡的元素對應到的multiset裡的元素
				auto multiset_iterator_left = find_iterator_in_set(usedPaths, iter);
				multiset_iterator_left = usedPaths.erase(multiset_iterator_left);
				int new_slot_begin = old_slot_begin - ni;
				int new_slot_num = old_slot_num + ni;
				UsingPathDetail updated_using_path_detail{ path, new_slot_begin , new_slot_num };
				multiset_iterator_left = usedPaths.insert(multiset_iterator_left, updated_using_path_detail);
				//也要把更新的元素寫回vector
				iter->slot_begin = new_slot_begin;
				iter->slot_num = new_slot_num;


				/////輸出分配結果測試////
				print_slot_begin_and_num(new_slot_begin, new_slot_num);
				print_path(graph, path);

				//先在sstream往前2個byte, 因為上面path_print有換行
				result_ss.seekp(-2, std::ios_base::end);

				//當 path 用 max_block 排序時用這個
				result_ss << "(" << calculate_path_max_block(path, bit_mask_map) << ")" << std::endl;

				//當 path 用 d' 排序時用這個
				//result_ss << "(" << calculate_path_d_prime(path, graph, bit_mask_map) << ")" << std::endl;

				///////////////////////
			}
			
			//如果左邊擴充即可滿足需求, 跳出迴圈
			if (req_state)
				break;
			//假如左邊無法滿足需求, 開始對右邊擴充, 先算出在右邊要擴充的數量
			else if (max_right_range != 0)
			{
				tie(req_state, ni) = countNi(max_right_range, mi, request);
				
				/////輸出分配結果測試////
				int path_max_block = calculate_path_max_block(path, bit_mask_map);
				////////////////////////

				//對右邊擴充
				for (const auto& edge : path)
				{
					auto& edge_bit_mask = get(bit_mask_map, edge);

					for (int i = bitmask_right - 1; i != bitmask_right + ni - 1; ++i)
						edge_bit_mask[i] = 1;

					//最後面加上GB
					edge_bit_mask[bitmask_right + ni - 1] = GB;
				}

				//更改g_usingPaths裡的multiset內容
				int old_slot_begin = iter->slot_begin;
				int old_slot_num = iter->slot_num;
				//找到目前在vector裡的元素對應到的multiset裡的元素
				auto multiset_iterator_right = find_iterator_in_set(usedPaths, iter);
				multiset_iterator_right = usedPaths.erase(multiset_iterator_right);
				int new_slot_begin = old_slot_begin;
				int new_slot_num = old_slot_num + ni;
				UsingPathDetail updated_using_path_detail{ path, new_slot_begin , new_slot_num };
				multiset_iterator_right = usedPaths.emplace_hint(multiset_iterator_right, updated_using_path_detail);
				//也要把更新的元素寫回vector
				iter->slot_begin = new_slot_begin;
				iter->slot_num = new_slot_num;


				/////輸出分配結果測試////
				print_slot_begin_and_num(new_slot_begin, new_slot_num);
				print_path(graph, path);

				//先在sstream往前2個byte, 因為上面path_print有換行
				//result_ss.seekp(-2, std::ios_base::end);

				//當 path 用 max_block 排序時用這個
				//result_ss << "(" << path_max_block << ")" << std::endl;

				//當 path 用 d' 排序時用這個
				//result_ss << "(" << calculate_path_d_prime(path, graph, bit_mask_map) << ")" << std::endl;

				///////////////////////
			}				

			//如果右邊擴充可滿足需求, 跳出迴圈
			if (req_state)
				break;




			if (iter != usedPaths_vector.end())
				++iter;
		}

		return req_state;
	}



}


#endif //EXPAND_H