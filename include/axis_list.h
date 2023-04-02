#pragma once

#include <unordered_set>
#include <aoi_entity.h>
#include <ostream>
#include "frozen_pool.h"

namespace spiritsaway::aoi
{

	enum class list_node_type
	{
		anchor = 0,
		left = 1,
		center = 2,
		right = 4,
	};

	struct list_node
	{
		union
		{
			aoi_pos_entity* pos_entity = nullptr;
			aoi_radius_entity* radius_entity;
		};
		
		list_node* prev = nullptr;
		list_node* next = nullptr;
		pos_unit_t pos;
		list_node_type node_type;
		void dump(std::ostream& out_debug) const
		{
			if (!pos_entity)
			{
				out_debug << "anchor node with pos " << pos << std::endl;
				return;
			}
			if (node_type == list_node_type::center)
			{
				out_debug << "node for guid " << pos_entity->guid() << " aoi_idx " << pos_entity->pos_idx().value << " radius_id " << 0 << " node_type " << int(node_type) << " pos " << pos << std::endl;
			}
			else
			{
				out_debug << "node for guid " << radius_entity->owner().guid() << " aoi_idx " << radius_entity->owner().pos_idx().value << " radius_id " << radius_entity->radius_idx().value << " node_type " << int(node_type) << " pos " << pos << std::endl;
			}
			
		}
	};




	struct sweep_result
	{
		std::vector<aoi_radius_entity*> left;
		std::vector<aoi_pos_entity*> middle;
		std::vector<aoi_radius_entity*> right;
		void clear()
		{
			left.clear();
			middle.clear();
			right.clear();
		}
	};


	class axis_list
	{
	private:
		frozen_pool<list_node> m_nodes_buffer;
		list_node m_head;
		list_node m_tail;
		std::vector<list_node*> m_nodes_for_pos;
		std::vector<std::pair<list_node*, list_node*>> m_nodes_for_radius;
		std::vector<list_node*> m_nodes_for_anchor;
		std::vector<list_node> m_anchor_buffer;
		std::vector<pos_unit_t> m_anchor_poses;
		const std::uint32_t m_node_per_anchor;
		const std::uint32_t m_max_entity_count;
		const pos_unit_t m_min_pos;
		const pos_unit_t m_max_pos;
		const pos_unit_t m_max_radius;
		const int m_xz_pos_idx;
		sweep_result m_sweep_buffer[3];
		void insert_before(list_node* next, list_node* cur);
		void remove_node(list_node* cur);
		void move_forward(list_node* cur, sweep_result& visited_nodes, std::uint8_t flag);
		void move_backward(list_node* cur, sweep_result& visited_nodes, std::uint8_t flag);
		list_node* find_boundary(pos_unit_t pos, bool is_lower) const;
		void insert_node(list_node* cur, bool is_lower);
	public:
		axis_list(std::uint32_t max_entity_count,std::uint32_t max_radius_count, pos_unit_t max_aoi_radius, int xz_pos_idx, pos_unit_t min_pos, pos_unit_t max_pos);
		void update_anchors();
		void insert_pos_entity(aoi_pos_entity* pos_entity);

		void remove_pos_entity(aoi_pos_entity* pos_entity);

		void insert_radius_entity(aoi_radius_entity* radius_entity);
		void remove_radius_entity(aoi_radius_entity* radius_entity);
		void update_entity_pos(aoi_pos_entity* pos_entity, pos_unit_t offset);
		void update_entity_radius(aoi_radius_entity* radius_entity, pos_unit_t delta_radius);
		std::vector<aoi_pos_entity*> entity_in_range(pos_unit_t range_begin, pos_unit_t range_end) const;
	};
}