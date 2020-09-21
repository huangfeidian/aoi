#pragma once

#include <cstdint>
#include <unordered_set>
#include <array>
#include <algorithm>

// 这里的单位为最小单位cm 不再存在浮点
using guid_t = std::uint64_t;
using pos_t = std::array<std::int32_t, 3>;
enum class aoi_flag:std::uint8_t
{
	ignore_enter_callback = 1,// 不处理enter消息通知
	ignore_leave_callback = 2,// 不处理leave 消息通知
	forbid_interested_by = 3,// 不会进入其他entity的aoi
	forbid_interest_in = 4,// 其他entity 不会进入当前entity的aoi
	limited_by_max = 5,// 当前类型的entity会被aoi的max_interested限制住 导致无法进入相关entity的aoi

};

//set_result = set_1 - set_2
template <typename T>
void unordered_set_diff(const std::unordered_set<T>& set_1, const std::unordered_set<T>& set_2, std::unordered_set<T>& set_result)
{
	for(auto one_item: set_1)
	{
		if(set_2.find(one_guid) == set_2.end())
		{
			set_result.insert(one_item);
		}
	}
}

struct aoi_entity
{
	guid_t guid;// 唯一标识符
	std::uint16_t radius;// aoi的接收半径 为0 代表不接受aoi信息
	std::uint16_t height;// aoi的接收高度 为0 代表无视高度
	pos_t pos;// 自己的位置
	std::unordered_set<guid_t> interest_in;// 当前进入自己aoi半径的entity guid 集合
	std::unordered_set<guid_t> interested_by;// 当前自己进入了那些entity的aoi 的guid集合
	// 上一轮计算的interest_in 集合
	std::vector<guid_t> prev_interest_in;
	// 上一轮计算的interested_by 集合
	std::vector<guid_t> prev_interested_by;

	std::unordered_set<guid_t> leave_guids;//离开当前entity aoi 半径的guid列表
	std::unordered_set<guid_t> enter_guids;// 进入当前entity aoi半径的guid列表
	std::unordered_set<guid_t> force_interest_in;// 忽略半径强制加入到当前entity aoi列表的guid
	std::unordered_set<guid_t> force_interested_by;// 因为强制aoi而进入的其他entity的guid 集合
	std::uint32_t flag;//aoi相关flag
	std::uint16_t max_interest_in;//当前aoi内最大可接受数量 这个并不是硬性数量 而是超过此数量时某些类型的entity将无法进入aoi
	// 这个是aoi计算器所需的数据 只在计算器内部使用
	void* cacl_data = nullptr;

	// 判断是否有特定标志位
	
	bool has_flag(aoi_flag cur_flag) const
	{
		return (flag & (1 << std::uint8_t(cur_flag))) != 0;
	}
	// 判断能否加入到aoi
	bool pos_in_aoi_radius(const aoi_entity& other) const
	{
		int32_t diff_x = pos[0] - other.pos[0];
		int32_t diff_y = pos[1] - other.pos[1];
		int32_t diff_z = pos[2] - other.pos[2];
		if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
		{
			return false;
		}
		if(height)
		{
			if(diff_z > height || diff_z < -height)
			{
				return false;
			}
		}
		return true;
	}
	bool can_add_enter(const aoi_entity& other, bool ignore_dist = true, bool force_add = false) const
	{
		if(other.guid == guid)
		{
			return false;
		}
		if(has_flag(aoi_flag::forbid_interest_in))
		{
			return false;
		}
		if(other.has_flag(aoi_flag::forbid_interested_by))
		{
			return false;
		}
		if(!force_add && other.has_flag(aoi_flag::limited_by_max) && interest_in.size() >= max_interest_in)
		{
			return false;
		}
		if(interest_in.find(other.guid) == interest_in.end())
		{
			return false;
		}
		if(!ignore_dist)
		{
			if(!pos_in_aoi_radius(other))
			{
				return false;
			}
		}
		return true;
	}
	bool enter_by_pos(aoi_entity& other, bool ignore_dist = false)
	{
		if(!can_add_enter(other, ignore_dist))
		{
			return false;
		}
		enter_impl(other);
		return true;
	}
	void enter_impl(aoi_entity& other)
	{
		if(!leave_guids.erase(other.guid))
		{
			enter_guids.insert(other.guid);
		}
		
		interest_in.insert(other.guid);
		other.interested_by.insert(guid);
	}

	bool enter_by_force(aoi_entity& other)
	{
		if(!can_add_enter(other, true, true))
		{
			return false;
		}
		if(force_interest_in.find(other.guid) != force_interest_in.end())
		{
			return false;
		}
		force_interest_in.insert(other.guid);
		other.force_interested_by.insert(guid);
		enter_impl(other);
		return true;
	}
	void leave_impl(aoi_entity& other)
	{
		if(!enter_guids.erase(other.guid))
		{
			leave_guids.insert(other.guid);
		}
		interest_in.erase(other.guid);
		other.interested_by.erase(guid);

	}
	bool leave_by_pos(aoi_entity& other)
	{
		if(interest_in.find(other.guid) == interest_in.end())
		{
			return false;
		}
		if(force_interest_in.find(other.guid) != force_interest_in.end())
		{
			return false;
		}
		leave_impl(other);
	}
	bool leave_by_force(aoi_entity& other)
	{
		if(force_interest_in.erase(other.guid) != 1)
		{
			return false;
		}
		other.force_interested_by.erase(guid);
		
		if(interest_in.find(other.guid) == interest_in.end())
		{
			return false;
		}
		if(!pos_in_aoi_radius(other))
		{
			leave_impl(other);
			return true;
		}
		else
		{
			return false;
		}
	}
	void after_update()
	{
		// 处理完成之后 更新之前的记录 
		prev_interest_in.clear();
		prev_interested_by.clear();
		std::insert(prev_interest_in.end(), interest_in.begin(), interest_in.end());
		std::insert(prev_interested_by.end(), interested_by.begin(), interested_by.end());

		enter_guids.clear();
		leave_guids.clear();
	}
};



