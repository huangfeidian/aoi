#include "aoi_manager.h"
#include "aoi_caclator.h"
// 判断是否在扇形范围内
static bool is_in_fan(const pos_t& center, float yaw, float yaw_range, const pos_t& cur);

aoi_manager::aoi_manager(aoi_caclator* in_aoi_impl, std::uint32_t in_max_entity_size, pos_t in_min, pos_t in_max)
: entity_pool(in_max_entity_size)
, aoi_impl(in_aoi_impl)
{

}
bool aoi_manager::add_entity(guid_t guid, std::uint16_t radius, std::uint16_t height, pos_t pos, std::uint32_t flag, std::uint16_t max_interest_in)
{
	if(!aoi_impl)
	{
		return false;
	}
	if(all_guids.find(guid) != all_guids.end())
	{
		return false;
	}
	auto cur_entity = entity_pool.request();
	if(cur_entity == nullptr)
	{
		return false;
	}
	cur_entity->flag = flag;
	cur_entity->guid = guid;
	cur_entity->radius = radius;
	cur_entity->height = height;
	cur_entity->pos = pos;
	cur_entity->max_interest_in = max_interest_in;
	aoi_impl->add_entity(cur_entity);
	return true;
}

bool aoi_manager::remove_entity(guid_t guid)
{
	auto cur_iter = all_guids.find(guid);
	if(cur_iter == all_guids.end())
	{
		return false;
	}
	auto cur_entity = cur_iter->second;
	aoi_impl->remove_entity(cur_entity);
	std::unordered_set<guid_t> temp_guids;
	temp_guids = cur_entity->interested_by;
	for(auto one_guid: temp_guids)
	{
		auto other_iter = all_guids.find(one_guid);
		if(other_iter == all_guids.end())
		{
			continue;
		}
		if(!other_iter->second->leave_by_force(*cur_entity))
		{
			other_iter->second->leave_by_pos(*cur_entity);
		}
	}
	temp_guids = cur_entity->interest_in;
	for(auto one_guid: temp_guids)
	{
		auto other_iter = all_guids.find(one_guid);
		if(other_iter == all_guids.end())
		{
			continue;
		}
		if(!cur_entity->leave_by_force(*(other_iter->second)))
		{
			cur_entity->leave_by_pos(*(other_iter->second));
		}
	}
	temp_guids = cur_entity->enter_guids;
	for(auto one_guid: temp_guids)
	{
		auto other_iter = all_guids.find(one_guid);
		if(other_iter == all_guids.end())
		{
			continue;
		}
		if(!cur_entity->leave_by_force(*(other_iter->second)))
		{
			cur_entity->leave_by_pos(*(other_iter->second));
		}
	}
	entity_pool.renounce(cur_entity);
	all_guids.erase(cur_iter);
	return true;

}

bool aoi_manager::change_entity_radius(guid_t guid, std::uint16_t radius)
{
	auto cur_iter = all_guids.find(guid);
	if(cur_iter == all_guids.end())
	{
		return false;
	}
	auto cur_entity = cur_iter->second;
	aoi_impl->on_radius_update(cur_entity, radius);
	return true;
}

bool aoi_manager::change_entity_pos(guid_t guid, pos_t pos)
{
	auto cur_iter = all_guids.find(guid);
	if(cur_iter == all_guids.end())
	{
		return false;
	}
	auto cur_entity = cur_iter->second;
	aoi_impl->on_position_update(cur_entity, pos);
	return true;
}

bool aoi_manager::add_force_aoi(guid_t from, guid_t to)
{
	auto from_iter = all_guids.find(from);
	auto to_iter = all_guids.find(to);
	if(from_iter == all_guids.end() || to_iter == all_guids.end())
	{
		return false;
	}
	return from_iter->second->enter_by_force(to_iter->second);
}
bool aoi_manager::remove_force_aoi(guid_t from, guid_t to)
{
	auto from_iter = all_guids.find(from);
	auto to_iter = all_guids.find(to);
	if(from_iter == all_guids.end() || to_iter == all_guids.end())
	{
		return false;
	}
	return from_iter->second->leave_by_force(to_iter->second);
}
const std::vector<guid_t>& aoi_manager::interest_in(guid_t guid)const
{
	auto cur_iter = all_guids.find(guid);
	if(cur_iter == all_guids.end())
	{
		return invalid_set;
	}
	return cur_iter->second->interest_in;
}
const std::vector<guid_t>& aoi_manager::interested_by(guid_t guid)const
{
	auto cur_iter = all_guids.find(guid);
	if(cur_iter == all_guids.end())
	{
		return invalid_set;
	}
	return cur_iter->second->interested_by;
}

std::vector<guid_t> aoi_manager::entity_in_circle(pos_t center, std::uint16_t radius) const
{
	if(!aoi_impl)
	{
		return {};
	}
	return aoi_impl->entity_in_circle(center, radius);
}

std::vector<guid_t> aoi_manager::entity_in_cylinder(pos_t center, std::uint16_t radius, std::uint16_t height) const
{
	if(!aoi_impl)
	{
		return {};
	}
	return aoi_impl->entity_in_cylinder(center, radius, height);
}

std::vector<guid_t> aoi_manager::entity_in_rectangle(pos_t center, std::uint16_t x_width, std::uint16_t z_width) const
{
	if(!aoi_impl)
	{
		return {};
	}
	return aoi_impl->entity_in_rectangle(center, x_width, z_width);
}

std::vector<guid_t> aoi_manager::entity_in_square(pos_t center, std::uint16_t width) const
{
	return entity_in_rectangle(center, width, width);
}

std::vector<guid_t> aoi_manager::entity_in_cuboid(pos_t center, std::uint16_t x_width, std::uint16_t z_width, std::uint16_t y_height) const
{
	if(!aoi_impl)
	{
		return {};
	}
	return aoi_impl->entity_in_cuboid(center, x_width, z_width, y_height);
}

std::vector<guid_t> aoi_manager::entity_in_cube(pos_t center, std::uint16_t width)const
{
	return entity_in_cuboid(center, width, width, width);
}
std::vector<guid_t> aoi_manager::entity_in_fan(pos_t center, std::uint16_t radius, float yaw, float yaw_range) const
{
	auto circle_guids = entity_in_circle(center, radius);
	std::vector<guid_t> result;
	result.reserve(circle_guids.size() / 2 + 1);
	for(auto one_guid: circle_guids)
	{
		auto cur_iter = all_guids.find(one_guid);
		if(cur_iter == all_guids.end())
		{
			continue;
		}
		if(!is_in_fan(center, yaw, yaw_range, cur_iter->second.pos))
		{
			continue;
		}
		result.push_back(one_guid);

	}
	return result;
}

