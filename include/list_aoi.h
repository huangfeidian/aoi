#include "aoi_caclator.h"
#include "frozen_pool.h"

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
	std::int32_t pos;
	list_node_type node_type;
};

struct axis_nodes_for_entity
{
	list_node left;
	list_node middle;
	list_node right;
	void set_entity(aoi_entity* cur_entity, int pos, std::uint16_t radius)
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
	void set_entity(aoi_entity* cur_entity)
	{
		x_nodes.set_entity(cur_entity, cur_entity->pos[0], cur_entity->radius);
		z_nodes.set_entity(cur_entity, cur_entity->pos[2], cur_entity->radius);
	}
};

struct sweep_result
{
	std::unordered_set<aoi_entity*> left;
	std::unordered_set<aoi_entity*> middle;
	std::unordered_set<aoi_entity*> right;
};
struct move_result
{
	std::unordered_set<aoi_entity*> enter_entities;
	std::unordered_set<aoi_entity*> leave_entities;

	std::unordered_set<aoi_entity*> enter_notify_entities;
	std::unordered_set<aoi_entity*> leave_notify_entities;
	void clear()
	{
		enter_notify_entities.clear();
		enter_entities.clear();
		leave_notify_entities.clear();
		leave_entities.clear();
	}
	void merge(const move_result& a, const move_result& b)
	{
		unordered_set_join(a.enter_entities, b.enter_entities, enter_entities);
		unordered_set_union(a.leave_entities, b.leave_entities, leave_entities);
		unordered_set_union(a.leave_notify_entities, b.leave_notify_entities, leave_notify_entities);
		unordered_set_join(a.enter_notify_entities, b.enter_notify_entities, enter_notify_entities);
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
	const std::int32_t min_pos;
	const std::int32_t max_pos;
	sweep_result sweep_buffer[3];

	move_result update_info;

	std::size_t insert_hint(std::int32_t pos, std::uint32_t last_hint = 0);
	void insert_before(list_node* prev, list_node* cur);
	void remove(list_node* cur);
	void insert_after_hint(list_node* hint, list_node* cur);
	void move_forward(list_node* cur, sweep_result& visited_nodes, std::uint8_t flag);
	void move_backward(list_node* cur, sweep_result& visited_nodes, std::uint8_t flag);
public:
	axis_list(std::uint32_t max_entity_count, std::int32_t min_pos, std::int32_t max_pos);
	void update_anchors();
	void insert_entity(axis_nodes_for_entity* entity_nodes);

	void remove_entity(axis_nodes_for_entity* entity_nodes);
	void update_entity_pos(axis_nodes_for_entity* entity_nodes, std::int32_t offset);
	void update_entity_radius(axis_nodes_for_entity* entity_nodes, std::int32_t delta_radius);
	const move_result& get_update_info() const;
};

class list_2d_aoi
{
private:
	axis_list x_axis;
	axis_list z_axis;
	frozen_pool<axis_2d_nodes_for_entity> nodes_buffer;
	move_result update_info;
public:
	list_2d_aoi(std::uint32_t max_entity_size, pos_t min, pos_t max);
	void insert_entity(aoi_entity* cur_entity);
	void remove_entity(aoi_entity* cur_entity);
	void update_entity_pos(aoi_entity* cur_entity, pos_t new_pos);
	void update_entity_radius(aoi_entity* cur_entity, std::uint16_t radius);
};