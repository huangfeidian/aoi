#pragma once

#include <cstdint>
#include <unordered_set>
#include <array>
#include <algorithm>


using guid_t = std::uint64_t;
using pos_unit_t = float;
using pos_t = std::array<pos_unit_t, 3>;
enum class aoi_flag:std::uint8_t
{
	ignore_enter_callback = 1,// 不处理enter消息通知
	ignore_leave_callback = 2,// 不处理leave 消息通知
	forbid_interested_by = 3,// 不会进入其他entity的aoi
	forbid_interest_in = 4,// 其他entity 不会进入当前entity的aoi
	limited_by_max = 5,// 当前类型的entity会被aoi的max_interested限制住 导致无法进入相关entity的aoi

};


struct aoi_entity
{
	guid_t guid;// 唯一标识符
	pos_unit_t radius;// aoi的接收半径 为0 代表不接受aoi信息
	pos_unit_t height;// aoi的接收高度 为0 代表无视高度
	pos_t pos;// 自己的位置
	std::unordered_set<guid_t> interest_in;// 当前进入自己aoi半径的entity guid 集合
	std::unordered_set<guid_t> interested_by;// 当前自己进入了那些entity的aoi 的guid集合
	// 上一轮计算的interest_in 集合
	std::vector<guid_t> prev_interest_in;
	// 上一轮计算的interested_by 集合
	std::vector<guid_t> prev_interested_by;

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
	bool pos_in_aoi_radius(const aoi_entity& other) const;
	bool can_pass_flag_check(const aoi_entity& other) const;
	bool can_add_enter(const aoi_entity& other, bool ignore_dist = true, bool force_add = false) const;
	bool enter_by_pos(aoi_entity& other, bool ignore_dist = false);
	void enter_impl(aoi_entity& other);

	bool enter_by_force(aoi_entity& other);
	void leave_impl(aoi_entity& other);
	bool leave_by_pos(aoi_entity& other);
	bool leave_by_force(aoi_entity& other);
	void after_update(std::unordered_set<guid_t>& enter_guids, std::unordered_set<guid_t>& leave_guids);
};



