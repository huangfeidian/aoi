#pragma once
#include "aoi_entity.h"
#include <ostream>
namespace spiritsaway::aoi
{

	class aoi_interface
	{
	public:
		aoi_interface(aoi_idx_t in_max_agent, pos_unit_t in_max_aoi_radius, pos_t in_border_min, pos_t in_border_max)
			: max_agent(in_max_agent)
			, max_aoi_radius(in_max_aoi_radius)
			, border_min(in_border_min)
			, border_max(in_border_max)
		{

		}
		virtual ~aoi_interface()
		{

		}
		virtual bool add_radius_entity(aoi_radius_entity* entity) = 0;
		virtual bool remove_radius_entity(aoi_radius_entity* entity) = 0;
		virtual bool add_pos_entity(aoi_pos_entity* entity) = 0;
		virtual bool remove_pos_entity(aoi_pos_entity* entity) = 0;
		// 获取平面区域内圆形范围内的entity列表
		virtual std::vector<aoi_pos_entity*> entity_in_circle(pos_t center, pos_unit_t radius) const = 0;
		// 获取平面区域内圆柱形区域内的entity列表
		virtual std::vector<aoi_pos_entity*> entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height) const = 0;

		// 获取 平面内矩形区域内的entity列表
		virtual std::vector<aoi_pos_entity*> entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width) const = 0;
		// 获取长方体区域内的entity列表
		virtual std::vector<aoi_pos_entity*> entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height) const = 0;
		virtual void on_position_update(aoi_pos_entity* entity, pos_t new_pos) = 0;
		virtual void on_radius_update(aoi_radius_entity* entity, pos_unit_t diff_radius) = 0;
		virtual void update_all() = 0;
		virtual void dump(std::ostream& out_debug) const = 0;
		const aoi_idx_t max_agent;
		const pos_unit_t max_aoi_radius;
		const pos_t border_min;
		const pos_t border_max;
	};
}