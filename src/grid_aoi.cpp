#include "grid_aoi.h"

inline std::uint32_t computegridHash(int x, int y, const std::uint32_t mask)
{
	const unsigned int h1 = 0x8da6b343; // Large multiplicative constants;
	const unsigned int h2 = 0xd8163841; // here arbitrarily chosen primes
	unsigned int n = h1 * x + h2 * y;
	return n & mask;
}
// set_result = set_1 - set_2


grid_aoi::grid_aoi(std::uint32_t max_entity_size, std::uint32_t max_aoi_radius, pos_t border_min, pos_t border_max, std::uint32_t in_grid_size,  std::uint32_t in_grid_blocks)
: aoi_interface(max_entity_size, max_aoi_radius, border_min, border_max)
, grid_size(in_grid_size)
, entry_pool(max_entity_size)
, grid_blocks(in_grid_blocks)
, grid_buckets(in_grid_blocks)
{

}

grid_aoi::~grid_aoi()
{

}

std::uint32_t grid_aoi::grid_hash(const pos_t& pos) const
{
	return computegridHash(int(std::floor(pos[0] * 1.0/grid_size)), int(std::floor(pos[2]*1.0/grid_size)), grid_blocks);
}
void grid_aoi::link(grid_entry* cur_entry, const pos_t& pos)
{
	int grid_x = int(std::floor(pos[0] * 1.0/grid_size));
	int grid_z = int(std::floor(pos[2]*1.0/grid_size));
	cur_entry->grid_x = grid_x;
	cur_entry->grid_z = grid_z;
	auto cur_bucket_idx = computegridHash(grid_x, grid_z, grid_blocks);
	auto pre_next = grid_buckets[cur_bucket_idx].next;
	pre_next->prev = cur_entry;
	cur_entry->next = pre_next;
	cur_entry->prev = &grid_buckets[cur_bucket_idx];
	cur_entry->prev->next = cur_entry;
}

void grid_aoi::unlink(grid_entry* cur_entry)
{
	auto prev = cur_entry->prev;
	auto next = cur_entry->next;
	prev->next = next;
	next->prev = prev;
	cur_entry->prev = nullptr;
	cur_entry->next = nullptr;
}
bool grid_aoi::add_entity(aoi_entity* entity)
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
bool grid_aoi::remove_entity(aoi_entity* entity)
{
	if(!entity->cacl_data)
	{
		return false;
	}
	grid_entry* cur_entry = (grid_entry*)entity->cacl_data;
	unlink(cur_entry);
	entity->cacl_data = nullptr;
	entry_pool.renounce(cur_entry);
	return true;

}

void grid_aoi::on_position_update(aoi_entity* entity, pos_t new_pos)
{
	if(!entity->cacl_data)
	{
		return;
	}
	grid_entry* cur_entry = (grid_entry*)entity->cacl_data;
	auto pre_bucket = grid_hash(entity->pos);
	auto cur_bucket = grid_hash(new_pos);
	entity->pos = new_pos;
	if(pre_bucket == cur_bucket)
	{
		return;
	}
	unlink(cur_entry);
	link(cur_entry, new_pos);
}

void grid_aoi::on_radius_update(aoi_entity* entity, std::uint16_t new_radius)
{
	entity->radius = new_radius;
	return;
}


std::unordered_set<aoi_entity*> grid_aoi::update_all()
{
	// 清空之前的临时状态
	for(int i = 0; i<grid_blocks; i++)
	{
		auto temp_entry = grid_buckets[i].next;
		while(temp_entry)
		{
			auto cur_entity = temp_entry->entity;
			cur_entity->interested_by.clear();
			cur_entity->interest_in.clear();
			temp_entry = temp_entry->next;
		}
	}
	// 计算所有因为force 建立的 interest

	for(int i = 0; i<grid_blocks; i++)
	{
		auto temp_entry = grid_buckets[i].next;
		while(temp_entry)
		{
			auto cur_entity = temp_entry->entity;
			cur_entity->interest_in = cur_entity->force_interest_in;
			cur_entity->interested_by = cur_entity->force_interested_by;
			temp_entry = temp_entry->next;
		}
	}

	// 计算所有的因为radius建立的interested
	std::unordered_set<aoi_entity*> temp_aoi_result;
	for(int i = 0; i<grid_blocks; i++)
	{
		auto temp_entry = grid_buckets[i].next;
		while(temp_entry)
		{
			auto one_entity = temp_entry->entity;
			if(one_entity->radius == 0 || one_entity->has_flag(aoi_flag::forbid_interest_in))
			{
				continue;
			}
			temp_aoi_result.clear();
			// 过滤所有在范围内的entity
			int grid_x_min = std::floor((one_entity->pos[0] - one_entity->radius) * 1.0 / grid_size);
			int grid_z_min =  std::floor((one_entity->pos[2] - one_entity->radius) * 1.0 / grid_size);
			int grid_x_max = std::floor((one_entity->pos[0] + one_entity->radius) * 1.0 / grid_size);
			int grid_z_max =  std::floor((one_entity->pos[2] + one_entity->radius) * 1.0 / grid_size);
			auto radius_square = one_entity->radius * one_entity->radius;
			const auto& pos_1 = one_entity->pos;
			auto filter_lambda = [&](const pos_t& pos_2){
				int32_t diff_x = pos_1[0] - pos_2[0];
				int32_t diff_z = pos_1[2] - pos_2[2];
				return diff_z * diff_z + diff_x*diff_x <= radius_square;
			};
			for(int m = grid_x_min; m <= grid_x_max; m++)
			{
				for(int n = grid_z_min; n <= grid_z_max; n++)
				{
					filter_pos_entity_in_grid(m, n, filter_lambda, temp_aoi_result);
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
	std::unordered_set<aoi_entity*> result;
	for(int i = 0; i<grid_blocks; i++)
	{
		auto temp_entry = grid_buckets[i].next;
		while(temp_entry)
		{
			auto cur_entity = temp_entry->entity;
			result.insert(cur_entity);
			temp_entry = temp_entry->next;
		}
	}
	return result;
}

std::unordered_set<aoi_entity*> grid_aoi::entity_in_circle(pos_t center, std::uint16_t radius) const
{
	int grid_x_min = std::floor((center[0] - radius) * 1.0 / grid_size);
	int grid_z_min =  std::floor((center[2] - radius) * 1.0 / grid_size);
	int grid_x_max = std::floor((center[0] + radius) * 1.0 / grid_size);
	int grid_z_max =  std::floor((center[2] + radius) * 1.0 / grid_size);
	auto radius_square = radius * radius;
	const auto& pos_1 = center;

	auto filter_lambda = [&](const pos_t& pos_2){
		int32_t diff_x = pos_1[0] - pos_2[0];
		int32_t diff_z = pos_1[2] - pos_2[2];
		return diff_z * diff_z + diff_x*diff_x <= radius_square;
	};
	return filter_pos_entity_in_grids(grid_x_min, grid_z_min, grid_x_max, grid_z_max, filter_lambda);

}
std::unordered_set<aoi_entity*> grid_aoi::entity_in_cylinder(pos_t center, std::uint16_t radius, std::uint16_t height) const
{
	int grid_x_min = std::floor((center[0] - radius) * 1.0 / grid_size);
	int grid_z_min =  std::floor((center[2] - radius) * 1.0 / grid_size);
	int grid_x_max = std::floor((center[0] + radius) * 1.0 / grid_size);
	int grid_z_max =  std::floor((center[2] + radius) * 1.0 / grid_size);
	auto radius_square = radius * radius;
	const auto& pos_1 = center;
	auto filter_lambda = [&](const pos_t& pos_2){
		int32_t diff_x = pos_1[0] - pos_2[0];
		int32_t diff_z = pos_1[2] - pos_2[2];
		int32_t diff_y = pos_1[1] - pos_2[1];
		return diff_z * diff_z + diff_x*diff_x <= radius_square && diff_y <= height && diff_y >= -height;
	};
	return filter_pos_entity_in_grids(grid_x_min, grid_z_min, grid_x_max, grid_z_max, filter_lambda);

}

std::unordered_set<aoi_entity*> grid_aoi::entity_in_rectangle(pos_t center, std::uint16_t x_width, std::uint16_t z_width) const
{
	int grid_x_min = std::floor((center[0] - x_width) * 1.0 / grid_size);
	int grid_z_min =  std::floor((center[2] - z_width) * 1.0 / grid_size);
	int grid_x_max = std::floor((center[0] + x_width) * 1.0 / grid_size);
	int grid_z_max =  std::floor((center[2] + z_width) * 1.0 / grid_size);
	const auto& pos_1 = center;
	auto filter_lambda = [&](const pos_t& pos_2){
		int32_t diff_x = pos_1[0] - pos_2[0];
		int32_t diff_z = pos_1[2] - pos_2[2];
		return diff_x <= x_width && diff_x >= -x_width && diff_z <= z_width && diff_z >= -z_width;
	};
	return filter_pos_entity_in_grids(grid_x_min, grid_z_min, grid_x_max, grid_z_max, filter_lambda);
}

std::unordered_set<aoi_entity*> grid_aoi::entity_in_cuboid(pos_t center, std::uint16_t x_width, std::uint16_t z_width, std::uint16_t y_height) const
{
	int grid_x_min = std::floor((center[0] - x_width) * 1.0 / grid_size);
	int grid_z_min =  std::floor((center[2] - z_width) * 1.0 / grid_size);
	int grid_x_max = std::floor((center[0] + x_width) * 1.0 / grid_size);
	int grid_z_max =  std::floor((center[2] + z_width) * 1.0 / grid_size);
	const auto& pos_1 = center;
	auto filter_lambda = [&](const pos_t& pos_2){
		int32_t diff_x = pos_1[0] - pos_2[0];
		int32_t diff_z = pos_1[2] - pos_2[2];
		int32_t diff_y = pos_1[1] - pos_2[1];
		return diff_x <= x_width && diff_x >= -x_width && diff_z <= z_width && diff_z >= -z_width && diff_y <= y_height && diff_y >= -y_height;
	};
	return filter_pos_entity_in_grids(grid_x_min, grid_z_min, grid_x_max, grid_z_max, filter_lambda);
}



