#pragma once

#include "aoi_caclator.h"
#include "frozen_pool.h"

class tiled_aoi: public aoi_caclator
{
	struct tile_entry
	{
		aoi_entity* entity = nullptr;
		tile_entry* next = nullptr;
		tile_entry* prev = nullptr;
		int tile_x;
		int tile_z;
	};
public:
	tiled_aoi(std::uint32_t tile_size, std::uint32_t max_entity_size, std::uint32_t tile_blocks);
	~tiled_aoi();
	bool add_entity(aoi_entity* entity) override;
	bool remove_entity(aoi_entity* entity) override;
	void on_position_update(aoi_entity* entity, pos_t new_pos) override;
	void on_radius_update(aoi_entity* entity, std::uint16_t new_radius) override;
	void update_all() override;
	std::vector<guid_t> entity_in_circle(pos_t center, std::uint16_t radius) const override;

	std::vector<guid_t> entity_in_cylinder(pos_t center, std::uint16_t radius, std::uint16_t height) const override;
	std::vector<guid_t> entity_in_rectangle(pos_t center, std::uint16_t x_width, std::uint16_t z_width) const override;

	std::vector<guid_t> entity_in_cuboid(pos_t center, std::uint16_t x_width, std::uint16_t z_width, std::uint16_t y_height) const override;
private:
	void unlink(tile_entry* cur_entry);
	void link(tile_entry* cur_entry, const pos_t& pos);
public:
	std::uint32_t tile_hash(const pos_t& pos) const;

	template <typename T>
	void filter_pos_entity_in_tile(int tile_x, int tile_z, const T& pred, std::vector<aoi_entity*>& result) const
	{
		auto cur_bucket_idx = computeTileHash(tile_x, tile_z, tile_blocks);
		auto temp_entry = tile_buckets[cur_bucket_idx].next;
		while(temp_entry)
		{
			if(temp_entry->tile_x == tile_x && temp_entry->tile_z == tile_z)
			{
				if(pred(temp_entry->entity->pos))
				{
					result.push_back(temp_entry->entity);
				}
			}
			temp_entry = temp_entry->next;
			
		}
	}
	
	template <typename T>
	std::vector<guid_t> filter_pos_guid_in_tiles(int tile_x_min, int tile_z_min, int tile_x_max, int tile_z_max, const T& pred) const
	{
		std::vector<aoi_entity*> temp_aoi_result;
		for(int m = tile_x_min; m <= tile_x_max; m++)
		{
			for(int n = tile_z_min; n <= tile_z_max; n++)
			{
				filter_pos_entity_in_tile(m, n, pred, temp_aoi_result);
			}
		}
		std::vector<guid_t> result;
		result.reserve(temp_aoi_result.size());
		for(auto one_entity: temp_aoi_result)
		{
			result.push_back(one_entity->guid);
		}
		return result;
	}
private:
	const std::uint32_t tile_blocks;
	const std::uint32_t tile_size;
	frozen_pool<tile_entry> entry_pool;
	std::vector<tile_entry> tile_buckets;
};