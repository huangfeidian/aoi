#pragma once
#include "aoi_interface.h"
#include "frozen_pool.h"
#include "axis_list.h"

namespace spiritsaway::aoi
{


	// 由于这个是2d坐标轴的 区间比较 所以只支持矩形区域 不支持圆形区域
	// 所以 enter_by_pos 这个ignore dist需要为false
	class list_2d_aoi : public aoi_interface
	{
	private:
		axis_list x_axis;
		axis_list z_axis;
		mutable std::vector<std::uint8_t> m_entity_byteset;
	public:
		list_2d_aoi(aoi_idx_t max_entity_size, pos_unit_t max_aoi_radius, pos_t border_min, pos_t border_max);
		bool add_pos_entity(aoi_pos_entity* cur_entity) override;
		bool remove_pos_entity(aoi_pos_entity* cur_entity) override;
		bool add_radius_entity(aoi_radius_entity* cur_entity) override;
		bool remove_radius_entity(aoi_radius_entity* cur_entity) override;
		void on_position_update(aoi_pos_entity* cur_entity, pos_t new_pos) override;
		void on_radius_update(aoi_radius_entity* cur_entity, pos_unit_t radius) override;
		std::vector<aoi_pos_entity*> entity_in_circle(pos_t center, pos_unit_t radius) const override;
		std::vector<aoi_pos_entity*> entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height)const override;
		std::vector<aoi_pos_entity*> entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width)const override;
		std::vector<aoi_pos_entity*> entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height)const override;
		void update_all() override;
		void dump(std::ostream& out_debug) const override;
		std::vector<aoi_pos_entity*> merge_result(const std::vector<aoi_pos_entity*>& axis_x_result, const std::vector<aoi_pos_entity*>& axis_z_result) const;
	};
}