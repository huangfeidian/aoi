#include "axis_list.h"
#include <algorithm>
#include <cmath>
using namespace spiritsaway::aoi;

axis_list::axis_list(std::uint32_t in_max_entity_count, std::uint32_t in_max_radius_count, pos_unit_t in_max_aoi_radius, int in_xz_pos_idx, pos_unit_t in_min_pos, pos_unit_t in_max_pos)
: m_max_entity_count(in_max_entity_count)
, m_node_per_anchor(40)
, m_min_pos(in_min_pos)
, m_max_pos(in_max_pos)
, m_max_radius(in_max_aoi_radius)
, m_nodes_buffer((in_max_radius_count  + in_max_entity_count) * 2)
, m_xz_pos_idx(in_xz_pos_idx)
, m_nodes_for_radius(in_max_radius_count, {nullptr, nullptr})
, m_nodes_for_pos(in_max_entity_count, nullptr)
{
	m_anchor_buffer = std::vector<list_node>((m_max_entity_count + in_max_radius_count*2) / m_node_per_anchor + 100);
	m_head.next = &m_tail;
	m_tail.prev = &m_head;
	m_head.node_type = list_node_type::anchor;
	m_tail.node_type = list_node_type::anchor;
	m_head.prev = nullptr;
	m_tail.next = nullptr;
	m_head.pos = m_min_pos - 2 * in_max_aoi_radius;
	m_tail.pos = m_max_pos + 2 * in_max_aoi_radius;
	update_anchors();
}

void axis_list::update_anchors()
{
	for(std::size_t i = 0; i< m_nodes_for_anchor.size(); i++)
	{
		list_node* cur_node = m_nodes_for_anchor[i];
		remove_node(cur_node);
	}
	m_nodes_for_anchor.clear();
	m_anchor_poses.clear();
	auto prev = &m_head;
	auto cur_node = m_head.next;
	std::uint32_t count = 0;
	pos_unit_t pos = m_head.pos;
	while(cur_node != &m_tail)
	{
		if(count >= m_node_per_anchor && cur_node->pos != pos)
		{
			list_node* anchor_node = m_anchor_buffer.data() + m_nodes_for_anchor.size();
			anchor_node->node_type = list_node_type::anchor;
			anchor_node->pos = cur_node->pos;
			m_anchor_poses.push_back(cur_node->pos);
			anchor_node->pos_entity = nullptr;
			m_nodes_for_anchor.push_back(anchor_node);
			insert_before(cur_node, m_nodes_for_anchor.back());
			count = 0;
		}
		pos = cur_node->pos;
		cur_node = cur_node->next;
		count++;
	}
	list_node* anchor_node = m_anchor_buffer.data() + m_nodes_for_anchor.size();
	anchor_node->node_type = list_node_type::anchor;
	anchor_node->pos = cur_node->pos;
	m_anchor_poses.push_back(cur_node->pos);
	anchor_node->pos_entity = nullptr;
	m_nodes_for_anchor.push_back(anchor_node);
	insert_before(cur_node, m_nodes_for_anchor.back());
}



list_node* axis_list::find_boundary(pos_unit_t pos, bool is_lower) const
{
	auto hint = std::lower_bound(m_anchor_poses.begin(), m_anchor_poses.end(), pos);
	auto temp_node = m_nodes_for_anchor[std::distance(m_anchor_poses.begin(), hint)];
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

void axis_list::remove_node(list_node* cur)
{
	cur->next->prev = cur->prev;
	cur->prev->next = cur->next;
	cur->next = nullptr;
	cur->prev = nullptr;
}

void axis_list::insert_radius_entity(aoi_radius_entity* radius_entity)
{
	if (m_nodes_for_anchor.empty() || m_nodes_for_anchor.size() * m_node_per_anchor < m_nodes_buffer.used_count() / 2)
	{
		update_anchors();
	}
	std::uint32_t last_hint = 0;
	list_node* last_node = &m_head;
	auto radius_left = m_nodes_buffer.pull();
	if (!radius_left)
	{
		return;
	}
	auto radius_right = m_nodes_buffer.pull();
	if (!radius_right)
	{
		m_nodes_buffer.push(radius_left);
		return;
	}
	radius_left->node_type = list_node_type::left;
	radius_right->node_type = list_node_type::right;
	radius_left->radius_entity = radius_right->radius_entity = radius_entity;
	radius_left->pos = radius_entity->pos()[m_xz_pos_idx] - radius_entity->radius();
	radius_right->pos = radius_entity->pos()[m_xz_pos_idx] + radius_entity->radius();

	insert_node(radius_left, true);
	insert_node(radius_right, false);
	m_nodes_for_radius[radius_entity->radius_idx().value] = std::make_pair(radius_left, radius_right);
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
	auto& cur_radius_node = m_nodes_for_radius[radius_entity->radius_idx().value];
	// 这里只负责删除节点 leave callback 由外部负责
	remove_node(cur_radius_node.first);
	remove_node(cur_radius_node.second);
	m_nodes_buffer.push(cur_radius_node.first);
	m_nodes_buffer.push(cur_radius_node.second);
	cur_radius_node.first = nullptr;
	cur_radius_node.second = nullptr;
}

void axis_list::insert_pos_entity(aoi_pos_entity* pos_entity)
{
	if (m_nodes_for_anchor.empty() || m_nodes_for_anchor.size() * m_node_per_anchor < m_nodes_buffer.used_count()/ 2)
	{
		update_anchors();
	}
	auto center_node = m_nodes_buffer.pull();
	if (!center_node)
	{
		return;
	}
	center_node->node_type = list_node_type::center;
	center_node->pos = pos_entity->pos()[m_xz_pos_idx];
	center_node->pos_entity = pos_entity;
	std::uint32_t last_hint = 0;
	list_node* last_node = &m_head;
	insert_node(center_node, true);
	auto boundary_left = find_boundary(pos_entity->pos()[m_xz_pos_idx] - 2* m_max_radius, true);
	auto boundary_right = find_boundary(pos_entity->pos()[m_xz_pos_idx] + 2*m_max_radius, false);
	m_nodes_for_pos[pos_entity->pos_idx().value] = center_node;
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
	auto cur_node = m_nodes_for_pos[pos_entity->pos_idx().value];
	remove_node(cur_node);
	m_nodes_buffer.push(cur_node);
	m_nodes_for_pos[pos_entity->pos_idx().value] = nullptr;
}
void axis_list::move_forward(list_node* cur, sweep_result& visited_nodes, std::uint8_t flag)
{
	auto next = cur->next;
	remove_node(cur);
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
	remove_node(cur);
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
	auto boundary_left = find_boundary(std::max(m_min_pos + 2, range_begin), true);
	auto boundary_right = find_boundary(std::min(m_max_pos - 2, range_end), false);
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
	if(offset == 0)
	{
		return;
	}
	auto cur_pos_node = m_nodes_for_pos[pos_entity->pos_idx().value];
	cur_pos_node->pos += offset;
	m_sweep_buffer[1].left.clear();
	m_sweep_buffer[1].right.clear();
	if (offset > 0)
	{
		move_forward(cur_pos_node, m_sweep_buffer[1], (std::uint8_t)list_node_type::left + (std::uint8_t)list_node_type::right);
		for (auto one_ent : m_sweep_buffer[1].left)
		{
			one_ent->try_enter(*pos_entity);
		}
		for (auto one_ent : m_sweep_buffer[1].right)
		{
			one_ent->try_leave(*pos_entity);
		}
		for (auto one_radius_entity : pos_entity->radius_entities())
		{
			m_sweep_buffer[0].middle.clear();
			m_sweep_buffer[2].middle.clear();
			auto cur_radius_nodes = m_nodes_for_radius[one_radius_entity->radius_idx().value];
			auto cur_left_node = cur_radius_nodes.first;
			auto cur_right_node = cur_radius_nodes.second;
			cur_left_node->pos += offset;
			cur_right_node->pos += offset;
			move_forward(cur_right_node, m_sweep_buffer[0], (std::uint8_t)list_node_type::center);

			move_forward(cur_left_node, m_sweep_buffer[2], (std::uint8_t)list_node_type::center);
			for (auto one_ent : m_sweep_buffer[0].middle)
			{
				one_radius_entity->try_enter(*one_ent);
			}
			for (auto one_ent : m_sweep_buffer[2].middle)
			{
				one_radius_entity->try_leave(*one_ent);
			}
			

		}
		
	}
	else
	{
		move_backward(cur_pos_node, m_sweep_buffer[1], (std::uint8_t)list_node_type::left + (std::uint8_t)list_node_type::right);
		for (auto one_ent : m_sweep_buffer[1].right)
		{
			one_ent->try_enter(*pos_entity);
		}
		for (auto one_ent : m_sweep_buffer[1].left)
		{
			one_ent->try_leave(*pos_entity);
		}
		for (auto one_radius_entity : pos_entity->radius_entities())
		{
			m_sweep_buffer[0].middle.clear();
			m_sweep_buffer[2].middle.clear();
			auto cur_radius_nodes = m_nodes_for_radius[one_radius_entity->radius_idx().value];
			auto cur_left_node = cur_radius_nodes.first;
			auto cur_right_node = cur_radius_nodes.second;
			cur_left_node->pos += offset;
			cur_right_node->pos += offset;
			move_backward(cur_left_node, m_sweep_buffer[0], (std::uint8_t)list_node_type::center);

			move_backward(cur_right_node, m_sweep_buffer[2], (std::uint8_t)list_node_type::center);
			for (auto one_ent : m_sweep_buffer[0].middle)
			{
				one_radius_entity->try_enter(*one_ent);
			}
			for (auto one_ent : m_sweep_buffer[2].middle)
			{
				one_radius_entity->try_leave(*one_ent);
			}
		}
		
		
	}
	

}
void axis_list::update_entity_radius(aoi_radius_entity* radius_entity, pos_unit_t delta_radius)
{
	if(delta_radius == 0)
	{
		return;
	}
	auto cur_nodes = m_nodes_for_radius[radius_entity->radius_idx().value];
	auto left_node = cur_nodes.first;
	auto right_node = cur_nodes.second;
	left_node->pos += delta_radius;
	right_node->pos -= delta_radius;
	m_sweep_buffer[0].middle.clear();
	m_sweep_buffer[1].middle.clear();
	if(delta_radius > 0)
	{
		move_forward(right_node, m_sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_backward(left_node, m_sweep_buffer[1], (std::uint8_t)list_node_type::center);
		for (auto one_ent : m_sweep_buffer[0].middle)
		{
			radius_entity->try_enter(*one_ent);
		}
		for (auto one_ent : m_sweep_buffer[1].middle)
		{
			radius_entity->try_enter(*one_ent);
		}

	}
	else
	{
		move_forward(left_node, m_sweep_buffer[0], (std::uint8_t)list_node_type::center);
		move_backward(right_node, m_sweep_buffer[1], (std::uint8_t)list_node_type::center);
		for (auto one_ent : m_sweep_buffer[0].middle)
		{
			radius_entity->try_leave(*one_ent);
		}
		for (auto one_ent : m_sweep_buffer[1].middle)
		{
			radius_entity->try_leave(*one_ent);
		}

	}
	
}

void axis_list::dump(std::ostream& out_debug) const
{
	auto cur_node = m_head.next;
	while (cur_node != &m_tail)
	{
		out_debug << "type " << int(cur_node->node_type) << " pos " << cur_node->pos << " ";
		switch (cur_node->node_type)
		{
		case list_node_type::center:
		{
			out_debug << " guid " << cur_node->pos_entity->guid() << " pos_idx " << cur_node->pos_entity->pos_idx().value;
			break;
		}
		case list_node_type::left:
		case list_node_type::right:
		{
			out_debug << " guid " << cur_node->radius_entity->guid() << " radius_idx " << cur_node->radius_entity->radius_idx().value;
			break;
		}
		default:
			break;
		}
		out_debug << "\n";
		cur_node = cur_node->next;
	}

}


