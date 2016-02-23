#include "yen_ksp.hpp"

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/utility.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <iostream>
#include <list>
#include <string>
#include <fstream>
#include <utility>
#include <map>
#include <tuple>

using namespace boost;


//vertex & edge properties
//巢狀 property typedef 下面為 property 模板的原型, 第三個參數預設為 no_property
//template <class PropertyTag, class T, class NextProperty = no_property>
//struct property;
typedef property<vertex_name_t, std::string> vertex_properties;
typedef property < edge_weight_t, int, property<edge_capacity_t, int, 
	property<edge_residual_capacity_t, int> > > edge_properties;

//graph typedef
//對於 EdgeList 使用 hash_setS 強制 graph 不會出現平行邊
typedef adjacency_list< hash_setS, vecS, undirectedS, vertex_properties, edge_properties >
	graph_t;

//vertex & edge descriptor
typedef graph_traits<graph_t>::vertex_descriptor vertex_t;
typedef graph_traits<graph_t>::edge_descriptor edge_t;

//property map typedef
typedef property_map<graph_t, vertex_name_t>::type vertex_name_map_t;
typedef property_map<graph_t, edge_weight_t>::type edge_weight_map_t;
typedef property_map<graph_t, edge_capacity_t>::type edge_capacity_map_t;
typedef property_map<graph_t, edge_residual_capacity_t>::type edge_residual_capacity_map_t;

//自定義 strcut, 用來表示網路需求
struct Request
{
	std::string src;	//soucre
	std::string dst;	//destination
	int cap;			//capacity
};

//從 filestream 建立 graph, 參數都以參照傳入
template<typename graph_t>
void construct_graph(std::ifstream &file_in, graph_t &g)
{
	//property map 宣告, 以 get(property, g) 函式取得 property map 物件
	vertex_name_map_t name_map = get(vertex_name, g);	
	edge_weight_map_t weight_map = get(edge_weight, g);	
	edge_capacity_map_t capacity_map = get(edge_capacity, g);
	edge_residual_capacity_map_t residual_capacity_map = get(edge_residual_capacity, g);
	
	//以 property_traits 取得 property 對應的 value type
	typename property_traits<vertex_name_map_t>::value_type s, t;
	typename property_traits<edge_weight_map_t>::value_type weight;
	typename property_traits<edge_capacity_map_t>::value_type capacity;
	typename property_traits<edge_residual_capacity_map_t>::value_type residual_capacity;

	//宣告 map 物件作為檔案讀入時的安插用 map
	typedef std::map<std::string, vertex_t> name_map_t;
	name_map_t vertex_name_map;

	//用 getline() 一次讀一行
	for (std::string line; std::getline(file_in, line);)
	{
		std::istringstream(line) >> s >> t >> weight >> capacity;
		
		
		//insert 函式回傳 map 的安插結果, 型別為一個pair 
		//first 元素為新安插的位置 or 先前已安插過的位置
		//second 元素為是否有安插成功
		//安插成功的話則進行 add_vertex(), name_map 的安插
		//否則將 vertex descriptor 設定為已經在 map 裡的 vertex
		name_map_t::iterator pos;
		bool inserted;
		vertex_t u, v;
		tie(pos, inserted) = vertex_name_map.insert(std::make_pair(s, vertex_t()));
		if (inserted)
		{
			u = add_vertex(g);
			name_map[u] = s;
			pos->second = u;
		}
		else
			u = pos->second;

		tie(pos, inserted) = vertex_name_map.insert(std::make_pair(t, vertex_t()));
		if (inserted)
		{
			v = add_vertex(g);
			name_map[v] = t;
			pos->second = v;
		}
		else
			v = pos->second;

		//add edge
		//若對 edge 安插失敗則代表 graph_input.txt 中有重複的 edge, 顯示錯誤訊息
		edge_t e;
		tie(e, inserted) = add_edge(u, v, g);
		if (inserted)
		{
			weight_map[e] = weight;
			capacity_map[e] = capacity;
			residual_capacity = capacity;
		}
		else
		{
			std::cerr << "Detect reapat edge in graph_input.txt file!!" << std::endl << std::endl;
		}


	}


}


int main()
{
	std::ifstream file_in("graph_input.txt");

	//如果沒 graph_input.txt, 顯示錯誤訊息並離開程式
	if (!file_in)
	{
		std::cerr << "No graph_input.txt file!!" << std::endl;
		return EXIT_FAILURE;
	}

	graph_t g;

	//property map
	vertex_name_map_t name_map = get(vertex_name, g);
	edge_weight_map_t weight_map = get(edge_weight, g);
	edge_capacity_map_t capacity_map = get(edge_capacity, g);
	edge_residual_capacity_map_t residual_capacity_map = get(edge_residual_capacity, g);

	//construct the graph
	construct_graph(file_in, g);

	//k-shortest test
	typedef std::list<edge_t> Path;
	std::list<std::pair<int, Path>> r;
	vertex_t src = *(vertices(g).first);
	vertex_t dst = *(vertices(g).first + 10);
	r = yen_ksp(g, src, dst, 3);


	//k-shortest path output
	//以下的 for 迴圈使用 range-for
	int k = 1;
	for (auto k_path : r)
	{
		std::cout << k << " path: " << " weight " << k_path.first << ", ";
		std::cout << get(name_map, src) << " ";
		for (auto v : k_path.second)
		{
			std::cout << get(name_map, target(v, g)) << " ";
		}
		++k;
		std::cout << std::endl;
	}


	return 0;
}

