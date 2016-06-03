#ifndef FR_H
#define FR_H

#include <numeric> //accumulate
#include "add.hpp"

namespace boost
{

	int check_allfree_slot(const std::vector<int>& edge_bit_mask)
	{
		int all = 0;

		for (int i = 0; i<300; i++)
		{
			if (edge_bit_mask[i] == 0)
			{
				all++;
			}
		}
		return all;
	}

	int check_maxfree_slot(const std::vector<int>& edge_bit_mask)
	{
		int max_free_slot = max_block(edge_bit_mask).second;
		return max_free_slot;
	}

	template<typename Path, typename BitMaskMap>
	double path_fr(const Path& path, const BitMaskMap& bit_mask_map)
	{
		double fr;
		std::vector <int> path_maxfree_slot;
		std::vector <int> path_allfree_slot;

		int edge_maxfree_slot;
		int edge_allfree_slot;

		for (const auto& edge : path)
		{
			auto& edge_bit_mask = get(bit_mask_map, edge);
			path_allfree_slot.push_back(check_allfree_slot(edge_bit_mask));
			path_maxfree_slot.push_back(check_maxfree_slot(edge_bit_mask));


		}

		edge_maxfree_slot = std::accumulate(path_maxfree_slot.begin(), path_maxfree_slot.end(), 0);
		edge_allfree_slot = std::accumulate(path_allfree_slot.begin(), path_allfree_slot.end(), 0);
		if (edge_allfree_slot == 0)
		{
			fr = 0;
		}
		else
		{
			fr = double(edge_maxfree_slot) / double(edge_allfree_slot);
		}
		
		return fr;
	}
	//對using_path_detail_vector以fr大小進行排序, 由大到小
    template<typename UsingPathDetailVector, typename BitMaskMap>
    void resort_by_fr(UsingPathDetailVector& using_path_detail_vector,const BitMaskMap& bit_mask_map)
    {
	auto cmp = [&](const typename UsingPathDetailVector::value_type& lhs,const typename UsingPathDetailVector::value_type& rhs)
	{	
		int lhs_result = path_fr(lhs.edge_list, bit_mask_map);
		int rhs_result = path_fr(rhs.edge_list, bit_mask_map);
		return lhs_result > rhs_result; 
	};
		

		std::sort(using_path_detail_vector.begin(), using_path_detail_vector.end(), cmp);
    }

    template<typename Edge, typename BitMaskMap>
    double edge_fr(const Edge& edge, const BitMaskMap& bit_mask_map)
    {
        auto& edge_bit_mask = get(bit_mask_map, edge);
		double result;

        int maxfree_slot = check_maxfree_slot(edge_bit_mask);
        int allfree_slot = check_allfree_slot(edge_bit_mask);
        
		if (maxfree_slot == 0 && allfree_slot == 0)
			result = 0;
		else
			result = (double)maxfree_slot / (double)allfree_slot;
        
		return result;
    }
}

#endif