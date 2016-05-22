#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/utility.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <iostream>
#include <list>
#include <string>
#include <fstream>
#include <sstream>
#include <utility>
#include <map>
#include <tuple>
#include <ctime>
#include <vector>
#include <iterator>
#include <chrono>
#include "yen_ksp.hpp"

#define G 1
#define M_MAX 4
#define B 300
#define Cslot 12.5
#define GB 7

//全域變數 
//std::stringstream result_ss;
std::ofstream result_ss("result.txt", std::ios_base::trunc);

#include "add.hpp"
#include "debug.hpp"
#include "expand.hpp"
#include "reduce_algo.hpp"
#include "delete_algo.hpp"

using namespace boost;


//vertex & edge properties 別名
//巢狀 property typedef 下面為 property 模板的原型, 第三個參數預設為 no_property
//property<class PropertyTag, class T, class NextProperty = no_property>
using VertexProperties = property<vertex_name_t, std::string>;
using EdgeProperties = property < edge_weight_t, int, property<edge_capacity_t, int,
	property<edge_index_t, int, property<edge_weight2_t, double> > > >;

//graph 別名
//對於 EdgeList 使用 hash_setS 強制 graph 不會出現平行邊
//vecS
//undirectedS  
//VertexProperties 頂點的屬性(頂點名稱,型別string)
//EdgeProperties 邊的屬性(權重,型別int)(容量,型別int)(邊號碼,型別int)(權重2,型別int)
using Graph = adjacency_list< hash_setS, vecS, undirectedS, VertexProperties, EdgeProperties >;

//vertex & edge descriptor 別名
//可以把 descriptor 當成獨特的 ID, 每個 vertex 或 edge 都有屬於自己的 descriptor(ID)
using Vertex = graph_traits<Graph>::vertex_descriptor;		//以 Vertex 做為 vertex_descriptor 的別名
using Edge = graph_traits<Graph>::edge_descriptor;			//以 Edge 做為 edge_descriptor 的別名

//property map type 別名
//用來表示 property map 物件的型別
using VertexNameMap = property_map<Graph, vertex_name_t>::type;
using EdgeWeightMap = property_map<Graph, edge_weight_t>::type;
using EdgeCapacityMap = property_map<Graph, edge_capacity_t>::type;
using EdgeIndexMap = property_map<Graph, edge_index_t>::type;

//std::map 別名
//宣告 map 物件作為檔案讀入時的安插用 map (並非 BGL 的 property map), 為全域變數
//property map 為(Vertex, name), 對於 name 當 key 的尋找不方便
//故另行使用(name, Vertex)的 map 方便以 name 當 key 的尋找
using MapOfName = std::map<std::string, Vertex>;
MapOfName g_vertexNameMap;





//使用 map 來記錄對於同個 src & dst 的 Request 的路徑
struct UsingPathDetail
{
	std::list<Graph::edge_descriptor> edge_list;
	int slot_begin;
	int slot_num;

	bool operator== (const UsingPathDetail& rhs) const
	{
		return (this->edge_list == rhs.edge_list) && (this->slot_begin == rhs.slot_begin)
			&& (this->slot_num == rhs.slot_num);
	}

};

//UsingPathDetail 的比較原則
struct UsingPathCmp
{
	bool operator() (const UsingPathDetail& lhs, const UsingPathDetail& rhs) const
	{
		return lhs.edge_list < rhs.edge_list;
	}

};

std::map<std::pair<Vertex, Vertex>, std::multiset<UsingPathDetail, UsingPathCmp> > g_usingPaths;



//自定義 strcut, 用來表示網路需求
struct Request
{
	Vertex src;		//soucre
	Vertex dst;		//destination
	double cap;		//capacity
};

//從 filestream 建立 graph, 參數都以參照傳入
template<typename Graph>
void construct_graph(std::ifstream& file_in, Graph& g)
{
	//property map 宣告, 以 get(property_tag, graph) 函式取得 property map 物件
	VertexNameMap name_map = get(vertex_name, g);
	EdgeWeightMap weight_map = get(edge_weight, g);
	EdgeCapacityMap capacity_map = get(edge_capacity, g);
	EdgeIndexMap edge_index_map = get(edge_index, g);

	//以 property_traits 取得 property map 裡存放 value 所對應的 type
	//(key, →value←)
	//其實就是property<class PropertyTag, class T> 裡的class T
	//這邊並沒有另行使用別名, 直接宣告變數
	//typename 代表後面 →property_traits<>::value_type← 為 type
	typename property_traits<VertexNameMap>::value_type s, t;
	typename property_traits<EdgeWeightMap>::value_type weight;
	typename property_traits<EdgeCapacityMap>::value_type capacity;
	typename property_traits<EdgeIndexMap>::value_type edge_index;

	edge_index = 0;

	//用 getline() 一次讀一行
	for (std::string line; std::getline(file_in, line);)
	{
		std::istringstream(line) >> s >> t >> capacity >> weight;


		//insert 函式回傳 map 的安插結果, 型別為一個pair 
		//first 元素為新安插的位置 or 先前已安插過的位置
		//second 元素為是否有安插成功
		//安插成功的話則進行 add_vertex(), name_map 的安插
		//否則將 vertex descriptor 設定為已經在 map 裡的 vertex
		MapOfName::iterator pos;//map 物件的 iterator, 對其提領得到一組 map, (name, vertex_descriptor)
		bool inserted;
		Vertex u, v;//u,v=vertex_descriptor
		tie(pos, inserted) = g_vertexNameMap.insert(std::make_pair(s, Vertex()));
		if (inserted)//安插成功
		{
			u = add_vertex(g);//add_vertex()回傳新增到 graph 裡的 vertex 的 vertex_descriptor
			name_map[u] = s;//(property map) name_map[vertex_descriptor]=s, s 為頂點名稱
			pos->second = u;//(非 property map) g_vertexNameMap[name] = u, u 為 vertex_descriptor 
		}
		else
			u = pos->second;

		tie(pos, inserted) = g_vertexNameMap.insert(std::make_pair(t, Vertex()));
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
		Edge e;
		tie(e, inserted) = add_edge(u, v, g); //add_edge 回傳 (edge_descriptor, inserted)
		if (inserted)
		{
			weight_map[e] = weight;
			capacity_map[e] = capacity;
			edge_index_map[e] = edge_index++;
		}
		else
		{
			std::cerr << "Detect repeat edge in graph_input.txt file!!" << std::endl << std::endl;
		}


	}


}
void randreq()
{
	std::fstream reqfile("request1.txt");
	if (!reqfile)
	{
		std::cerr << "Not request1.txt file!!" << std::endl;
		//return EXIT_FAILURE;
	}
	int i, s, d;
	double C;
	std::srand((unsigned)time(NULL)); // 以時間序列當亂數種子
	for (i = 0; i < 10; ++i)
	{
		s = (std::rand() % 18);
		while (s == 0 || s == 13 || s == 15 || s == 17 || s == 16)
		{
			s = (std::rand() % 18);
		}
		d = (std::rand() % 18);
		while (d == 0 || d == 13 || d == 15 || d == 17 || d == 16 || d == s)
		{
			d = (std::rand() % 18);
		}
		C = (std::rand() % 1885) + 125;
		C = C*0.1;
		reqfile << s << " " << d << " " << C << "\n";
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

	Graph graph;

	//property map
	//以 get(property_tag, graph) 函式取得 property map 物件
	VertexNameMap name_map = get(vertex_name, graph);
	EdgeWeightMap weight_map = get(edge_weight, graph);
	EdgeCapacityMap capacity_map = get(edge_capacity, graph);


	//construct the graph把圖建立
	construct_graph(file_in, graph);

	Request request;//(src,dst,cap)

	//測試add & expand 使用request1.txt//////////////////
	//std::ifstream file_request("request1.txt");

	//測試expand 使用request2.txt/////////////////////////
	//std::ifstream file_request("request2.txt");

	//測試 add expand delete reduce 使用 resquest_test.txt
	std::ifstream file_request("request_test.txt");


	if (!file_request)
	{
		std::cerr << "No request1.txt file!!" << std::endl;
		return EXIT_FAILURE;
	}

	//exterior_properties test
	//用 2 維 vector 儲存每條 edge 的 bit_mask, 以 exterior_properties 的方式儲存
	std::vector<std::vector<int>> edge_bit_mask(num_edges(graph), std::vector<int>(B, 0));
	EdgeIndexMap edge_index_map = get(edge_index, graph);
	using IterType = std::vector<std::vector<int>>::iterator;
	using IterMap = iterator_property_map<IterType, EdgeIndexMap>;
	IterType bit_mask_iter = edge_bit_mask.begin();


	int req_num = 1;

	//設定ostringstream buffer大小為10K
	result_ss.rdbuf()->pubsetbuf(NULL, 10240);

	//計時
	auto timer_1 = std::chrono::high_resolution_clock::now();

	for (std::string line; std::getline(file_request, line);)
	{
		/////輸出分配結果測試////
		result_ss << "request " << req_num << "  ";
		/////////////////////////////

		std::string src_name, dst_name;
		std::istringstream(line) >> src_name >> dst_name >> request.cap;

		//find(key) 回傳所尋找到的第一組 (key, vaule) 的位置 (也是iterator)
		//在此使用 find 而不使用更簡單的 map[key], 確保如果所搜尋的 key 若不在 map 中會使程式報錯
		auto src_pos = g_vertexNameMap.find(src_name);
		auto dst_pos = g_vertexNameMap.find(dst_name);
		request.src = src_pos->second;
		request.dst = dst_pos->second;


		//在此判斷需求類型
		std::string req_type;
		std::pair<Vertex, Vertex> vertex_pair = std::make_pair(request.src, request.dst);
		auto find_result = g_usingPaths.find(vertex_pair);

		//(src, dst) 在 g_usingPath 未出現過 代表新增
		if (find_result == g_usingPaths.end() && request.cap > 0)
			req_type = "add";
		else if (request.cap == 0)
			req_type = "delete";
		else if (find_result != g_usingPaths.end() && request.cap > 0)
			req_type = "expand";
		else if (request.cap < 0)
			req_type = "reduce";

		/////輸出分配結果測試////
		result_ss << "mode: " << req_type << std::endl;
		result_ss << "request detail: " << "s=" << src_name << ", d=" << dst_name << ", C=" << request.cap;
		result_ss << std::endl << std::endl;
		////////////////////////

		//開始針對需求進行分配
		bool success = false;
		if (req_type == "add")
			success = add(graph, request, IterMap(bit_mask_iter, edge_index_map));
		else if (req_type == "delete")
			success = delete_algo(graph, g_usingPaths, request, IterMap(bit_mask_iter, edge_index_map));
		else if (req_type == "expand")
		{
			success = expand(graph, request, IterMap(bit_mask_iter, edge_index_map));
			if (!success)//擴充失敗時改用新增
				success = add(graph, request, IterMap(bit_mask_iter, edge_index_map));
		}
		else if (req_type == "reduce")
			success = reduce_algo(graph, g_usingPaths, request, IterMap(bit_mask_iter, edge_index_map));


		//統計數據
		if (req_num % 10 == 0)
		{
			//計時
			auto timer_2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> during_time = timer_2 - timer_1;
			//std::cout << "after " << req_num << " requests, during time: " << during_time.count() << "sec" << std::endl;
			timer_1 = std::chrono::high_resolution_clock::now();
		}

		/////輸出分配結果測試////
		result_ss << std::endl << "result: ";
		if (success)
			result_ss << "success!" << std::endl;
		else
			result_ss << "block!" << std::endl;
		result_ss << std::endl << "------------------------------------" << std::endl;
		////////////////////////

		//debug//測試//////////
		//bit_mask_print_separate(graph, edge_bit_mask, IterMap(bit_mask_iter, edge_index_map));

		req_num++;
	}

	/*std::ofstream file_result("result.txt", std::ios_base::trunc);
	file_result.rdbuf()->pubsetbuf(NULL, 10240);
	file_result << result_ss.rdbuf();
	file_result.close();*/


	//測試/////////////////
	//印出usingPaths
	/*print_usingPaths(graph, g_usingPaths);*/
	//////////////////////

	return 0;
}

