#ifndef RA_H
#define RA_H

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
#include "online_path_computation.hpp"



#define G1 1
#define M_MAX 4
#define B 300
#define Cslot 12.5
#define GB 7
#define MAX 1e9 
#define MIN 0


namespace boost
{
	void swap(double **data, int i, int j) {
		double tmp0 = data[i][0];
		double tmp1 = data[i][1];
		data[i][0] = data[j][0];
		data[i][1] = data[j][1];
		data[j][0] = tmp0;
		data[j][1] = tmp1;
	}
	//Sort排序
	void bubbleSort(double **data, int n, int leftbig) {
		if (leftbig == 0)//左->右,小->大
		{
			for (int i = n - 1; i >= 0; i--) {
				for (int j = 0; j < n - i-1; j++) {
					if (data[j + 1][1] < data[j][1]) {
						swap(data, j + 1, j);
					}
				}
			}
		}
		else if (leftbig == 1)//左->右,大->小
		{
			for (int i = 0; i < n; i++) {
				for (int j = 0; j < n - i; j++) {
					if (data[j + 1][1] > data[j][1]) {
						swap(data, j + 1, j);
					}
				}
			}
		}
	}
	int not_using_slot(std::vector<int> &b)
	{
		int nslot = 0;
		for (int i = 0; i < b.size(); ++i)
		{
			if (b[i] == 0)
			{
				nslot++;
			}
		}
		return nslot;
	}
	/*破碎率 FR越小表示越破碎!!!!!*/
	template<typename BitMaskMap, typename UsingPathDetail>
	double fragmentation_rate(UsingPathDetail& vertex, BitMaskMap& bit_mask_map, int& leftbig)
	{
		leftbig = 0;//左->右,小->大
		int frag_max = 0, frag_slot = 0;
		double frag = 0.0;
		std::vector<int> b;
		for (const auto& edge : (*vertex).edge_list)
		{
			b = get(bit_mask_map, edge);//得到bitmask 
			frag_max = frag_max + max_block(b).second;//拿到max_block的大小
			frag_slot = frag_slot + not_using_slot(b);
		}
		frag = (double)frag_max / (double)frag_slot;

		return frag;
	}
	/*路徑長度*(路徑配置的邊 !=0的slot數加起來)*/
	template<typename Graph, typename BitMaskMap, typename UsingPathDetail>
	double pathweight_slot(Graph& graph, UsingPathDetail& vertex, BitMaskMap& bit_mask_map, int& leftbig)
	{
		leftbig = 1;//左->右,大->小
		double path_slot = 0;
		int weight = 0, frag_slot = 0;
		std::vector<int> b;
		for (const auto& edge : (*vertex).edge_list)//對第一條~最後一條邊
		{
			b = get(bit_mask_map, edge);//得到bitmask 
			weight = weight + get(edge_weight, graph, edge); //得到某條邊的weight ->取得路徑總長度
			frag_slot = frag_slot + (300 - not_using_slot(b));//得到某條邊的!=0slot數 ->取得路徑!=0slot數
		}
		path_slot = (double)((weight * frag_slot)*0.0001);
		return path_slot;
	}
	//全從右開始縮減
	void cut_right(std::vector<int> &b, int start, int num, int slot)
	{
		for (int i = start + num - slot; i < start + num; i++)
			b[i] = 0;
		b[start + num - slot - 1] = GB;
	}
	//全從左開始縮減
	void cut_left(std::vector<int> &b, int& start, int num, int slot)
	{
		for (int i = start; i < start + slot; i++)
			b[i] = 0;
	}
	/**************************************************************************************************/
	//對路徑經過的邊做縮減
	//1,全刪 2,刪cap 3,刪到剩G 4,擴充
	template<typename Request, typename BitMaskMap, typename UsingPath_Detail, typename UsingPath>
	int reduce(UsingPath &result, UsingPath_Detail& vertex, BitMaskMap& bit_mask_map, Request& request, int mode, int n, int dowhat,bool &deletepath)
	{
		double cap = request.cap;//需縮減頻寬
		int slot, can_ex = 1;
		int start/*path配置的slot起點*/, num/*path配置的slot數量*/;
		start = (*vertex).slot_begin;
		num = (*vertex).slot_num;
		//計算頻寬換slot
		if ((int)(cap * 10) % (int)(mode *Cslot * 10) > 0)
		{
			slot = (int)(cap / (mode *Cslot)) + 1;
		}
		else if ((int)(cap * 10) % (int)(mode *Cslot * 10) == 0)
		{
			slot = (int)(cap / (mode *Cslot));
		}
		///////////////////////////////////////////////////////////////
		if (dowhat == 1)//路徑全刪 v
		{
			//std::cout << "dowhat == 1 縮減後僅剩gb 路徑全刪 \n";
			for (const auto& edge : (*vertex).edge_list)
			{
				std::vector<int>& b = get(bit_mask_map, edge); //取得邊的bit mask map
				for (int i = start; i < start + num; i++)
					b[i] = 0;
			}
			request.cap = request.cap - ((num - 1)* mode*Cslot);
			vertex = (result->second).erase(vertex);
			deletepath = true;
			if (request.cap <= 0)
				return 1;
			else
				return 0;
		}
		else if (dowhat == 2)//縮減後不小於保障頻寬G//縮cap
		{
			//std::cout << "dowhat==2 縮cap num="<<num<<" slot="<<slot<<"\n";
			for (const auto& edge : (*vertex).edge_list)
			{
				std::vector<int>& b = get(bit_mask_map, edge);//得到bitmask 
				cut_left(b, start, num, slot);
			}
			//cut_left要加
			start = start + (slot);
			/////////////////////////////////////
			auto path = (*vertex).edge_list;
			request.cap = 0;
			vertex = (result->second).erase(vertex);
			UsingPathDetail update_upd{ path ,start,num - slot };
			vertex = (result->second).insert(update_upd);
			return 1;
		}
		else if (dowhat == 3)//刪到剩G 
		{
			//std::cout << "dowhat==3 刪到剩G\n";
			for (const auto& edge : (*vertex).edge_list)
			{
				std::vector<int>& b = get(bit_mask_map, edge);//得到bitmask 
				cut_left(b, start, num, num - G - 1);
			}
			//cut_left要加
			start = start + (num - G - 1);
			/////////////////////////////////////
			request.cap = request.cap - ((num - 1 - G)* mode*Cslot);
			auto path = (*vertex).edge_list;
			vertex = (result->second).erase(vertex);
			UsingPathDetail update_upd{ path ,start,G + 1 };
			vertex = (result->second).insert(update_upd);
			if (request.cap == 0)
				return 1;
			else
				return 0;
		}
	}
	template<typename Graph, typename Request, typename BitMaskMap, typename UsingPaths>
	bool reduce_algo(Graph& graph, UsingPaths& usingPaths, Request& request, BitMaskMap& bit_mask_map)//need bitmask,request,start,num,path_num
	{
		double **priority_sort;
		int **do_num;
		int n = 0, leftbig = 0, start/*path配置的slot起點*/, num/*path配置的slot數量*/, i;
		int path_weight = 0, have_bandwidth = 0;
		double priority = 0;//暫存priority
		int success_re = 0;
		int mode;
		request.cap = request.cap*-1;//把縮減數量換為正數

		auto vertex_pair = std::make_pair(request.src, request.dst);
		auto result = usingPaths.find(vertex_pair);
		if (result == usingPaths.end())
		{
			std::cout << "縮減連線尚未建立，無法縮減，忽略!!\n";
			return false;
		}
		/*得到 std::map<std::pair<src, dst>的 size*/
		n = (int)(result->second.size());
		priority_sort = new double*[n];
		for (i = 0; i <= n; i++)
		{
			priority_sort[i] = new double[2];
		}
		do_num = new int*[n];
		for (i = 0; i <= n; i++)
		{
			do_num[i] = new int[2];
		}
		/*指標指到 std::map<std::pair<src, dst>的第一個std::multiset<UsingPathDetail */
		auto vertex = (result->second).begin();
		for (i = 0; i < n; i++)
		{
			std::cout << "i=" << i << " ";
			for (const auto& edge : (*vertex).edge_list)//對第一條~最後一條邊
			{
				path_weight = path_weight + get(edge_weight, graph, edge); //得到某條邊的weight ->取得路徑總長度
			}
			mode = mlvl(path_weight);
			double slot_bandwidth = (((*vertex).slot_num - 1)*mode*Cslot);

			priority = slot_bandwidth; leftbig = 1;//取得路徑的優先權資料
			//pathweight_slot(graph,vertex, bit_mask_map,leftbig); //路徑長度*(路徑配置的邊 !=0的slot數加起來)
			//fragmentation_rate(vertex,bit_mask_map,leftbig);     //破碎率 FR越小表示越破碎!!!!!
			//slot_bandwidth; leftbig = 1;                         //路徑頻寬由大到小排
			//slot_bandwidth; leftbig = 0;                         //路徑頻寬由小到大排

			priority_sort[i][0] = (double)i;//第i條路徑
			priority_sort[i][1] = priority;//第i條路徑的優先權資料

			have_bandwidth = have_bandwidth + slot_bandwidth;
			std::advance(vertex, 1);
			path_weight = 0; mode = 0;
		}
		if (have_bandwidth < request.cap)
		{
			std::cout << "縮減頻寬大於連線頻寬，刪除連線!!\n";
			//清掉
			return false;
		}
		bubbleSort(priority_sort, n, leftbig);//依優先權資料排序

		double cap = request.cap,stay_cap;
		double wid;
		for (i = 0; i < n; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				do_num[i][j] = 0;
			}
		}
		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		for (i = 0; i < n; i++)//依排序後的路徑編號一個一個縮減
		{
			int path_num = (int)priority_sort[i][0];//path_num有問題
			do_num[i][0] = path_num;
			//std::cout << "path_num="<< path_num;
			auto vertex_num = (result->second).begin();
			//(*vertex_num).slot_num = num;
			std::advance(vertex_num, path_num);//利用path_num去取得"第path_num條"路徑

			for (const auto& edge : (*vertex_num).edge_list)//對第一條~最後一條邊
			{
				path_weight = path_weight + get(edge_weight, graph, edge); //得到某條邊的weight ->取得路徑總長度
			}
			mode = mlvl(path_weight);

			wid = ((*vertex_num).slot_num - 1)* mode*Cslot;
			int capmore = 0;
			double mid = mode*Cslot;
			if(((int)(cap* 10) % (int)(mid * 10))>0)capmore = 1;
			stay_cap = ((int)(cap / mid) + capmore)*mid;
			if ((wid - stay_cap) >= (mode*Cslot*G))//這條路徑的頻寬-需求在這個mode下所需頻寬>=保障頻寬
			{
				//std::cout << "1-1 刪cap cap= " << cap << "\n";
				do_num[i][1] = 2;//刪cap
				cap = 0;
				break;
			}
			else if ((wid - stay_cap) == 0)//這條路徑的頻寬=需求在這個mode下所需頻寬
			{
				//std::cout << "1-2 全刪 cap=" << cap << "\n";
				do_num[i][1] = 1;//全刪
				cap = 0;
				break;
			}
			else if ((wid - stay_cap) < (mode*Cslot*G) && (wid - stay_cap) > 0 && G > 1)
			//比g小// 0<這條路徑的頻寬-需求在這個mode下所需頻寬<保障頻寬 且 保障頻寬>1
				//保留 保障頻寬 往下查找有無還可以刪的
			{
				do_num[i][1] = 3;//保留 保障頻寬
				cap = cap - (wid - (mode*Cslot*G));
				//std::cout << "1-3 cap=" << cap << "\n";
				for (int j = i + 1; j < n; j++)
				{
					int path_num_j = (int)priority_sort[j][0];
					do_num[j][0] = path_num_j;
					auto vertex_num_j = (result->second).begin();
					std::advance(vertex_num_j, path_num_j);//利用path_num去取得"第path_num條"路徑

					for (const auto& edge : (*vertex_num_j).edge_list)//對第一條~最後一條邊
					{
						path_weight = path_weight + get(edge_weight, graph, edge); //得到某條邊的weight ->取得路徑總長度
					}
					mode = mlvl(path_weight);
					wid = ((*vertex_num).slot_num - 1)* mode*Cslot;
					capmore = 0, mid = mode*Cslot;
					if (((int)(cap * 10) % (int)(mid * 10))>0)capmore = 1;
					stay_cap = ((int)(cap / mid) + capmore)*mid;
					if ((wid - stay_cap) >= (mode*Cslot*G))//這條路徑的頻寬-需求在這個mode下所需頻寬>=保障頻寬 刪除所需頻寬 結束
					{
						//std::cout << "1-3-1 in 刪cap cap=" << cap << "\n";
						do_num[j][1] = 2;//刪cap
						cap = 0;
						break;
					}
					else if ((wid - stay_cap) == 0)//這條路徑的頻寬=需求在這個mode下所需頻寬 刪除路徑 結束
					{
						//std::cout << "1-3-2 in 全刪 cap=" << cap << "\n";
						do_num[j][1] = 1;//全刪
						cap = 0;
						break;
					}
					else //刪到剩保障頻寬 繼續
					{
						//std::cout << "1-3-3 in 全刪 cap=" << cap << "\n";
						cap = cap - (wid - (mode*Cslot*G));
						do_num[j][1] = 3;////刪到剩g
					}
				}
				if (cap > 0) //全做完如果還有沒刪的 浪費的頻寬數
				{
					std::cout << "浪費" << cap << "\n";
					break;
				}
				else break;
			}
			else // 這條路徑的頻寬<需求在這個mode下所需頻寬 需求頻寬有剩 刪掉這條路徑 繼續
			{
				//std::cout << "1-4 " << cap << "\n";
				cap = cap - wid;
				do_num[i][1] = 1;//全刪
			}
			path_weight = 0;//path_weight重設
		}
		path_weight = 0;//path_weight重設
		bool deletepath = false;
		/////////////////////////////////////////////////////////////
		for (i = 0; i < n; i++)//依排序後的路徑編號一個一個縮減
		{
			int path_num = do_num[i][0];
			//std::cout << "path_num="<< path_num;
			auto vertex_num = (result->second).begin();
			std::advance(vertex_num, path_num);//利用path_num去取得"第path_num條"路徑

			for (const auto& edge : (*vertex_num).edge_list)//對第一條~最後一條邊
			{
				path_weight = path_weight + get(edge_weight, graph, edge); //得到某條邊的weight ->取得路徑總長度
			}
			mode = mlvl(path_weight);//求路徑調變等級

			success_re = reduce(result, vertex_num, bit_mask_map, request, mode, n, do_num[i][1], deletepath);
			path_weight = 0;//path_weight重設
			if (deletepath)//這條路徑被刪了 在map裡所有在這條路徑以下的全部 編號-1
			{
				for (int j = 0; j < n; j++)
				{
					if (do_num[j][0]>path_num)
					{
						do_num[j][0]= do_num[j][0] - 1;
					}
				}
			}
			if (success_re == 1)
			{
				break;
			}
		}
		if (success_re == 1)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}
#endif //OPC_H
