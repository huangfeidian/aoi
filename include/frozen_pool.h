#pragma once

#include <vector>

template <typename T>
class frozen_pool
{
private:
	std::vector<T> data;
	std::vector<std::size_t> avail_indexes;
	std::size_t cur_avail_idx = 0;
	std::size_t size;
public:
	frozen_pool(std::size_t in_size)
	: data(in_size)
	, avail_indexes(in_size)
	, size(in_size)
	{
		for(std::size_t i = 0; i < size; i++)
		{
			avail_indexes[i] = i + 1;
		}
	}
	T* request()
	{
		if(cur_avail_idx >= size)
		{
			return nullptr;
		}
		T* result = data.data() + cur_avail_idx;
		auto& next = avail_indexes[cur_avail_idx];
		cur_avail_idx = next;
		next = size;
		return result;
	}
	void renounce(T* element)
	{
		auto offset = element - data.data();
		if(offset < 0 || offset >= size)
		{
			return;
		}
		if(avail_indexes[offset] != size)
		{
			return;
		}
		avail_indexes[offset] = cur_avail_idx;
		cur_avail_idx = offset;
	}

};