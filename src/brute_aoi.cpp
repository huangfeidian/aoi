#include "brute_aoi.h"

brute_aoi::brute_aoi(std::uint32_t in_max_agent, pos_unit_t in_max_aoi_radius, pos_t in_border_min, pos_t in_border_max)
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
	entity->pos = new_pos;
}

void brute_aoi::on_radius_update(aoi_entity* entity, pos_unit_t new_radius)
{
	entity->radius = new_radius;
}
std::unordered_set<aoi_entity*> brute_aoi::update_all()
{
	std::vector<aoi_entity*> temp_entities;
	temp_entities.insert(temp_entities.end(), entities.begin(), entities.end());
	
	for(auto one_entity: temp_entities)
	{
		one_entity->interest_in = one_entity->force_interest_in;
		one_entity->interested_by = one_entity->force_interested_by;
	}
	for(auto one_entity: temp_entities)
	{
		for(auto other_entity: temp_entities)
		{
			if(one_entity->guid == other_entity->guid)
			{
				continue;
			}
			one_entity->enter_by_pos(*other_entity);
		}
	}
	return entities;
}
std::unordered_set<aoi_entity*> brute_aoi::entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width)const
{
	std::unordered_set<aoi_entity*> entity_result;
	for(auto one_entity: entities)
	{
		auto diff_x = center[0] - one_entity->pos[0];
		auto diff_y = center[1] - one_entity->pos[1];
		auto diff_z = center[2] - one_entity->pos[2];
		if(diff_x < -x_width || diff_x > x_width || diff_z < -z_width || diff_z > z_width)
		{
			continue;
		}
		entity_result.insert(one_entity);
	}
	return entity_result;
}
std::unordered_set<aoi_entity*> brute_aoi::entity_in_circle(pos_t center, pos_unit_t radius)const
{
	std::unordered_set<aoi_entity*> entity_result;
	for(auto one_entity: entities)
	{
		auto diff_x = center[0] - one_entity->pos[0];
		auto diff_z = center[2] - one_entity->pos[2];
		if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
		{
			continue;
		}
		entity_result.insert(one_entity);
	}
	return entity_result;
}
std::unordered_set<aoi_entity*> brute_aoi::entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height)const
{
	std::unordered_set<aoi_entity*> entity_result;
	for(auto one_entity: entities)
	{
		auto diff_x = center[0] - one_entity->pos[0];
		auto diff_y = center[1] - one_entity->pos[1];
		auto diff_z = center[2] - one_entity->pos[2];
		if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
		{
			continue;
		}
		if(diff_y > height || diff_y < -height)
		{
			continue;
		}
		entity_result.insert(one_entity);
	}
	return entity_result;
}
std::unordered_set<aoi_entity*> brute_aoi::entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height)const
{
	std::unordered_set<aoi_entity*> entity_result;
	for(auto one_entity: entities)
	{
		auto diff_x = center[0] - one_entity->pos[0];
		auto diff_y = center[1] - one_entity->pos[1];
		auto diff_z = center[2] - one_entity->pos[2];
		if(diff_x < -x_width || diff_x > x_width || diff_z < -z_width || diff_z > z_width)
		{
			continue;
		}
		if(diff_y > y_height || diff_y < -y_height)
		{
			continue;
		}
	}
	return entity_result;
}

void brute_aoi::dump(std::ostream& out_debug) const
{
	return;
}