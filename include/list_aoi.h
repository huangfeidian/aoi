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
	aoi_entity* entity;
	list_node* prev;
	list_node* next;
	std::int32_t pos;
	list_node_type node_type;
};

struct axis_nodes_for_entity
{
	list_node left;
	list_node middle;
	list_node right;
};

struct axis_list
{
	list_node head;
	list_node tail;
	std::vector<list_node*> anchors;
	std::vector<std::int32_t> anchor_poses;
	const std::uint32_t node_per_anchor;

	void update_anchors()
	{
		for(std::size_t i = 0; i< anchors.size(); i++)
		{
			list_node* cur_node = anchors[i];
			remove(cur_node);
			delete cur_node;
		}
		anchors.clear();
		auto prev = &head;
		auto cur_node = head.next;
		std::uint32_t count = 0;
		std::int32_t pos = head.pos;
		while(cur_node != &tail)
		{
			if(count >= node_per_anchor && cur_node->pos != pos)
			{
				list_node* anchor_node = new list_node;
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
	std::size_t insert_hint(std::int32_t pos, std::uint32_t last_hint = 0)
	{
		auto hint = std::lower_bound(anchor_poses.begin() + last_hint, anchor_poses.end(), pos);
		return std::distance(anchor_poses.begin(), hint);
	}
	void insert_before(list_node* prev, list_node* cur)
	{
		cur->next = prev;
		cur->prev = prev->prev;
		cur->prev->next = cur;
		cur->next->prev = cur;
	}
	void remove(list_node* cur)
	{
		cur->next->prev = cur->next;
		cur->prev->next = cur;
	}
	void insert_after_hint(list_node* hint, list_node* cur)
	{
		while(hint->pos >= cur->pos && hint->next != nullptr)
		{
			hint = hint->next;
		}
		insert_before(hint, cur);
	}
	void insert_nodes_for_entity(axis_nodes_for_entity* entity_nodes)
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
	void remove_nodes_for_entity(axis_nodes_for_entity* entity_nodes)
	{
		list_node* temp_nodes[3] = {&(entity_nodes->left), &(entity_nodes->middle), &(entity_nodes->right)};
		for(int i = 0; i< 3; i++)
		{
			auto cur_node = temp_nodes[i];
			remove(cur_node);
		}
	}
};