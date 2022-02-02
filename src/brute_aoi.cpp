#include "brute_aoi.h"

using namespace spiritsaway::aoi;
brute_aoi::brute_aoi(aoi_idx_t in_max_agent, pos_unit_t in_max_aoi_radius, pos_t in_border_min, pos_t in_border_max)
: aoi_interface(in_max_agent, in_max_aoi_radius, in_border_min, in_border_max)
{

}
brute_aoi::~brute_aoi()
{

}

bool brute_aoi::add_entity(aoi_entity* entity)
{
	entities.insert(entity);
	return true;
}
bool brute_aoi::remove_entity(aoi_entity* entity)
{
	entities.erase(entity);
	return true;
}

void brute_aoi::on_position_update(aoi_entity* entity, pos_t new_pos)
{
	entity->set_pos(new_pos);
}

void brute_aoi::on_radius_update(aoi_entity* entity, pos_unit_t new_radius)
{
	entity->set_radius(new_radius);
}
void brute_aoi::update_all(const std::vector<aoi_entity*>& all_entities)
{
	std::vector<aoi_entity*> temp_entities;
	std::vector<std::uint8_t> diff_vec(max_agent + 2);
	temp_entities.insert(temp_entities.end(), entities.begin(), entities.end());
	
	for(auto one_entity: temp_entities)
	{
		std::vector<aoi_entity*> cur_remain_interest_in;
		for(auto other_entity: temp_entities)
		{
			if(one_entity->guid() == other_entity->guid())
			{
				continue;
			}
			if (!one_entity->can_pass_flag_check(*other_entity))
			{
				continue;
			}
			if (!one_entity->pos_in_aoi_radius(*other_entity))
			{
				continue;
			}
			cur_remain_interest_in.push_back(other_entity);
		}
		one_entity->update_by_pos(cur_remain_interest_in, all_entities, diff_vec);
	}
}
std::vector<aoi_entity*> brute_aoi::entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width)const
{
	std::vector<aoi_entity*> entity_result;
	for(auto one_entity: entities)
	{
		auto diff_x = center[0] - one_entity->pos()[0];
		auto diff_y = center[1] - one_entity->pos()[1];
		auto diff_z = center[2] - one_entity->pos()[2];
		if(diff_x < -x_width || diff_x > x_width || diff_z < -z_width || diff_z > z_width)
		{
			continue;
		}
		entity_result.push_back(one_entity);
	}
	return entity_result;
}
std::vector<aoi_entity*> brute_aoi::entity_in_circle(pos_t center, pos_unit_t radius)const
{
	std::vector<aoi_entity*> entity_result;
	for(auto one_entity: entities)
	{
		auto diff_x = center[0] - one_entity->pos()[0];
		auto diff_z = center[2] - one_entity->pos()[2];
		if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
		{
			continue;
		}
		entity_result.push_back(one_entity);
	}
	return entity_result;
}
std::vector<aoi_entity*> brute_aoi::entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height)const
{
	std::vector<aoi_entity*> entity_result;
	for(auto one_entity: entities)
	{
		auto diff_x = center[0] - one_entity->pos()[0];
		auto diff_y = center[1] - one_entity->pos()[1];
		auto diff_z = center[2] - one_entity->pos()[2];
		if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
		{
			continue;
		}
		if(diff_y > height || diff_y < -height)
		{
			continue;
		}
		entity_result.push_back(one_entity);
	}
	return entity_result;
}
std::vector<aoi_entity*> brute_aoi::entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height)const
{
	std::vector<aoi_entity*> entity_result;
	for(auto one_entity: entities)
	{
		auto diff_x = center[0] - one_entity->pos()[0];
		auto diff_y = center[1] - one_entity->pos()[1];
		auto diff_z = center[2] - one_entity->pos()[2];
		if(diff_x < -x_width || diff_x > x_width || diff_z < -z_width || diff_z > z_width)
		{
			continue;
		}
		if(diff_y > y_height || diff_y < -y_height)
		{
			continue;
		}
		entity_result.push_back(one_entity);
	}
	return entity_result;
}

void brute_aoi::dump(std::ostream& out_debug) const
{
	return;
}