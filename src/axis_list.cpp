#include "axis_list.h"
#include <algorithm>
#include <cmath>
using namespace spiritsaway::aoi;

axis_list::axis_list(std::uint32_t in_max_entity_count, pos_unit_t in_max_aoi_radius, int in_xz_pos_idx, pos_unit_t in_min_pos, pos_unit_t in_max_pos)
: max_entity_count(in_max_entity_count)
, node_per_anchor(std::min(std::uint32_t(std::floor(std::sqrt(in_max_entity_count))), 40u))
, min_pos(in_min_pos)
, max_pos(in_max_pos)
, max_radius(in_max_aoi_radius)
, nodes_buffer(in_max_entity_count * 4)
, xz_pos_idx(in_xz_pos_idx)
, nodes_for_radius(in_max_entity_count, {nullptr, nullptr})
, nodes_for_pos(in_max_entity_count, nullptr)
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
			anchor_node->pos_entity = nullptr;
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
	anchor_node->pos_entity = nullptr;
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

void axis_list::insert_radius_entity(aoi_radius_entity* radius_entity)
{
	check_update_anchors();
	std::uint32_t last_hint = 0;
	list_node* last_node = &head;
	auto radius_left = nodes_buffer.pull();
	if (!radius_left)
	{
		return;
	}
	auto radius_right = nodes_buffer.pull();
	if (!radius_right)
	{
		nodes_buffer.push(radius_left);
		return;
	}
	radius_left->node_type = list_node_type::left;
	radius_right->node_type = list_node_type::right;
	radius_left->radius_entity = radius_right->radius_entity = radius_entity;
	radius_left->pos = radius_entity->pos()[xz_pos_idx] - radius_entity->radius();
	radius_right->pos = radius_entity->pos()[xz_pos_idx] + radius_entity->radius();

	insert_node(radius_left, true);
	insert_node(radius_right, false);
	nodes_for_radius[radius_entity->radius_idx().value] = std::make_pair(radius_left, radius_right);
	auto temp = radius_left;
	while(temp != radius_right)
	{
		if(temp->node_type == list_node_type::center)
		{
			radius_entity->try_enter(*(temp->pos_entity));
		}
		temp = temp->next;
	}
	
}

void axis_list::remove_radius_entity(aoi_radius_entity* radius_entity)
{
	// TODO
}

void axis_list::insert_pos_entity(aoi_pos_entity* pos_entity)
{
	auto center_node = nodes_buffer.pull();
	if (!center_node)
	{
		return;
	}
	center_node->node_type = list_node_type::center;
	center_node->pos = pos_entity->pos()[xz_pos_idx];
	center_node->pos_entity = pos_entity;
	std::uint32_t last_hint = 0;
	list_node* last_node = &head;
	insert_node(center_node, true);
	auto boundary_left = find_boundary(pos_entity->pos()[xz_pos_idx] - max_radius, true);
	auto boundary_right = find_boundary(pos_entity->pos()[xz_pos_idx] + max_radius, false);
	nodes_for_pos[pos_entity->pos_idx().value] = center_node;
	while (boundary_left != boundary_right)
	{
		if (boundary_left->node_type == list_node_type::left)
		{
			boundary_left->radius_entity->try_enter(*pos_entity);
		}
		boundary_left = boundary_left->next;
	}
}

void axis_list::remove_pos_entity(aoi_pos_entity* pos_entity)
{
	check_update_anchors();
	auto cur_node = nodes_for_pos[pos_entity->pos_idx().value];
	remove(cur_node);
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
				visited_nodes.left.push_back(next->radius_entity);
			}
			break;
		case list_node_type::center:
			if(flag & std::uint8_t(list_node_type::center))
			{
				visited_nodes.middle.push_back(next->pos_entity);
			}
			break;
		case list_node_type::right:
			if(flag & std::uint8_t(list_node_type::right))
			{
				visited_nodes.right.push_back(next->radius_entity);
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
				visited_nodes.left.push_back(prev->radius_entity);
			}
			break;
		case list_node_type::center:
			if(flag & std::uint8_t(list_node_type::center))
			{
				visited_nodes.middle.push_back(prev->pos_entity);
			}
			break;
		case list_node_type::right:
			if(flag & std::uint8_t(list_node_type::right))
			{
				visited_nodes.right.push_back(prev->radius_entity);
			}
			break;
		default:
			break;
		}
		prev = prev->prev;
	}
	insert_before(prev->next, cur);
}

std::vector<aoi_pos_entity*> axis_list::entity_in_range(pos_unit_t range_begin, pos_unit_t range_end) const
{
	std::vector<aoi_pos_entity*> result;
	auto boundary_left = find_boundary(std::max(min_pos + 2, range_begin), true);
	auto boundary_right = find_boundary(std::min(max_pos - 2, range_end), false);
	while(boundary_left != boundary_right)
	{
		if(boundary_left->node_type == list_node_type::center)
		{
			result.push_back(boundary_left->pos_entity);
		}
		boundary_left = boundary_left->next;
	}
	return result;
}

void axis_list::update_entity_pos(aoi_pos_entity* pos_entity, pos_unit_t offset)
{
	check_update_anchors();
	if(offset == 0)
	{
		return;
	}
	auto cur_pos_node = nodes_for_pos[pos_entity->pos_idx().value];
	cur_pos_node->pos += offset;
	sweep_buffer[1].left.clear();
	sweep_buffer[1].right.clear();
	if (offset > 0)
	{
		move_forward(cur_pos_node, sweep_buffer[1], (std::uint8_t)list_node_type::left + (std::uint8_t)list_node_type::right);
		for (auto one_ent : sweep_buffer[1].left)
		{
			one_ent->try_enter(*pos_entity);
		}
		for (auto one_ent : sweep_buffer[1].right)
		{
			one_ent->try_leave(*pos_entity);
		}
		for (auto one_radius_entity : pos_entity->radius_entities())
		{
			sweep_buffer[0].middle.clear();
			sweep_buffer[2].middle.clear();
			auto cur_radius_nodes = nodes_for_radius[one_radius_entity->radius_idx().value];
			auto cur_left_node = cur_radius_nodes.first;
			auto cur_right_node = cur_radius_nodes.second;
			cur_left_node->pos += offset;
			cur_right_node->pos += offset;
			move_forward(cur_right_node, sweep_buffer[0], (std::uint8_t)list_node_type::center);

			move_forward(cur_left_node, sweep_buffer[2], (std::uint8_t)list_node_type::center);
			for (auto one_ent : sweep_buffer[0].middle)
			{
				one_radius_entity->try_enter(*one_ent);
			}
			for (auto one_ent : sweep_buffer[2].middle)
			{
				one_radius_entity->try_leave(*one_ent);
			}
			

		}
		
	}
	else
	{
		move_backward(cur_pos_node, sweep_buffer[1], (std::uint8_t)list_node_type::left + (std::uint8_t)list_node_type::right);
		for (auto one_ent : sweep_buffer[1].right)
		{
			one_ent->try_enter(*pos_entity);
		}
		for (auto one_ent : sweep_buffer[1].left)
		{
			one_ent->try_leave(*pos_entity);
		}
		for (auto one_radius_entity : pos_entity->radius_entities())
		{
			sweep_buffer[0].middle.clear();
			sweep_buffer[2].middle.clear();
			auto cur_radius_nodes = nodes_for_radius[one_radius_entity->radius_idx().value];
			auto cur_left_node = cur_radius_nodes.first;
			auto cur_right_node = cur_radius_nodes.second;
			cur_left_node->pos += offset;
			cur_right_node->pos += offset;
			move_backward(cur_left_node, sweep_buffer[0], (std::uint8_t)list_node_type::center);

			move_backward(cur_right_node, sweep_buffer[2], (std::uint8_t)list_node_type::center);
			for (auto one_ent : sweep_buffer[0].middle)
			{
				one_radius_entity->try_enter(*one_ent);
			}
			for (auto one_ent : sweep_buffer[2].middle)
			{
				one_radius_entity->try_leave(*one_ent);
			}
		}
		
		
	}
	


}
void axis_list::update_entity_radius(aoi_radius_entity* radius_entity, pos_unit_t delta_radius)
{
	check_update_anchors();
	if(delta_radius == 0)
	{
		return;
	}
	auto cur_nodes = nodes_for_radius[radius_entity->radius_idx().value];
	auto left_node = cur_nodes.first;
	auto right_node = cur_nodes.second;
	left_node->pos += delta_radius;
	right_node->pos -= delta_radius;
	sweep_buffer[0].middle.clear();
	sweep_buffer[1].middle.clear();
	if(delta_radius > 0)
	{
		move_forward(right_node, sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_backward(left_node, sweep_buffer[1], (std::uint8_t)list_node_type::center);
		for (auto one_ent : sweep_buffer[0].middle)
		{
			radius_entity->try_enter(*one_ent);
		}
		for (auto one_ent : sweep_buffer[1].middle)
		{
			radius_entity->try_enter(*one_ent);
		}

	}
	else
	{
		move_forward(left_node, sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_backward(right_node, sweep_buffer[1], (std::uint8_t)list_node_type::center);
		for (auto one_ent : sweep_buffer[0].middle)
		{
			radius_entity->try_leave(*one_ent);
		}
		for (auto one_ent : sweep_buffer[1].middle)
		{
			radius_entity->try_leave(*one_ent);
		}

	}
	
}


