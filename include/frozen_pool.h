#pragma once

#include <vector>

namespace spiritsaway::aoi
{

	template <typename T>
	class frozen_pool
	{
	private:
		std::vector<T> data;
		std::vector<std::size_t> avail_indexes;
		std::size_t cur_avail_idx = 0;
		std::size_t size;
		std::size_t m_used_count = 0;
	public:
		frozen_pool(std::size_t in_size)
			: data(in_size)
			, avail_indexes(in_size)
			, size(in_size)
		{
			for (std::size_t i = 0; i < size; i++)
			{
				avail_indexes[i] = i + 1;
			}
		}
		T* pull()
		{
			if (cur_avail_idx >= size)
			{
				return nullptr;
			}
			T* result = data.data() + cur_avail_idx;
			auto& next = avail_indexes[cur_avail_idx];
			cur_avail_idx = next;
			next = size;
			m_used_count++;
			return result;
		}
		void push(T* element)
		{
			auto offset = element - data.data();
			if (offset < 0 || offset >= int(size))
			{
				return;
			}
			if (avail_indexes[offset] != size)
			{
				return;
			}
			avail_indexes[offset] = cur_avail_idx;
			cur_avail_idx = offset;
			m_used_count--;
		}
		std::size_t used_count() const
		{
			return m_used_count;
		}

	};
}