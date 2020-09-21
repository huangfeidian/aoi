#include "aoi_entity.h"

void unordered_set_diff(const std::unordered_set<guid_t>& set_1, const std::unordered_set<guid_t>& set_2, std::unordered_set<guid_t>& set_result)
{
	for(auto one_guid: set_1)
	{
		if(set_2.find(one_guid) == set_2.end())
		{
			set_result.insert(one_guid);
		}
	}
}
