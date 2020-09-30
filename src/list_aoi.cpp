#include "list_aoi.h"
#include <cmath>


list_2d_aoi::list_2d_aoi(std::uint32_t in_max_agent, std::uint16_t in_max_aoi_radius, pos_t in_border_min, pos_t in_border_max)
: aoi_interface(in_max_agent, in_max_aoi_radius, in_border_min, in_border_max)
, x_axis(in_max_agent, in_max_aoi_radius, in_border_min[0], in_border_max[0])
, z_axis(in_max_agent, in_max_aoi_radius, in_border_min[2], in_border_max[2])
, nodes_buffer(in_max_agent)
{

}
bool list_2d_aoi::add_entity(aoi_entity* cur_entity)
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
	update_info.clear();
	update_info.merge(x_axis.get_update_info(), z_axis.get_update_info());
	for(auto one_entity: update_info.enter_entities)
	{
		cur_nodes->enter(one_entity);
	}
	dirty_entities.insert(cur_entity);
	for(auto one_entity: update_info.enter_notify_entities)
	{
		if(((axis_2d_nodes_for_entity*)(one_entity->cacl_data))->enter(cur_entity))
		{
			dirty_entities.insert(one_entity);
		}
	}
	return true;
}

bool list_2d_aoi::remove_entity(aoi_entity* cur_entity)
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
	dirty_entities.insert(cur_entity);
	for(auto one_entity:cur_nodes->xz_interested_by)
	{
		dirty_entities.insert(one_entity);
	}
	cur_nodes->detach();
	return true;
}

void list_2d_aoi::on_radius_update(aoi_entity* cur_entity, std::uint16_t radius)
{
	auto delta_radius = (int)(cur_entity->radius) - radius;
	auto cur_nodes = (axis_2d_nodes_for_entity*)(cur_entity->cacl_data);
	auto x_nodes = &(cur_nodes->x_nodes);
	auto z_nodes = &(cur_nodes->z_nodes);
	x_axis.update_entity_radius(x_nodes, delta_radius);
	z_axis.update_entity_radius(z_nodes, delta_radius);
	update_info.clear();
	update_info.merge(x_axis.get_update_info(), z_axis.get_update_info());
	for(auto one_entity: update_info.enter_entities)
	{
		cur_nodes->enter(one_entity);
	}
	for(auto one_entity: update_info.leave_entities)
	{
		cur_nodes->leave(one_entity);
	}
	dirty_entities.insert(cur_entity);
}

void list_2d_aoi::on_position_update(aoi_entity* cur_entity, pos_t new_pos)
{
	auto cur_nodes = (axis_2d_nodes_for_entity*)(cur_entity->cacl_data);
	auto x_nodes = &(cur_nodes->x_nodes);
	auto z_nodes = &(cur_nodes->z_nodes);
	x_axis.update_entity_pos(x_nodes, new_pos[0]);
	z_axis.update_entity_pos(z_nodes, new_pos[2]);
	cur_entity->pos = new_pos;
	update_info.clear();
	update_info.merge(x_axis.get_update_info(), z_axis.get_update_info());
	for(auto one_entity: update_info.enter_entities)
	{
		cur_nodes->enter(one_entity);
	}
	for(auto one_entity: update_info.leave_notify_entities)
	{
		cur_nodes->leave(one_entity);
	}
	dirty_entities.insert(cur_entity);

	for(auto one_entity: update_info.enter_notify_entities)
	{
		if(((axis_2d_nodes_for_entity*)(one_entity->cacl_data))->enter(cur_entity))
		{
			dirty_entities.insert(one_entity);
		}
	}
	for(auto one_entity: update_info.leave_notify_entities)
	{
		if(((axis_2d_nodes_for_entity*)(one_entity->cacl_data))->leave(cur_entity))
		{
			dirty_entities.insert(one_entity);
		}
	}
}

std::unordered_set<aoi_entity*> list_2d_aoi::update_all()
{
	for(auto cur_entity: dirty_entities)
	{
		cur_entity->interested_by = cur_entity->force_interested_by;
		cur_entity->interest_in = cur_entity->force_interest_in;
	}
	for(auto cur_entity: dirty_entities)
	{
		auto cur_node = ((axis_2d_nodes_for_entity*)(cur_entity->cacl_data));
		for(auto one_entity: cur_node->xz_interest_in)
		{
			cur_entity->enter_by_pos(*one_entity);
		}
	}

	for(auto cur_entity: dirty_entities)
	{
		auto cur_node = ((axis_2d_nodes_for_entity*)(cur_entity->cacl_data));
		if(cur_node->is_leaving)
		{
			cur_node->is_leaving = false;
			cur_node->entity = nullptr;
			cur_entity->cacl_data = nullptr;
			nodes_buffer.renounce(cur_node);
		}
	}
	std::unordered_set<aoi_entity*> result;
	result.swap(dirty_entities);
	return result;
}

std::unordered_set<aoi_entity*> list_2d_aoi::entity_in_rectangle(pos_t center, std::uint16_t x_width, std::uint16_t z_width)const
{
	auto axis_x_result = x_axis.entity_in_range(center[0] - x_width, center[0] + x_width);
	auto axis_z_result = z_axis.entity_in_range(center[2] - z_width, center[2] + z_width);
	std::unordered_set<aoi_entity*> entity_result;
	unordered_set_join(axis_x_result, axis_z_result, entity_result);
	return entity_result;
}
std::unordered_set<aoi_entity*> list_2d_aoi::entity_in_circle(pos_t center, std::uint16_t radius)const
{
	auto axis_x_result = x_axis.entity_in_range(center[0] - radius, center[0] + radius);
	auto axis_z_result = z_axis.entity_in_range(center[2] - radius, center[2] + radius);
	std::unordered_set<aoi_entity*> entity_result;
	unordered_set_join(axis_x_result, axis_z_result, entity_result);
	std::unordered_set<aoi_entity*> find_result;
	for(auto one_entity:entity_result)
	{
		int32_t diff_x = center[0] - one_entity->pos[0];
		int32_t diff_z = center[2] - one_entity->pos[2];
		if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
		{
			continue;
		}
		find_result.insert(one_entity);
	}
	return find_result;
}
std::unordered_set<aoi_entity*> list_2d_aoi::entity_in_cylinder(pos_t center, std::uint16_t radius, std::uint16_t height)const
{
	auto axis_x_result = x_axis.entity_in_range(center[0] - radius, center[0] + radius);
	auto axis_z_result = z_axis.entity_in_range(center[2] - radius, center[2] + radius);
	std::unordered_set<aoi_entity*> entity_result;
	unordered_set_join(axis_x_result, axis_z_result, entity_result);
	std::unordered_set<aoi_entity*> find_result;
	for(auto one_entity:entity_result)
	{
		int32_t diff_x = center[0] - one_entity->pos[0];
		int32_t diff_y = center[1] - one_entity->pos[1];
		int32_t diff_z = center[2] - one_entity->pos[2];
		if((diff_x * diff_x + diff_z * diff_z) > radius * radius)
		{
			continue;
		}
		if(diff_y > height || diff_y < -height)
		{
			continue;
		}
		find_result.insert(one_entity);
	}
	return find_result;
}
std::unordered_set<aoi_entity*> list_2d_aoi::entity_in_cuboid(pos_t center, std::uint16_t x_width, std::uint16_t z_width, std::uint16_t y_height)const
{
	auto axis_x_result = x_axis.entity_in_range(center[0] - x_width, center[0] + x_width);
	auto axis_z_result = z_axis.entity_in_range(center[2] - z_width, center[2] + z_width);
	std::unordered_set<aoi_entity*> entity_result;
	unordered_set_join(axis_x_result, axis_z_result, entity_result);
	std::unordered_set<aoi_entity*> find_result;
	for(auto one_entity:entity_result)
	{
		int32_t diff_y = center[1] - one_entity->pos[1];

		if(diff_y > y_height || diff_y < -y_height)
		{
			continue;
		}
		find_result.insert(one_entity);
	}
	return find_result;
}
