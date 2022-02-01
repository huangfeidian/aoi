#pragma once

#include <cstdint>
#include <unordered_set>
#include <array>
#include <algorithm>
#include <functional>


using guid_t = std::uint64_t;
using aoi_idx_t = std::uint16_t; // aoi entity 的索引 0代表非法
using pos_unit_t = float;
using pos_t = std::array<pos_unit_t, 3>;
using aoi_callback_t = std::function<void(guid_t, guid_t, bool)>; // self_guid other_guid, is_enter
enum class aoi_flag:std::uint8_t
{
	ignore_enter_callback = 1,// 不处理enter消息通知
	ignore_leave_callback = 2,// 不处理leave 消息通知
	forbid_interested_by = 3,// 不会进入其他entity的aoi
	forbid_interest_in = 4,// 其他entity 不会进入当前entity的aoi
	limited_by_max = 5,// 当前类型的entity会被aoi的max_interested限制住 导致无法进入相关entity的aoi

};

template <typename T>
bool remove_in_vec(std::vector<T>& vec, const T& val)
{
	auto cur_idx = vec.size();
	for (int i = 0; i < vec.size(); i++)
	{
		if (vec[i] == val)
		{
			cur_idx = i;
			break;
		}
	}
	if (cur_idx == vec.size())
	{
		return false;
	}
	if (cur_idx + 1 == vec.size())
	{
		vec.pop_back();
	}
	else
	{
		std::swap(vec[cur_idx], vec.back());
		vec.pop_back();
	}
	return true;
}

struct aoi_controler
{
	std::uint8_t flag; //当前entity的特殊flag
	std::uint32_t interest_in_flag; // 当前entity允许哪些flag进入自己的aoi
	std::uint32_t interested_by_flag; // 当前entity带有哪些aoi flag
	std::uint16_t max_interest_in;//当前aoi内最大可接受数量 这个并不是硬性数量 而是超过此数量时某些类型的entity将无法进入aoi
	pos_unit_t radius;// aoi的接收半径 为0 代表不接受aoi信息
	pos_unit_t height;// aoi的接收高度 为0 代表无视高度
};

class aoi_entity
{
public:
	const aoi_idx_t aoi_idx; // 在aoi系统里的唯一标识符
	// 这个是aoi计算器所需的数据 只在计算器内部使用
	void* cacl_data = nullptr;


private:
	pos_t m_pos;// 自己的位置
	std::vector<aoi_idx_t> m_interest_in;// 当前进入自己aoi半径的entity guid 集合
	std::vector<aoi_idx_t> m_interested_by;// 当前自己进入了那些entity的aoi 的guid集合


	std::vector<aoi_idx_t> m_force_interest_in;// 忽略半径强制加入到当前entity aoi列表的guid
	std::vector<aoi_idx_t> m_force_interested_by;// 因为强制aoi而进入的其他entity的guid 集合
	std::uint8_t* interest_in_bitset; // bitmap 
	std::uint8_t* interested_by_bitset;
	aoi_controler m_aoi_ctrl;
	aoi_callback_t m_aoi_callback;
	guid_t m_guid;// 唯一标识符

public:
	inline const std::vector<aoi_idx_t>& interest_in() const
	{
		return m_interest_in;
	}
	inline const std::vector<aoi_idx_t>& interested_by() const
	{
		return m_interested_by;
	}
	inline const std::vector<aoi_idx_t>& force_interest_in() const
	{
		return m_force_interest_in;
	}
	inline const std::vector<aoi_idx_t>& force_interested_by() const
	{
		return m_force_interested_by;
	}
	inline const pos_t& pos() const
	{
		return m_pos;
	}
	inline void set_pos(const pos_t& new_pos)
	{
		m_pos = new_pos;
	}
	inline void set_radius(const pos_unit_t new_radius)
	{
		m_aoi_ctrl.radius = new_radius;
	}
	inline pos_unit_t radius() const
	{
		return m_aoi_ctrl.radius;
	}
public:
	aoi_entity(aoi_idx_t in_aoi_idx, aoi_idx_t max_ent_sz)
		: aoi_idx(in_aoi_idx)
		, interested_by_bitset(new std::uint8_t[max_ent_sz/ 8 + 1])
		, interest_in_bitset(new std::uint8_t[max_ent_sz / 8 + 1])
	{

	}
	// 判断是否有特定标志位
	
	~aoi_entity()
	{
		delete[] interest_in_bitset;
		delete[] interested_by_bitset;
	}
	const aoi_controler& aoi_ctrl() const
	{
		return m_aoi_ctrl;
	}
	bool has_flag(aoi_flag cur_flag) const
	{
		return (m_aoi_ctrl.flag & (1 << std::uint8_t(cur_flag))) != 0;
	}
	inline bool has_interest_in(const aoi_idx_t other_aoi_idx) const
	{
		auto byte_offset = other_aoi_idx / 8;
		auto bit_offset = other_aoi_idx % 8;
		return (interest_in_bitset[byte_offset] & (1 << bit_offset));
	}
	inline void set_interest_in(const aoi_idx_t other_aoi_idx)
	{
		auto byte_offset = other_aoi_idx / 8;
		auto bit_offset = other_aoi_idx % 8;
		interest_in_bitset[byte_offset] &= (1 << bit_offset);
		m_interest_in.push_back(other_aoi_idx);
	}
	inline void remove_interest_in(const aoi_idx_t other_aoi_idx)
	{
		auto byte_offset = other_aoi_idx / 8;
		auto bit_offset = other_aoi_idx % 8;
		interest_in_bitset[byte_offset] ^= (1 << bit_offset);
		remove_in_vec(m_interest_in, other_aoi_idx);
	}
	inline bool has_interested_by(const aoi_idx_t other_aoi_idx) const
	{
		auto byte_offset = other_aoi_idx / 8;
		auto bit_offset = other_aoi_idx % 8;
		return (interested_by_bitset[byte_offset] & (1 << bit_offset));
	}
	inline void set_interested_by(const aoi_idx_t other_aoi_idx)
	{
		auto byte_offset = other_aoi_idx / 8;
		auto bit_offset = other_aoi_idx % 8;
		interested_by_bitset[byte_offset] &= (1 << bit_offset);
		m_interested_by.push_back(other_aoi_idx);
	}
	inline void remove_interested_by(const aoi_idx_t other_aoi_idx)
	{
		auto byte_offset = other_aoi_idx / 8;
		auto bit_offset = other_aoi_idx % 8;
		interested_by_bitset[byte_offset] ^= (1 << bit_offset);
		remove_in_vec(m_interested_by, other_aoi_idx);
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
	bool leave_by_force(aoi_entity& other); // 取消基于强制关注的aoi 此时可能停留在aoi列表里 因为还有基于距离的aoi
	bool try_enter(aoi_entity& other);
	bool try_leave(aoi_entity& other);
	
	inline guid_t guid() const
	{
		return m_guid;
	}
	void activate(guid_t in_guid, const aoi_controler& aoi_ctrl, const pos_t& pos, const aoi_callback_t aoi_cb);
	void deactivate();

	void update_by_pos(const std::vector<aoi_entity*>& new_interest_in, const std::vector<aoi_entity*>& all_entities, std::vector<std::uint8_t>& diff_vec);
};



