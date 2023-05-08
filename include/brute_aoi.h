#pragma once
#include "aoi_interface.h"

namespace spiritsaway::aoi
{
	class brute_aoi : public aoi_interface
	{
	private:
		std::unordered_set<aoi_pos_entity*> m_pos_entities;
	public:
		brute_aoi(aoi_idx_t in_max_agent, pos_unit_t in_max_aoi_radius, pos_t in_border_min, pos_t in_border_max);

		~brute_aoi();

		bool add_pos_entity(aoi_pos_entity* entity) override;
		bool remove_pos_entity(aoi_pos_entity* entity) override;

		void on_position_update(aoi_pos_entity* entity, pos_t new_pos) override;

		bool add_radius_entity(aoi_radius_entity* entity)override;
		bool remove_radius_entity(aoi_radius_entity* entity)override ;
		void on_ctrl_update(aoi_radius_entity* entity, const aoi_radius_controler& pre_ctrl) override;
		void update_all() override;
		std::vector<aoi_pos_entity*> entity_in_circle(pos_t center, pos_unit_t radius) const override;

		std::vector<aoi_pos_entity*> entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height) const override;
		std::vector<aoi_pos_entity*> entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width) const override;

		std::vector<aoi_pos_entity*> entity_in_cuboid(pos_t center, pos_t extend) const override;
		void dump(std::ostream& out_debug) const override;
	};
}
