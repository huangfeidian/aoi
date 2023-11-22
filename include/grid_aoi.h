#pragma once

#include "aoi_interface.h"
#include "frozen_pool.h"

namespace spiritsaway::aoi
{

	class grid_aoi : public aoi_interface
	{
		struct grid_entry
		{
			aoi_pos_entity* entity = nullptr;
			grid_entry* next = nullptr;
			grid_entry* prev = nullptr;
			int grid_x = 0;
			int grid_z = 0;
		};
	public:
		grid_aoi(aoi_idx_t max_entity_size, pos_unit_t max_aoi_radius, pos_t border_min, pos_t border_max, std::uint32_t grid_size, std::uint32_t grid_blocks);
		~grid_aoi();
		bool add_pos_entity(aoi_pos_entity* entity) override;
		bool remove_pos_entity(aoi_pos_entity* entity) override;
		bool add_radius_entity(aoi_radius_entity* entity) override;
		bool remove_radius_entity(aoi_radius_entity* entity) override;
		void on_position_update(aoi_pos_entity* entity, pos_t new_pos) override;
		void on_ctrl_update(aoi_radius_entity* entity, const aoi_radius_controler& pre_ctrl) override;
		void update_all() override;
		std::vector<aoi_pos_entity*> entity_in_circle(pos_t center, pos_unit_t radius) const override;

		std::vector<aoi_pos_entity*> entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height) const override;
		std::vector<aoi_pos_entity*> entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width) const override;

		std::vector<aoi_pos_entity*> entity_in_cuboid(pos_t center, pos_t extend) const override;
		void dump(std::ostream& out_debug) const override;
		void on_flag_update(aoi_pos_entity* entity) override;
	private:
		void unlink(grid_entry* cur_entry);
		void link(grid_entry* cur_entry, const pos_t& pos);
		int cacl_grid_id(pos_unit_t pos) const;
	public:
		static std::uint32_t computegridHash(int grid_x, int grid_z, const std::uint32_t grid_bucket_num);
		std::uint32_t grid_hash(const pos_t& pos) const;

		template <typename T>
		void filter_pos_entity_in_grid(int grid_x, int grid_z, const T& pred, std::vector<aoi_pos_entity*>& result) const
		{
			auto cur_bucket_idx = computegridHash(grid_x, grid_z, grid_bucket_num);
			auto temp_entry = grid_buckets[cur_bucket_idx].next;
			while (temp_entry)
			{
				if (temp_entry->grid_x == grid_x && temp_entry->grid_z == grid_z)
				{
					if (pred(temp_entry->entity->pos()) && m_entity_byteset[temp_entry->entity->pos_idx().value] == 0)
					{
						result.push_back(temp_entry->entity);
						m_entity_byteset[temp_entry->entity->pos_idx().value] = 0;
					}
				}
				temp_entry = temp_entry->next;

			}
		}

		template <typename T>
		std::vector<aoi_pos_entity*> filter_pos_entity_in_grids(int grid_x_min, int grid_z_min, int grid_x_max, int grid_z_max, const T& pred) const
		{
			std::vector<aoi_pos_entity*> result;
			for (int m = grid_x_min; m <= grid_x_max; m++)
			{
				for (int n = grid_z_min; n <= grid_z_max; n++)
				{
					filter_pos_entity_in_grid(m, n, pred, result);
				}
			}
			for (auto one_result : result)
			{
				m_entity_byteset[one_result->pos_idx().value] = 0;
			}
			return result;
		}
	private:
		const std::uint32_t grid_bucket_num;
		const std::uint32_t grid_size;
		frozen_pool<grid_entry> entry_pool;
		std::vector<grid_entry> grid_buckets;
		mutable std::vector<std::uint8_t> m_entity_byteset;
		std::unordered_map<aoi_pos_idx, aoi_pos_entity*> m_pos_entities;
	};
}