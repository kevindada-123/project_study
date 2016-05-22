#ifndef DELE_H
#define DELE_H

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
#include <iterator>

#include "yen_ksp.hpp"
#include "add.hpp"

namespace boost
{
	//request.cap為要求量 (此為刪除)
	template<typename Graph, typename Request, typename BitMaskMap, typename UsingPaths>
	bool delete_algo(Graph& graph, UsingPaths usingPaths, Request& request, BitMaskMap& bit_mask_map)//need bitmask,request,start,num,path_num
	{
		int n = 0, start/*path配置的slot起點*/, num/*path配置的slot數量*/, i;
		/*find pair(request.src, request.dst) 得到 std::map<std::pair<src, dst>,*/
		auto vertex_pair = std::make_pair(request.src, request.dst);
		auto result = usingPaths.find(vertex_pair);
		if (result == usingPaths.end())
		{
			std::cout << "連線尚未建立，無法刪除，忽略!!\n";
			return false;
		}
		else
		{
			for (auto& detail : result->second)
			{
				start = detail.slot_begin;
				num = detail.slot_num;
				for (auto& edge : detail.edge_list)
				{
					std::vector<int>& b = get(bit_mask_map, edge);//得到bitmask 
					for (int i = start; i < num; i++)
					{
						b[i] = 0;
					}
				}
			}
			usingPaths.erase(result);
			return true;
		}
	}
}
#endif 
