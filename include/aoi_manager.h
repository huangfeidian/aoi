#pragma once
#include "aoi_entity.h"
#include "frozen_pool.h"

#include <unordered_map>
#include <functional>
#include <ostream>

namespace spiritsaway::aoi
{

	class aoi_interface;

	class aoi_manager
	{
	public:
		aoi_manager(aoi_interface* aoi_impl, aoi_idx_t max_entity_size, pos_unit_t max_aoi_radius, pos_t min, pos_t max);
		~aoi_manager();

		
		aoi_pos_idx add_pos_entity(guid_t guid, const pos_t& in_pos, std::uint64_t in_interested_by_flag);

		bool remove_pos_entity(aoi_pos_idx pos_idx);
		const aoi_pos_entity* get_pos_entity(aoi_pos_idx pos_idx) const;

		aoi_radius_idx add_radius_entity(aoi_pos_idx in_pos_idx, const aoi_radius_controler& aoi_radius_ctrl, const  aoi_callback_t aoi_callback);
		bool remove_radius_entity(aoi_radius_idx radius_idx);
		const aoi_radius_entity* get_radius_entity(aoi_radius_idx radius_idx) const;

		bool change_entity_radius(aoi_radius_idx radius_idx, pos_unit_t radius);
		bool change_entity_pos(aoi_pos_idx pos_idx, pos_t pos);
		// 将guid_from 加入到guid_to的强制关注列表里
		bool add_force_aoi(aoi_pos_idx pos_idx_from, aoi_radius_idx radius_idx_to);
		// 将guid_from 移除出guid_to 的强制关注列表
		bool remove_force_aoi(aoi_pos_idx pos_idx_from, aoi_radius_idx radius_idx_to);
		const std::unordered_map<aoi_pos_idx, aoi_pos_entity*>& interest_in(aoi_radius_idx radius_idx)const;
		std::vector<guid_t> interest_in_guids(aoi_radius_idx radius_idx)const;
		const std::unordered_set<aoi_radius_idx>& interested_by(aoi_pos_idx pos_idx) const;
		// 更新内部信息 返回所有有callback消息通知的entity guid 列表
		// 只有在调用update之后 所有entity的aoi interested 才是最新的正确的值
		// T 的类型应该能转换到aoi_callback_t 这里用模板是为了避免直接用aoi_callback_t时引发的虚函数开销
		void update();

		bool is_in_border(const pos_t& pos) const;
	public:
		// 获取平面区域内圆形范围内的entity列表
		std::vector<guid_t> entity_in_circle(pos_t center, pos_unit_t radius);
		// 获取平面区域内圆柱形区域内的entity列表
		std::vector<guid_t> entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height);
		// 获取 平面内矩形区域内的entity列表
		std::vector<guid_t> entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width);
		// 获取平面内正方形区域内的entity 列表
		std::vector<guid_t> entity_in_square(pos_t center, pos_unit_t width);
		// 获取长方体区域内的entity列表
		std::vector<guid_t> entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height);
		// 获取立方体区域内的entity列表
		std::vector<guid_t> entity_in_cube(pos_t center, pos_unit_t width);
		// 获取2d 扇形区域内的entity 列表 yaw为扇形中心朝向的弧度， yaw range为左右两侧的弧度偏移
		std::vector<guid_t> entity_in_fan(pos_t center, pos_unit_t radius, float yaw, float yaw_range);
		void dump(std::ostream& out_debug) const;
	private:
		aoi_pos_idx request_pos_entity_slot();
		aoi_radius_idx request_radius_entity_slot();
		void renounce_pos_entity_idx(aoi_pos_idx pos_idx);
		void renounce_radius_entity_idx(aoi_radius_idx radius_idx);
	private:
		
		std::vector<aoi_pos_entity*> m_pos_entities;
		std::vector<aoi_pos_idx> m_avail_pos_slots;
		std::unordered_set<aoi_idx_t> m_pos_entities_removed;

		std::vector<aoi_radius_entity*> m_radius_entities;
		std::vector<aoi_radius_idx> m_avail_radius_slots;
		std::unordered_set<aoi_idx_t> m_radius_entities_removed;
		aoi_interface* aoi_impl;
		pos_t min;
		pos_t max;
		pos_unit_t max_aoi_radius;
		std::unordered_map<aoi_pos_idx, aoi_pos_entity*> m_invalid_pos_result;
		std::unordered_set<aoi_radius_idx> m_invalid_radius_result;

	};
}