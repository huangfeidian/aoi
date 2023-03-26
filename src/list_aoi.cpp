#include "list_aoi.h"
#include <cmath>
#include <iostream>
using namespace spiritsaway::aoi;
list_2d_aoi::list_2d_aoi(aoi_idx_t in_max_agent, pos_unit_t in_max_aoi_radius, pos_t in_border_min, pos_t in_border_max)
: aoi_interface(in_max_agent, in_max_aoi_radius, in_border_min, in_border_max)
, x_axis(in_max_agent + 1, in_max_aoi_radius, in_border_min[0], in_border_max[0])
, z_axis(in_max_agent + 1, in_max_aoi_radius, in_border_min[2], in_border_max[2])
, nodes_buffer(in_max_agent + 1)
, m_entity_byteset(in_max_agent + 1)
{

}
bool list_2d_aoi::add_entity(aoi_radius_entity* cur_entity)
{
	if(cur_entity->cacl_data)
	{
		return false;
	}
	auto cur_nodes = nodes_buffer.request();
	if(cur_nodes == nullptr)
	{
		return false;
	}
	cur_entity->cacl_data = cur_nodes;
	cur_nodes->set_entity(cur_entity);
	auto x_nodes = &(cur_nodes->x_nodes);
	auto z_nodes = &(cur_nodes->z_nodes);
	x_axis.insert_entity(x_nodes);
	z_axis.insert_entity(z_nodes);

	return true;
}

bool list_2d_aoi::remove_entity(aoi_radius_entity* cur_entity)
{
	auto cur_nodes = (axis_2d_nodes_for_entity*)(cur_entity->cacl_data);
	if(!cur_nodes)
	{
		return false;
	}
	auto x_nodes = &(cur_nodes->x_nodes);
	auto z_nodes = &(cur_nodes->z_nodes);
	x_axis.remove_entity(x_nodes);
	z_axis.remove_entity(z_nodes);

	cur_nodes->detach();
	return true;
}

void list_2d_aoi::on_radius_update(aoi_radius_entity* cur_entity, pos_unit_t radius)
{
	auto delta_radius = (int)(cur_entity->radius()) - radius;
	cur_entity->set_radius(radius);
	auto cur_nodes = (axis_2d_nodes_for_entity*)(cur_entity->cacl_data);
	auto x_nodes = &(cur_nodes->x_nodes);
	auto z_nodes = &(cur_nodes->z_nodes);
	x_axis.update_entity_radius(x_nodes, delta_radius);
	z_axis.update_entity_radius(z_nodes, delta_radius);

}

void list_2d_aoi::on_position_update(aoi_radius_entity* cur_entity, pos_t new_pos)
{
	auto cur_nodes = (axis_2d_nodes_for_entity*)(cur_entity->cacl_data);
	auto x_nodes = &(cur_nodes->x_nodes);
	auto z_nodes = &(cur_nodes->z_nodes);
	cur_entity->set_pos(new_pos);

	x_axis.update_entity_pos(x_nodes, new_pos[0] - x_nodes->middle.pos);
	z_axis.update_entity_pos(z_nodes, new_pos[2] - z_nodes->middle.pos);
	
}

void list_2d_aoi::update_all(const std::vector<aoi_radius_entity*>& all_entities)
{

}

std::vector<aoi_radius_entity*> list_2d_aoi::entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width)const
{
	auto axis_x_result = x_axis.entity_in_range(center[0] - x_width, center[0] + x_width);
	auto axis_z_result = z_axis.entity_in_range(center[2] - z_width, center[2] + z_width);
	for (auto one_ent : axis_x_result)
	{
		m_entity_byteset[one_ent->aoi_idx]++;
	}
	for (auto one_ent : axis_z_result)
	{
		m_entity_byteset[one_ent->aoi_idx]++;
	}
	std::vector<aoi_radius_entity*> entity_result;
	entity_result.reserve(std::min(axis_x_result.size(), axis_z_result.size()));
	for (auto one_ent : axis_x_result)
	{
		if (m_entity_byteset[one_ent->aoi_idx] == 2)
		{
			entity_result.push_back(one_ent);
		}
		m_entity_byteset[one_ent->aoi_idx] = 0;
	}
	for (auto one_ent : axis_z_result)
	{
		m_entity_byteset[one_ent->aoi_idx] = 0;
	}
	return entity_result;
}
std::vector<aoi_radius_entity*> list_2d_aoi::entity_in_circle(pos_t center, pos_unit_t radius)const
{
	auto axis_x_result = x_axis.entity_in_range(center[0] - radius, center[0] + radius);
	auto axis_z_result = z_axis.entity_in_range(center[2] - radius, center[2] + radius);
	std::vector<aoi_radius_entity*> entity_result;
	entity_result.reserve(std::min(axis_x_result.size(), axis_z_result.size()));
	for (auto one_ent : axis_x_result)
	{
		if (m_entity_byteset[one_ent->aoi_idx] == 2)
		{
			entity_result.push_back(one_ent);
		}
		m_entity_byteset[one_ent->aoi_idx] = 0;
	}
	for (auto one_ent : axis_z_result)
	{
		m_entity_byteset[one_ent->aoi_idx] = 0;
	};
	std::vector<aoi_radius_entity*> find_result;
	for(auto one_entity:entity_result)
	{
		pos_unit_t diff_x = center[0] - one_entity->pos()[0];
		pos_unit_t diff_z = center[2] - one_entity->pos()[2];
		if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
		{
			continue;
		}
		find_result.push_back(one_entity);
	}
	return find_result;
}
std::vector<aoi_radius_entity*> list_2d_aoi::entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height)const
{
	auto axis_x_result = x_axis.entity_in_range(center[0] - radius, center[0] + radius);
	auto axis_z_result = z_axis.entity_in_range(center[2] - radius, center[2] + radius);
	std::vector<aoi_radius_entity*> entity_result;
	entity_result.reserve(std::min(axis_x_result.size(), axis_z_result.size()));
	for (auto one_ent : axis_x_result)
	{
		if (m_entity_byteset[one_ent->aoi_idx] == 2)
		{
			entity_result.push_back(one_ent);
		}
		m_entity_byteset[one_ent->aoi_idx] = 0;
	}
	for (auto one_ent : axis_z_result)
	{
		m_entity_byteset[one_ent->aoi_idx] = 0;
	};
	std::vector<aoi_radius_entity*> find_result;
	for(auto one_entity:entity_result)
	{
		pos_unit_t diff_x = center[0] - one_entity->pos()[0];
		pos_unit_t diff_y = center[1] - one_entity->pos()[1];
		pos_unit_t diff_z = center[2] - one_entity->pos()[2];
		if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
		{
			continue;
		}
		if(diff_y > height || diff_y < -height)
		{
			continue;
		}
		find_result.push_back(one_entity);
	}
	return find_result;
}
std::vector<aoi_radius_entity*> list_2d_aoi::entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height)const
{
	auto axis_x_result = x_axis.entity_in_range(center[0] - x_width, center[0] + x_width);
	auto axis_z_result = z_axis.entity_in_range(center[2] - z_width, center[2] + z_width);
	std::vector<aoi_radius_entity*> entity_result;
	entity_result.reserve(std::min(axis_x_result.size(), axis_z_result.size()));
	for (auto one_ent : axis_x_result)
	{
		if (m_entity_byteset[one_ent->aoi_idx] == 2)
		{
			entity_result.push_back(one_ent);
		}
		m_entity_byteset[one_ent->aoi_idx] = 0;
	}
	for (auto one_ent : axis_z_result)
	{
		m_entity_byteset[one_ent->aoi_idx] = 0;
	};
	std::vector<aoi_radius_entity*> find_result;

	for(auto one_entity:entity_result)
	{
		pos_unit_t diff_y = center[1] - one_entity->pos()[1];

		if(diff_y > y_height || diff_y < -y_height)
		{
			continue;
		}
		find_result.push_back(one_entity);
	}
	return find_result;
}

void list_2d_aoi::dump(std::ostream& out_debug) const
{
	out_debug << "x_aoi list is" << std::endl;
	x_axis.dump(out_debug);
	out_debug << "z_aoi list is" << std::endl;
	auto result = z_axis.dump(out_debug);
	for (auto one_item : result)
	{
		out_debug << "xz_interest_in for guid " << one_item->entity->guid() << " begin" << std::endl;
		for (auto one_guid : one_item->xz_interest_in)
		{
			out_debug << one_guid->guid() << ", ";
		}
		out_debug << std::endl;
		out_debug << "xz_interested_by for guid " << one_item->entity->guid() << " begin" << std::endl;
		for (auto one_guid : one_item->xz_interested_by)
		{
			out_debug << one_guid->guid() << ", ";
		}
		out_debug << std::endl;
	}
}