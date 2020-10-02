#include "aoi_manager.h"
#include "aoi_interface.h"
#include <cmath>
// 判断是否在扇形范围内
#define PI 3.14159f
static bool is_in_fan(const pos_t& center, float yaw, float yaw_range, const pos_t& cur)
{
	if (yaw_range > PI / 2)
	{
		if (yaw <= PI)
		{
			return !is_in_fan(center, yaw + PI, PI - yaw_range, cur);
		}
		else
		{
			return !is_in_fan(center, yaw - PI, PI - yaw_range, cur);
		}
	}
	auto diff_x = cur[0] - center[0];
	auto diff_z = cur[2] - center[2];

	auto len = std::sqrtf(diff_x * diff_x + diff_z * diff_z);

	auto cos_range = std::cos(yaw_range);
	auto sin_yaw = std::sin(yaw);
	auto cos_yaw = std::cos(yaw);

	auto area = (cos_yaw * diff_x + sin_yaw * diff_z) / len;
	return area >= cos_range;

}

static std::vector<guid_t> entity_set_to_guid(const std::unordered_set<aoi_entity*> entities)
{
	std::vector<guid_t> result;
	result.reserve(entities.size());
	for(auto one_entity: entities)
	{
		result.push_back(one_entity->guid);
	}
	return result;
}
aoi_manager::aoi_manager(aoi_interface* in_aoi_impl, std::uint32_t in_max_entity_size, pos_unit_t in_max_aoi_radius, pos_t in_min, pos_t in_max)
: entity_pool(in_max_entity_size)
, aoi_impl(in_aoi_impl)
, min(in_min)
, max(in_max)
, max_entity_size(in_max_entity_size)
, max_aoi_radius(in_max_aoi_radius)
{

}
aoi_manager::~aoi_manager()

{
	if (aoi_impl)
	{
		delete aoi_impl;
		aoi_impl = nullptr;
	}
}
bool aoi_manager::is_in_border(const pos_t& pos) const
{
	if(pos[0] <= min[0] || pos[0] >= max[0])
	{
		return false;
	}
	if(pos[1] <= min[1] || pos[1] >= max[1])
	{
		return false;
	}
	if(pos[2] <= min[2] || pos[2] >= max[2])
	{
		return false;
	}
	return true;
}
bool aoi_manager::add_entity(guid_t guid, pos_unit_t radius, pos_unit_t height, pos_t pos, std::uint32_t flag, std::uint16_t max_interest_in)
{
	if(!aoi_impl)
	{
		return false;
	}
	if(radius >= max_aoi_radius)
	{
		return false;
	}
	if(!is_in_border(pos))
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
	all_guids[guid] = cur_entity;
	if (!aoi_impl->add_entity(cur_entity))
	{
		all_guids.erase(guid);
		return false;
	}
	else
	{
		return true;
	}
}

bool aoi_manager::remove_entity(guid_t guid)
{
	auto cur_iter = all_guids.find(guid);
	if(cur_iter == all_guids.end())
	{
		return false;
	}
	
	auto cur_entity = cur_iter->second;
	if(entities_removed.find(cur_entity) != entities_removed.end())
	{
		return false;
	}
	aoi_impl->remove_entity(cur_entity);
	std::unordered_set<guid_t> temp;
	temp = cur_entity->force_interest_in;
	for(auto one_guid: temp)
	{
		auto to_iter = all_guids.find(one_guid);
		if(to_iter == all_guids.end())
		{
			continue;
		}
		cur_entity->leave_by_force(*(to_iter->second));
	}
	temp = cur_entity->force_interested_by;
	for(auto one_guid: temp)
	{
		auto to_iter = all_guids.find(one_guid);
		if(to_iter == all_guids.end())
		{
			continue;
		}
		to_iter->second->leave_by_force(*cur_entity);
	}

	temp = cur_entity->interest_in;
	for(auto one_guid: temp)
	{
		auto to_iter = all_guids.find(one_guid);
		if(to_iter == all_guids.end())
		{
			continue;
		}
		cur_entity->leave_impl(*(to_iter->second));
	}
	temp = cur_entity->interested_by;
	for(auto one_guid: temp)
	{
		auto to_iter = all_guids.find(one_guid);
		if(to_iter == all_guids.end())
		{
			continue;
		}
		to_iter->second->leave_impl(*cur_entity);
	}

	entities_removed.insert(cur_entity);
	return true;

}

bool aoi_manager::change_entity_radius(guid_t guid, pos_unit_t radius)
{
	auto cur_iter = all_guids.find(guid);
	if(radius >= max_aoi_radius)
	{
		return false;
	}
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
	if(!is_in_border(pos))
	{
		return false;
	}
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
	return from_iter->second->enter_by_force(*(to_iter->second));
}
bool aoi_manager::remove_force_aoi(guid_t from, guid_t to)
{
	auto from_iter = all_guids.find(from);
	auto to_iter = all_guids.find(to);
	if(from_iter == all_guids.end() || to_iter == all_guids.end())
	{
		return false;
	}
	return from_iter->second->leave_by_force(*(to_iter->second));
}
const std::unordered_set<guid_t>& aoi_manager::interest_in(guid_t guid)const
{
	auto cur_iter = all_guids.find(guid);
	if(cur_iter == all_guids.end())
	{
		return invalid_result;
	}
	return cur_iter->second->interest_in;
}
const std::unordered_set<guid_t>& aoi_manager::interested_by(guid_t guid)const
{
	auto cur_iter = all_guids.find(guid);
	if(cur_iter == all_guids.end())
	{
		return invalid_result;
	}
	return cur_iter->second->interested_by;
}

std::vector<guid_t> aoi_manager::entity_in_circle(pos_t center, pos_unit_t radius)
{
	if(!aoi_impl)
	{
		return {};
	}
	return entity_set_to_guid(aoi_impl->entity_in_circle(center, radius));
}

std::vector<guid_t> aoi_manager::entity_in_cylinder(pos_t center, pos_unit_t radius, pos_unit_t height)
{
	if(!aoi_impl)
	{
		return {};
	}
	return entity_set_to_guid(aoi_impl->entity_in_cylinder(center, radius, height));
}

std::vector<guid_t> aoi_manager::entity_in_rectangle(pos_t center, pos_unit_t x_width, pos_unit_t z_width)
{
	if(!aoi_impl)
	{
		return {};
	}
	return entity_set_to_guid(aoi_impl->entity_in_rectangle(center, x_width, z_width));
}

std::vector<guid_t> aoi_manager::entity_in_square(pos_t center, pos_unit_t width)
{
	return entity_in_rectangle(center, width, width);
}

std::vector<guid_t> aoi_manager::entity_in_cuboid(pos_t center, pos_unit_t x_width, pos_unit_t z_width, pos_unit_t y_height)
{
	if(!aoi_impl)
	{
		return {};
	}
	return entity_set_to_guid(aoi_impl->entity_in_cuboid(center, x_width, z_width, y_height));
}

std::vector<guid_t> aoi_manager::entity_in_cube(pos_t center, pos_unit_t width)
{
	return entity_set_to_guid(aoi_impl->entity_in_cuboid(center, width, width, width));
}
std::vector<guid_t> aoi_manager::entity_in_fan(pos_t center, pos_unit_t radius, float yaw, float yaw_range)
{
	if (yaw < 0 || yaw >= 2 * PI)
	{
		return {};
	}
	if (yaw_range <= 0 || yaw_range >= PI)
	{
		return {};
	}
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
		if(!is_in_fan(center, yaw, yaw_range, cur_iter->second->pos))
		{
			continue;
		}
		result.push_back(one_guid);

	}
	return result;
}

void aoi_manager::dump(std::ostream& out_debug) const
{
	for (auto one_item : all_guids)
	{
		auto guid = one_item.first;
		auto entity = one_item.second;
		out_debug << "info for guid " << guid << " pos: " <<entity->pos[0]<<","<<entity->pos[2]<<" radius:"<<entity->radius<< std::endl;
		std::vector<guid_t> temp;
		temp.insert(temp.end(), entity->interest_in.begin(), entity->interest_in.end());
		std::sort(temp.begin(), temp.end());
		out_debug << "interest in is:";
		for (auto one_guid : temp)
		{
			out_debug << one_guid << ", ";
		}
		out_debug << std::endl;

	}
	out_debug << "aoi_impl begin" << std::endl;
	if (aoi_impl)
	{
		aoi_impl->dump(out_debug);
	}
}