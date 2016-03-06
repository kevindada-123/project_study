#include <string>
#include <algorithm>
#include <vector>
#include <limits>
#include <numeric>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/tuple/tuple.hpp>

#define G 1
#define M_MAX 4
#define B 300

using namespace boost;

//this function will return maximum size of available slot blocks in b
int max_block(std::vector<int> &b)
{
	int slot = 0;
	int max = 0;
	

	for (auto i : b)
	{
		if (i == 0)
			++slot;
		else
		{
			if(slot > max)
				max = slot;
			slot = 0;
		}			
	}

	//當 bit mask 為 300 時, 經過上面的判斷式的 max 仍是 0, 在此修正
	if (slot == B)
		max = B;

	return max;
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
	return std::accumulate(b.begin(), b.end(), 0);
}

//將 d 轉換成 d', 並且寫入 edge 的 weight2 屬性
template<typename Graph, typename WeightMap,typename BitMaskMap>
void d_prime_convert(Graph &graph, WeightMap weight_map , BitMaskMap bit_mask_map)
{
	
	double d_prime;
	int we;

	using EdgeIter = typename graph_traits<Graph>::edge_iterator;
	EdgeIter edge_iter, edge_end;
	tie(edge_iter, edge_end) = edges(graph);

	typename property_traits<WeightMap>::value_type de;
	typename property_traits<BitMaskMap>::value_type bit_mask;

	using WeightMap2 = property_map<Graph, edge_weight2_t>::type;
	WeightMap2 weight_map_2 = get(edge_weight2, graph);

	for (; edge_iter != edge_end; ++edge_iter)
	{
		bit_mask = get(bit_mask_map, *edge_iter);
		de = get(weight_map, *edge_iter);

		if (max_block(bit_mask) < G)
			d_prime = INT_MAX;
		else
		{
			we = M_MAX - mlvl(de) + 1;
			//將除數轉為 double, 避免結果變為 0
			d_prime = we * (sum_bit_mask(bit_mask) + G) / double(B);
		}

		weight_map_2[*edge_iter] = d_prime;
	}
}