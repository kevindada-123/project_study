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
#include "online_path_computation.hpp"


namespace boost
{

	//計算mi
	template<typename Graph, typename Request, typename Path>
	int compute_mi(const Graph& graph, const Request& request, const Path& path)
	{
		//先計算出調變等級mi
		auto weight_map = get(edge_weight, graph);
		double path_weight = 0;
		for (const auto& edge : path)
			path_weight += get(weight_map, edge);
		int mi = mlvl(path_weight);

		return mi;
	}

	int checkleft(int bitmask_left, const std::vector<int>& edge_bit_mask)//檢測可以最多擴充多少
	{
		int range = 0;//代表可以擴充的slot數量

		//假如bitmask_left已經等於bitmask的最左邊 代表左邊沒有可擴充空間
		if (bitmask_left == -1)
			return 0;

		for (int i = bitmask_left; i!= -1 ; ++i)//檢測可以最多擴充多少
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


	//以下放在另外一個hpp	
	//下面檢測是否可以擴充
		//目前只修改外部屬性的bitmask,還沒修改 usingpathdetail的slot_begin和slot_number
	template<typename Graph, typename Request, typename BitMaskMap>
	bool expand(const Graph& graph, Request& request, BitMaskMap& bit_mask_map)
	{
		bool req_state = false; //表示需求是否分配完成
		auto sourece = request.src;
		auto destination = request.dst;
	
		int ni;//剩餘未分配的slot數
	
		int bitmask_left;
		int bitmask_right;
		
		auto& usedPaths = g_usingPaths.find(std::make_pair(sourece, destination))->second;//這裡找到edge_list(path)的set


		for(auto iter = usedPaths.begin(); iter != usedPaths.end();)//根據每條path去檢查和分配
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
				
				if(bitmask_left == -1)
				{ }
				else if(edge_bit_mask[bitmask_left]==0)//向左邊檢查 如果slot正在被使用(1)或是當gb(7) 代表左邊無法擴充 反之則可以擴充
				{
					left_range.push_back(checkleft(bitmask_left, edge_bit_mask));
				}

				if(bitmask_right == B)
				{ }
				else if(edge_bit_mask[bitmask_right]==0)//向右邊檢查 如果slot正在被使用(1)或是當gb(7) 代表左邊無法擴充 反之則可以擴充
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
			


			int mi = compute_mi(graph, request, path);
			if (max_left_range != 0)
			{
				//先以左邊去擴充
				tie(req_state, ni) = countNi(max_left_range, mi, request);

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
				iter = usedPaths.erase(iter);
				int new_slot_begin = old_slot_begin - ni;
				int new_slot_num = old_slot_num + ni;
				UsingPathDetail updated_using_path_detail{ path, new_slot_begin , new_slot_num };
				iter = usedPaths.emplace_hint(iter, updated_using_path_detail);
			}
			
			//如果左邊擴充即可滿足需求, 跳出迴圈
			if (req_state)
				break;
			//假如左邊無法滿足需求, 開始對右邊擴充, 先算出在右邊要擴充的數量
			else if (max_right_range != 0)
			{
				tie(req_state, ni) = countNi(max_right_range, mi, request);

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
				iter = usedPaths.erase(iter);
				int new_slot_begin = old_slot_begin;
				int new_slot_num = old_slot_num + ni;
				UsingPathDetail updated_using_path_detail{ path, new_slot_begin , new_slot_num };
				iter = usedPaths.emplace_hint(iter, updated_using_path_detail);
			}				

			//如果右邊擴充可滿足需求, 跳出迴圈
			if (req_state)
				break;

			if (iter != usedPaths.end())
				++iter;
		}

		return req_state;
	}



}


#endif //EXPAND_H