#include "list_aoi.h"
#include <cmath>
#include <iostream>
using namespace spiritsaway::aoi;
list_2d_aoi::list_2d_aoi(aoi_idx_t in_max_agent,  aoi_idx_t in_max_radius_size, pos_unit_t in_max_aoi_radius, pos_t in_border_min, pos_t in_border_max)
: aoi_interface(in_max_agent, in_max_aoi_radius, in_border_min, in_border_max)
, m_x_axis(in_max_agent, in_max_radius_size, in_max_aoi_radius, 0, in_border_min[0], in_border_max[0])
, m_z_axis(in_max_agent, in_max_radius_size, in_max_aoi_radius, 2, in_border_min[2], in_border_max[2])
, m_entity_byteset(in_max_agent)
{

}
bool list_2d_aoi::add_pos_entity(aoi_pos_entity* cur_entity)
{
	
	m_x_axis.insert_pos_entity(cur_entity);
	m_z_axis.insert_pos_entity(cur_entity);
	m_pos_entity_num++;
	return true;
}

bool list_2d_aoi::remove_pos_entity(aoi_pos_entity* cur_entity)
{
	

	m_x_axis.remove_pos_entity(cur_entity);
	m_z_axis.remove_pos_entity(cur_entity);
	m_pos_entity_num--;
	return true;
}

bool list_2d_aoi::add_radius_entity(aoi_radius_entity* cur_entity)
{

	m_x_axis.insert_radius_entity(cur_entity);
	m_z_axis.insert_radius_entity(cur_entity);
	m_radius_entity_num++;
	return true;
}

bool list_2d_aoi::remove_radius_entity(aoi_radius_entity* cur_entity)
{


	m_x_axis.remove_radius_entity(cur_entity);
	m_z_axis.remove_radius_entity(cur_entity);
	m_radius_entity_num--;
	return true;
}

void list_2d_aoi::on_radius_update(aoi_radius_entity* cur_entity, pos_unit_t radius)
{
	auto delta_radius = (int)(cur_entity->radius()) - radius;
	cur_entity->set_radius(radius);

	m_x_axis.update_entity_radius(cur_entity, delta_radius);
	m_z_axis.update_entity_radius(cur_entity, delta_radius);

}

void list_2d_aoi::on_position_update(aoi_pos_entity* cur_entity, pos_t new_pos)
{
	auto pre_pos = cur_entity->pos();
	cur_entity->set_pos(new_pos);

	m_x_axis.update_entity_pos(cur_entity, new_pos[0] - pre_pos[0]);
	m_z_axis.update_entity_pos(cur_entity, new_pos[2] - pre_pos[2]);
	
}

void list_2d_aoi::update_all()
{
	m_dirty_count++;
	if (m_dirty_count * m_dirty_count > (m_pos_entity_num + m_radius_entity_num + 100))
	{
		m_x_axis.update_anchors();
		m_z_axis.update_anchors();
		m_dirty_count = 0;
		return;
	}
}
std::vector<aoi_pos_entity*> list_2d_aoi::merge_result(const std::vector<aoi_pos_entity*>& axis_x_result, const std::vector<aoi_pos_entity*>& axis_z_result) const
{
	for (auto one_ent : axis_x_result)
	{
		m_entity_byteset[one_ent->pos_idx().value]++;
	}
	for (auto one_ent : axis_z_result)
	{
		m_entity_byteset[one_ent->pos_idx().value]++;
	}
	std::vector<aoi_pos_entity*> entity_result;
	entity_result.reserve(std::min(axis_x_result.size(), axis_z_result.size()));
	for (auto one_ent : axis_x_result)
	{
		if (m_entity_byteset[one_ent->pos_idx().value] == 2)
		{
			entity_result.push_back(one_ent);
		}
		m_entity_byteset[one_ent->pos_idx().value] = 0;
	}
	for (auto one_ent : axis_z_result)
	{
		m_entity_byteset[one_ent->pos_idx().value] = 0;
	}
	return entity_result;
}
std::vector<aoi_pos_entity*> list_2d_aoi::entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width)const
{
	auto axis_x_result = m_x_axis.entity_in_range(center[0] - x_width, center[0] + x_width);
	auto axis_z_result = m_z_axis.entity_in_range(center[2] - z_width, center[2] + z_width);
	return merge_result(axis_x_result, axis_z_result);
	
}
std::vector<aoi_pos_entity*> list_2d_aoi::entity_in_circle(pos_t center, pos_unit_t radius)const
{
	auto axis_x_result = m_x_axis.entity_in_range(center[0] - radius, center[0] + radius);
	auto axis_z_result = m_z_axis.entity_in_range(center[2] - radius, center[2] + radius);
	auto  entity_result = merge_result(axis_x_result, axis_z_result);
	std::vector<aoi_pos_entity*> find_result;
	find_result.reserve(entity_result.size());
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
std::vector<aoi_pos_entity*> list_2d_aoi::entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height)const
{
	auto axis_x_result = m_x_axis.entity_in_range(center[0] - radius, center[0] + radius);
	auto axis_z_result = m_z_axis.entity_in_range(center[2] - radius, center[2] + radius);
	auto  entity_result = merge_result(axis_x_result, axis_z_result);
	std::vector<aoi_pos_entity*> find_result;
	find_result.reserve(entity_result.size());
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
std::vector<aoi_pos_entity*> list_2d_aoi::entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height)const
{
	auto axis_x_result = m_x_axis.entity_in_range(center[0] - x_width, center[0] + x_width);
	auto axis_z_result = m_z_axis.entity_in_range(center[2] - z_width, center[2] + z_width);
	auto  entity_result = merge_result(axis_x_result, axis_z_result);
	
	std::vector<aoi_pos_entity*> find_result;
	find_result.reserve(entity_result.size());
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
	//out_debug << "x_aoi list is" << std::endl;
	//x_axis.dump(out_debug);
	//out_debug << "z_aoi list is" << std::endl;
	//auto result = z_axis.dump(out_debug);
	//for (auto one_item : result)
	//{
	//	out_debug << "xz_interest_in for guid " << one_item->entity->guid() << " begin" << std::endl;
	//	for (auto one_guid : one_item->xz_interest_in)
	//	{
	//		out_debug << one_guid->guid() << ", ";
	//	}
	//	out_debug << std::endl;
	//	out_debug << "xz_interested_by for guid " << one_item->entity->guid() << " begin" << std::endl;
	//	for (auto one_guid : one_item->xz_interested_by)
	//	{
	//		out_debug << one_guid->guid() << ", ";
	//	}
	//	out_debug << std::endl;
	//}
}