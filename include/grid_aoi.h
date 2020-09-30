#pragma once

#include "aoi_interface.h"
#include "frozen_pool.h"

class grid_aoi: public aoi_interface
{
	struct grid_entry
	{
		aoi_entity* entity = nullptr;
		grid_entry* next = nullptr;
		grid_entry* prev = nullptr;
		int grid_x;
		int grid_z;
	};
public:
	grid_aoi(std::uint32_t max_entity_size, std::uint32_t max_aoi_radius, pos_t border_min, pos_t border_max, std::uint32_t grid_size,  std::uint32_t grid_blocks);
	~grid_aoi();
	bool add_entity(aoi_entity* entity) override;
	bool remove_entity(aoi_entity* entity) override;
	void on_position_update(aoi_entity* entity, pos_t new_pos) override;
	void on_radius_update(aoi_entity* entity, std::uint16_t new_radius) override;
	std::unordered_set<aoi_entity*> update_all() override;
	std::unordered_set<aoi_entity*> entity_in_circle(pos_t center, std::uint16_t radius) const override;

	std::unordered_set<aoi_entity*> entity_in_cylinder(pos_t center, std::uint16_t radius, std::uint16_t height) const override;
	std::unordered_set<aoi_entity*> entity_in_rectangle(pos_t center, std::uint16_t x_width, std::uint16_t z_width) const override;

	std::unordered_set<aoi_entity*> entity_in_cuboid(pos_t center, std::uint16_t x_width, std::uint16_t z_width, std::uint16_t y_height) const override;
private:
	void unlink(grid_entry* cur_entry);
	void link(grid_entry* cur_entry, const pos_t& pos);
public:
	std::uint32_t grid_hash(const pos_t& pos) const;

	template <typename T>
	void filter_pos_entity_in_grid(int grid_x, int grid_z, const T& pred, std::unordered_set<aoi_entity*>& result) const
	{
		auto cur_bucket_idx = computegridHash(grid_x, grid_z, grid_blocks);
		auto temp_entry = grid_buckets[cur_bucket_idx].next;
		while(temp_entry)
		{
			if(temp_entry->grid_x == grid_x && temp_entry->grid_z == grid_z)
			{
				if(pred(temp_entry->entity->pos))
				{
					result.insert(temp_entry->entity);
				}
			}
			temp_entry = temp_entry->next;
			
		}
	}
	
	template <typename T>
	std::unordered_set<aoi_entity*> filter_pos_entity_in_grids(int grid_x_min, int grid_z_min, int grid_x_max, int grid_z_max, const T& pred) const
	{
		std::unordered_set<aoi_entity*> result;
		for(int m = grid_x_min; m <= grid_x_max; m++)
		{
			for(int n = grid_z_min; n <= grid_z_max; n++)
			{
				filter_pos_entity_in_grid(m, n, pred, result);
			}
		}
		return result;
	}
private:
	const std::uint32_t grid_blocks;
	const std::uint32_t grid_size;
	frozen_pool<grid_entry> entry_pool;
	std::vector<grid_entry> grid_buckets;
};