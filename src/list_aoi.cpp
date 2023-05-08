#include "list_aoi.h"
#include <cmath>
#include <iostream>
using namespace spiritsaway::aoi;
list_2d_aoi::list_2d_aoi(aoi_idx_t in_max_agent,  aoi_idx_t in_max_radius_size, pos_unit_t in_max_aoi_radius, pos_t in_border_min, pos_t in_border_max)
: aoi_interface(in_max_agent, in_max_aoi_radius, in_border_min, in_border_max)
, m_x_axis(in_max_agent, in_max_radius_size, in_max_aoi_radius, 0, in_border_min[0], in_border_max[0])
, m_y_axis(in_max_agent, in_max_radius_size, in_max_aoi_radius, 1, in_border_min[1], in_border_max[1])
, m_z_axis(in_max_agent, in_max_radius_size, in_max_aoi_radius, 2, in_border_min[2], in_border_max[2])
, m_entity_byteset(in_max_agent)
{
	m_axis_ptrs[0] = &m_x_axis;
	m_axis_ptrs[1] = &m_y_axis;
	m_axis_ptrs[2] = &m_z_axis;
}
bool list_2d_aoi::add_pos_entity(aoi_pos_entity* cur_entity)
{
	
	m_x_axis.insert_pos_entity(cur_entity);
	m_y_axis.insert_pos_entity(cur_entity);
	m_z_axis.insert_pos_entity(cur_entity);
	m_pos_entity_num++;
	return true;
}

bool list_2d_aoi::remove_pos_entity(aoi_pos_entity* cur_entity)
{
	

	m_x_axis.remove_pos_entity(cur_entity);
	m_y_axis.remove_pos_entity(cur_entity);
	m_z_axis.remove_pos_entity(cur_entity);
	m_pos_entity_num--;
	return true;
}

bool list_2d_aoi::add_radius_entity(aoi_radius_entity* cur_entity)
{

	m_x_axis.insert_radius_entity(cur_entity, cur_entity->radius(), 0);
	m_z_axis.insert_radius_entity(cur_entity, cur_entity->radius(), 0);
	if (!cur_entity->aoi_radius_ctrl().ignore_height())
	{
		auto min_height = cur_entity->aoi_radius_ctrl().min_height;
		auto max_height = cur_entity->aoi_radius_ctrl().max_height;
		m_y_axis.insert_radius_entity(cur_entity, (max_height - min_height) / 2, (max_height + min_height) / 2);
	}
	m_radius_entity_num++;

	return true;
}

bool list_2d_aoi::remove_radius_entity(aoi_radius_entity* cur_entity)
{


	m_x_axis.remove_radius_entity(cur_entity);
	m_z_axis.remove_radius_entity(cur_entity);
	
	if (!cur_entity->aoi_radius_ctrl().ignore_height())
	{
		m_y_axis.remove_radius_entity(cur_entity);
	}
	m_radius_entity_num--;
	
	return true;
}

void list_2d_aoi::on_ctrl_update(aoi_radius_entity* cur_entity, const aoi_radius_controler& pre_ctrl)
{
	const auto& new_aoi_ctrl = cur_entity->aoi_radius_ctrl();
	auto pre_radius = pre_ctrl.radius;
	auto new_radius = new_aoi_ctrl.radius;
	auto diff_radius = new_radius - pre_radius;
	if (std::abs(diff_radius) > 0.00001f)
	{
		m_x_axis.update_entity_radius(cur_entity, diff_radius);
		m_z_axis.update_entity_radius(cur_entity, diff_radius);
	}

	if (new_aoi_ctrl.ignore_height())
	{
		if (!pre_ctrl.ignore_height())
		{
			m_y_axis.remove_radius_entity(cur_entity);
		}
	}
	else
	{
		auto min_height = new_aoi_ctrl.min_height;
		auto max_height = new_aoi_ctrl.max_height;
		if (pre_ctrl.ignore_height())
		{

			m_y_axis.insert_radius_entity(cur_entity, (max_height - min_height) / 2, (max_height + min_height) / 2);
		}
		else
		{
			auto pre_center = (pre_ctrl.min_height + pre_ctrl.max_height) / 2;
			auto pre_height_radius = (pre_ctrl.max_height - pre_ctrl.min_height) / 2;
			auto diff_center = (max_height + min_height) / 2 - pre_center;
			auto diff_height_radius = (max_height - min_height) / 2 - pre_height_radius;
			if (std::abs(diff_center) > 0.00001f || std::abs(diff_height_radius) > 0.00001f)
			{
				m_y_axis.update_entity_radius(cur_entity, diff_height_radius, diff_center);
			}
		}
	}
	

}

void list_2d_aoi::on_position_update(aoi_pos_entity* cur_entity, pos_t new_pos)
{
	auto pre_pos = cur_entity->pos();
	cur_entity->set_pos(new_pos);

	m_x_axis.update_entity_pos(cur_entity, new_pos[0] - pre_pos[0]);
	m_y_axis.update_entity_pos(cur_entity, new_pos[1] - pre_pos[1]);
	m_z_axis.update_entity_pos(cur_entity, new_pos[2] - pre_pos[2]);
	
}

void list_2d_aoi::update_all()
{
	m_dirty_count++;
	if (m_dirty_count * m_dirty_count > (m_pos_entity_num + m_radius_entity_num + 100))
	{
		m_x_axis.update_anchors();
		m_y_axis.update_anchors();
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

std::vector<aoi_pos_entity*> list_2d_aoi::merge_result(const std::vector<aoi_pos_entity*>& axis_x_result, const std::vector<aoi_pos_entity*>& axis_y_result, const std::vector<aoi_pos_entity*>& axis_z_result) const
{
	for (auto one_ent : axis_x_result)
	{
		m_entity_byteset[one_ent->pos_idx().value]++;
	}
	for (auto one_ent : axis_y_result)
	{
		m_entity_byteset[one_ent->pos_idx().value]++;
	}
	for (auto one_ent : axis_z_result)
	{
		m_entity_byteset[one_ent->pos_idx().value]++;
	}
	std::vector<aoi_pos_entity*> entity_result;
	entity_result.reserve(std::min(std::min(axis_x_result.size(), axis_z_result.size()), axis_y_result.size()));
	for (auto one_ent : axis_x_result)
	{
		if (m_entity_byteset[one_ent->pos_idx().value] == 3)
		{
			entity_result.push_back(one_ent);
		}
		m_entity_byteset[one_ent->pos_idx().value] = 0;
	}
	for (auto one_ent : axis_y_result)
	{
		m_entity_byteset[one_ent->pos_idx().value] = 0;
	}
	for (auto one_ent : axis_z_result)
	{
		m_entity_byteset[one_ent->pos_idx().value] = 0;
	}
	return entity_result;
}

int list_2d_aoi::choose_axis(pos_t center, pos_t extend, bool ignore_y) const
{
	int result = 0;
	int min_num = 65535;
	for (int i = 0; i < 3; i++)
	{
		if (i == 1 && ignore_y)
		{
			continue;
		}
		auto temp_result = m_axis_ptrs[i]->anchor_num_in_range(center[i] - extend[i], center[i] + extend[i]);
		if (temp_result < min_num)
		{
			result = i;
			min_num = temp_result;
		}
	}
	return result;
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
	auto extend = pos_t{ radius, height, radius };
	auto best_axis = choose_axis(center, extend);
	auto axis_result = m_axis_ptrs[best_axis]->entity_in_range(center[best_axis], extend[best_axis]);

	std::vector<aoi_pos_entity*> find_result;
	find_result.reserve(axis_result.size());
	auto radius_sqr = radius * radius;
	for(auto one_entity: axis_result)
	{
		pos_unit_t diff_x = center[0] - one_entity->pos()[0];
		pos_unit_t diff_y = center[1] - one_entity->pos()[1];
		pos_unit_t diff_z = center[2] - one_entity->pos()[2];
		
		if((diff_x * diff_x + diff_z * diff_z) > radius_sqr)
		{
			continue;
		}
		if (std::abs(diff_y) > height)
		{
			continue;
		}
		find_result.push_back(one_entity);
	}
	return find_result;
}
std::vector<aoi_pos_entity*> list_2d_aoi::entity_in_cuboid(pos_t center, pos_t extend)const
{
	auto best_axis = choose_axis(center, extend);
	auto axis_result = m_axis_ptrs[best_axis]->entity_in_range(center[best_axis], extend[best_axis]);
	pos_unit_t x_width, z_width, y_height;
	x_width = extend[0];
	z_width = extend[2];
	y_height = extend[1];
	std::vector<aoi_pos_entity*> find_result;
	find_result.reserve(axis_result.size());
	for (auto one_entity : axis_result)
	{
		pos_unit_t diff_x = center[0] - one_entity->pos()[0];
		pos_unit_t diff_y = center[1] - one_entity->pos()[1];
		pos_unit_t diff_z = center[2] - one_entity->pos()[2];

		if (std::abs(diff_x) > x_width)
		{
			continue;
		}
		if (std::abs(diff_y) > y_height)
		{
			continue;
		}
		if (std::abs(diff_z) > z_width)
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
	m_x_axis.dump(out_debug);
	out_debug << "y_aoi list is" << std::endl;
	m_y_axis.dump(out_debug);
	out_debug << "z_aoi list is" << std::endl;
	m_z_axis.dump(out_debug);
}