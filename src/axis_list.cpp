#include "axis_list.h"
#include <algorithm>

axis_list::axis_list(std::uint32_t in_max_entity_count, std::uint16_t in_max_aoi_radius, std::int32_t in_min_pos, std::int32_t in_max_pos)
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
	std::int32_t pos = head.pos;
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
	}
	list_node* anchor_node = new list_node;
	anchor_node->node_type = list_node_type::anchor;
	anchor_node->pos = cur_node->pos;
	anchor_poses.push_back(cur_node->pos);
	anchor_node->entity = nullptr;
	anchors.push_back(anchor_node);
	insert_before(cur_node, anchors.back());
}


list_node* axis_list::find_boundary(std::int32_t pos, bool is_lower) const
{
	auto hint = std::lower_bound(anchor_poses.begin(), anchor_poses.end(), pos);
	auto temp_node = anchors[*hint];
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
void axis_list::insert_before(list_node* prev, list_node* cur)
{
	cur->next = prev;
	cur->prev = prev->prev;
	cur->prev->next = cur;
	cur->next->prev = cur;
}

void axis_list::remove(list_node* cur)
{
	cur->next->prev = cur->next;
	cur->prev->next = cur;
	cur->next = nullptr;
	cur->prev = nullptr;
}

void axis_list::insert_entity(axis_nodes_for_entity* entity_nodes)
{
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
			update_info.enter_entities.insert(temp->entity);
		}
		temp = temp->next;
	}
	auto boundary_left = find_boundary(entity_nodes->middle.pos - max_radius, true);
	auto boundary_right = find_boundary(entity_nodes->middle.pos + max_radius, false);
	while(boundary_left != boundary_right)
	{
		if(boundary_left->node_type == list_node_type::center)
		{
			update_info.enter_notify_entities.insert(boundary_left->entity);
		}
		boundary_left = boundary_left->next;
	}
}

void axis_list::remove_entity(axis_nodes_for_entity* entity_nodes)
{
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
				visited_nodes.left.insert(next->entity);
			}
			break;
		case list_node_type::center:
			if(flag & std::uint8_t(list_node_type::center))
			{
				visited_nodes.middle.insert(next->entity);
			}
			break;
		case list_node_type::right:
			if(flag & std::uint8_t(list_node_type::right))
			{
				visited_nodes.right.insert(next->entity);
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
				visited_nodes.left.insert(prev->entity);
			}
			break;
		case list_node_type::center:
			if(flag & std::uint8_t(list_node_type::center))
			{
				visited_nodes.middle.insert(prev->entity);
			}
			break;
		case list_node_type::right:
			if(flag & std::uint8_t(list_node_type::right))
			{
				visited_nodes.right.insert(prev->entity);
			}
			break;
		default:
			break;
		}
		prev = prev->prev;
	}
	insert_before(prev, cur);
}

std::unordered_set<aoi_entity*> axis_list::entity_in_range(std::int32_t range_begin, std::int32_t range_end) const
{
	std::unordered_set<aoi_entity*> result;
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

void axis_list::update_entity_pos(axis_nodes_for_entity* entity_nodes, std::int32_t offset)
{
	if(offset == 0)
	{
		return;
	}
	entity_nodes->left.pos += offset;
	entity_nodes->right.pos += offset;
	entity_nodes->middle.pos += offset;
	auto cur_entity = entity_nodes->left.entity;

	update_info.clear();
	sweep_buffer[0].middle.clear();
	sweep_buffer[2].middle.clear();
	sweep_buffer[1].left.clear();
	sweep_buffer[1].right.clear();
	if(offset > 0)
	{
		move_forward(&entity_nodes->right, sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_forward(&entity_nodes->middle, sweep_buffer[1], (std::uint8_t)list_node_type::left + (std::uint8_t)list_node_type::right);
		move_forward(&entity_nodes->left, sweep_buffer[2], (std::uint8_t)list_node_type::center);
		unordered_set_diff(sweep_buffer[0].middle, sweep_buffer[2].middle, update_info.enter_entities);
		unordered_set_diff(sweep_buffer[2].middle, sweep_buffer[0].middle, update_info.leave_entities);
		unordered_set_diff(sweep_buffer[1].left, sweep_buffer[1].right, update_info.enter_notify_entities);
		unordered_set_diff(sweep_buffer[1].right, sweep_buffer[1].left, update_info.leave_notify_entities);
	}
	else
	{
		move_backward(&entity_nodes->left, sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_backward(&entity_nodes->middle, sweep_buffer[1], (std::uint8_t)list_node_type::left + (std::uint8_t)list_node_type::right);
		move_backward(&entity_nodes->right, sweep_buffer[2], (std::uint8_t)list_node_type::center);
		unordered_set_diff(sweep_buffer[0].middle, sweep_buffer[2].middle, update_info.leave_entities);
		unordered_set_diff(sweep_buffer[2].middle, sweep_buffer[0].middle, update_info.enter_entities);
		unordered_set_diff(sweep_buffer[1].left, sweep_buffer[1].right, update_info.leave_notify_entities);
		unordered_set_diff(sweep_buffer[1].right, sweep_buffer[1].left, update_info.enter_notify_entities);
	}
}
void axis_list::update_entity_radius(axis_nodes_for_entity* entity_nodes, std::int32_t delta_radius)
{
	update_info.clear();
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
		for(auto one_entity: sweep_buffer[1].middle)
		{
			sweep_buffer[0].middle.insert(one_entity);
		}
		std::swap(update_info.enter_entities, sweep_buffer[0].middle);

	}
	else
	{
		move_forward(&(entity_nodes->left), sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_backward(&(entity_nodes->right), sweep_buffer[1], (std::uint8_t)list_node_type::center);
		for(auto one_entity: sweep_buffer[1].middle)
		{
			sweep_buffer[0].middle.insert(one_entity);
		}
		std::swap(update_info.leave_entities, sweep_buffer[0].middle);

	}
	
}

const move_result& axis_list::get_update_info() const
{
	return update_info;
}

bool axis_2d_nodes_for_entity::in_range(const aoi_entity* other_entity)
{
	auto other_x = other_entity->pos[0];
	auto other_z = other_entity->pos[2];
	auto cur_x = entity->pos[0];
	auto cur_z = entity->pos[2];
	auto radius = entity->radius;
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
bool axis_2d_nodes_for_entity::enter(aoi_entity* other_entity)
{
	if(!in_range(other_entity))
	{
		return false;
	}
	if(xz_interest_in.size() >= 2 * entity->max_interest_in)
	{
		return false;
	}
	if(!entity->can_pass_flag_check(*other_entity))
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
bool axis_2d_nodes_for_entity::leave(aoi_entity* other_entity)
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
