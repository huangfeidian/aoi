#pragma once
#include "aoi_interface.h"

class brute_aoi: public aoi_interface
{
private:
	std::unordered_set<aoi_entity*> entities;
public:
	brute_aoi(std::uint32_t in_max_agent, pos_unit_t in_max_aoi_radius, pos_t in_border_min, pos_t in_border_max);

	~brute_aoi();

	bool add_entity(aoi_entity* entity);
	bool remove_entity(aoi_entity* entity);

	void on_position_update(aoi_entity* entity, pos_t new_pos) override;
	void on_radius_update(aoi_entity* entity, pos_unit_t new_radius) override;
	std::unordered_set<aoi_entity*> update_all() override;
	std::unordered_set<aoi_entity*> entity_in_circle(pos_t center, pos_unit_t radius) const override;

	std::unordered_set<aoi_entity*> entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height) const override;
	std::unordered_set<aoi_entity*> entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width) const override;

	std::unordered_set<aoi_entity*> entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height) const override;
	void dump(std::ostream& out_debug) const override;
};