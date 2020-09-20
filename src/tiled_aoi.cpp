#include "tiled_aoi.h"

inline std::uint32_t computeTileHash(int x, int y, const std::uint32_t mask)
{
	const unsigned int h1 = 0x8da6b343; // Large multiplicative constants;
	const unsigned int h2 = 0xd8163841; // here arbitrarily chosen primes
	unsigned int n = h1 * x + h2 * y;
	return n & mask;
}
inline std::uint32_t computeTileHash(pos_t pos, const std::uint32_t tile_size, const std::uint32_t mask)
{
	
}
// set_result = set_1 - set_2
static void unordered_set_diff(const std::unordered_set<guid_t>& set_1, const std::unordered_set<guid_t>& set_2, std::unordered_set<guid_t>& set_result)
{
	for(auto one_guid: set_1)
	{
		if(set_2.find(one_guid) == set_2.end())
		{
			set_result.insert(one_guid);
		}
	}
}

tiled_aoi::tiled_aoi(std::uint32_t in_tile_size, std::uint32_t in_max_entity_size, std::uint32_t in_tile_blocks)
: aoi_caclator()
, tile_size(in_tile_size)
, entry_pool(in_max_entity_size)
, tile_blocks(in_tile_blocks)
, tile_buckets(in_tile_blocks)
{

}

tiled_aoi::~tiled_aoi()
{

}

std::uint32_t tiled_aoi::tile_hash(const pos_t& pos) const
{
	return computeTileHash(int(std::floor(pos[0] * 1.0/tile_size)), int(std::floor(pos[2]*1.0/tile_size)), tile_blocks);
}
void tiled_aoi::link(tile_entry* cur_entry, const pos_t& pos)
{
	int tile_x = int(std::floor(pos[0] * 1.0/tile_size));
	int tile_z = int(std::floor(pos[2]*1.0/tile_size));
	cur_entry->tile_x = tile_x;
	cur_entry->tile_z = tile_z;
	auto cur_bucket_idx = computeTileHash(tile_x, tile_z, tile_blocks);
	auto pre_next = tile_buckets[cur_bucket_idx].next;
	pre_next->prev = cur_entry;
	cur_entry->next = pre_next;
	cur_entry->prev = &tile_buckets[cur_bucket_idx];
	cur_entry->prev->next = cur_entry;
}

void tiled_aoi::unlink(tile_entry* cur_entry)
{
	auto prev = cur_entry->prev;
	auto next = cur_entry->next;
	prev->next = next;
	next->prev = prev;
	cur_entry->prev = nullptr;
	cur_entry->next = nullptr;
}
bool tiled_aoi::add_entity(aoi_entity* entity)
{
	if(entity->cacl_data)
	{
		return false;
	}
	auto cur_entry = entry_pool.request();
	if(!cur_entry)
	{
		return false;
	}
	entity->cacl_data = cur_entry;
	cur_entry->entity = entity;
	link(cur_entry, entity->pos);

	return true;
}
bool tiled_aoi::remove_entity(aoi_entity* entity)
{
	if(!entity->cacl_data)
	{
		return false;
	}
	tile_entry* cur_entry = (tile_entry*)entity->cacl_data;
	unlink(cur_entry);
	entity->cacl_data = nullptr;
	entry_pool.renounce(cur_entry);
	return true;

}

void tiled_aoi::on_position_update(aoi_entity* entity, pos_t new_pos)
{
	if(!entity->cacl_data)
	{
		return;
	}
	tile_entry* cur_entry = (tile_entry*)entity->cacl_data;
	auto pre_bucket = tile_hash(entity->pos);
	auto cur_bucket = tile_hash(new_pos);
	entity->pos = new_pos;
	if(pre_bucket == cur_bucket)
	{
		return;
	}
	unlink(cur_entry);
	link(cur_entry, new_pos);
}

void tiled_aoi::on_radius_update(aoi_entity* entity, std::uint16_t new_radius)
{
	entity->radius = new_radius;
	return;
}


void tiled_aoi::update_all()
{
	// 清空之前的临时状态
	for(int i = 0; i<tile_blocks; i++)
	{
		auto temp_entry = tile_buckets[i].next;
		while(temp_entry)
		{
			auto cur_entity = temp_entry->entity;
			cur_entity->interested_by.clear();
			cur_entity->interest_in.clear();
			cur_entity->enter_guids.clear();
			cur_entity->leave_guids.clear();
			temp_entry = temp_entry->next;
		}
	}
	// 计算所有因为force 建立的 interest

	for(int i = 0; i<tile_blocks; i++)
	{
		auto temp_entry = tile_buckets[i].next;
		while(temp_entry)
		{
			auto cur_entity = temp_entry->entity;
			cur_entity->interest_in = cur_entity->force_interest_in;
			temp_entry = temp_entry->next;
		}
	}

	// 计算所有的因为radius建立的interested
	std::vector<aoi_entity*> temp_aoi_result;
	for(int i = 0; i<tile_blocks; i++)
	{
		auto temp_entry = tile_buckets[i].next;
		while(temp_entry)
		{
			auto one_entity = temp_entry->entity;
			if(one_entity->radius == 0 || one_entity->has_flag(aoi_flag::forbid_interest_in))
			{
				continue;
			}
			temp_aoi_result.clear();
			// 过滤所有在范围内的entity
			int tile_x_min = std::floor((one_entity->pos[0] - one_entity->radius) * 1.0 / tile_size);
			int tile_z_min =  std::floor((one_entity->pos[2] - one_entity->radius) * 1.0 / tile_size);
			int tile_x_max = std::floor((one_entity->pos[0] + one_entity->radius) * 1.0 / tile_size);
			int tile_z_max =  std::floor((one_entity->pos[2] + one_entity->radius) * 1.0 / tile_size);
			auto radius_square = one_entity->radius * one_entity->radius;
			const auto& pos_1 = one_entity->pos;
			auto filter_lambda = [&](const pos_t& pos_2){
				int32_t diff_x = pos_1[0] - pos_2[0];
				int32_t diff_z = pos_1[2] - pos_2[2];
				return diff_z * diff_z + diff_x*diff_x <= radius_square;
			};
			for(int m = tile_x_min; m <= tile_x_max; m++)
			{
				for(int n = tile_z_min; n <= tile_z_max; n++)
				{
					filter_pos_entity_in_tile(m, n, filter_lambda, temp_aoi_result);
				}
			}
			// 计算完了之后 更新Interested
			for(auto one_aoi_result: temp_aoi_result)
			{
				if(one_entity->can_add_enter(*one_aoi_result, true))
				{
					one_entity->interest_in.insert(one_aoi_result->guid);
					one_aoi_result->interested_by.insert(one_entity->guid);
				}
			}
			temp_entry = temp_entry->next;
		}
	}
	// 建立所有的enter leave

	for(int i = 0; i<tile_blocks; i++)
	{
		auto temp_entry = tile_buckets[i].next;
		while(temp_entry)
		{
			auto cur_entity = temp_entry->entity;
			std::unordered_set<guid_t> prev_interest_in = std::unordered_set<guid_t>(cur_entity->prev_interest_in.begin(), cur_entity->prev_interest_in.end());

			unordered_set_diff(cur_entity->interest_in, prev_interest_in, cur_entity->enter_guids);

			unordered_set_diff(prev_interest_in, cur_entity->interest_in, cur_entity->leave_guids);;

			temp_entry = temp_entry->next;

		}
	}
}

std::vector<guid_t> tiled_aoi::entity_in_circle(pos_t center, std::uint16_t radius) const
{
	int tile_x_min = std::floor((center[0] - radius) * 1.0 / tile_size);
	int tile_z_min =  std::floor((center[2] - radius) * 1.0 / tile_size);
	int tile_x_max = std::floor((center[0] + radius) * 1.0 / tile_size);
	int tile_z_max =  std::floor((center[2] + radius) * 1.0 / tile_size);
	auto radius_square = radius * radius;
	const auto& pos_1 = center;

	auto filter_lambda = [&](const pos_t& pos_2){
		int32_t diff_x = pos_1[0] - pos_2[0];
		int32_t diff_z = pos_1[2] - pos_2[2];
		return diff_z * diff_z + diff_x*diff_x <= radius_square;
	};
	return filter_pos_guid_in_tiles(tile_x_min, tile_z_min, tile_x_max, tile_z_max, filter_lambda);

}
std::vector<guid_t> tiled_aoi::entity_in_cylinder(pos_t center, std::uint16_t radius, std::uint16_t height) const
{
	int tile_x_min = std::floor((center[0] - radius) * 1.0 / tile_size);
	int tile_z_min =  std::floor((center[2] - radius) * 1.0 / tile_size);
	int tile_x_max = std::floor((center[0] + radius) * 1.0 / tile_size);
	int tile_z_max =  std::floor((center[2] + radius) * 1.0 / tile_size);
	auto radius_square = radius * radius;
	const auto& pos_1 = center;
	auto filter_lambda = [&](const pos_t& pos_2){
		int32_t diff_x = pos_1[0] - pos_2[0];
		int32_t diff_z = pos_1[2] - pos_2[2];
		int32_t diff_y = pos_1[1] - pos_2[1];
		return diff_z * diff_z + diff_x*diff_x <= radius_square && diff_y <= height && diff_y >= -height;
	};
	return filter_pos_guid_in_tiles(tile_x_min, tile_z_min, tile_x_max, tile_z_max, filter_lambda);

}

std::vector<guid_t> tiled_aoi::entity_in_rectangle(pos_t center, std::uint16_t x_width, std::uint16_t z_width) const
{
	int tile_x_min = std::floor((center[0] - x_width) * 1.0 / tile_size);
	int tile_z_min =  std::floor((center[2] - z_width) * 1.0 / tile_size);
	int tile_x_max = std::floor((center[0] + x_width) * 1.0 / tile_size);
	int tile_z_max =  std::floor((center[2] + z_width) * 1.0 / tile_size);
	const auto& pos_1 = center;
	auto filter_lambda = [&](const pos_t& pos_2){
		int32_t diff_x = pos_1[0] - pos_2[0];
		int32_t diff_z = pos_1[2] - pos_2[2];
		return diff_x <= x_width && diff_x >= -x_width && diff_z <= z_width && diff_z >= -z_width;
	};
	return filter_pos_guid_in_tiles(tile_x_min, tile_z_min, tile_x_max, tile_z_max, filter_lambda);
}

std::vector<guid_t> tiled_aoi::entity_in_cuboid(pos_t center, std::uint16_t x_width, std::uint16_t z_width, std::uint16_t y_height) const
{
	int tile_x_min = std::floor((center[0] - x_width) * 1.0 / tile_size);
	int tile_z_min =  std::floor((center[2] - z_width) * 1.0 / tile_size);
	int tile_x_max = std::floor((center[0] + x_width) * 1.0 / tile_size);
	int tile_z_max =  std::floor((center[2] + z_width) * 1.0 / tile_size);
	const auto& pos_1 = center;
	auto filter_lambda = [&](const pos_t& pos_2){
		int32_t diff_x = pos_1[0] - pos_2[0];
		int32_t diff_z = pos_1[2] - pos_2[2];
		int32_t diff_y = pos_1[1] - pos_2[1];
		return diff_x <= x_width && diff_x >= -x_width && diff_z <= z_width && diff_z >= -z_width && diff_y <= y_height && diff_y >= -y_height;
	};
	return filter_pos_guid_in_tiles(tile_x_min, tile_z_min, tile_x_max, tile_z_max, filter_lambda);
}



