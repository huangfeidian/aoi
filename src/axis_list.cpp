#include "axis_list.h"
#include <algorithm>
#include <cmath>
using namespace spiritsaway::aoi;

axis_list::axis_list(std::uint32_t in_max_entity_count, pos_unit_t in_max_aoi_radius, pos_unit_t in_min_pos, pos_unit_t in_max_pos)
: max_entity_count(in_max_entity_count)
, node_per_anchor(std::min(std::uint32_t(std::floor(std::sqrt(in_max_entity_count))), 40u))
, min_pos(in_min_pos)
, max_pos(in_max_pos)
, max_radius(in_max_aoi_radius)
{
	anchor_buffer = std::vector<list_node>(3 *max_entity_count / node_per_anchor + 100);
	head.next = &tail;
	tail.prev = &head;
	head.node_type = list_node_type::anchor;
	tail.node_type = list_node_type::anchor;
	head.prev = nullptr;
	tail.next = nullptr;
	head.pos = min_pos - 2 * in_max_aoi_radius;
	tail.pos = max_pos + 2 * in_max_aoi_radius;
	update_anchors();
}

void axis_list::update_anchors()
{
	for(std::size_t i = 0; i< anchors.size(); i++)
	{
		list_node* cur_node = anchors[i];
		remove(cur_node);
	}
	anchors.clear();
	anchor_poses.clear();
	auto prev = &head;
	auto cur_node = head.next;
	std::uint32_t count = 0;
	pos_unit_t pos = head.pos;
	while(cur_node != &tail)
	{
		if(count >= node_per_anchor && cur_node->pos != pos)
		{
			list_node* anchor_node = anchor_buffer.data() + anchors.size();
			anchor_node->node_type = list_node_type::anchor;
			anchor_node->pos = cur_node->pos;
			anchor_poses.push_back(cur_node->pos);
			anchor_node->entity = nullptr;
			anchors.push_back(anchor_node);
			insert_before(cur_node, anchors.back());
			count = 0;
		}
		pos = cur_node->pos;
		cur_node = cur_node->next;
		count++;
	}
	list_node* anchor_node = new list_node;
	anchor_node->node_type = list_node_type::anchor;
	anchor_node->pos = cur_node->pos;
	anchor_poses.push_back(cur_node->pos);
	anchor_node->entity = nullptr;
	anchors.push_back(anchor_node);
	insert_before(cur_node, anchors.back());
	dirty_count = std::max(100u, std::uint32_t(std::sqrt(max_entity_count * 10)));
}

void axis_list::check_update_anchors()
{
	if (dirty_count == 0)
	{
		update_anchors();
	}
	dirty_count--;
}

list_node* axis_list::find_boundary(pos_unit_t pos, bool is_lower) const
{
	auto hint = std::lower_bound(anchor_poses.begin(), anchor_poses.end(), pos);
	auto temp_node = anchors[std::distance(anchor_poses.begin(), hint)];
	if(is_lower)
	{
		while(temp_node->pos >= pos)
		{
			temp_node = temp_node->prev;
		}
		return temp_node->next;
	}
	else
	{
		while(temp_node->pos > pos)
		{
			temp_node = temp_node->prev;
		}
		return temp_node->next;
	}
	
}
void axis_list::insert_node(list_node* cur, bool is_lower)
{
	auto next_node = find_boundary(cur->pos, is_lower);
	insert_before(next_node, cur);
}
void axis_list::insert_before(list_node* next, list_node* cur)
{
	cur->next = next;
	cur->prev = next->prev;
	cur->prev->next = cur;
	cur->next->prev = cur;
}

void axis_list::remove(list_node* cur)
{
	cur->next->prev = cur->prev;
	cur->prev->next = cur->next;
	cur->next = nullptr;
	cur->prev = nullptr;
}

void axis_list::insert_entity(axis_nodes_for_entity* entity_nodes)
{
	check_update_anchors();
	std::uint32_t last_hint = 0;
	list_node* last_node = &head;
	insert_node(&entity_nodes->left, true);
	insert_node(&entity_nodes->middle, true);
	insert_node(&entity_nodes->right, false);
	auto temp = entity_nodes->left.next;
	while(temp != &(entity_nodes->right))
	{
		if(temp->node_type == list_node_type::center)
		{
			entity_nodes->left.entity->try_enter(*(temp->entity));
		}
		temp = temp->next;
	}
	auto boundary_left = find_boundary(entity_nodes->middle.pos - max_radius, true);
	auto boundary_right = find_boundary(entity_nodes->middle.pos + max_radius, false);
	while(boundary_left != boundary_right)
	{
		if(boundary_left->node_type == list_node_type::center)
		{
			boundary_left->entity->try_enter(*(entity_nodes->left.entity));
		}
		boundary_left = boundary_left->next;
	}
}

void axis_list::remove_entity(axis_nodes_for_entity* entity_nodes)
{
	check_update_anchors();
	list_node* temp_nodes[3] = {&(entity_nodes->left), &(entity_nodes->middle), &(entity_nodes->right)};
	for(int i = 0; i< 3; i++)
	{
		auto cur_node = temp_nodes[i];
		remove(cur_node);
	}
}
void axis_list::move_forward(list_node* cur, sweep_result& visited_nodes, std::uint8_t flag)
{
	auto next = cur->next;
	remove(cur);
	while(next->pos <= cur->pos)
	{
		switch (next->node_type)
		{
		case list_node_type::left:
			if(flag & std::uint8_t(list_node_type::left))
			{
				visited_nodes.left.push_back(next->entity);
			}
			break;
		case list_node_type::center:
			if(flag & std::uint8_t(list_node_type::center))
			{
				visited_nodes.middle.push_back(next->entity);
			}
			break;
		case list_node_type::right:
			if(flag & std::uint8_t(list_node_type::right))
			{
				visited_nodes.right.push_back(next->entity);
			}
			break;
		default:
			break;
		}
		next = next->next;
	}
	insert_before(next, cur);
}

void axis_list::move_backward(list_node* cur, sweep_result& visited_nodes, std::uint8_t flag)
{
	auto prev = cur->prev;
	remove(cur);
	while(prev->pos >= cur->pos)
	{
		switch (prev->node_type)
		{
		case list_node_type::left:
			if(flag & std::uint8_t(list_node_type::left))
			{
				visited_nodes.left.push_back(prev->entity);
			}
			break;
		case list_node_type::center:
			if(flag & std::uint8_t(list_node_type::center))
			{
				visited_nodes.middle.push_back(prev->entity);
			}
			break;
		case list_node_type::right:
			if(flag & std::uint8_t(list_node_type::right))
			{
				visited_nodes.right.push_back(prev->entity);
			}
			break;
		default:
			break;
		}
		prev = prev->prev;
	}
	insert_before(prev->next, cur);
}

std::unordered_set<aoi_radius_entity*> axis_list::entity_in_range(pos_unit_t range_begin, pos_unit_t range_end) const
{
	std::unordered_set<aoi_radius_entity*> result;
	auto boundary_left = find_boundary(std::max(min_pos + 2, range_begin), true);
	auto boundary_right = find_boundary(std::min(max_pos - 2, range_end), false);
	while(boundary_left != boundary_right)
	{
		if(boundary_left->node_type == list_node_type::center)
		{
			result.insert(boundary_left->entity);
		}
		boundary_left = boundary_left->next;
	}
	return result;
}

void axis_list::update_entity_pos(axis_nodes_for_entity* entity_nodes, pos_unit_t offset)
{
	check_update_anchors();
	if(offset == 0)
	{
		return;
	}
	entity_nodes->left.pos += offset;
	entity_nodes->right.pos += offset;
	entity_nodes->middle.pos += offset;
	auto cur_entity = entity_nodes->left.entity;

	sweep_buffer[0].middle.clear();
	sweep_buffer[2].middle.clear();
	sweep_buffer[1].left.clear();
	sweep_buffer[1].right.clear();
	if(offset > 0)
	{
		move_forward(&entity_nodes->right, sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_forward(&entity_nodes->middle, sweep_buffer[1], (std::uint8_t)list_node_type::left + (std::uint8_t)list_node_type::right);
		move_forward(&entity_nodes->left, sweep_buffer[2], (std::uint8_t)list_node_type::center);
		for (auto one_ent : sweep_buffer[0].middle)
		{
			cur_entity->try_enter(*one_ent);
		}
		for (auto one_ent : sweep_buffer[2].middle)
		{
			cur_entity->try_leave(*one_ent);
		}
		for (auto one_ent : sweep_buffer[1].left)
		{
			one_ent->try_enter(*cur_entity);
		}
		for (auto one_ent : sweep_buffer[1].right)
		{
			one_ent->try_leave(*cur_entity);
		}
	}
	else
	{
		move_backward(&entity_nodes->left, sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_backward(&entity_nodes->middle, sweep_buffer[1], (std::uint8_t)list_node_type::left + (std::uint8_t)list_node_type::right);
		move_backward(&entity_nodes->right, sweep_buffer[2], (std::uint8_t)list_node_type::center);
		for (auto one_ent : sweep_buffer[0].middle)
		{
			cur_entity->try_enter(*one_ent);
		}
		for (auto one_ent : sweep_buffer[2].middle)
		{
			cur_entity->try_leave(*one_ent);
		}
		for (auto one_ent : sweep_buffer[1].right)
		{
			one_ent->try_enter(*cur_entity);
		}
		for (auto one_ent : sweep_buffer[1].left)
		{
			one_ent->try_leave(*cur_entity);
		}
	}
}
void axis_list::update_entity_radius(axis_nodes_for_entity* entity_nodes, pos_unit_t delta_radius)
{
	check_update_anchors();
	if(delta_radius == 0)
	{
		return;
	}
	auto cur_entity = entity_nodes->left.entity;

	entity_nodes->right.pos += delta_radius;
	entity_nodes->left.pos -= delta_radius;
	sweep_buffer[0].middle.clear();
	sweep_buffer[1].middle.clear();
	if(delta_radius > 0)
	{
		move_forward(&(entity_nodes->right), sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_backward(&(entity_nodes->left), sweep_buffer[1], (std::uint8_t)list_node_type::center);
		for (auto one_ent : sweep_buffer[0].middle)
		{
			cur_entity->try_enter(*one_ent);
		}
		for (auto one_ent : sweep_buffer[1].middle)
		{
			cur_entity->try_enter(*one_ent);
		}

	}
	else
	{
		move_forward(&(entity_nodes->left), sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_backward(&(entity_nodes->right), sweep_buffer[1], (std::uint8_t)list_node_type::center);
		for (auto one_ent : sweep_buffer[0].middle)
		{
			cur_entity->try_leave(*one_ent);
		}
		for (auto one_ent : sweep_buffer[1].middle)
		{
			cur_entity->try_leave(*one_ent);
		}

	}
	
}



bool axis_2d_nodes_for_entity::in_range(const aoi_radius_entity* other_entity)
{
	auto other_x = other_entity->pos()[0];
	auto other_z = other_entity->pos()[2];
	auto cur_x = entity->pos()[0];
	auto cur_z = entity->pos()[2];
	auto radius = entity->radius();
	if(other_x < cur_x - radius || other_x > cur_x + radius)
	{
		return false;
	}
	if(other_z < cur_z - radius || other_z > cur_z + radius)
	{
		return false;
	}
	return true;
}
bool axis_2d_nodes_for_entity::enter(aoi_radius_entity* other_entity)
{
	if(!in_range(other_entity))
	{
		return false;
	}
	if(entity->aoi_radius_ctrl().max_interest_in && xz_interest_in.size() >= 2 * entity->aoi_radius_ctrl().max_interest_in)
	{
		return false;
	}
	if(!entity->check_flag(*other_entity))
	{
		return false;
	}
	if(xz_interest_in.insert(other_entity).second)
	{
		((axis_2d_nodes_for_entity*)(other_entity->cacl_data))->xz_interested_by.insert(entity);
		return true;
	}
	else
	{
		return false;
	}
	
}
bool axis_2d_nodes_for_entity::leave(aoi_radius_entity* other_entity)
{
	if(xz_interest_in.erase(other_entity))
	{
		((axis_2d_nodes_for_entity*)(other_entity->cacl_data))->xz_interested_by.erase(entity);
		return true;
	}
	else
	{
		return false;
	}
}

void axis_2d_nodes_for_entity::detach()
{
	for(auto one_entity: xz_interest_in)
	{
		((axis_2d_nodes_for_entity*)(one_entity->cacl_data))->xz_interested_by.erase(entity);
	}
	for(auto one_entity: xz_interested_by)
	{
		((axis_2d_nodes_for_entity*)(one_entity->cacl_data))->xz_interest_in.erase(entity);
	}
	xz_interest_in.clear();
	xz_interested_by.clear();
	is_leaving = true;
}

std::vector<axis_2d_nodes_for_entity*> axis_list::dump(std::ostream& out_debug) const
{
	auto temp = head.next;
	std::vector<axis_2d_nodes_for_entity*> result;
	while (temp != &tail)
	{
		temp->dump(out_debug);
		if (temp->node_type == list_node_type::center)
		{
			result.push_back(reinterpret_cast<axis_2d_nodes_for_entity*>(temp->entity->cacl_data));
		}
		temp = temp->next;
	}
	return result;
}