#pragma once
#include <unordered_set>
//set_result = set_1 - set_2
template <typename T>
void unordered_set_diff(const std::unordered_set<T>& set_1, const std::unordered_set<T>& set_2, std::unordered_set<T>& set_result)
{
	for(auto one_item: set_1)
	{
		if(set_2.find(one_item) == set_2.end())
		{
			set_result.insert(one_item);
		}
	}
}
//set_result = item in set_1 or item in set_2 but not in both sets
template <typename T>
void unordered_set_sym_diff(const std::unordered_set<T>& set_1, const std::unordered_set<T>& set_2, std::unordered_set<T>& set_result)
{
	for(auto one_item: set_1)
	{
		if(set_2.find(one_item) == set_2.end())
		{
			set_result.insert(one_item);
		}
	}
	for(auto one_item: set_2)
	{
		if(set_1.find(one_item) == set_1.end())
		{
			set_result.insert(one_item);
		}
	}
}

template <typename T>
void unordered_set_join(const std::unordered_set<T>& set_1, const std::unordered_set<T>& set_2, std::unordered_set<T>& set_result)
{
	for(auto one_item: set_1)
	{
		if(set_2.find(one_item) != set_2.end())
		{
			set_result.insert(one_item);
		}
	}
}

template <typename T>
void unordered_set_union(const std::unordered_set<T>& set_1, const std::unordered_set<T>& set_2, std::unordered_set<T>& set_result)
{
	set_result = set_1;
	for(auto one_item: set_2)
	{
		set_result.insert(one_item);
	}
}