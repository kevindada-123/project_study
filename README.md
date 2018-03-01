# project_study
independent study project

<br>
此為 Visual Studio Community 2015 的專案  
使用 Boost Graph Library (BGL)  
<br>
Input  Graph  
![Alt text](https://github.com/a59566/project_study/raw/master/graph_1.PNG "Graph")

<br>
從 graph_input.txt 輸入圖  
格式: 
>vertex_name　vertex_name　edge_capacity　edge_weight

<br>
k-shortest path 使用 https://svn.boost.org/trac/boost/ticket/11838 的實作  
- custom_dijkstra_call.hpp  
- exclude_filter.hpp  
- yen_ksp.hpp  
