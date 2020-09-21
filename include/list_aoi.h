#include "aoi_caclator.h"

enum class list_node_type
{
	anchor = 1,
	left = 2,
	center = 3,
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
};

struct sweep_result
{
	std::unordered_set<aoi_entity*> left;
	std::unordered_set<aoi_entity*> middle;
	std::unordered_set<aoi_entity*> right;
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

	std::unordered_set<aoi_entity*> enter_entities;
	std::unordered_set<aoi_entity*> leave_entities;

	std::unordered_set<aoi_entity*> enter_notify_entities;
	std::unordered_set<aoi_entity*> leave_notify_entities;

	std::size_t insert_hint(std::int32_t pos, std::uint32_t last_hint = 0);
	void insert_before(list_node* prev, list_node* cur);
	void remove(list_node* cur);
	void insert_after_hint(list_node* hint, list_node* cur);
	void move_forward(list_node* cur, sweep_result& visited_nodes);
	void move_backward(list_node* cur, sweep_result& visited_nodes);
public:
	axis_list(std::uint32_t max_entity_count, std::int32_t min_pos, std::int32_t max_pos);
	void update_anchors();
	void insert_entity(axis_nodes_for_entity* entity_nodes);
	void remove_entity(axis_nodes_for_entity* entity_nodes);
	void update_entity_pos(axis_nodes_for_entity* entity_nodes, std::int32_t offset);
	void update_entity_radius(axis_nodes_for_entity* entity_nodes, std::int32_t delta_radius);
};