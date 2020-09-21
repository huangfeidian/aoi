#include "list_aoi.h"
#include <cmath>
axis_list::axis_list(std::uint32_t in_max_entity_count, std::int32_t in_min_pos, std::int32_t in_max_pos)
: max_entity_count(in_max_entity_count)
, node_per_anchor(std::uint32_t(std::floor(std::sqrt(in_max_entity_count))))
, min_pos(in_min_pos)
, max_pos(in_max_pos)
{
	anchor_buffer = std::vector<list_node>(3 *max_entity_count / node_per_anchor + 100);
	head.next = &tail;
	tail.prev = &head;
	head.node_type = list_node_type::anchor;
	tail.node_type = list_node_type::anchor;
	head.prev = nullptr;
	tail.next = nullptr;
	head.pos = min_pos;
	tail.pos = max_pos;
	cur_entity_count = 0;
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

std::size_t axis_list::insert_hint(std::int32_t pos, std::uint32_t last_hint)
{
	auto hint = std::lower_bound(anchor_poses.begin() + last_hint, anchor_poses.end(), pos);
	return std::distance(anchor_poses.begin(), hint);
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
}

void axis_list::insert_after_hint(list_node* hint, list_node* cur)
{
	while(hint->pos >= cur->pos && hint->next != nullptr)
	{
		hint = hint->next;
	}
	insert_before(hint, cur);
}
void axis_list::insert_entity(axis_nodes_for_entity* entity_nodes)
{
	std::uint32_t last_hint = 0;
	list_node* last_node = &head;
	list_node* temp_nodes[3] = {&(entity_nodes->left), &(entity_nodes->middle), &(entity_nodes->right)};
	for(int i = 0; i< 3; i++)
	{
		auto cur_node = temp_nodes[i];
		auto cur_hint = insert_hint(cur_node->pos, last_hint);
		if(cur_hint != last_hint)
		{
			last_node = anchors[cur_hint];
			last_hint = cur_hint;
		}
		insert_after_hint(last_node, cur_node);
		last_node = cur_node;
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
void axis_list::move_forward(list_node* cur, sweep_result& visited_nodes)
{
	auto next = cur->next;
	remove(cur);
	while(next->pos <= cur->pos)
	{
		switch (next->node_type)
		{
		case list_node_type::left:
			visited_nodes.left.insert(next->entity);
			break;
		case list_node_type::center:
			visited_nodes.middle.insert(next->entity);
			break;
		case list_node_type::right:
			visited_nodes.right.insert(next->entity);
			break;
		default:
			break;
		}
		next = next->next;
	}
	insert_before(next, cur);
}

void axis_list::move_forward(list_node* cur, sweep_result& visited_nodes)
{
	auto prev = cur->prev;
	remove(cur);
	while(prev->pos >= cur->pos)
	{
		switch (prev->node_type)
		{
		case list_node_type::left:
			visited_nodes.left.insert(prev->entity);
			break;
		case list_node_type::center:
			visited_nodes.middle.insert(prev->entity);
			break;
		case list_node_type::right:
			visited_nodes.right.insert(prev->entity);
			break;
		default:
			break;
		}
		prev = prev->prev;
	}
	insert_before(prev, cur);
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

	enter_entities.clear();
	leave_entities.clear();
	enter_notify_entities.clear();
	leave_notify_entities.clear();

	if(offset > 0)
	{
		move_forward(&entity_nodes->right, sweep_buffer[0]);
		move_forward(&entity_nodes->middle, sweep_buffer[1]);
		move_forward(&entity_nodes->left, sweep_buffer[2]);
		unordered_set_diff(sweep_buffer[0].middle, sweep_buffer[2].middle, enter_entities);
		unordered_set_diff(sweep_buffer[2].middle, sweep_buffer[0].middle, leave_entities);
		unordered_set_diff(sweep_buffer[1].left, sweep_buffer[1].right, enter_notify_entities);
		unordered_set_diff(sweep_buffer[1].right, sweep_buffer[1].left, leave_notify_entities);
	}
	else
	{
		move_backward(&entity_nodes->left, sweep_buffer[0]);
		move_backward(&entity_nodes->middle, sweep_buffer[1]);
		move_backward(&entity_nodes->right, sweep_buffer[2]);
		unordered_set_diff(sweep_buffer[0].middle, sweep_buffer[2].middle, leave_entities);
		unordered_set_diff(sweep_buffer[2].middle, sweep_buffer[0].middle, enter_entities);
		unordered_set_diff(sweep_buffer[1].left, sweep_buffer[1].right, leave_notify_entities);
		unordered_set_diff(sweep_buffer[1].right, sweep_buffer[1].left, enter_notify_entities);
	}
	for(auto one_entity: enter_entities)
	{
		cur_entity->enter_by_pos(*one_entity, true);
	}
	for(auto one_entity: leave_entities)
	{
		cur_entity->leave_by_pos(*one_entity);
	}
	for(auto one_entity: enter_notify_entities)
	{
		one_entity->enter_by_pos(*cur_entity, true);
	}
	for(auto one_entity: leave_notify_entities)
	{
		one_entity->leave_by_pos(*cur_entity);
	}
}
void axis_list::update_entity_radius(axis_nodes_for_entity* entity_nodes, std::int32_t delta_radius)
{
	if(delta_radius == 0)
	{
		return;
	}
	auto cur_entity = entity_nodes->left.entity;

	entity_nodes->right.pos += delta_radius;
	entity_nodes->left.pos -= delta_radius;
	if(delta_radius > 0)
	{
		move_forward(&(entity_nodes->right), sweep_buffer[0]);
		move_backward(&(entity_nodes->left), sweep_buffer[1]);
		for(auto one_entity: sweep_buffer[1].middle)
		{
			sweep_buffer[0].middle.insert(one_entity);
		}
		for(auto one_entity: sweep_buffer[0].middle)
		{
			cur_entity->enter_by_pos(*one_entity, true);
		}
	}
	else
	{
		move_forward(&(entity_nodes->left), sweep_buffer[0]);
		move_backward(&(entity_nodes->right), sweep_buffer[1]);
		for(auto one_entity: sweep_buffer[1].middle)
		{
			sweep_buffer[0].middle.insert(one_entity);
		}
		for(auto one_entity: sweep_buffer[0].middle)
		{
			cur_entity->leave_by_pos(*one_entity);
		}
	}
	
}