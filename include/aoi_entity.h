#pragma once

#include <cstdint>
#include <unordered_set>
#include <array>
#include <algorithm>
#include <functional>

namespace spiritsaway::aoi
{


	using guid_t = std::uint64_t;

	using aoi_idx_t = std::uint16_t; // aoi entity 的索引 0代表非法
	struct aoi_pos_idx
	{
		aoi_idx_t value;
		bool operator==(const aoi_pos_idx& other) const
		{
			return value == other.value;
		}
		bool operator<(const aoi_pos_idx& other) const
		{
			return value < other.value;
		}
	};
	struct aoi_radius_idx
	{
		aoi_idx_t value;
		bool operator==(const aoi_radius_idx& other) const
		{
			return value == other.value;
		}

		bool operator<(const aoi_radius_idx& other) const
		{
			return value < other.value;
		}
	};
	using pos_unit_t = double;
	using pos_t = std::array<pos_unit_t, 3>;
	struct aoi_notify_info
	{
		guid_t other_guid;
		aoi_radius_idx self_radius_idx;
		aoi_pos_idx other_pos_idx;

		bool is_enter;
	};


}
namespace std
{
	template <>
	struct hash< spiritsaway::aoi::aoi_pos_idx>
	{
		std::size_t operator()(const spiritsaway::aoi::aoi_pos_idx& a) const
		{
			return std::hash< spiritsaway::aoi::aoi_idx_t>()(a.value);
		}
	};

	template <>
	struct hash< spiritsaway::aoi::aoi_radius_idx>
	{
		std::size_t operator()(const spiritsaway::aoi::aoi_radius_idx& a) const
		{
			return std::hash< spiritsaway::aoi::aoi_idx_t>()(a.value);
		}
	};
}

namespace spiritsaway::aoi
{

	struct aoi_radius_controler
	{
		std::uint64_t any_flag = 0; // 拥有其中任何一个flag都可以进入当前radius
		std::uint64_t need_flag = 0; // 必须拥有这些flag才能进入当前radius
		std::uint64_t forbid_flag = 0;// 禁止有这些flag的entity进入当前radius
		std::uint16_t max_interest_in = 10;//当前aoi内最大可接受数量 这个并不是硬性数量 而是超过此数量时某些类型的entity将无法进入aoi
		pos_unit_t radius = 0;// aoi的接收半径 为0 代表不接受aoi信息
		pos_unit_t min_height = 0; // 目标与当前entity的高度差不得小于这个值
		pos_unit_t max_height = 0; // 目标与当前entity的高度差不得大于这个值
		
	};


	class aoi_pos_entity;
	class aoi_radius_entity
	{
	public:
		friend class aoi_pos_entity;
		const aoi_radius_idx m_radius_idx; // 在aoi系统里的唯一标识符
		const bool m_is_radius_circle; // 是否用圆来判断半径


	private:
		
		std::unordered_map<aoi_pos_idx, aoi_pos_entity*> m_interest_in;// 当前进入自己aoi半径的entity guid 集合
		


		std::unordered_map<aoi_pos_idx, aoi_pos_entity*> m_force_interest_in;// 忽略半径强制加入到当前entity aoi列表的guid
		std::vector<std::uint8_t> interest_in_bitset; // bitmap 
		
		aoi_radius_controler m_aoi_radius_ctrl;
		aoi_pos_entity* m_owner = nullptr;
		
	public:
		aoi_radius_idx radius_idx() const
		{
			return m_radius_idx;
		}
		inline const std::unordered_map<aoi_pos_idx, aoi_pos_entity*>& interest_in() const
		{
			return m_interest_in;
		}
		
		inline const std::unordered_map<aoi_pos_idx, aoi_pos_entity*>& force_interest_in() const
		{
			return m_force_interest_in;
		}
		
		

		inline void set_radius(const pos_unit_t new_radius)
		{
			m_aoi_radius_ctrl.radius = new_radius;
		}
		inline pos_unit_t radius() const
		{
			return m_aoi_radius_ctrl.radius;
		}
	public:
		aoi_radius_entity(bool is_radius_circle, aoi_radius_idx in_radius_idx, aoi_idx_t max_ent_sz)
			: m_radius_idx(in_radius_idx)
			, interest_in_bitset(max_ent_sz / 8 + 1)
			, m_is_radius_circle(is_radius_circle)
		{

		}
		aoi_pos_entity& owner() 
		{
			return *m_owner;
		}
		~aoi_radius_entity()
		{

		}
		bool check_radius(pos_unit_t diff_x, pos_unit_t diff_z) const;
		bool check_height(pos_unit_t diff_height) const;
		
		const aoi_radius_controler& aoi_radius_ctrl() const
		{
			return m_aoi_radius_ctrl;
		}

		inline bool has_interest_in(const aoi_pos_idx other_aoi_idx) const
		{
			auto byte_offset = other_aoi_idx.value / 8;
			auto bit_offset = other_aoi_idx.value % 8;
			return (interest_in_bitset[byte_offset] & (1 << bit_offset));
		}
		
		void set_interest_in(aoi_pos_entity* other);

		void remove_interest_in(aoi_pos_entity* other);

		// 判断能否加入到aoi
		bool pos_in_aoi_radius(const aoi_pos_entity & other) const;
		bool check_flag(const aoi_pos_entity& other) const;
		bool can_add_enter(const aoi_pos_entity& other, bool ignore_dist = true, bool force_add = false) const;
		bool enter_by_pos(aoi_pos_entity& other, bool ignore_dist = false);
		void enter_impl(aoi_pos_entity& other);

		bool enter_by_force(aoi_pos_entity& other);
		void leave_impl(aoi_pos_entity& other);
		bool leave_by_pos(aoi_pos_entity& other);
		bool leave_by_force(aoi_pos_entity& other); // 取消基于强制关注的aoi 此时可能停留在aoi列表里 因为还有基于距离的aoi
		bool try_enter(aoi_pos_entity& other);
		bool try_leave(aoi_pos_entity& other);

		inline guid_t guid() const;
		void activate(aoi_pos_entity* in_owner, const aoi_radius_controler& aoi_radius_ctrl);
		void deactivate();
		void check_remove(); // 检查剩下的是否已经非法了


		const pos_t& pos() const;
		


	};

	class aoi_pos_entity
	{
	private:
		guid_t m_guid = 0;
		const aoi_pos_idx m_pos_idx;
		pos_t m_pos = { 0.0, 0.0, 0.0 };// 自己的位置
		std::unordered_set<aoi_radius_idx> m_interested_by;// 当前自己进入了那些radius 的 radius_idx集合
		std::vector<std::uint8_t> interested_by_bitset;
		std::vector<aoi_radius_entity*> m_radius_entities; // 注册在当前pos上的所有radius radius从大到小排序
		std::uint64_t m_interested_by_flag = 0;// 自己能被哪些flag的radius关注
		std::vector<aoi_notify_info> m_aoi_notify_infos;
		bool m_during_aoi_cb = false; // 避免在触发aoi cb时递归触发
	public:
		// 这个是aoi计算器所需的数据 只在计算器内部使用
		void* cacl_data = nullptr;

		aoi_pos_entity(aoi_pos_idx in_pos_idx, aoi_idx_t max_radius_sz)
			: interested_by_bitset(max_radius_sz /8 + 1)
			, m_pos_idx(in_pos_idx)
		{

		}
		inline const pos_t& pos() const
		{
			return m_pos;
		}
		void set_pos(const pos_t& new_pos)
		{
			m_pos = new_pos;
		}
		aoi_pos_idx pos_idx() const
		{
			return m_pos_idx;
		}
		inline const std::unordered_set<aoi_radius_idx>& interested_by() const
		{
			return m_interested_by;
		}
		inline bool has_interested_by(const aoi_radius_idx other_aoi_idx) const
		{
			auto byte_offset = other_aoi_idx.value / 8;
			auto bit_offset = other_aoi_idx.value % 8;
			return (interested_by_bitset[byte_offset] & (1 << bit_offset));
		}
		inline void set_interested_by(const aoi_radius_idx other_aoi_idx)
		{
			auto byte_offset = other_aoi_idx.value / 8;
			auto bit_offset = other_aoi_idx.value % 8;
			interested_by_bitset[byte_offset] |= (1 << bit_offset);
			m_interested_by.insert(other_aoi_idx);
		}
		inline void remove_interested_by(const aoi_radius_idx other_aoi_idx)
		{
			auto byte_offset = other_aoi_idx.value / 8;
			auto bit_offset = other_aoi_idx.value % 8;
			interested_by_bitset[byte_offset] ^= (1 << bit_offset);
			m_interested_by.erase(other_aoi_idx);
		}

		guid_t guid() const
		{
			return m_guid;
		}
		pos_unit_t max_radius() const;
		std::uint64_t interested_by_flag() const
		{
			return m_interested_by_flag;
		}

		void activate(const pos_t& in_pos, std::uint64_t in_interested_by_flag, guid_t in_guid);
		void deactivate();
		void add_radius_entity(aoi_radius_entity* in_radius_entity, const aoi_radius_controler& aoi_radius_ctrl);

		void check_add(aoi_pos_entity* other);
		void check_remove();

		aoi_radius_entity* remove_radius_entity(aoi_radius_idx cur_radius_idx);

		const std::vector<aoi_radius_entity*>& radius_entities() const
		{
			return m_radius_entities;
		}
		void add_aoi_notify(const aoi_pos_entity& other, aoi_radius_idx radius_idx, bool is_enter);

		void invoke_aoi_cb(const std::function<void(guid_t, const std::vector<aoi_notify_info>&)>& aoi_cb);
	};



}

