#pragma once

#include <unordered_set>
#include <aoi_entity.h>
#include <ostream>

enum class list_node_type
{
	anchor = 0,
	left = 1,
	center = 2,
	right = 4,
};

struct list_node
{
	aoi_entity* entity = nullptr;
	list_node* prev = nullptr;
	list_node* next = nullptr;
	pos_unit_t pos;
	list_node_type node_type;
	void dump(std::ostream& out_debug) const
	{
		if (!entity)
		{
			out_debug << "anchor node with pos " << pos << std::endl;
			return;
		}
		out_debug << "node for guid " << entity->guid() << " node_type " << int(node_type) << " pos " << pos << std::endl;
	}
};

struct axis_nodes_for_entity
{
	list_node left;
	list_node middle;
	list_node right;
	void set_entity(aoi_entity* cur_entity, pos_unit_t pos, pos_unit_t radius)
	{
		left.entity = right.entity=middle.entity=cur_entity;
		left.node_type = list_node_type::left;
		middle.node_type = list_node_type::center;
		right.node_type = list_node_type::right;

		left.pos = pos - radius;
		right.pos = pos + radius;
		middle.pos = pos;
	}
};
struct axis_2d_nodes_for_entity
{
	axis_nodes_for_entity x_nodes;
	axis_nodes_for_entity z_nodes;
	// 由于两个轴上计算出来的只是2d矩形区域 而aoi计算区域则是圆形的  
	// 所以2d计算出来的结果需要先存在这里 然后再计算是否真的进入aoi
	std::unordered_set<aoi_entity*> xz_interested_by;
	std::unordered_set<aoi_entity*> xz_interest_in;
	aoi_entity* entity;
	bool is_leaving = false;
	void set_entity(aoi_entity* cur_entity)
	{
		x_nodes.set_entity(cur_entity, cur_entity->pos()[0], cur_entity->radius());
		z_nodes.set_entity(cur_entity, cur_entity->pos()[2], cur_entity->radius());
		xz_interest_in.clear();
		xz_interested_by.clear();
		entity = cur_entity;
	}
	bool enter(aoi_entity* other_entity);
	bool leave(aoi_entity* other_entity);
	void move();
	void update_radius(std::uint16_t radius);
	bool in_range(const aoi_entity* other_entity);
	void detach();
};

struct sweep_result
{
	std::vector<aoi_entity*> left;
	std::vector<aoi_entity*> middle;
	std::vector<aoi_entity*> right;
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
	list_node head;
	list_node tail;
	std::vector<list_node*> anchors;
	std::vector<list_node> anchor_buffer;
	std::vector<std::int32_t> anchor_poses;
	const std::uint32_t node_per_anchor;
	const std::uint32_t max_entity_count;
	const pos_unit_t min_pos;
	const pos_unit_t max_pos;
	const std::uint16_t max_radius;
	sweep_result sweep_buffer[3];
	std::uint32_t dirty_count = 0;
	void insert_before(list_node* next, list_node* cur);
	void remove(list_node* cur);
	void move_forward(list_node* cur, sweep_result& visited_nodes, std::uint8_t flag);
	void move_backward(list_node* cur, sweep_result& visited_nodes, std::uint8_t flag);
	list_node* find_boundary(pos_unit_t pos, bool is_lower) const;
	void insert_node(list_node* cur, bool is_lower);
	void check_update_anchors();
public:
	axis_list(std::uint32_t max_entity_count, pos_unit_t max_aoi_radius, pos_unit_t min_pos, pos_unit_t max_pos);
	void update_anchors();
	void insert_entity(axis_nodes_for_entity* entity_nodes);

	void remove_entity(axis_nodes_for_entity* entity_nodes);
	void update_entity_pos(axis_nodes_for_entity* entity_nodes, pos_unit_t offset);
	void update_entity_radius(axis_nodes_for_entity* entity_nodes, pos_unit_t delta_radius);
	std::unordered_set<aoi_entity*> entity_in_range(pos_unit_t range_begin, pos_unit_t range_end) const;
	std::vector<axis_2d_nodes_for_entity*> dump(std::ostream& out_debug) const;
};