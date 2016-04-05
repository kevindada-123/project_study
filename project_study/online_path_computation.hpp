#ifndef OPC_H
#define OPC_H

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

#define G 1
#define M_MAX 4
#define B 300
#define Cslot 12.5

namespace boost
{

	//this function will return maximum size of available slot blocks in b
	std::pair<int, int> max_block(std::vector<int> &b)
	{
		int slot = 0;
		int max = 0;

		//回傳結果 (position, maxsize)
		//std::pair<int, int> 
		int result_first, result_second;

		for (int i = 0; i < b.size(); ++i)
		{
			if (b[i] == 0 && i == (b.size() - 1))
			{
				slot++;
				if (slot > max)
				{
					max = slot;
					result_first = i + 1 - max;
					result_second = max;
				}
				slot = 0;
			}
			else if (b[i] == 0)
			{
				++slot;
			}
			else
			{
				if (slot > max)
				{
					max = slot;
					result_first = i - max;
					result_second = max;
				}
				slot = 0;
			}
		}

		//當 bit mask 為 300 時, 經過上面的判斷式的 max 仍是 0, 在此修正
		if (slot == B)
		{
			max = B;
			result_first = 0;
			result_second = max;
		}

		return std::make_pair(result_first, result_second);
	}

	template<typename T>
	int mlvl(T distance)
	{
		int m;

		if (distance <= 1200)
			m = 4;
		else if (distance <= 2400)
			m = 3;
		else if (distance <= 4800)
			m = 2;
		else if (distance <= 9600)
			m = 1;
		else
			m = 1;		//大於 9600 的 m 暫時設定為 1

		return m;
	}

	inline int sum_bit_mask(std::vector<int> &b)
	{
		auto filter = [](int sum, int element) { if (element != -1) return sum + element; else return sum; };
		return std::accumulate(b.begin(), b.end(), 0, filter);
	}

	//將 d 轉換成 d', 並且寫入 edge 的 weight2 屬性
	template<typename Graph, typename WeightMap, typename BitMaskMap>
	void d_prime_convert(Graph& graph, WeightMap& weight_map, BitMaskMap& bit_mask_map)
	{

		double d_prime;
		int we;

		using EdgeIter = typename graph_traits<Graph>::edge_iterator;
		EdgeIter edge_iter, edge_end;
		tie(edge_iter, edge_end) = edges(graph);//edges() 得到 graph 對於所有 edge 的 iterator

		typename property_traits<WeightMap>::value_type de;
		typename property_traits<BitMaskMap>::value_type bit_mask;

		using WeightMap2 = property_map<Graph, edge_weight2_t>::type;
		WeightMap2 weight_map_2 = get(edge_weight2, graph);

		for (; edge_iter != edge_end; ++edge_iter)//對第一條~最後一條邊
		{
			bit_mask = get(bit_mask_map, *edge_iter); //得到某條邊的bit_mask
			de = get(weight_map, *edge_iter); //得到某條邊的de 長度(weight_1)

			if (max_block(bit_mask).second < G)
				d_prime = INT_MAX;
			else
			{
				we = M_MAX - mlvl(de) + 1;
				//將除數轉為 double, 避免結果變為 0
				d_prime = we * (sum_bit_mask(bit_mask) + G) / double(B); //公式
			}

			weight_map_2[*edge_iter] = d_prime;
		}
	}

	template <typename Graph, typename Request, typename WeightMap>
	std::list < std::pair<typename WeightMap::value_type,
		std::list<typename Graph::edge_descriptor> > >
		k_shortest_path(const Graph& graph, Request& request, WeightMap& weight_map, optional<unsigned> K = optional<unsigned>())
	{
		typename Graph::vertex_descriptor src, dst;
		src = request.src;
		dst = request.dst;

		return yen_ksp(graph, src, dst, weight_map, get(vertex_index, graph), K);
	}


	//一條路徑最多可配置的slot數 Ni 公式(3)
	template<typename T, typename Request>
	std::pair<bool, int> countNi(T Maxblockai, T Mi, Request& request)//路徑i的ai mask的Maxblock,路徑i的調變等級M
	{
		int Ni;
		bool req_state = false;

		if (((Maxblockai - 1)*Mi*Cslot) >= request.cap)//最後一條路徑,完成連線
		{
			if ((int)(request.cap * 10) % (int)(Mi*Cslot * 10) > 0)
			{
				Ni = (int)(request.cap / (Mi*Cslot)) + 1 + 1;
			}
			else if ((int)(request.cap * 10) % (int)(Mi*Cslot * 10) == 0)
			{
				Ni = (int)(request.cap / (Mi*Cslot)) + 1;
			}
			req_state = true;//完成連線
		}
		else if (((Maxblockai - 1)*Mi*Cslot) < request.cap)//未完成連線,還需要路徑
		{
			request.cap = request.cap - ((Maxblockai - 1)*Mi*Cslot);
			Ni = Maxblockai;
			req_state = false;//未完成連線
		}

		//回傳連線狀態 & 分配的slot數
		return std::make_pair(req_state, Ni);


	}


	template<typename Graph, typename Path, typename Request, typename BitMaskMap>
	bool algorithm_detail(Graph& graph, Request& request, Path& k_path, BitMaskMap& bit_mask_map)
	{
		//表示這個請求是否阻塞
		bool success;

		//表示是否配置完成
		bool req_state = false;

		//宣告 BitMask 作為 BitMaskMap 的 value type
		using BitMask = typename property_traits<BitMaskMap>::value_type;
		BitMask bit_mask_b;

		VertexNameMap name_map = get(vertex_name, graph);

		//自定義 struct, 儲存某個 request 對某個 edge 分配的 slot, 方便歸還時使用
		using Edge = typename Graph::edge_descriptor;
		struct alloc_record
		{
			Edge e;
			int pos; //開始位置
			int slots; //數量
		};
		
		///////////測試用////////////
		int numb = 0;
		double req_cap = request.cap;
		int k = 1, kp = 1;
		////////////end////////////
		
		//建立儲存每個紀錄的 vector
		std::vector<alloc_record> record_vector;
		for (auto path : k_path)//5條路徑
		{
			std::vector<int> bit_mask_a(B, 0); //大小=300的陣列(ai),內容預設為0

											   //對路徑上的每條邊的be做OR
			for (auto edge : path.second)
			{
				bit_mask_b = get(bit_mask_map, edge); //取得邊的bit mask map

				for (int i = 0; i < B; ++i)
				{
					bit_mask_a[i] = bit_mask_b[i] || bit_mask_a[i]; //做OR
				}

			}

			//////////////*測試一條路徑不夠時會不會分流*/
			/*
			for (int i = 0; i < 290; ++i)
			{
			bit_mask_a[i] = 1;
			}
			*/
			///////////////end/////////////////////////

			//NOT,得到ai[]
			/*for (int i = 0; i < B; ++i)
			{
			bit_mask_a[i] = !bit_mask_a[i];
			}*/
			//算出ai []的Maxblock
			std::pair<int, int> max_block_a = max_block(bit_mask_a); //得到aimax的 起始位置,大小

																	 //算出路徑i的調變等級Mi(要用初始graph的距離)
			int d_weight_sum = 0;
			for (auto edge : path.second)//路徑的每條邊
			{
				int weight = get(edge_weight, graph, edge);
				d_weight_sum += weight;
			}
			int mi = mlvl(d_weight_sum);
			//////////////////////測試用///////////////////////////////
			numb++;
			if (1 == numb)
			{
				int need_slot;
				if ((int)(req_cap * 10) % (int)(mi*Cslot * 10) > 0)
					need_slot = (int)(req_cap / (mi*Cslot)) + 1;
				else if ((int)(req_cap * 10) % (int)(mi*Cslot * 10) == 0)
					need_slot = (int)(req_cap / (mi*Cslot));
				std::cout << "共需求slot數(不包含gb):" << need_slot << std::endl << std::endl;
			}
			/////////////////////////測試用///////////////////////////////////
			std::cout << "k_path " << kp << " : ";
			std::cout << get(name_map, request.src) << " ";
			for (auto v : path.second)
			{
				std::cout << get(name_map, target(v, graph)) << " ";
			}
			++kp;
			std::cout << std::endl;
			//////////////////////////end///////////////////////////////////
			if (max_block_a.second >= G) //這條路徑可以用
			{
				/////////////////////////測試用///////////////////////////////////
				std::cout << "可用路徑 " << k << ": ";
				std::cout << get(name_map, request.src) << " ";
				for (auto v : path.second)
				{
					std::cout << get(name_map, target(v, graph)) << " ";
				}
				++k;
				std::cout << std::endl;
				//////////////////////////end///////////////////////////////////

				int ni;
				tie(req_state, ni) = countNi(max_block_a.second, mi, request); //大小,調變,要求
																			   //對經過的每條 edge(link) 做 slot 的分配
				for (auto edge : path.second)
				{
					std::vector<int>& bit_mask = get(bit_mask_map, edge);
					int pos = max_block_a.first;
					int block_start = max_block_a.first;
					for (; pos != (max_block_a.first + ni) - 1; ++pos)//分配到的slot設為1
					{
						bit_mask[pos] = 1;
					}
					//在尾部加上GB
					bit_mask[pos] = -1;

					//紀錄分配到哪條 edge(link), solt 的起始位置, solt的數量
					alloc_record record{ edge, block_start , ni };
					record_vector.push_back(record);
				}
				std::cout << "起始點:" << max_block_a.first << " slot數量:" << ni << std::endl << std::endl;

			}

			//需求已分配完成, 跳出迴圈
			if (req_state)
			{
				break;
			}

		}


		//在此判斷 req_state, 為 false 時代表使用了所有路徑仍然無法完成配置
		//歸還已分配的 slot, 並且回報這個請求阻塞
		if (req_state)
			success = true;
		else if (!req_state)
		{
			success = false;

			//分配失敗, 進行歸還
			for (auto rec : record_vector)//對整個record_vector
			{
				BitMask& bit_mask = get(bit_mask_map, rec.e);
				for (int pos = rec.pos; pos != rec.pos + rec.slots; ++pos)
				{
					bit_mask[pos] = 0;
				}
			}
		}

		return success;
	}


	template<typename Graph, typename Request, typename BitMaskMap>
	bool online_path_computation(Graph& graph, Request& request, BitMaskMap& bit_mask_map)
	{
		//和演算法 2 不同的地方, 除下面這兩個函式呼叫之外 algorithm_detail 應該皆同(未實測)
		//G(V, E, B, D) → G'(V, E, D')
		d_prime_convert(graph, get(edge_weight, graph), bit_mask_map);
		//k-shortest path with D'
		auto k_path = k_shortest_path(graph, request, get(edge_weight2, graph), 5);


		bool result = algorithm_detail(graph, request, k_path, bit_mask_map);

		return result;
	}

}//boost

#endif //OPC_H