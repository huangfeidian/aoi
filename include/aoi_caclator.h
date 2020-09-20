#pragma once
#include "aoi_entity.h"

class aoi_caclator
{
	public:
	aoi_caclator();
	virtual ~aoi_caclator();
	virtual bool add_entity(aoi_entity* entity) = 0;
	virtual bool remove_entity(aoi_entity* entity) = 0;
	// 获取平面区域内圆形范围内的entity列表
	virtual std::vector<guid_t> entity_in_circle(pos_t center, std::uint16_t radius) const = 0;
	// 获取平面区域内圆柱形区域内的entity列表
	virtual std::vector<guid_t> entity_in_cylinder(pos_t center, std::uint16_t radius, std::uint16_t height) const = 0;

	// 获取 平面内矩形区域内的entity列表
	virtual std::vector<guid_t> entity_in_rectangle(pos_t center, std::uint16_t x_width, std::uint16_t z_width) const = 0;
	// 获取长方体区域内的entity列表
	virtual std::vector<guid_t> entity_in_cuboid(pos_t center, std::uint16_t x_width, std::uint16_t z_width, std::uint16_t y_height) const = 0;
	virtual void on_position_update(aoi_entity* entity, pos_t new_pos) = 0;
	virtual void on_radius_update(aoi_entity* entity, std::uint16_t new_radius) = 0;
	virtual void update_all() = 0;
}